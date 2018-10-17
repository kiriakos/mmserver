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

#ifndef _MOUSEINTERFACE_HPP_
#define _MOUSEINTERFACE_HPP_

#include <libevdev/libevdev-uinput.h>

class MouseInterface
{
	public:
		enum MouseState {
			UP,
			DOWN,
		};

		enum MouseButton {
			LEFT = BTN_LEFT,
			MIDDLE = BTN_MIDDLE,
			RIGHT = BTN_RIGHT,
		};

		MouseInterface();
		~MouseInterface();

		void MouseClick(MouseButton button, MouseState state);
		void MouseScroll(int x, int y);
		void MouseMove(int x, int y);

	private:
		void SetButtonState(MouseButton button, MouseState state);
		MouseInterface::MouseState GetButtonState(MouseButton button);

		struct libevdev *m_dev;
		struct libevdev_uinput *m_uidev;
		MouseState left, middle, right;
};
#endif
