/*
   Mobile Mouse Linux Server 
   Copyright (C) 2011 Erik Lax <erik@datahack.se>
   Copyright (C) Avahi Project (skeleton code)

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

#include "avahi.hpp"

#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-common/thread-watch.h>
#include <avahi-common/error.h>
#include <assert.h>

#include <syslog.h>

static AvahiEntryGroup *avahiGroup = NULL;

void AvahiGroupCallback(AvahiEntryGroup *_avahiGroup, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void *userdata)
{
	assert(_avahiGroup == avahiGroup || avahiGroup == NULL);
	avahiGroup = _avahiGroup;
	switch (state)
	{
		case AVAHI_ENTRY_GROUP_ESTABLISHED:
			{
				syslog(LOG_INFO, "avahi successfully established");
			}
			break;
		case AVAHI_ENTRY_GROUP_COLLISION:
		case AVAHI_ENTRY_GROUP_FAILURE:
			{
				syslog(LOG_ERR, "avahi entry group failure: %s", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(avahiGroup))));
			}
			break;
		case AVAHI_ENTRY_GROUP_UNCOMMITED:
		case AVAHI_ENTRY_GROUP_REGISTERING:
			{
			}
			break;
	}
	return;
}

void AvahiCreateService(AvahiClient *avahiClient, Configuration* appConfig)
{
	assert(avahiClient);
	if (!avahiGroup)
	{
		avahiGroup = avahi_entry_group_new(avahiClient, AvahiGroupCallback, (void*)appConfig);
		if (!avahiGroup)
		{
			syslog(LOG_ERR, "avahi_entry_group_new() failed: %s", avahi_strerror(avahi_client_errno(avahiClient)));
			return;
		}
	}
	if (avahi_entry_group_is_empty(avahiGroup))
	{
		int ret = avahi_entry_group_add_service(
				avahiGroup,
				AVAHI_IF_UNSPEC,
				AVAHI_PROTO_UNSPEC,
				(AvahiPublishFlags)0,
				appConfig->getHostname().c_str(),
				"_mobileremote._tcp",
				NULL,
				NULL,
				appConfig->getPort(),
				NULL,
				NULL,
				NULL
				);
		if (ret < 0)
		{
			syslog(LOG_ERR, "avahi_entry_group_add_service() failed: %s", avahi_strerror(ret));
			return;
		}
		ret = avahi_entry_group_commit(avahiGroup);
		if (ret < 0)
		{
			syslog(LOG_ERR, "avahi_entry_group_commit() failed: %s", avahi_strerror(ret));
			return;
		}
	}
	return;
}

void AvahiCallback(AvahiClient *avahiClient, AvahiClientState state, void *data)
{
	assert(avahiClient);
	assert(data);

	Configuration* appConfig = static_cast<Configuration*>(data);

	switch (state)
	{
		case AVAHI_CLIENT_S_RUNNING:
			{
				AvahiCreateService(avahiClient, appConfig);
			}
			break;
		case AVAHI_CLIENT_FAILURE:
			{
				syslog(LOG_ERR, "avahi client failure: %s", avahi_strerror(avahi_client_errno(avahiClient)));
			}
			break;
		case AVAHI_CLIENT_S_COLLISION:
		case AVAHI_CLIENT_S_REGISTERING:
			{
				if (avahiGroup)
					avahi_entry_group_reset(avahiGroup);
			}
			break;
		case AVAHI_CLIENT_CONNECTING:
			{
			}
			break;
	}
}

void StartAvahi(Configuration& appConfig)
{
	AvahiThreadedPoll* avahiThread = avahi_threaded_poll_new(); /* lost to never be found.. */
	if (!avahiThread)
	{
		syslog(LOG_ERR, "avahi_threaded_poll_new failed");
		return;
	}
	int error;
	AvahiClient* avahiClient = avahi_client_new(
			avahi_threaded_poll_get(avahiThread),
			(AvahiClientFlags)0,
			AvahiCallback,
			&appConfig,
			&error
			);
	if (!avahiClient)
	{
		syslog(LOG_ERR, "avahi_client_new failed: %s", avahi_strerror(error));
		return;
	}
	if (avahi_threaded_poll_start(avahiThread) < 0)
	{
		syslog(LOG_ERR, "avahi_threaded_poll_start failed");
		return;
	}
}
