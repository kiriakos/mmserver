/*
   Mobile Mouse Linux Server 
   Copyright (C) 2011 Erik Lax <erik@datahack.se>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdio.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <syslog.h>
#include <memory.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <linux/limits.h>

//#define TOOLBAR_ICON
#ifdef TOOLBAR_ICON
#include <gtk/gtk.h>
#include <sys/wait.h>
#define TOOLBAR_LABEL_DEFAULT "Mobile Mouse Linux Server"
#define TOOLBAR_ICON_DEFAULT "/usr/share/mmserver/icons/mm-idle.svg"
#define TOOLBAR_ICON_CONNECTED "/usr/share/mmserver/icons/mm-connected.svg"
#endif
#define DEFAULT_CONFIG "/usr/share/mmserver/mmserver.conf"
#define USER_CONFIG_DIR ".mmserver"

#include <libconfig.h++>
#include "configuration.hpp"
#include "avahi.hpp"
#include "session.hpp"

#include "version.hpp.in"

#ifdef TOOLBAR_ICON
void* GTKStartup(void *arg);
GtkStatusIcon* tray = NULL;
#endif
int _argc;
char** _argv;

bool CheckUserConfig(char *fpath, size_t fpathlen);
bool CheckSystemConfig(char *fpath, size_t fpathlen);
bool FileExists(const char *filePath);

int main(int argc, char* argv[])
{
	signal(SIGPIPE, SIG_IGN);
	char path[PATH_MAX];
	bool foundConfig = false;
#ifdef TOOLBAR_ICON	
	bool userConfig = false;
#endif
	
	// store for later
	_argc = argc;
	_argv = argv;

	/* syslog */
	int logOpt = LOG_PID | LOG_CONS | LOG_PERROR;
	openlog("mmserver", logOpt, LOG_DAEMON); 

	/* parse configuration */
	Configuration appConfig;
	if (argc == 1) {
		/* look for config file in user or system directory */		
		if (CheckUserConfig(path, sizeof path)) {
			foundConfig = true;
#ifdef TOOLBAR_ICON
			userConfig = true;
#endif
		} else if (CheckSystemConfig(path, sizeof path)) {
			foundConfig = true;
		}
	} else if (argc == 2) {
		/* interpret the argument as the path to a particular config file */
		snprintf(path, sizeof path, "%s", argv[1]);
		foundConfig = true;
	} else {
		fprintf(stderr, "Mobile Mouse Server for Linux (%s.%s.%s)\n",
				MMSERVER_VERSION_MAJOR, MMSERVER_VERSION_MINOR, MMSERVER_VERSION_PATCH);
		fprintf(stderr, "Website: https://github.com/anoved/mmserver/\n");
		fprintf(stderr, "Usage: %s [/path/to/mmserver.conf]\n", argv[0]);
		return 1;
	}

	if (foundConfig) {
		syslog(LOG_INFO, "reading configuration from %s", path);
		try {
			appConfig.Read(path);
		}
		catch (const libconfig::FileIOException &err) {
			syslog(LOG_ERR, "Cannot read configuration from: %s", path);
			exit(1);
		}
	}

	syslog(LOG_INFO, "started on port %d", appConfig.getPort());
	daemon(1, 1);

	if (appConfig.getZeroconf()) {
		StartAvahi(appConfig);
	}
	
	/* bind.. */
	struct sockaddr_in serv_addr;
	bzero((char *)&serv_addr, sizeof serv_addr);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(appConfig.getPort());

	/* setup network.. */
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		syslog(LOG_ERR, "socket: %s", strerror(errno));
		exit(1);
	}

	int optval = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) < 0)
	{
		syslog(LOG_ERR, "setsockopt: %s", strerror(errno));
		exit(1);
	}

	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof serv_addr) < 0)
	{
		syslog(LOG_ERR, "bind: %s", strerror(errno));
		exit(1);
	}

	if (listen(sockfd, 1) < 0)
	{
		syslog(LOG_ERR, "listen: %s", strerror(errno));
		exit(1);
	}

#ifdef TOOLBAR_ICON
	pthread_t toolbarpid;
	if (pthread_create(&toolbarpid, 0x0, GTKStartup, (void*)(userConfig ? path : NULL)) == -1)
	{
		syslog(LOG_WARNING, "pthread_create failed: %s", strerror(errno));
	}
#endif

	/* server loop.. */
	while(1)
	{
		struct sockaddr_in caddr;
		int clen = sizeof caddr;

		/* accept client.. */
		int client = accept(sockfd, (struct sockaddr *)&caddr, (socklen_t*)&clen);
		if (client < 0)
		{
			syslog(LOG_WARNING, "accept failed: %s", strerror(errno));
			continue;
		}

		/* start new session.. */
		SessionContext * clientContext = new SessionContext(appConfig,
				client,
				inet_ntoa(caddr.sin_addr));

#ifdef TOOLBAR_ICON
		gtk_status_icon_set_tooltip(tray, std::string(clientContext->m_address + " connected").c_str());
		gtk_status_icon_set_from_file(tray, TOOLBAR_ICON_CONNECTED);
#endif

		pthread_t pid;
		if (pthread_create(&pid, 0x0, MobileMouseSession, (void*)clientContext) == -1)
		{
			syslog(LOG_WARNING, "pthread_create failed: %s", strerror(errno));
			delete clientContext;
			close(client);
		} else 
			pthread_join(pid, 0x0);

#ifdef TOOLBAR_ICON
		gtk_status_icon_set_tooltip(tray, TOOLBAR_LABEL_DEFAULT);
		gtk_status_icon_set_from_file(tray, TOOLBAR_ICON_DEFAULT);
#endif
	}

	return 0;
}

/*
 * Parameters:
 *   p, return path
 *   psize, length of return path string
 * 
 * Return Value:
 *   true if a user config file is found
 *   false if a user config file is not found
 * 
 * Result:
 *   p is set to path to user config file if found
 *   (user config file may be created as a copy of system config file)
 *   p is not modified if no user config file is found
 */
bool CheckUserConfig(char *p, size_t psize) {
	
	char upath[PATH_MAX];
	
	if (!getenv("HOME")) {
		return false;
	}
	
	snprintf(upath, sizeof upath, "%s/%s/mmserver.conf", getenv("HOME"), USER_CONFIG_DIR);
	if (!FileExists(upath)) {
		// attempt to install system conf file in user dir before failing
		if (	system("mkdir -p ~/" USER_CONFIG_DIR "/ >/dev/null 2>&1 ") ||
				system("cp " DEFAULT_CONFIG " ~/" USER_CONFIG_DIR "/ >/dev/null 2>&1")) {
			return false;
		}
		syslog(LOG_INFO, "creating %s", upath);
	}
	
	strncpy(p, upath, psize);
	return true;
}

/*
 * Parameters:
 *   p, return path
 *   psize, length of return path string
 * 
 * Return Value:
 *   true if a system config file is found
 *   false if a system config file is not found
 * 
 * Result:
 *   p is set to path to system config file if found
 *   p is not modified if no system config file is found
 */
bool CheckSystemConfig(char *p, size_t psize) {
	
	char cpath[PATH_MAX];
	
	snprintf(cpath, sizeof cpath, DEFAULT_CONFIG);
	if (!FileExists(cpath)) {
		return false;
	}
	
	strncpy(p, cpath, psize);
	return true; 
}

/* Return Value:
 *   true if file at filePath exists and is a regular file
 *   false otherwise
 */
bool FileExists(const char *filePath) {
	struct stat st;
	if (stat(filePath, &st) != 0 || !(st.st_mode & S_IFREG)) {
		return false;
	}
	return true;
}

#ifdef TOOLBAR_ICON
void GTKTrayAbout(GtkMenuItem* item __attribute__((unused)), gpointer uptr __attribute__((unused))) 
{
	const gchar* authors[] = { "Erik Lax <erik@datahack.se>\nhttp://sourceforge.net/projects/mmlinuxserver/\n\nKiriakos Krastillis\nhttp://github.com/kiriakos/mmserver\n\nJim DeVona\nhttp://github.com/anoved/mmserver", NULL }; 
	const gchar* license = "GNU GENERAL PUBLIC LICENSE\nVersion 2, June 1991\n\nhttp://www.gnu.org/licenses/gpl-2.0.txt";

	GtkWidget* about = gtk_about_dialog_new();
	gtk_about_dialog_set_name((GtkAboutDialog*)about, "Mobile Mouse Server for Linux");
	gtk_window_set_icon((GtkWindow*)about, gtk_widget_render_icon(about, GTK_STOCK_ABOUT, GTK_ICON_SIZE_MENU, NULL));
	gtk_about_dialog_set_copyright((GtkAboutDialog*)about, "Copyright (C) 2011 Erik Lax,\n2013 Kiriakos Krastillis, Jim DeVona");
	gtk_about_dialog_set_version((GtkAboutDialog*)about, MMSERVER_VERSION_MAJOR"."MMSERVER_VERSION_MINOR"."MMSERVER_VERSION_PATCH);
	gtk_about_dialog_set_website((GtkAboutDialog*)about, "https://github.com/anoved/mmserver/");
	gtk_about_dialog_set_comments((GtkAboutDialog*)about, "An unofficial Mobile Mouse server for Linux. Use your iOS or Android device as a network mouse and keyboard for your Linux computer using Mobile Mouse and this software.");
	gtk_about_dialog_set_authors((GtkAboutDialog*)about, authors);
	gtk_about_dialog_set_license((GtkAboutDialog*)about, license);

	gtk_dialog_run((GtkDialog*)about);
	gtk_widget_destroy(about);
}

void GTKPreferences(GtkMenuItem* item __attribute__((unused)), gpointer uptr) 
{
	char cmd[PATH_MAX];
	snprintf(cmd, sizeof cmd, "gnome-text-editor %s &", (char *)uptr);
	system(cmd);
}

void GTKTrayQuit(GtkMenuItem* item __attribute__((unused)), gpointer uptr __attribute__((unused))) 
{
	syslog(LOG_INFO, "quitting from tray menu");
	gtk_main_quit();
}

void GTKTrayMenu(GtkStatusIcon* tray, guint button, guint32 time, gpointer uptr)
{
	gtk_menu_popup(GTK_MENU(uptr), NULL, NULL, gtk_status_icon_position_menu, tray, button, time);
}

void* GTKStartup(void *arg)
{
	gtk_init(0, 0x0);
	char *preferencesPath = (char *)arg;
		
	tray = gtk_status_icon_new();
	
	GtkWidget *menu = gtk_menu_new(),
		  *menuPreferences = gtk_menu_item_new_with_label("Preferences"),
		  *menuAbout = gtk_menu_item_new_with_label("About"),
		  *menuQuit = gtk_menu_item_new_with_label("Quit");
	
	if (preferencesPath != NULL) {
		g_signal_connect(G_OBJECT(menuPreferences), "activate", G_CALLBACK(GTKPreferences), (gpointer)preferencesPath);
	}
	g_signal_connect(G_OBJECT(menuAbout), "activate", G_CALLBACK(GTKTrayAbout), NULL);
	g_signal_connect(G_OBJECT(menuQuit), "activate", G_CALLBACK(GTKTrayQuit), NULL);
	
	if (preferencesPath != NULL) {
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuPreferences);
	}
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuAbout);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuQuit);
	gtk_widget_show_all(menu);

	g_signal_connect(G_OBJECT(tray), "popup-menu", G_CALLBACK(GTKTrayMenu), menu);
	gtk_status_icon_set_tooltip(tray, TOOLBAR_LABEL_DEFAULT);
	gtk_status_icon_set_from_icon_name(tray, TOOLBAR_ICON_DEFAULT);
	gtk_status_icon_set_from_file(tray, TOOLBAR_ICON_DEFAULT);
	gtk_status_icon_set_visible(tray, TRUE);

	gtk_main();
	exit(0);
}
#endif
