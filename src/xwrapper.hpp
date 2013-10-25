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

#ifndef _XWRAPPER_HPP_
#define _XWRAPPER_HPP_

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <string>
#include <list>

class XMouseInterface
{
	public:
		
		enum MouseState {
			BTN_DOWN,
			BTN_UP,
		};
		
		enum MouseButton {
			BTN_LEFT = 1,
			BTN_MIDDLE = 2,
			BTN_RIGHT = 3
		}; 
		
		XMouseInterface(const std::string display = "");
		~XMouseInterface();
		
		void MouseClick(MouseButton button, MouseState state);
		void MouseScroll(int x, int y);
		void MouseMove(int x, int y);
	private:
		
		void SetButtonState(MouseButton button, MouseState state);
		XMouseInterface::MouseState GetButtonState(MouseButton button);
		
		Display *m_display;
		MouseState left, middle, right;
};

class XKeyboardInterface
{
	public:
		XKeyboardInterface(bool enabled = true, const std::string display = "");
		~XKeyboardInterface();

		void SendKey(const std::list<int>& keycode);
		void SendKey(int keycode);
		
		void PressKeys(const std::list<int>& keys);
		void ReleaseKeys(const std::list<int>& keys);
		
		bool keysymIsShiftVariant(KeySym key);
		
	private:

		Display *m_display;
		bool keyboardEnabled;
};

class XClipboardInterface
{
	public:
		XClipboardInterface(const std::string display = "");
		~XClipboardInterface();

		bool Update(void);
		const std::string GetString(void);
		const char *GetCStr(void);
	
	private:
		Display *m_display;
		Window m_window;
		std::string clipcache;
};

#endif
