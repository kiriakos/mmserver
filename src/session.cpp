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

#include "session.hpp"

#include <stdio.h>
#include <sys/time.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <math.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <X11/extensions/XTest.h> 
#include <iconv.h>

#include <pcrecpp.h>

#include "keyboardinterface.hpp"
#include "mouseinterface.hpp"
#include "clipboardinterface.hpp"
#include "utils.hpp"

// pushes keysyms for any modifier keys named in `modifiers` onto the end of `keys`
void SetModKeys(const std::string& modifiers, std::list<int>& keys) {
	std::list<std::string> modlist = SplitString(modifiers, '+');
	for (std::list<std::string>::const_iterator i = modlist.begin(); i != modlist.end(); i++) {
		if (*i == "CTRL") {
			keys.push_back(XK_Control_L);
		}
		if (*i == "OPT") {
			keys.push_back(XK_Super_L);
		}
		if (*i == "ALT") {
			keys.push_back(XK_Alt_L);
		}
		if (*i == "SHIFT") {
			keys.push_back(XK_Shift_L);
		}
		// consider logging any unhandled tokens
	}
}

/*
 * Parameters:
 *   command, string containing command to execute (unless recognized as special control code)
 *   clip, reference to the clipboard interface
 *   client, for use by any control codes that need to communicate with the client app
 * 
 * Return:
 *   2 if a memory allocation or string operation failed
 *   1 if error occurred communicating with client
 *   0 otherwise
 */
int InvokeCommand(const std::string command, ClipboardInterface& clip, int client) {
	
	if (command.empty()) {
		return 0;
	}
	
	/* special case commands */
	if (command.compare("SYNC_CLIPBOARD") == 0) {
		char *message = NULL;
		const char *content = NULL;
		
		clip.Update();
		content = clip.GetCStr();
		
		/*
		15 - CLIPBOARDUPDATE
		4  - TEXT
		3  - (control bytes)
		1  - (null end byte)
		--------------------
		23 - (message setup)
		*/
		
		if ((message = (char *)malloc(23 + strlen(content))) == NULL) {
			return 1;
		}
		
		strcpy(message, "CLIPBOARDUPDATE\x1e" "TEXT\x1f");
		strcat(message, content);
		strcat(message, "\x04");
		
		syslog(LOG_INFO, "clipboard update message length: %ld", (unsigned long)strlen(message));
		
		if (write(client, (const char*)message, strlen(message)) < 1) {
			free(message);
			return 2;
		}
		
		free(message);
	}
	else {
		
		/* interpret all other commands literally */
		system(command.c_str());
	}
	
	return 0;
}

void* MobileMouseSession(void* context)
{
	Configuration& appConfig = static_cast<SessionContext*>(context)->m_appConfig;
	int client = static_cast<SessionContext*>(context)->m_sock;
	std::string address = static_cast<SessionContext*>(context)->m_address;
	delete static_cast<SessionContext*>(context);

	MouseInterface mousePointer;
	KeyboardInterface keyBoard(appConfig.getKeyboardEnabled());
	ClipboardInterface clipboard;

	syslog(LOG_INFO, "[%s] connected", address.c_str());

	char buffer[1024];
	ssize_t n;

	n = read(client, buffer, sizeof(buffer));
	if (n < 1)
	{
		syslog(LOG_INFO, "[%s] disconnected (connection failed: %s)", address.c_str(), strerror(errno));
		if (appConfig.getDebug()) {
			dumpPacket(buffer);
		}
		close(client);
		return NULL;
	}
	buffer[n] = '\0';

	/* hello */
	std::string password, id, name;
	if (!pcrecpp::RE("CONNECT\x1e(.*?)\x1e(.*?)\x1e(.*?)\x1e(?:.*)\x04").FullMatch(buffer,
				&password, &id, &name))
	{
		syslog(LOG_INFO, "[%s] disconnected (invalid protocol)", address.c_str());

		/* dump unhandled packets */
		if (appConfig.getDebug())
		{
			dumpPacket(buffer);
		}
		close(client);
		return NULL;
	}
	
	if (appConfig.getDebug()) {
		syslog(LOG_INFO, "[%s] device id: %s", address.c_str(), id.c_str());
		syslog(LOG_INFO, "[%s] device name: %s", address.c_str(), name.c_str());
	}
	
	/* verify device.id */
	if (!appConfig.getDevices().empty() &&
			appConfig.getDevices().find(id) == appConfig.getDevices().end())
	{
		char m[1024];
		snprintf(m, sizeof(m), "CONNECTED\x1e"
				"NO\x1e"
				"%s\x1e"
				"%s\x1e"
				"Device is not allowed\x1e"
				"00:00:00:00:00:00\x1e"
				"4\x04",
				appConfig.getPlatform().c_str(),
				appConfig.getHostname().c_str());
		if (write(client, (const char*)m, strlen((const char*)m)) > 0)
			n = read(client, m, sizeof(m)); /* let client disconnect */
		syslog(LOG_INFO, "[%s] disconnected (device not allowed %s)", address.c_str(), id.c_str());
		close(client);
		return NULL;
	}

	/* verify device.password */
	if (!appConfig.getPassword().empty() &&
			password != appConfig.getPassword())
	{
		char m[1024];
		snprintf(m, sizeof(m), "CONNECTED\x1e"
				"NO\x1e"
				"%s\x1e"
				"%s\x1e"
				"Incorrect password\x1e"
				"00:00:00:00:00:00\x1e"
				"4\x04",
				appConfig.getPlatform().c_str(),
				appConfig.getHostname().c_str());
		if (write(client, (const char*)m, strlen((const char*)m)) > 0)
			n = read(client, m, sizeof(m)); /* let client disconnect */
		syslog(LOG_INFO, "[%s] disconnected (incorrect password)", address.c_str());
		close(client);
		return NULL;
	}

	/* complete handshake... */
	{
		char m[1024];
		snprintf(m, sizeof(m), "CONNECTED\x1e"
				"YES\x1e"
				"%s\x1e"
				"%s\x1e"
				"Welcome\x1e"
				"00:00:00:00:00:00\x1e"
				"4\x04",
				appConfig.getPlatform().c_str(),
				appConfig.getHostname().c_str());
		if (write(client, (const char*)m, strlen((const char*)m)) < 1)
		{
			syslog(LOG_INFO, "[%s] disconnected (write failed: %s)", address.c_str(), strerror(errno));
			close(client);
			return NULL;
		}
	}

	/* register hotkeys */
	// Disabled if reported client id is "Android" to circumvent connection problems.
	// Android Mobile Mouse Lite appears to not support hotkey name definitions (and, critically, fails to connect if supplied)
	// Untested with Android Mobile Mouse Pro
	if (id != "Android") {
		char m[1024];
		snprintf(m, sizeof(m), "HOTKEYS\x1e"
				"%s\x1e"
				"%s\x1e"
				"%s\x1e"
				"%s\x04",
				appConfig.getHotKeyName(1).c_str(),
				appConfig.getHotKeyName(2).c_str(),
				appConfig.getHotKeyName(3).c_str(),
				appConfig.getHotKeyName(4).c_str()
				);
		if (write(client, (const char*)m, strlen((const char*)m)) < 1)
		{
			syslog(LOG_INFO, "[%s] disconnected (write failed: %s)", address.c_str(), strerror(errno));
			close(client);
			return NULL;
		}
	}

	struct timeval lastMouseEvent;
	timerclear(&lastMouseEvent);

	enum WindowMode {
		WM_OTHER,
		WM_MEDIA,
		WM_WEB,
		WM_PRESENTATION
	} currentWindowMode = WM_OTHER;

	enum PresentationStatus
	{
		PS_STOPPED,
		PS_STARTED
	} presentationStatus = PS_STOPPED;

	/* protocol loop */
	std::string packet_buffer;
	std::string packet;
	while(1)
	{
		if (packet_buffer.find('\x04') != std::string::npos)
		{
			packet = packet_buffer.substr(0, packet_buffer.find('\x04') + 1);
			packet_buffer.erase(0, packet_buffer.find('\x04') + 1);
		} else {
			n = read(client, buffer, sizeof(buffer));
			if (n < 1)
			{
				syslog(LOG_INFO, "[%s] disconnected (read failed: %s)", address.c_str(), strerror(errno));
				if (appConfig.getDebug()) {
					dumpPacket(buffer);
				}
				close(client);
				break;
			}
			packet_buffer.append(buffer, n);
			continue;
		}
		
		/* options */
		std::string option, optval;
		if (pcrecpp::RE("SETOPTION\x1e(.*?)\x1e(.*?)\x04").FullMatch(packet, &option, &optval))
		{
			if (option == "CLIPBOARDSYNC") {
				syslog(LOG_INFO, "Clipboard sync: %s", optval.c_str());
			}
			else if (option == "PRESENTATION") {
				/* Presumably concerns the extra in-app purchase "pro presentation module" */
				syslog(LOG_INFO, "Presentation mode: %s", optval.c_str());
			}
			else {
				syslog(LOG_ERR, "Unknown option: %s", option.c_str());
			}
			continue;
		}
		
		/* mouse clicks */
		std::string key, state, modifier;
 		if (pcrecpp::RE("CLICK\x1e([LR])\x1e([DU])\x1e(.*?)\x04").FullMatch(packet, &key, &state, &modifier))
		{
			std::list<int> modkeys;
			SetModKeys(modifier, modkeys);
			
			if (!modkeys.empty() && state == "D") {
				keyBoard.PressKeys(modkeys);
			}
			
			mousePointer.MouseClick(
					key == "L" ? MouseInterface::LEFT : MouseInterface::RIGHT,
					state == "D" ? MouseInterface::DOWN : MouseInterface::UP);
			
			if (!modkeys.empty() && state == "U") {
				keyBoard.ReleaseKeys(modkeys);
			}
			
			continue;
		}

		/* mouse movements */
		std::string xp, yp;
		if (pcrecpp::RE("MOVE\x1e(-?[\\d\x2e]+)\x1e(-?[\\d\x2e]+)\x1e[10]\x1e?\x04").FullMatch(packet, &xp, &yp))
		{
			struct timeval currentMouseEvent;
			gettimeofday(&currentMouseEvent, NULL);
			struct timeval diff;
			timersub(&currentMouseEvent, &lastMouseEvent, &diff);
			lastMouseEvent = currentMouseEvent;
			double usecdiff = ((double)diff.tv_sec * 1000000.0) + (double)diff.tv_usec;
			
			int dx, dy;
			double distance, speed;
			dx = (int)strtol(xp.c_str(), NULL, 10);
			dy = (int)strtol(yp.c_str(), NULL, 10);
			distance = sqrt((dx * dx) + (dy * dy));
			speed = distance / usecdiff;
			
			// if acceleration is enabled, apply when estimated cursor speed exceeds given rate (pixels per microsecond)
			if (appConfig.getMouseAcceleration() && (speed > appConfig.getMouseAccelerationSpeed())) {
				dx *= appConfig.getMouseAccelerationFactor();
				dy *= appConfig.getMouseAccelerationFactor();
			}
			
			mousePointer.MouseMove(dx, dy);
			continue;
		}

		/* mouse scrolling */
		std::string xs, ys;
		if (pcrecpp::RE("SCROLL\x1e(-?\\d+.?\\d+)\x1e(-?\\d+.?\\d+)\x1e(.*?)\x04").FullMatch(packet, &xs, &ys, &modifier))
		{
			int dx, dy;
			dx = appConfig.getMouseHorizontalScrolling() ? (int)strtol(xs.c_str(), NULL, 10) : 0;
			dy = (int)strtol(ys.c_str(), NULL, 10);
			
			// scrollmax is at least 1
			int maxd = appConfig.getMouseScrollMax();
			if (abs(dx) > maxd) {
				dx = maxd * dx / abs(dx);
			}
			if (abs(dy) > maxd) {
				dy = maxd * dy / abs(dy);
			}
			
			mousePointer.MouseScroll(dx, dy);
			continue;
		}
		
		/* zooming */
		/* consider handling in/out zoom as generic spread/pinch gesture hotkeys */ 
		std::string zoom;
		if (pcrecpp::RE("ZOOM\x1e(-?\\d+)\x04").FullMatch(packet, &zoom))
		{
			std::list<int> keys;
			for(int i = abs((int)strtol(zoom.c_str(), 0x0, 10)); i > 0; i--)
			{
				keys.push_back(XK_Control_L);
				if (zoom[0] == '-')
					keys.push_back('-');
				else
					keys.push_back('+');
			}
			keyBoard.SendKey(keys);
			continue;
		}

		/* key board */
		std::string chr, utf8;
		if (pcrecpp::RE("KEY\x1e(.*?)\x1e(.*?)\x1e(.*?)\x04").FullMatch(packet, &chr, &utf8, &modifier))
		{
			std::list<int> keys;
			int keyCode = 0;
			if (chr == "-61")
			{
				iconv_t cd;
				if ((cd = iconv_open(appConfig.getKeyboardLayout().c_str(), "UTF-8")) == (iconv_t)-1)
				{
					syslog(LOG_ERR, "iconv_open failed: %s", strerror(errno));
					continue;
				}
				char* inptr = (char*)utf8.c_str();
				size_t inlen = utf8.size();

				char out[1];
				bzero(out, sizeof(out));
				size_t outlen = sizeof(out);
				char *outptr = out;
				if (iconv(cd, &inptr, &inlen, &outptr, &outlen) == (size_t)-1)
				{
					iconv_close(cd);
					syslog(LOG_ERR, "iconv failed: %s", strerror(errno));
					continue;
				}
				iconv_close(cd);
				keyCode = 0xff & out[0];
			}
			else if (chr == "-1")
			{
				/* keyboard page */
				if (utf8 == "ENTER") keyCode = XK_Return;
				if (utf8 == "BACKSPACE") keyCode = XK_BackSpace;
				if (utf8 == "TAB") keyCode = XK_Tab;

				/* keypad */
				if (utf8 == "NUM_DIVIDE") keyCode = XK_KP_Divide;
				if (utf8 == "NUM_MULTIPLY") keyCode = XK_KP_Multiply;
				if (utf8 == "NUM_SUBTRACT") keyCode = XK_KP_Subtract;
				if (utf8 == "NUM_ADD") keyCode = XK_KP_Add;
				if (utf8 == "NUM_ENTER") keyCode = XK_KP_Enter;
				if (utf8 == "NUM_EQUAL") keyCode = XK_KP_Equal;
				if (utf8 == "NUM_DECIMAL") keyCode = XK_KP_Decimal;
				if (utf8 == "INSERT") keyCode = XK_KP_Insert;
				if (utf8 == "NUM0") keyCode = XK_KP_0;
				if (utf8 == "NUM1") keyCode = XK_KP_1;
				if (utf8 == "NUM2") keyCode = XK_KP_2;
				if (utf8 == "NUM3") keyCode = XK_KP_3;
				if (utf8 == "NUM4") keyCode = XK_KP_4;
				if (utf8 == "NUM5") keyCode = XK_KP_5;
				if (utf8 == "NUM6") keyCode = XK_KP_6;
				if (utf8 == "NUM7") keyCode = XK_KP_7;
				if (utf8 == "NUM8") keyCode = XK_KP_8;
				if (utf8 == "NUM9") keyCode = XK_KP_9;
				
				// non-num-locked keypad equivalents are received directly (as HOME, END, PGUP, etc)
				// so prefix keypad input with shift to interpet as regular numerals
				if (keyCode == XK_KP_0 ||
						keyCode == XK_KP_1 || keyCode == XK_KP_2 || keyCode == XK_KP_3
						|| keyCode == XK_KP_4 || keyCode == XK_KP_5 || keyCode == XK_KP_6
						|| keyCode == XK_KP_7 || keyCode == XK_KP_8 || keyCode == XK_KP_9) {
					keys.push_back(XK_Shift_L);
				}
				
				/* function page */
				if (utf8 == "ESCAPE") keyCode = XK_Escape;
				if (utf8 == "DELETE") keyCode = XK_Delete;
				if (utf8 == "HOME") keyCode = XK_Home;
				if (utf8 == "END") keyCode = XK_End;
				if (utf8 == "PGUP") keyCode = XK_Page_Up;
				if (utf8 == "PGDN") keyCode = XK_Page_Down;
				if (utf8 == "UP") keyCode = XK_Up;
				if (utf8 == "DOWN") keyCode = XK_Down;
				if (utf8 == "RIGHT") keyCode = XK_Right;
				if (utf8 == "LEFT") keyCode = XK_Left;
				if (utf8 == "F1") keyCode = XK_F1;
				if (utf8 == "F2") keyCode = XK_F2;
				if (utf8 == "F3") keyCode = XK_F3;
				if (utf8 == "F4") keyCode = XK_F4;
				if (utf8 == "F5") keyCode = XK_F5;
				if (utf8 == "F6") keyCode = XK_F6;
				if (utf8 == "F7") keyCode = XK_F7;
				if (utf8 == "F8") keyCode = XK_F8;
				if (utf8 == "F9") keyCode = XK_F9;
				if (utf8 == "F10") keyCode = XK_F10;
				if (utf8 == "F11") keyCode = XK_F11;
				if (utf8 == "F12") keyCode = XK_F12;

				/* media player */
				if (utf8 == "VOLDOWN") keyCode = XF86XK_AudioLowerVolume;
				if (utf8 == "VOLUP") keyCode = XF86XK_AudioRaiseVolume;
				if (utf8 == "VOLMUTE") keyCode = XF86XK_AudioMute;
				if (utf8 == "EJECT") keyCode = XF86XK_Eject;

				if (keyCode == 0) {
					// utf8 could contain multibyte characters; currently unhandled
					keyCode = utf8[0];

					// check if the shift key is required to generate input character (hack)
					if (keyBoard.keysymIsShiftVariant((KeySym)strtol(chr.c_str(), NULL, 10))) {
						keys.push_back(XK_Shift_L);
					}
				}
			}
			else
			{
				// utf8 could contain multibyte characters; currently unhandled
				keyCode = utf8[0];
				
				// check if the shift key is required to generate input character (hack)
				if (keyBoard.keysymIsShiftVariant((KeySym)strtol(chr.c_str(), NULL, 10))) {
					keys.push_back(XK_Shift_L);
				}
			}
			
			SetModKeys(modifier, keys);
			
			if (keyCode > 0)
			{
				keys.push_back(keyCode);
				keyBoard.SendKey(keys);
				continue;
			}
		}
		
		/* keystrings */
		std::string keystring;
		if (pcrecpp::RE("KEYSTRING\x1e(.*?)\x04").FullMatch(packet, &keystring))
		{
			std::list<int> keys;
			for (std::string::const_iterator i = keystring.begin(); i != keystring.end(); i++)
			{
				// keystrings are currently only generated when the client's shift-lock button
				// is toggled on... so every character that could be a shift variant presumably is.
				// Except for the last character, because keystrings are only sent once shift-lock
				// is disabled and another character is entered (which is included). Weird.
				if (keyBoard.keysymIsShiftVariant((KeySym)*i)) {
					keys.push_back(XK_Shift_L);
				}
				
				keys.push_back(*i);
			}
			keyBoard.SendKey(keys);
			continue;
		}
		
		/* gestures */
		std::string gesture;
		if (pcrecpp::RE("GESTURE\x1e(.*?)\x04").FullMatch(packet, &gesture)) {
			int hotkey = 0;
			if (gesture == "TWOFINGERDOUBLETAP")   hotkey = 7;
			if (gesture == "THREEFINGERSINGLETAP") hotkey = 8;
			if (gesture == "THREEFINGERDOUBLETAP") hotkey = 9;
			if (gesture == "FOURFINGERPINCH")      hotkey = 10;
			if (gesture == "FOURFINGERSPREAD")     hotkey = 11;
			if (gesture == "FOURFINGERSWIPELEFT")  hotkey = 12;
			if (gesture == "FOURFINGERSWIPERIGHT") hotkey = 13;
			if (gesture == "FOURFINGERSWIPEUP")    hotkey = 14;
			if (gesture == "FOURFINGERSWIPEDOWN")  hotkey = 15;
			if (hotkey != 0) {
				std::string command = appConfig.getHotKeyCommand(hotkey);
				if (InvokeCommand(command, clipboard, client)) {
					syslog(LOG_INFO, "[%s] disconnected (write failed: %s)", address.c_str(), strerror(errno));
					close(client);
					break;
				}
				continue;
			}
		}

		/* run hotkey commands */
		std::string hotkey;
		if (pcrecpp::RE("HOTKEY\x1eHK(\\d)\x04").FullMatch(packet, &hotkey))
		{
			std::string command = appConfig.getHotKeyCommand((unsigned int)strtoul(hotkey.c_str(), 0x0, 10));
			if(InvokeCommand(command, clipboard, client)) {
				syslog(LOG_INFO, "[%s] disconnected (write failed: %s)", address.c_str(), strerror(errno));
				close(client);
				break;
			}
			continue;
		}
		if (pcrecpp::RE("HOTKEY\x1e(B[12])\x04").FullMatch(packet, &hotkey))
		{
			std::string command;
			
			// B1 is invoked when scroll pad is tapped (but not scrolled),
			// like clicking the middle mouse button of a scroll mouse.
			// So, if no hotkey command is defined, fake a middle button click.
			if (hotkey == "B1") {
				command = appConfig.getHotKeyCommand(5);
				if (command.empty()) {
					mousePointer.MouseClick(MouseInterface::MIDDLE, MouseInterface::DOWN);
					mousePointer.MouseClick(MouseInterface::MIDDLE, MouseInterface::UP);
				}
			}
			// I don't know how to invoke B2.
			if (hotkey == "B2") {
				command = appConfig.getHotKeyCommand(6);
			}
			
			if (InvokeCommand(command, clipboard, client)) {
				syslog(LOG_INFO, "[%s] disconnected (write failed: %s)", address.c_str(), strerror(errno));
				close(client);
				break;
			}
			continue;
		}
		
		/* screens */
		std::string mode;
		if (pcrecpp::RE("SWITCHMODE\x1e(.*?)\x04").FullMatch(packet, &mode))
		{
			if (mode == "MEDIA")
			{
				currentWindowMode = WM_MEDIA;
				{
					/* current implementation supports totem */
					char m[1024];
					snprintf(m, sizeof(m), "MEDIACUSTOMKEYS\x1e"
							"Playlist\x1e"
							"\x1e"
							"\x1e"
							"\x1e"
							"Full Screen\x1e"
							"\x1e"
							"\x1e"
							"\x04"
							);
					if (write(client, (const char*)m, strlen((const char*)m)) < 1)
					{
						syslog(LOG_INFO, "[%s] disconnected (write failed: %s)", address.c_str(), strerror(errno));
						close(client);
						return NULL;
					}
				}
				/* launch mediaplayer */
				//keyBoard.SendKey(XF86XK_AudioMedia);
				continue;
			}
			if (mode == "WEB")
			{
				currentWindowMode = WM_WEB;
				/* launch webbrowser */
				//keyBoard.SendKey(XF86XK_WWW);
				continue;
			}
			if (mode == "PRESENTATION")
			{
				/* launch presentation */
				currentWindowMode = WM_PRESENTATION;
				presentationStatus = PS_STOPPED;
				continue;
			}
		}

		/* program keys */
		if (pcrecpp::RE("PROGRAMKEY\x1e(.*?)\x04").FullMatch(packet, &key))
		{
			switch(currentWindowMode)
			{
				case WM_OTHER:
					{
					}
				break;
				case WM_MEDIA:
					{
						if (key == "PLAYPAUSE")
						{
							keyBoard.SendKey(XF86XK_AudioPlay);
							continue;
						}
						if (key == "TRACKPREV")
						{
							keyBoard.SendKey(XF86XK_AudioNext);
							continue;
						}
						if (key == "TRACKNEXT")
						{
							keyBoard.SendKey(XF86XK_AudioNext);
							continue;
						}
						if (key == "MEDIAPLUS")
						{
							keyBoard.SendKey(XF86XK_AudioRaiseVolume);
							continue;
						}
						if (key == "MEDIAMINUS")
						{
							keyBoard.SendKey(XF86XK_AudioLowerVolume);
							continue;
						}
						if (key == "MEDIACUSTOMKEY1")
						{
							keyBoard.SendKey(XK_F9);
							continue;
						}
						if (key == "MEDIACUSTOMKEY5")
						{
							keyBoard.SendKey(XK_F11);
							continue;
						}
					}
					break;
				case WM_WEB:
					{
						if (key == "BROWSERNEWWINDOW")
						{
							keyBoard.SendKey(XF86XK_WWW);
							continue;
						}
						if (key == "BROWSERNEWTAB")
						{
							std::list<int> keys;
							keys.push_back(XK_Control_L);
							keys.push_back('t');
							keyBoard.SendKey(keys);
							continue;
						}
						if (key == "BROWSERLOCATION")
						{
							std::list<int> keys;
							keys.push_back(XK_Control_L);
							keys.push_back('l');
							keyBoard.SendKey(keys);
							continue;
						}
						if (key == "BROWSERBACK")
						{
							std::list<int> keys;
							keys.push_back(XK_Alt_L);
							keys.push_back(XK_Left);
							keyBoard.SendKey(keys);
							continue;
						}
						if (key == "BROWSERNEXT")
						{
							std::list<int> keys;
							keys.push_back(XK_Alt_L);
							keys.push_back(XK_Right);
							keyBoard.SendKey(keys);
							continue;
						}
						if (key == "BROWSERHOME")
						{
							std::list<int> keys;
							keys.push_back(XK_Alt_L);
							keys.push_back(XK_Home);
							keyBoard.SendKey(keys);
							continue;
						}
						if (key == "BROWSERSEARCH")
						{
							std::list<int> keys;
							keys.push_back(XK_Control_L);
							keys.push_back('k');
							keyBoard.SendKey(keys);
							continue;
						}
						if (key == "BROWSERRELOAD")
						{
							std::list<int> keys;
							keys.push_back(XK_Control_L);
							keys.push_back('r');
							keyBoard.SendKey(keys);
							continue;
						}
						if (key == "BROWSERSTOP")
						{
							std::list<int> keys;
							keys.push_back(XK_Escape);
							keyBoard.SendKey(keys);
							continue;
						}
						if (key == "BROWSERBOOKMARKS")
						{
							std::list<int> keys;
							keys.push_back(XK_Control_L);
							keys.push_back('b');
							keyBoard.SendKey(keys);
							continue;
						}
						if (key == "BROWSERPLUS")
						{
							std::list<int> keys;
							keys.push_back(XK_Control_L);
							keys.push_back(XK_Tab);
							keyBoard.SendKey(keys);
							continue;
						}
						if (key == "BROWSERMINUS")
						{
							std::list<int> keys;
							keys.push_back(XK_Control_L);
							keys.push_back(XK_Shift_L);
							keys.push_back(XK_Tab);
							keyBoard.SendKey(keys);
							continue;
						}
					}
					break;
				case WM_PRESENTATION:
					{
						if (key == "PRESENTATIONSTART")
						{
							if (presentationStatus == PS_STOPPED)
							{
								keyBoard.SendKey(XK_F5);
								presentationStatus = PS_STARTED;
								continue;
							}
							if (presentationStatus == PS_STARTED)
							{
								keyBoard.SendKey(XK_Escape);
								presentationStatus = PS_STOPPED;
								continue;
							}
						}
						if (key == "PRESENTATIONNEXT")
						{
							keyBoard.SendKey(XK_Right);
							continue;
						}
						if (key == "PRESENTATIONBACK")
						{
							keyBoard.SendKey(XK_Left);
							continue;
						}
					}
					break;
			}
		}


		/* promotional links */
		std::string url;
		if (pcrecpp::RE("OPENLINK\x1e(.*?)\x04").FullMatch(packet, &url)) {
			syslog(LOG_INFO, "Promotional link: %s", url.c_str());
			continue;
		}

		syslog(LOG_INFO, "[%s] unhandled packet: size(%lu)", address.c_str(), (long unsigned int)packet.size());

		/* dump unhandled packets */
		if (appConfig.getDebug())
		{
			dumpPacket(packet.c_str());
		}
	}

	syslog(LOG_INFO, "[%s] session ended", address.c_str());
	return NULL;
}
