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

#include "clipboardinterface.hpp"

#include <X11/extensions/XTest.h>
#include <X11/XKBlib.h>
#include <stdexcept>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Atoms.h>

#include "xclib.h"

ClipboardInterface::ClipboardInterface(const std::string display) {
	if ((m_display = XOpenDisplay(display.empty()?NULL:display.c_str())) == NULL) {
		throw std::runtime_error("cannot open xdisplay");
	}

	/* we need a window in order to access the clipboard. not displayed. */
	m_window = XCreateSimpleWindow(m_display, DefaultRootWindow(m_display), 0, 0, 1, 1, 0, 0, 0);
}

ClipboardInterface::~ClipboardInterface() {
	XDestroyWindow(m_display, m_window);
	XCloseDisplay(m_display);
}

bool ClipboardInterface::Update(void) {
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
const std::string ClipboardInterface::GetString() {
	return clipcache;
}

/* return the cached clipboard content as a null-terminated C string */
const char *ClipboardInterface::GetCStr(void) {
	return clipcache.c_str();
}
