/*
* Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef DCPLUSPLUS_MAINCONSOLE
#define DCPLUSPLUS_MAINCONSOLE

/*#include "../client/concurrency.h"

#include "../client/ClientManagerListener.h"
#include "../client/TimerManager.h"
#include "../client/FavoriteManager.h"
#include "../client/QueueManagerListener.h"
#include "../client/LogManagerListener.h"
#include "../client/SettingsManager.h"
#include "../client/DirectoryListingManagerListener.h"
#include "../client/UpdateManagerListener.h"
#include "../client/ShareScannerManager.h"*/

//#include <client/forward.h>
//#include <client/typedefs.h>

#include <client/stdinc.h>
#include <client/LogManagerListener.h>
#include <client/ClientManagerListener.h>
#include <client/ClientListener.h>
#include <client/Client.h>
#include <client/ChatMessage.h>

using namespace dcpp;
//using namespace std;

class MainConsole : public LogManagerListener, public ClientManagerListener, public ClientListener {
public:
	void run();
private:
	void on(LogManagerListener::Message, time_t t, const string& m, uint8_t sev) noexcept;

	void on(ClientManagerListener::ClientCreated, Client*) noexcept;

	vector<Client*> clients;

	// ClientListener
	void on(ClientListener::Connecting, const Client*) noexcept;
	void on(ClientListener::Connected, const Client*) noexcept;
	//void on(UserConnected, const Client*, const OnlineUserPtr&) noexcept;
	//void on(UserUpdated, const Client*, const OnlineUserPtr&) noexcept;
	//void on(UsersUpdated, const Client*, const OnlineUserList&) noexcept;
	//void on(ClientListener::UserRemoved, const Client*, const OnlineUserPtr&) noexcept;
	void on(ClientListener::Redirect, const Client*, const string&) noexcept;
	void on(ClientListener::Failed, const string&, const string&) noexcept;
	void on(ClientListener::GetPassword, const Client*) noexcept;
	void on(ClientListener::HubUpdated, const Client*) noexcept;
	void on(ClientListener::Message, const Client*, const ChatMessage&) noexcept;
	void on(ClientListener::StatusMessage, const Client*, const string&, int = ClientListener::FLAG_NORMAL) noexcept;
	void on(ClientListener::NickTaken, const Client*) noexcept;
	void on(ClientListener::HubTopic, const Client*, const string&) noexcept;
	void on(ClientListener::AddLine, const Client*, const string&) noexcept;
};

#endif