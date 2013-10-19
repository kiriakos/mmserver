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

#include "xwrapper.hpp"

#include <X11/extensions/XTest.h>
#include <X11/XKBlib.h>
#include <stdexcept>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Atoms.h>

#include "xclib.h"

XMouseInterface::XMouseInterface(const std::string display)
{
	if ((m_display = XOpenDisplay(display.empty()?NULL:display.c_str())) == NULL)
	{
		// sorry for throwing in constructor...
		throw std::runtime_error("cannot open xdisplay");
	}
}

XMouseInterface::~XMouseInterface()
{
	if (m_display)
	{
		XCloseDisplay(m_display);
	}
}

void XMouseInterface::MouseClick(MouseButton button, MouseState state) {
	XTestFakeButtonEvent(m_display, button, state == BTN_DOWN ? True : False, CurrentTime);
	XFlush(m_display);
}

void XMouseInterface::MouseScroll(int dx, int dy)
{
	int i;
	int xbutton = (dx > 0 ? 7 : 6); // right and left scroll buttons
	int ybutton = (dy > 0 ? 5 : 4); // down and up scroll buttons
	
	for(i = abs(dx); i > 0; i--)
	{
		XTestFakeButtonEvent(m_display, xbutton, True, CurrentTime);
		XTestFakeButtonEvent(m_display, xbutton, False, CurrentTime);
	}
	for(i = abs(dy); i > 0; i--)
	{
		XTestFakeButtonEvent(m_display, ybutton, True, CurrentTime);
		XTestFakeButtonEvent(m_display, ybutton, False, CurrentTime);
	}
	XFlush(m_display);
}

void XMouseInterface::MouseMove(int x, int y)
{
	XTestFakeRelativeMotionEvent(m_display, x, y, CurrentTime);
	XFlush(m_display);
}

XKeyboardInterface::XKeyboardInterface(bool enabled, const std::string display)
{
	keyboardEnabled = enabled;
	
	if ((m_display = XOpenDisplay(display.empty()?NULL:display.c_str())) == NULL)
	{
		// sorry for throwing in constructor...
		throw std::runtime_error("cannot open xdisplay");
	}
}

XKeyboardInterface::~XKeyboardInterface()
{
	if (m_display)
	{
		XCloseDisplay(m_display);
	}
}

// this version of sendkey takes a single keycode.
// it justs packs it in a list and passes it on to the more general form of Sendkey
void XKeyboardInterface::SendKey(int keycode)
{
	std::list<int> keys;
	keys.push_back(keycode);
	SendKey(keys);
}

bool XKeyboardInterface::keysymIsShiftVariant(KeySym key)
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
void XKeyboardInterface::SendKey(const std::list<int>& keycode)
{
	if (!keyboardEnabled) {
		return;
	}
	
	PressKeys(keycode);
	ReleaseKeys(keycode);
}

void XKeyboardInterface::PressKeys(const std::list<int>& keys) {
	for (std::list<int>::const_iterator i = keys.begin(); i != keys.end(); i++) {
		KeyCode key = XKeysymToKeycode(m_display, *i);
		if (key == NoSymbol) {
			continue;
		}
		XTestFakeKeyEvent(m_display, key, True, CurrentTime);
	}
	XFlush(m_display);
}

void XKeyboardInterface::ReleaseKeys(const std::list<int>& keys) {
	for (std::list<int>::const_reverse_iterator i = keys.rbegin(); i != keys.rend(); i++) {
		KeyCode key = XKeysymToKeycode(m_display, *i);
		if (key == NoSymbol) {
			continue;
		}
		XTestFakeKeyEvent(m_display, key, False, CurrentTime);
	}
	XFlush(m_display);
}

XClipboardInterface::XClipboardInterface(const std::string display) {
	if ((m_display = XOpenDisplay(display.empty()?NULL:display.c_str())) == NULL) {
		throw std::runtime_error("cannot open xdisplay");
	}
	
	/* we need a window in order to access the clipboard. not displayed. */
	m_window = XCreateSimpleWindow(m_display, DefaultRootWindow(m_display), 0, 0, 1, 1, 0, 0, 0);
}

XClipboardInterface::~XClipboardInterface() {
	XDestroyWindow(m_display, m_window);
	XCloseDisplay(m_display);
}

bool XClipboardInterface::Update(void) {
	
	/* buffer for selection data */
	unsigned char *sel_buf;
	
	/* length of sel_buf */
	unsigned long sel_len = 0;
	
	/* X Event Structures */
	XEvent evt;
	
	/* reading clipboard can be a multi-step process; context tracks current step */
	unsigned int context = XCLIB_XCOUT_NONE;
	
	/* X selection to work with - use XA_PRIMARY for current selected text */
	Atom sseln = XA_CLIPBOARD(m_display);
	
	/* desired format (xcout actually tries to get as UTF-8 first) */
	Atom target = XA_STRING;
	
	/* whether this update has actually changed the cached clipboard content */
	bool updated = false;
	
	while (1) {
		
		/* initial context is NONE, so this is skipped... */
		/* second time through, context is SENT, so we request next event... */
		/* only get an event if xcout() is doing something */
		if (context != XCLIB_XCOUT_NONE) {
			XNextEvent(m_display, &evt);
		}
		
		/* with initial context NONE, xcout sends selection request to owner,
		 * changes context to SENT, and returns... */
		/* xcout could do a few things the second time through;
		 * first, it'll see if it can get selection as UTF8; if not, returns fallback, and reverts to string below to try again */
		/* alternatively, getting the result may require repeated attempts (INCRemental mode) */
		/* or, it might just get it, setting context back to NONE so we break out */
		/* fetch the selection, or part of it */
		xcout(m_display, m_window, evt, sseln, target, &sel_buf, &sel_len, &context);
		
		/* initially, context is SENT now */
		/* fallback is needed. set XA_STRING to target and restart the loop. */
		if (context == XCLIB_XCOUT_FALLBACK) {
			context = XCLIB_XCOUT_NONE;
			target = XA_STRING;
			continue;
		}
		
		/* only continue if xcout() is doing something */
		if (context == XCLIB_XCOUT_NONE) {
			break;
		}
	}
	
	if (sel_len) {
		
		/* sel_buf is current clipboard content; if different from cached
		 * content, update the cache and set the updated flag to true. */
		if (clipcache.compare(0, std::string::npos, (char *)sel_buf, sel_len) != 0) {
			clipcache.assign((char *)sel_buf, (int)sel_len);
			updated = true;
		}
		
		if (sseln == XA_STRING)
			XFree(sel_buf);
		else
			free(sel_buf);
    }
    
    return updated;
}

/* return the cached clipboard content as a std::string */
const std::string XClipboardInterface::GetString() {
	return clipcache;
}

/* return the cached clipboard content as a null-terminated C string */
const char *XClipboardInterface::GetCStr(void) {
	return clipcache.c_str();
}
