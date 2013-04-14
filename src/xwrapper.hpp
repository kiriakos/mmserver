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

		XMouseInterface(const std::string display = "");
		~XMouseInterface();

		void MouseLeft(MouseState state);
		void MouseRight(MouseState state);
		void MouseWheelY(int offset);
		void MouseMove(int x, int y);
	private:
		Display *m_display; 
};

class XKeyboardInterface
{
	public:
		XKeyboardInterface(const std::string display = "");
		~XKeyboardInterface();

		void SendKey(const std::list<int>& keycode);
		void SendKey(int keycode);
		bool KeyState(int keycode);
	private:

		Display *m_display; 
};

#endif
