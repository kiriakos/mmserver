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

#ifndef _SESSION_HPP_
#define _SESSION_HPP_

#include "configuration.hpp"

class SessionContext
{
	public:
		SessionContext(Configuration& appConfig, int sock, const std::string address)
		: m_appConfig(appConfig)
		, m_sock(sock)
		, m_address(address)
		{
		}

		Configuration& m_appConfig;
		int m_sock;
		std::string m_address;
};

void* MobileMouseSession(void* context);

#endif
