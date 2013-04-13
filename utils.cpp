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

#include "utils.hpp"
#include <sstream>
#include <string.h>
#include <stdio.h>

std::list<std::string> SplitString(const std::string &input, char delimiter)
{
	std::list<std::string> result;
	std::string tmp;
	std::stringstream ss(input);
	while(std::getline(ss, tmp, delimiter))
	{
		result.push_back(tmp);
	}
	return result;
}

void dumpPacket(const char* buffer) 
{
	unsigned int p = 0;
	while(p < strlen((char*)buffer))
	{
		printf("  0x%04x: ", p);
		for(unsigned int i = 0, i2 = p; i < 16; i++, i2++)
		{
			if (i2 < strlen((char*)buffer))
				printf(" %02x", 0xff & buffer[i2]);
			else
				printf("   ");
		}
		printf("    ");
		for(unsigned int i = 0, i2 = p; i < 16; i++, i2++)
		{
			if (i2 < strlen((char*)buffer))
				printf("%c", isprint(buffer[i2])?buffer[i2]:'.');
			else
				printf(" ");
		}
		p += 16;
		printf("\n");
	}
}
