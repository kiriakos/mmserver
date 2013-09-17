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

#define TOOLBAR_ICON
#ifdef TOOLBAR_ICON
#include <gtk/gtk.h>
#include <sys/wait.h>
#define TOOLBAR_LABEL_DEFAULT "Mobile Mouse Linux Server"
#define TOOLBAR_ICON_DEFAULT "/usr/share/mmserver/icons/mm-idle.svg"
#define TOOLBAR_ICON_CONNECTED "/usr/share/mmserver/icons/mm-connected.svg"
#endif
#define DEFAULT_CONFIG "/usr/share/mmserver/mmserver.conf"

#include <libconfig.h++>
#include "configuration.hpp"
#include "avahi.hpp"
#include "session.hpp"

#include "version.hpp.in"

#ifdef TOOLBAR_ICON
void* GTKStartup(void *);
GtkStatusIcon* tray = NULL;
#endif
char path[PATH_MAX];
int _argc;
char** _argv;

int main(int argc, char* argv[])
{
	signal(SIGPIPE, SIG_IGN);
	
	// store for later
	_argc = argc;
	_argv = argv;

	/* syslog */
	int logOpt = LOG_PID | LOG_CONS | LOG_PERROR;
	openlog("mmserver", logOpt, LOG_DAEMON); 

	/* parse configuration */
	Configuration appConfig;
	if (argc == 1) {
		if (getenv("HOME")) {
			snprintf(path, sizeof path, "%s/.mmserver/mmserver.conf", getenv("HOME"));
			struct stat st;
			if (stat(path, &st) != 0 || !(st.st_mode & S_IFREG))
				snprintf(path, sizeof path, DEFAULT_CONFIG);
		} else {
			snprintf(path, sizeof path, DEFAULT_CONFIG);
		}
		syslog(LOG_INFO, "reading configuration from %s", path);
		/* ParseExceptions should be allowed to terminate the program, so that
		 * the user is prompted to fix the problem. FileIOExceptions indicate
		 * that the config file is not found, so just use the default values. */
		try {
			appConfig.Read(path);
		} catch (const libconfig::FileIOException &err) {
			syslog(LOG_ERR, "Cannot find config file; using default values");
		}
	} else if (argc > 2 && strcmp(argv[1], "-f") == 0) {
		syslog(LOG_INFO, "reading configuration from %s", argv[2]);
		/* We do *not* catch file-not-found exceptions for explicitly named
		 * config files; the user wanted one specifically, so don't default. */
		appConfig.Read(argv[2]);
	} else {
		fprintf(stderr, "Mobile Mouse Server for Linux (%s.%s.%s) by Erik Lax <erik@datahack.se>\n",
			MMSERVER_VERSION_MAJOR,
			MMSERVER_VERSION_MINOR,
			MMSERVER_VERSION_PATCH	
		);
		fprintf(stderr, "Run: %s /path/to/mmserver.conf\n", argv[0]);
		return 1;
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
	if (pthread_create(&toolbarpid, 0x0, GTKStartup, (void*)0x0) == -1)
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

void GTKPreferences(GtkMenuItem* item __attribute__((unused)), gpointer uptr __attribute__((unused))) 
{
	char cmd[PATH_MAX];
	if (strcmp(path, DEFAULT_CONFIG) == 0) {
		system("mkdir -p ~/.mmserver/");
		system("cp /usr/share/mmserver/mmserver.conf ~/.mmserver/");
		snprintf(cmd, sizeof cmd, "gnome-text-editor ~/.mmserver/mmserver.conf &");
	} else {
		snprintf(cmd, sizeof cmd, "gnome-text-editor %s &", path);
	}
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

void* GTKStartup(void *)
{
	gtk_init(0, 0x0);
	tray = gtk_status_icon_new();
	
	GtkWidget *menu = gtk_menu_new(),
		  *menuPreferences = gtk_menu_item_new_with_label("Preferences"),
		  *menuAbout = gtk_menu_item_new_with_label("About"),
		  *menuQuit = gtk_menu_item_new_with_label("Quit");
	g_signal_connect(G_OBJECT(menuPreferences), "activate", G_CALLBACK(GTKPreferences), NULL);
	g_signal_connect(G_OBJECT(menuAbout), "activate", G_CALLBACK(GTKTrayAbout), NULL);
	g_signal_connect(G_OBJECT(menuQuit), "activate", G_CALLBACK(GTKTrayQuit), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuPreferences);
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
