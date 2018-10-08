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

#include "keyboardinterface.hpp"

#include <X11/extensions/XTest.h>
#include <X11/XKBlib.h>
#include <stdexcept>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

KeyboardInterface::KeyboardInterface(bool enabled, const std::string display)
{
	keyboardEnabled = enabled;

	if ((m_display = XOpenDisplay(display.empty()?NULL:display.c_str())) == NULL)
	{
		// sorry for throwing in constructor...
		throw std::runtime_error("cannot open xdisplay");
	}
}

KeyboardInterface::~KeyboardInterface()
{
	if (m_display)
	{
		XCloseDisplay(m_display);
	}
}

// this version of sendkey takes a single keycode.
// it justs packs it in a list and passes it on to the more general form of Sendkey
void KeyboardInterface::SendKey(int keycode)
{
	std::list<int> keys;
	keys.push_back(keycode);
	SendKey(keys);
}

bool KeyboardInterface::keysymIsShiftVariant(KeySym key)
{
	// get the physical keycode associated with this key
	KeyCode basekey = XKeysymToKeycode(m_display, key);

	// check if shift-modified variant keysym of base key matches input keysym
	if (XkbKeycodeToKeysym(m_display, basekey, 0, 1) == key) {
		return true;
	}

	return false;
}

// this version of sendkey takes a list of keycodes
void KeyboardInterface::SendKey(const std::list<int>& keycode)
{
	if (!keyboardEnabled) {
		return;
	}

	PressKeys(keycode);
	ReleaseKeys(keycode);
}

void KeyboardInterface::PressKeys(const std::list<int>& keys) {
	for (std::list<int>::const_iterator i = keys.begin(); i != keys.end(); i++) {
		KeyCode key = XKeysymToKeycode(m_display, *i);
		if (key == NoSymbol) {
			continue;
		}
		XTestFakeKeyEvent(m_display, key, True, CurrentTime);
	}
	XFlush(m_display);
}

void KeyboardInterface::ReleaseKeys(const std::list<int>& keys) {
	for (std::list<int>::const_reverse_iterator i = keys.rbegin(); i != keys.rend(); i++) {
		KeyCode key = XKeysymToKeycode(m_display, *i);
		if (key == NoSymbol) {
			continue;
		}
		XTestFakeKeyEvent(m_display, key, False, CurrentTime);
	}
	XFlush(m_display);
}
