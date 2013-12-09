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

#include "configuration.hpp"

#include <libconfig.h++>
#include <syslog.h>

Configuration::Configuration()
: m_hostname("localhost")
, m_platform("MAC")
, m_debug(true)
, m_port(9099)
, m_zeroconf(true)
, m_mouseAccelerate(true)
, m_mouseAccelerationSpeed(0.0004)
, m_mouseAccelerationFactor(4)
, m_mouseHorizontalScrolling(false)
, m_mouseScrollMax(1)
, m_keyboardEnabled(true)
, m_keyboardLayout("iso-8859-1")
{
	char hostname[256];
	gethostname(hostname, 256);
	m_hostname = hostname;
}

Configuration::~Configuration()
{
}

void Configuration::Read(const std::string& file)
{
	libconfig::Config config;
	config.readFile(file.c_str());

	if (config.exists("server.debug"))
	{
		m_debug = (bool)config.lookup("server.debug");
	}

	if (config.exists("server.port"))
	{
		m_port = (short)(unsigned int)config.lookup("server.port");
	}

	if (config.exists("server.zeroconf"))
	{
		m_zeroconf = (bool)config.lookup("server.zeroconf");
	}
	
	if (config.exists("server.platform"))
	{
		std::string platform;
		
		platform = (const char *)config.lookup("server.platform");
		if (platform == "MAC" || platform == "WIN") {
			m_platform = platform;
		} else {
			syslog(LOG_ERR, "server.platform must be MAC or WIN");
		}
	}
	
	if (config.exists("device.id"))
	{
		libconfig::Setting& id = config.lookup("device.id");
		if (id.isScalar())
		{
			m_devices.insert((const char*)id);	
		}
		else if (id.isArray())
		{
			for (int i = 0; i < id.getLength(); i++)
			{
				m_devices.insert((const char*)id[i]);
			}
		}
		else
		{
			syslog(LOG_ERR, "device.id must be an array of strings or a single string");
		}
	}

	if (config.exists("device.password"))
	{
		m_password = (const char*)config.lookup("device.password");
	}

	if (config.exists("mouse.accelerate"))
	{
		m_mouseAccelerate = (bool)config.lookup("mouse.accelerate");
	}

	if (config.exists("mouse.accelerationSpeed"))
	{
		m_mouseAccelerationSpeed = (double)config.lookup("mouse.accelerationSpeed");
	}

	if (config.exists("mouse.accelerationFactor"))
	{
		m_mouseAccelerationFactor = (int)config.lookup("mouse.accelerationFactor");
	}

	if (config.exists("mouse.horizontalScrolling"))
	{
		m_mouseHorizontalScrolling = (bool)config.lookup("mouse.horizontalScrolling");
	}

	if (config.exists("mouse.scrollMax"))
	{
		m_mouseScrollMax = (int)config.lookup("mouse.scrollMax");
		if (m_mouseScrollMax < 1) {
			syslog(LOG_ERR, "mouse.scrollMax must be at least 1");
			m_mouseScrollMax = 1;
		}
	}

	if (config.exists("keyboard.enabled")) 
	{
		m_keyboardEnabled = (bool)config.lookup("keyboard.enabled");
	}
	
	if (config.exists("keyboard.layout"))
	{
		m_keyboardLayout = (const char*)config.lookup("keyboard.layout");
	}

	if (config.exists("keyboard.hotkeys.key1.name") && config.exists("keyboard.hotkeys.key1.command"))
	{
		m_hotkeys[1] = std::make_pair(
				(const char*)config.lookup("keyboard.hotkeys.key1.name"),
				(const char*)config.lookup("keyboard.hotkeys.key1.command")
				);
	}

	if (config.exists("keyboard.hotkeys.key2.name") && config.exists("keyboard.hotkeys.key2.command"))
	{
		m_hotkeys[2] = std::make_pair(
				(const char*)config.lookup("keyboard.hotkeys.key2.name"),
				(const char*)config.lookup("keyboard.hotkeys.key2.command")
				);
	}

	if (config.exists("keyboard.hotkeys.key3.name") && config.exists("keyboard.hotkeys.key3.command"))
	{
		m_hotkeys[3] = std::make_pair(
				(const char*)config.lookup("keyboard.hotkeys.key3.name"),
				(const char*)config.lookup("keyboard.hotkeys.key3.command")
				);
	}

	if (config.exists("keyboard.hotkeys.key4.name") && config.exists("keyboard.hotkeys.key4.command"))
	{
		m_hotkeys[4] = std::make_pair(
				(const char*)config.lookup("keyboard.hotkeys.key4.name"),
				(const char*)config.lookup("keyboard.hotkeys.key4.command")
				);
	}

	if (config.exists("mouse.hotkeys.key1.command"))
	{
		m_hotkeys[5] = std::make_pair("",
				(const char*)config.lookup("mouse.hotkeys.key1.command")
				);
	}

	if (config.exists("mouse.hotkeys.key2.command"))
	{
		m_hotkeys[6] = std::make_pair("",
				(const char*)config.lookup("mouse.hotkeys.key2.command")
				);
	}
	
	/* gesture commands */

#define GESTURE_HOTKEY_CONFIG(path, key) if(config.exists((path))) m_hotkeys[(key)] = std::make_pair("", (const char*)config.lookup((path)))
	GESTURE_HOTKEY_CONFIG("gestures.twofingerdoubletap", 7);
	GESTURE_HOTKEY_CONFIG("gestures.threefingersingletap", 8);
	GESTURE_HOTKEY_CONFIG("gestures.threefingerdoubletap", 9);
	GESTURE_HOTKEY_CONFIG("gestures.fourfingerpinch", 10);
	GESTURE_HOTKEY_CONFIG("gestures.fourfingerspread", 11);
	GESTURE_HOTKEY_CONFIG("gestures.fourfingerswipeleft", 12);
	GESTURE_HOTKEY_CONFIG("gestures.fourfingerswiperight", 13);
	GESTURE_HOTKEY_CONFIG("gestures.fourfingerswipeup", 14);
	GESTURE_HOTKEY_CONFIG("gestures.fourfingerswipedown", 15);
#undef GESTURE_HOTKEY_CONFIG

}

const std::string& Configuration::getHostname() const
{
	return m_hostname;
}

const std::string& Configuration::getPlatform() const
{
	return m_platform;
}

bool Configuration::getDebug() const
{
	return m_debug;
}

unsigned short Configuration::getPort() const
{
	return m_port;
}

bool Configuration::getZeroconf() const
{
	return m_zeroconf;
}

const std::set<std::string>& Configuration::getDevices() const
{
	return m_devices;
}

const std::string& Configuration::getPassword() const
{
	return m_password;
}

bool Configuration::getMouseAcceleration() const
{
	return m_mouseAccelerate;
}

double Configuration::getMouseAccelerationSpeed() const
{
	return m_mouseAccelerationSpeed;
}

int Configuration::getMouseAccelerationFactor() const
{
	return m_mouseAccelerationFactor;
}

bool Configuration::getMouseHorizontalScrolling() const
{
	return m_mouseHorizontalScrolling;
}

int Configuration::getMouseScrollMax() const
{
	return m_mouseScrollMax;
}

bool Configuration::getKeyboardEnabled() const
{
	return m_keyboardEnabled;
}

const std::string& Configuration::getKeyboardLayout() const
{
	return m_keyboardLayout;
}

const std::string Configuration::getHotKeyName(unsigned int id) const
{
	std::map<unsigned int, std::pair<std::string, std::string> >::const_iterator i;
	i = m_hotkeys.find(id);
	if (i == m_hotkeys.end())
		return "";
	return i->second.first;
}

const std::string Configuration::getHotKeyCommand(unsigned int id) const
{
	std::map<unsigned int, std::pair<std::string, std::string> >::const_iterator i;
	i = m_hotkeys.find(id);
	if (i == m_hotkeys.end())
		return "";
	return i->second.second;
}
