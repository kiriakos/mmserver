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

#include "mouseinterface.hpp"

MouseInterface::MouseInterface()
{
	m_dev = libevdev_new();

	libevdev_set_name(m_dev, "mmouse device");
	libevdev_enable_event_type(m_dev, EV_REL);
	libevdev_enable_event_code(m_dev, EV_REL, REL_X, nullptr);
	libevdev_enable_event_code(m_dev, EV_REL, REL_Y, nullptr);
	libevdev_enable_event_code(m_dev, EV_REL, REL_HWHEEL, nullptr);
	libevdev_enable_event_code(m_dev, EV_REL, REL_WHEEL, nullptr);
	libevdev_enable_event_type(m_dev, EV_KEY);
	libevdev_enable_event_code(m_dev, EV_KEY, BTN_LEFT, nullptr);
	libevdev_enable_event_code(m_dev, EV_KEY, BTN_MIDDLE, nullptr);
	libevdev_enable_event_code(m_dev, EV_KEY, BTN_RIGHT, nullptr);

	libevdev_uinput_create_from_device(m_dev, LIBEVDEV_UINPUT_OPEN_MANAGED, &m_uidev);

	SetButtonState(LEFT, UP);
	SetButtonState(MIDDLE, UP);
	SetButtonState(RIGHT, UP);
}

MouseInterface::~MouseInterface()
{
	libevdev_uinput_destroy(m_uidev);
	libevdev_free(m_dev);
}

void MouseInterface::SetButtonState(MouseButton button, MouseState state) {
	if (button == LEFT) {
		left = state;
	} else if (button == MIDDLE) {
		middle = state;
	} else if (button == RIGHT) {
		right = state;
	}
}

MouseInterface::MouseState MouseInterface::GetButtonState(MouseButton button) {
	MouseState state = UP;
	if (button == LEFT) {
		state = left;
	} else if (button == MIDDLE) {
		state = middle;
	} else if (button == RIGHT) {
		state = right;
	}
	return state;
}

void MouseInterface::MouseClick(MouseButton button, MouseState state) {
	// do not relay unnecessary duplicate events
	if (GetButtonState(button) == state) {
		return;
	}

	SetButtonState(button, state);
	libevdev_uinput_write_event(m_uidev, EV_KEY, button, state); // != 0 - error
	libevdev_uinput_write_event(m_uidev, EV_SYN, SYN_REPORT, 0); // != 0 - error
}

void MouseInterface::MouseScroll(int dx, int dy)
{
	libevdev_uinput_write_event(m_uidev, EV_REL, REL_HWHEEL, dx); // != 0 - error
	libevdev_uinput_write_event(m_uidev, EV_REL, REL_WHEEL, dy); // != 0 - error
	libevdev_uinput_write_event(m_uidev, EV_SYN, SYN_REPORT, 0); // != 0 - error
}

void MouseInterface::MouseMove(int x, int y)
{
	libevdev_uinput_write_event(m_uidev, EV_REL, REL_X, x); // != 0 - error
	libevdev_uinput_write_event(m_uidev, EV_REL, REL_Y, y); // != 0 - error
	libevdev_uinput_write_event(m_uidev, EV_SYN, SYN_REPORT, 0); // != 0 - error
}
