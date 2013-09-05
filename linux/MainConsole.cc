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

#include <iostream>

#include "MainConsole.h"

#include <client/ConnectivityManager.h>
#include <client/LogManager.h>
#include <client/FavoriteManager.h>
#include <client/ClientManager.h>
#include <client/Thread.h>
#include <client/SettingsManager.h>

void MainConsole::run() {
	LogManager::getInstance()->addListener(this);
	ClientManager::getInstance()->addListener(this);
	try {
		ConnectivityManager::getInstance()->setup(true, true);
	} catch (const Exception& e) {
		std::cout << "ConnectivityManager failed" << std::endl;
		//showPortsError(e.getError());
	}

	if (SETTING(NICK).empty()) {
		SettingsManager::getInstance()->set(SettingsManager::NICK, "AirDC++_linux");
	}

	FavoriteManager::getInstance()->autoConnect();

	for (;;)
		Thread::sleep(1000);

	for (auto c : clients)
		c->removeListener(this);

	ClientManager::getInstance()->removeListener(this);
	LogManager::getInstance()->removeListener(this);
}

void MainConsole::on(LogManagerListener::Message, time_t t, const string& m, uint8_t sev) noexcept{
	std::cout << dcpp::Text::fromUtf8(m) << std::endl;
}

void MainConsole::on(ClientManagerListener::ClientCreated, Client* c) noexcept{
	std::cout << dcpp::Text::fromUtf8(c->getHubUrl()) + " created" << std::endl;
	clients.push_back(c);
	c->addListener(this);
	c->connect();
}


void MainConsole::on(ClientListener::Connecting, const Client* c) noexcept{
	std::cout << dcpp::Text::fromUtf8(c->getHubUrl()) + " connecting" << std::endl;
}

void MainConsole::on(ClientListener::Connected, const Client* c) noexcept{
	std::cout << dcpp::Text::fromUtf8(c->getHubUrl()) + " connected" << std::endl;
}

void MainConsole::on(ClientListener::Redirect, const Client*, const string&) noexcept{

}

void MainConsole::on(ClientListener::Failed, const string& aHub, const string& aReason) noexcept{
	std::cout << dcpp::Text::fromUtf8(aHub + " disconnected: " + aReason) << std::endl;
}

void MainConsole::on(ClientListener::GetPassword, const Client*) noexcept{

}

void MainConsole::on(ClientListener::HubUpdated, const Client*) noexcept{

}

void MainConsole::on(ClientListener::Message, const Client* c, const ChatMessage& aMsg) noexcept{
	std::cout << dcpp::Text::fromUtf8(c->getHubUrl() + ": " + aMsg.format()) << std::endl;
}

void MainConsole::on(ClientListener::StatusMessage, const Client* c, const string& aMsg, int /*ClientListener::FLAG_NORMAL*/) noexcept{
	std::cout << dcpp::Text::fromUtf8(c->getHubUrl() + ": " + aMsg) << std::endl;
}

void MainConsole::on(ClientListener::NickTaken, const Client*) noexcept{

}

void MainConsole::on(ClientListener::HubTopic, const Client*, const string&) noexcept{

}
void MainConsole::on(ClientListener::AddLine, const Client*, const string&) noexcept{

}