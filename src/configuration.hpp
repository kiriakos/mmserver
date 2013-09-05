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

#ifndef _CONFIGURATION_HPP_
#define _CONFIGURATION_HPP_

#include <set>
#include <map>
#include <string>
#include <unistd.h>

class Configuration
{
	public:
		Configuration();
		~Configuration();

		void Read(const std::string& file);

		const std::string& getHostname() const;
		bool getDebug() const;
		unsigned short getPort() const;
		const std::set<std::string>& getDevices() const;
		const std::string& getPassword() const;
		bool getMouseAcceleration() const;
		double getMouseAccelerationSpeed() const;
		int getMouseAccelerationFactor() const;
		bool getMouseHorizontalScrolling() const;
		const std::string& getKeyboardLayout() const;

		const std::string getHotKeyName(unsigned int id) const;
		const std::string getHotKeyCommand(unsigned int id) const;
	private:
		std::string m_hostname;
		bool m_debug;
		unsigned short m_port;

		std::set<std::string> m_devices;
		std::string m_password;

		bool m_mouseAccelerate;
		double m_mouseAccelerationSpeed;
		int m_mouseAccelerationFactor;
		bool m_mouseHorizontalScrolling;
		std::string m_keyboardLayout;

		std::map<unsigned int, std::pair<std::string, std::string> > m_hotkeys;
};

#endif
