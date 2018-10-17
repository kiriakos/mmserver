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

#ifndef _KEYBOARDINTERFACE_HPP_
#define _KEYBOARDINTERFACE_HPP_

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <string>
#include <list>

class KeyboardInterface
{
	public:
		KeyboardInterface(bool enabled = true, const std::string display = "");
		~KeyboardInterface();

		void SendKey(const std::list<int>& keycode);
		void SendKey(int keycode);

		void PressKeys(const std::list<int>& keys);
		void ReleaseKeys(const std::list<int>& keys);

		bool keysymIsShiftVariant(KeySym key);

	private:
		Display *m_display;
		bool keyboardEnabled;
};
#endif
