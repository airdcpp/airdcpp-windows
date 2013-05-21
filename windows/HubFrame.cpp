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

#include "stdafx.h"
#include "Resource.h"

#include "HubFrame.h"
#include "LineDlg.h"
#include "SearchFrm.h"
#include "PrivateFrame.h"
#include "TextFrame.h"
#include "ResourceLoader.h"
#include "MainFrm.h"
#include "IgnoreManager.h"

#include "../client/DirectoryListingManager.h"
#include "../client/ChatMessage.h"
#include "../client/QueueManager.h"
#include "../client/ShareManager.h"
#include "../client/UploadManager.h"
#include "../client/Util.h"
#include "../client/FavoriteManager.h"
#include "../client/LogManager.h"
#include "../client/SettingsManager.h"
#include "../client/Wildcards.h"
#include "../client/ColorSettings.h"
#include "../client/HighlightManager.h"
#include "../client/Localization.h"
#include "../client/GeoManager.h"

HubFrame::FrameMap HubFrame::frames;
HubFrame::IgnoreMap HubFrame::ignoreList;
bool HubFrame::shutdown = false;

int HubFrame::columnSizes[] = { 100, 75, 75, 75, 100, 75, 130, 130, 100, 50, 40, 40, 40, 40, 40, 300 };
int HubFrame::columnIndexes[] = { OnlineUser::COLUMN_NICK, OnlineUser::COLUMN_SHARED, OnlineUser::COLUMN_EXACT_SHARED,
	OnlineUser::COLUMN_DESCRIPTION, OnlineUser::COLUMN_TAG,	OnlineUser::COLUMN_ULSPEED, OnlineUser::COLUMN_DLSPEED, OnlineUser::COLUMN_IP4, OnlineUser::COLUMN_IP6, OnlineUser::COLUMN_EMAIL,
	OnlineUser::COLUMN_VERSION, OnlineUser::COLUMN_MODE4, OnlineUser::COLUMN_MODE6, OnlineUser::COLUMN_FILES, OnlineUser::COLUMN_HUBS, OnlineUser::COLUMN_SLOTS, OnlineUser::COLUMN_CID };

ResourceManager::Strings HubFrame::columnNames[] = { ResourceManager::NICK, ResourceManager::SHARED, ResourceManager::EXACT_SHARED, 
	ResourceManager::DESCRIPTION, ResourceManager::TAG, ResourceManager::SETCZDC_UPLOAD_SPEED, ResourceManager::SETCZDC_DOWNLOAD_SPEED, ResourceManager::IP_V4, ResourceManager::IP_V6, ResourceManager::EMAIL,
	ResourceManager::VERSION, ResourceManager::MODE_V4, ResourceManager::MODE_V6, ResourceManager::SHARED_FILES, ResourceManager::HUBS, ResourceManager::SLOTS, ResourceManager::CID };

LRESULT HubFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	ctrlClient.setClient(client);
	init(m_hWnd, rcDefault);
	ctrlMessageContainer.SubclassWindow(ctrlMessage.m_hWnd);
	ctrlClientContainer.SubclassWindow(ctrlClient.m_hWnd);

	ctrlUsers.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_USERS);
	ctrlUsers.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);

	SetSplitterPanes(ctrlClient.m_hWnd, ctrlUsers.m_hWnd, false);
	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
	
	if(hubchatusersplit) {
		m_nProportionalPos = hubchatusersplit;
	} else {
		m_nProportionalPos = 7500;
	}

	ctrlShowUsers.Create(ctrlStatus.m_hWnd, rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	ctrlShowUsers.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlShowUsers.SetFont(WinUtil::systemFont);
	ctrlShowUsers.SetCheck(showUsers ? BST_CHECKED : BST_UNCHECKED);
	ctrlShowUsersContainer.SubclassWindow(ctrlShowUsers.m_hWnd);
	
	CToolInfo ti(TTF_SUBCLASS, ctrlStatus.m_hWnd);
	ctrlTooltips.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON, WS_EX_TOPMOST);	 
	ctrlTooltips.SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);	 
	ctrlTooltips.AddTool(&ti);	 

	CToolInfo ti2(TTF_SUBCLASS, ctrlShowUsers.m_hWnd);
	ti2.cbSize = sizeof(CToolInfo);
	ti2.lpszText = (LPWSTR)CTSTRING(SHOW_USERLIST);
	ctrlTooltips.AddTool(&ti2);
	ctrlTooltips.SetDelayTime(TTDT_AUTOPOP, 15000);
	ctrlTooltips.Activate(TRUE);

	const FavoriteHubEntry *fhe = FavoriteManager::getInstance()->getFavoriteHubEntry(Text::fromT(server));
	if(fhe) {
		WinUtil::splitTokens(columnIndexes, fhe->getHeaderOrder(), OnlineUser::COLUMN_LAST);
		WinUtil::splitTokens(columnSizes, fhe->getHeaderWidths(), OnlineUser::COLUMN_LAST);
	} else {
		WinUtil::splitTokens(columnIndexes, SETTING(HUBFRAME_ORDER), OnlineUser::COLUMN_LAST);
		WinUtil::splitTokens(columnSizes, SETTING(HUBFRAME_WIDTHS), OnlineUser::COLUMN_LAST);                           
	}
    	
	for(uint8_t j = 0; j < OnlineUser::COLUMN_LAST; ++j) {
		ColumnType ctp = (j == OnlineUser::COLUMN_SHARED || 
			j == OnlineUser::COLUMN_EXACT_SHARED || 
			j == OnlineUser::COLUMN_SLOTS) ? COLUMN_NUMERIC : COLUMN_TEXT;
		int fmt = (j == OnlineUser::COLUMN_SHARED || j == OnlineUser::COLUMN_EXACT_SHARED || j == OnlineUser::COLUMN_SLOTS) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlUsers.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j, ctp);
	}

	filter.addFilterBox(m_hWnd);
	filter.addColumnBox(m_hWnd, ctrlUsers.getColumnList());
	filter.addMethodBox(m_hWnd);
	ctrlFilterContainer.SubclassWindow(filter.getFilterBox().m_hWnd);
	ctrlFilterSelContainer.SubclassWindow(filter.getFilterColumnBox().m_hWnd);
	ctrlFilterMethodContainer.SubclassWindow(filter.getFilterMethodBox().m_hWnd);

	ctrlUsers.setColumnOrderArray(OnlineUser::COLUMN_LAST, columnIndexes);
	
	if(fhe) {
		ctrlUsers.setVisible(fhe->getHeaderVisible());
    } else {
	    ctrlUsers.setVisible(SETTING(HUBFRAME_VISIBLE));
    }
	
	ctrlUsers.SetFont(WinUtil::listViewFont); //this will also change the columns font
	ctrlUsers.SetBkColor(WinUtil::bgColor);
	ctrlUsers.SetTextBkColor(WinUtil::bgColor);
	ctrlUsers.SetTextColor(WinUtil::textColor);
	ctrlUsers.setFlickerFree(WinUtil::bgBrush);
	ctrlUsers.setSortColumn(OnlineUser::COLUMN_NICK);
	ctrlUsers.SetImageList(ResourceLoader::getUserImages(), LVSIL_SMALL);

	WinUtil::SetIcon(m_hWnd, IDI_HUB);

	HubOpIcon = ResourceLoader::loadIcon(IDI_HUBOP, 16);
	HubRegIcon = ResourceLoader::loadIcon(IDI_HUBREG, 16);
	HubIcon =  ResourceLoader::loadIcon(IDI_HUB, 16);

	if(fhe != NULL){
		//retrieve window position
		CRect rc(fhe->getLeft(), fhe->getTop(), fhe->getRight(), fhe->getBottom());
		
		//check that we have a window position stored
		if(! (rc.top == 0 && rc.bottom == 0 && rc.left == 0 && rc.right == 0) )
			MoveWindow(rc, TRUE);
	}
	
	bHandled = FALSE;
	client->connect();

	FavoriteManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	::SetTimer(m_hWnd, 0, 500, 0);
	return 1;
}

void HubFrame::openWindow(const tstring& aServer, int chatusersplit, bool userliststate, ProfileToken aShareProfile, string sColumsOrder, string sColumsWidth, string sColumsVisible) {
	FrameIter i = frames.find(aServer);
	if(i == frames.end()) {
		HubFrame* frm = new HubFrame(aServer, chatusersplit, userliststate, aShareProfile);
		frames[aServer] = frm;

//		int nCmdShow = SW_SHOWDEFAULT;
		frm->CreateEx(WinUtil::mdiClient, frm->rcDefault);
//		if(windowtype)
//			frm->ShowWindow(((nCmdShow == SW_SHOWDEFAULT) || (nCmdShow == SW_SHOWNORMAL)) ? windowtype : nCmdShow);
	} else {
		if(::IsIconic(i->second->m_hWnd))
			::ShowWindow(i->second->m_hWnd, SW_RESTORE);
		i->second->MDIActivate(i->second->m_hWnd);
	}
}

HubFrame::~HubFrame() {
	ClientManager::getInstance()->putClient(client);

	dcassert(frames.find(server) != frames.end());
	dcassert(frames[server] == this);
	frames.erase(server);
	clearTaskList();
}

HubFrame::HubFrame(const tstring& aServer, int chatusersplit, bool userliststate, ProfileToken aShareProfile) : 
		waitingForPW(false), extraSort(false), server(aServer), closed(false), forceClose(false),
		showUsers(SETTING(GET_USER_INFO)), updateUsers(false), resort(false), countType(Client::COUNT_NORMAL),
		timeStamps(SETTING(TIME_STAMPS)),
		hubchatusersplit(chatusersplit),
		ctrlShowUsersContainer(WC_BUTTON, this, SHOW_USERS),
		ctrlFilterContainer(WC_EDIT, this, FILTER_MESSAGE_MAP),
		ctrlFilterSelContainer(WC_COMBOBOX, this, FILTER_MESSAGE_MAP),
		ctrlFilterMethodContainer(WC_COMBOBOX, this, FILTER_MESSAGE_MAP),
		ctrlMessageContainer(WC_EDIT, this, EDIT_MESSAGE_MAP),
		ctrlClientContainer(WC_EDIT, this, EDIT_MESSAGE_MAP),
		filter(OnlineUser::COLUMN_LAST, [this] { updateUserList(); updateUsers = true;}),
		statusDirty(true)
{
	client = ClientManager::getInstance()->createClient(Text::fromT(aServer));
	client->setShareProfile(aShareProfile);
	client->addListener(this);

	if(FavoriteManager::getInstance()->getFavoriteHubEntry(Text::fromT(server)) != NULL) {
		showUsers = userliststate;
	} else {
		showUsers = SETTING(GET_USER_INFO);
	}
		
	memset(statusSizes, 0, sizeof(statusSizes));
}

bool HubFrame::sendMessage(const tstring& aMessage, string& error_, bool isThirdPerson) {
	return client->hubMessage(Text::fromT(aMessage), error_, isThirdPerson);
}

bool HubFrame::checkFrameCommand(tstring& cmd, tstring& param, tstring& /*message*/, tstring& status, bool& /*thirdPerson*/) {	
	if(stricmp(cmd.c_str(), _T("join"))==0) {
		if(!param.empty()) {
			redirect = param;
			if(SETTING(JOIN_OPEN_NEW_WINDOW)) {
				HubFrame::openWindow(param);
			} else {
				BOOL whatever = FALSE;
				onFollow(0, 0, 0, whatever);
			}
		} else {
			status = TSTRING(SPECIFY_SERVER);
		}
	} else if(stricmp(cmd.c_str(), _T("ts")) == 0) {
		timeStamps = !timeStamps;
		if(timeStamps) {
			status = TSTRING(TIMESTAMPS_ENABLED);
		} else {
			status = TSTRING(TIMESTAMPS_DISABLED);
		}
	} else if( (stricmp(cmd.c_str(), _T("password")) == 0) && waitingForPW ) {
		client->setPassword(Text::fromT(param));
		client->password(Text::fromT(param));
		waitingForPW = false;
	} else if( stricmp(cmd.c_str(), _T("showjoins")) == 0 ) {
		if(client->changeBoolHubSetting(HubSettings::ShowJoins)) {
			status = TSTRING(JOIN_SHOWING_ON);
		} else {
			status = TSTRING(JOIN_SHOWING_OFF);
		}
	} else if( stricmp(cmd.c_str(), _T("favshowjoins")) == 0 ) {
		if(client->changeBoolHubSetting(HubSettings::FavShowJoins)) {
			status = TSTRING(FAV_JOIN_SHOWING_ON);
		} else {
			status = TSTRING(FAV_JOIN_SHOWING_OFF);
		}
	} else if(stricmp(cmd.c_str(), _T("close")) == 0) {
		PostMessage(WM_CLOSE);
	} else if(stricmp(cmd.c_str(), _T("userlist")) == 0) {
		ctrlShowUsers.SetCheck(showUsers ? BST_UNCHECKED : BST_CHECKED);
	} else if((stricmp(cmd.c_str(), _T("favorite")) == 0) || (stricmp(cmd.c_str(), _T("fav")) == 0)) {
		addAsFavorite();
	} else if((stricmp(cmd.c_str(), _T("removefavorite")) == 0) || (stricmp(cmd.c_str(), _T("removefav")) == 0)) {
		removeFavoriteHub();
	} else if(stricmp(cmd.c_str(), _T("getlist")) == 0){
		if(!param.empty() ){
			OnlineUserPtr ui = client->findUser(Text::fromT(param));
			if(ui) {
				ui->getList();
			}
		}
	} else if(stricmp(cmd.c_str(), _T("ignorelist"))==0) {
		tstring ignorelist = _T("Ignored users:");
		for(auto i = ignoreList.begin(); i != ignoreList.end(); ++i)
			ignorelist += _T(" ") + Text::toT(ClientManager::getInstance()->getNicks((*i)->getCID())[0]);
		status = ignorelist;
	} else if(stricmp(cmd.c_str(), _T("log")) == 0) {
		WinUtil::openFile(Text::toT(getLogPath(stricmp(param.c_str(), _T("status")) == 0)));
	} else if(stricmp(cmd.c_str(), _T("help")) == 0) {
		status = _T("*** ") + ChatFrameBase::commands + _T(", /join <hub-ip>, /ts, /showjoins, /favshowjoins, /close, /userlist, /favorite, /pm <user> [message], /getlist <user>, /ignorelist, /removefavorite");
	} else if(stricmp(cmd.c_str(), _T("pm")) == 0) {
		string::size_type j = param.find(_T(' '));
		if(j != string::npos) {
			tstring nick = param.substr(0, j);
			const OnlineUserPtr ui = client->findUser(Text::fromT(nick));

			if(ui) {
				if(param.size() > j + 1)
					PrivateFrame::openWindow(HintedUser(ui->getUser(), client->getHubUrl()), param.substr(j+1), client);
				else
					PrivateFrame::openWindow(HintedUser(ui->getUser(), client->getHubUrl()), Util::emptyStringT, client);
			}
		} else if(!param.empty()) {
			const OnlineUserPtr ui = client->findUser(Text::fromT(param));
			if(ui) {
				PrivateFrame::openWindow(HintedUser(ui->getUser(), client->getHubUrl()), Util::emptyStringT, client);
			}
		}
	} else if(stricmp(cmd.c_str(), _T("topic")) == 0) {
		addLine(_T("*** ") + Text::toT(client->getHubDescription()));
	} else if(stricmp(cmd.c_str(), _T("ctopic")) == 0) {
		openLinksInTopic();
	} else {
		return false;
	}

	return true;
}

struct CompareItems {
	CompareItems(uint8_t aCol) : col(aCol) { }
	bool operator()(const OnlineUser& a, const OnlineUser& b) const {
		return OnlineUser::compareItems(&a, &b, col) < 0;
	}
	const uint8_t col;
};

void HubFrame::addAsFavorite() {
	const FavoriteHubEntry* existingHub = FavoriteManager::getInstance()->getFavoriteHubEntry(client->getHubUrl());
	if(!existingHub) {
		FavoriteHubEntry aEntry;
		TCHAR buf[256];
		this->GetWindowText(buf, 255);
		aEntry.setServerStr(Text::fromT(server));
		aEntry.setName(Text::fromT(buf));
		aEntry.setDescription(Text::fromT(buf));
		aEntry.setConnect(true);
		aEntry.setShareProfile(ShareManager::getInstance()->getShareProfile(client->getShareProfile(), true));
		if(!client->getPassword().empty()) {
			aEntry.setPassword(client->getPassword());
		}
		aEntry.setConnect(true);
		FavoriteManager::getInstance()->addFavorite(aEntry);
		addStatus(TSTRING(FAVORITE_HUB_ADDED), WinUtil::m_ChatTextSystem );
	} else {
		addStatus(TSTRING(FAVORITE_HUB_ALREADY_EXISTS), WinUtil::m_ChatTextSystem);
	}
}

void HubFrame::removeFavoriteHub() {
	const FavoriteHubEntry* removeHub = FavoriteManager::getInstance()->getFavoriteHubEntry(client->getHubUrl());
	if(removeHub) {
		FavoriteManager::getInstance()->removeFavorite(removeHub);
		addStatus(TSTRING(FAVORITE_HUB_REMOVED), WinUtil::m_ChatTextSystem);
	} else {
		addStatus(TSTRING(FAVORITE_HUB_DOES_NOT_EXIST), WinUtil::m_ChatTextSystem);
	}
}

LRESULT HubFrame::onCopyHubInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if(client->isConnected()) {
        string sCopy;

		switch (wID) {
			case IDC_COPY_HUBNAME:
				sCopy += client->getHubName();
				break;
			case IDC_COPY_HUBADDRESS:
				sCopy += client->getHubUrl();
				break;
		}

		if (!sCopy.empty())
			WinUtil::setClipboard(Text::toT(sCopy));
    }
	return 0;
}

LRESULT HubFrame::onCopyUserInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring sCopy;

	int sel = -1;
	while((sel = ctrlUsers.GetNextItem(sel, LVNI_SELECTED)) != -1) {
		const OnlineUserPtr ou = ctrlUsers.getItemData(sel);
	
		if(!sCopy.empty())
			sCopy += _T("\r\n");

		sCopy += ou->getText(static_cast<uint8_t>(wID - IDC_COPY), true);
	}
	if (!sCopy.empty())
		WinUtil::setClipboard(sCopy);

	return 0;
}
LRESULT HubFrame::onCopyAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring sCopy;

	int sel = -1;
	while((sel = ctrlUsers.GetNextItem(sel, LVNI_SELECTED)) != -1) {
		if (!sCopy.empty())
			sCopy += _T("\r\n\r\n");
		sCopy += ctrlUsers.GetColumnTexts(sel);
	}

	if (!sCopy.empty())
		WinUtil::setClipboard(sCopy);

	return 0;
}

LRESULT HubFrame::onDoubleClickUsers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;
	if(item->iItem != -1 && (ctrlUsers.getItemData(item->iItem)->getUser() != ClientManager::getInstance()->getMe())) {
	    switch(SETTING(USERLIST_DBLCLICK)) {
		    case 0:
				ctrlUsers.getItemData(item->iItem)->getList();
		        break;
		    case 1: {
				tstring sUser = Text::toT(ctrlUsers.getItemData(item->iItem)->getIdentity().getNick());
	            int iSelBegin, iSelEnd;
	            ctrlMessage.GetSel(iSelBegin, iSelEnd);

	            if((iSelBegin == 0) && (iSelEnd == 0)) {
					sUser += _T(": ");
					if(ctrlMessage.GetWindowTextLength() == 0) {
			            ctrlMessage.SetWindowText(sUser.c_str());
			            ctrlMessage.SetFocus();
			            ctrlMessage.SetSel(ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength());
                    } else {
			            ctrlMessage.ReplaceSel(sUser.c_str());
			            ctrlMessage.SetFocus();
                    }
				} else {
					sUser += _T(" ");
                    ctrlMessage.ReplaceSel(sUser.c_str());
                    ctrlMessage.SetFocus();
	            }
				break;
		    }    
		    case 2:
				ctrlUsers.getItemData(item->iItem)->pm();
		        break;
		    case 3:
		        ctrlUsers.getItemData(item->iItem)->matchQueue();
		        break;
		    case 4:
		        ctrlUsers.getItemData(item->iItem)->grant();
		        break;
		    case 5:
		        ctrlUsers.getItemData(item->iItem)->addFav();
		        break;
			case 6:
				ctrlUsers.getItemData(item->iItem)->browseList();
				break;
		}	
	}
	return 0;
}

bool HubFrame::updateUser(const UserTask& u) {
	if(!showUsers) return false;
	
	if(!u.onlineUser->isInList) {
		u.onlineUser->update(-1);

		if(!u.onlineUser->isHidden()) {
			u.onlineUser->inc();
			ctrlUsers.insertItem(u.onlineUser.get(), UserInfoBase::getImage(u.onlineUser->getIdentity(), client));
		}

		if(!filter.empty())
			updateUserList(u.onlineUser);
		return true;
	} else {
		const int pos = ctrlUsers.findItem(u.onlineUser.get());

		if(pos != -1) {
			TCHAR buf[255];
			ListView_GetItemText(ctrlUsers, pos, ctrlUsers.getSortColumn(), buf, 255);
			
			resort = u.onlineUser->update(ctrlUsers.getSortColumn(), buf) || resort;
			if(u.onlineUser->isHidden()) {
				ctrlUsers.DeleteItem(pos);
				u.onlineUser->dec();				
			} else {
				ctrlUsers.updateItem(pos);
				ctrlUsers.SetItem(pos, 0, LVIF_IMAGE, NULL, UserInfoBase::getImage(u.onlineUser->getIdentity(), client), 0, 0, NULL);
			}
		}

		u.onlineUser->getIdentity().set("WO", u.onlineUser->getIdentity().isOp() ? "1" : Util::emptyString);
		updateUserList(u.onlineUser);
		return false;
	}
}

void HubFrame::removeUser(const OnlineUserPtr& aUser) {
	if(!showUsers) return;
	
	if(!aUser->isHidden()) {
		int i = ctrlUsers.findItem(aUser.get());
		if(i != -1) {
			ctrlUsers.DeleteItem(i);
			aUser->dec();
		}
	}
}

void HubFrame::onPrivateMessage(const ChatMessage& message) {
	const auto& identity = message.replyTo->getIdentity();
	tstring nick = Text::toT(identity.getNick());

	bool ignore = false, window = false;
	if(!message.from->getUser() || (ignoreList.find(message.from->getUser()) == ignoreList.end()) || (message.from->getIdentity().isOp() && !client->isOp())) {
		bool myPM = message.replyTo->getUser() == ClientManager::getInstance()->getMe();
		const UserPtr& user = myPM ? message.to->getUser() : message.replyTo->getUser();
		auto text = message.format();

		if(identity.isHub()) {
			if(SETTING(IGNORE_HUB_PMS)) {
				ignore = true;
			} else if(SETTING(POPUP_HUB_PMS) || PrivateFrame::isOpen(user)) {
				window = true;
			}
		} else if(identity.isBot()) {
			if(SETTING(IGNORE_BOT_PMS)) {
				ignore = true;
			} else if(SETTING(POPUP_BOT_PMS) || PrivateFrame::isOpen(user)) {
				window = true;
			}
		} else if(SETTING(POPUP_PMS) || PrivateFrame::isOpen(user) || myPM) {
			window = true;
		}

		if(ignore) {
			addStatus(TSTRING(IGNORED_MESSAGE) + _T(" ") + Text::toT(text), WinUtil::m_ChatTextSystem, false);
		} else {
			if (window) {
				PrivateFrame::gotMessage(message.from->getIdentity(), message.to->getUser(), message.replyTo->getUser(), Text::toT(text), client);
			} else {
				addLine(TSTRING(PRIVATE_MESSAGE_FROM) + _T(" ") + nick + _T(": ") + Text::toT(text), WinUtil::m_ChatTextPrivate);
			}

			if (!identity.isHub() && !identity.isBot()) {
				MainFrame::getMainFrame()->onChatMessage(true);
			}
		}
	}
}

void HubFrame::onChatMessage(const ChatMessage& msg) {
	const auto& identity = msg.from->getIdentity();
	if(!msg.from->getUser() || (ignoreList.find(msg.from->getUser()) == ignoreList.end()) || (identity.isOp() && !client->isOp())) {
		addLine(msg.from->getIdentity(), Text::toT(msg.format()), WinUtil::m_ChatTextGeneral);
		if(client->get(HubSettings::ChatNotify)) {
			MainFrame::getMainFrame()->onChatMessage(false);
		}
	}
}

void HubFrame::onDisconnected() {
	clearUserList();
	setDisconnected(true);
	wentoffline = true;

	if ((!SETTING(SOUND_HUBDISCON).empty()) && (!SETTING(SOUNDS_DISABLED)))
		WinUtil::playSound(Text::toT(SETTING(SOUND_HUBDISCON)));

	if(SETTING(POPUP_HUB_DISCONNECTED)) {
		WinUtil::showPopup(Text::toT(client->getAddress()), TSTRING(DISCONNECTED));
	}
}

void HubFrame::onConnected() {
	addStatus(TSTRING(CONNECTED), WinUtil::m_ChatTextServer);
	wentoffline = false;
	setDisconnected(false);

	tstring text = Text::toT(client->getCipherName());
	ctrlStatus.SetText(1, text.c_str());
	statusSizes[0] = WinUtil::getTextWidth(text, ctrlStatus.m_hWnd);

	if(SETTING(POPUP_HUB_CONNECTED)) {
		WinUtil::showPopup(Text::toT(client->getAddress()), TSTRING(CONNECTED));
	}

	if ((!SETTING(SOUND_HUBCON).empty()) && (!SETTING(SOUNDS_DISABLED)))
		WinUtil::playSound(Text::toT(SETTING(SOUND_HUBCON)));
}

void HubFrame::setWindowTitle(const string& aTitle) {
	SetWindowText(Text::toT(aTitle).c_str());
	MDIRefreshMenu();
}

void HubFrame::execTasks() {
	TaskQueue::List tl;
	tasks.get(tl);

	ctrlUsers.SetRedraw(FALSE);

	for(auto& t: tl) {
		if(t.first == UPDATE_USER) {

			updateUser(static_cast<UserTask&>(*t.second));
			/*UserTask& u = static_cast<UserTask&>(*t.second);
			if(IgnoreManager::getInstance()->isIgnored(u.onlineUser->getIdentity().getNick())) {
				ignoreList.insert(u.onlineUser->getUser());
			} else if(!IgnoreManager::getInstance()->isIgnored(u.onlineUser->getIdentity().getNick()) && (ignoreList.find(u.onlineUser->getUser()) != ignoreList.end())) {
				ignoreList.erase(u.onlineUser->getUser());
			}*/
			//updateUser(u);
		} else if(t.first == UPDATE_USER_JOIN) {
			UserTask& u = static_cast<UserTask&>(*t.second);
			if(updateUser(u)) {
				bool isFavorite = FavoriteManager::getInstance()->isFavoriteUser(u.onlineUser->getUser());
				if (isFavorite && (!SETTING(SOUND_FAVUSER).empty()) && (!SETTING(SOUNDS_DISABLED)))
					WinUtil::playSound(Text::toT(SETTING(SOUND_FAVUSER)));

				if(isFavorite && SETTING(POPUP_FAVORITE_CONNECTED)) {
					WinUtil::showPopup(Text::toT(u.onlineUser->getIdentity().getNick() + " - " + client->getHubName()), TSTRING(FAVUSER_ONLINE));
				}

				if (!u.onlineUser->isHidden() && client->get(HubSettings::ShowJoins) || (client->get(HubSettings::FavShowJoins) && isFavorite)) {
				 	addLine(_T("*** ") + TSTRING(JOINS) + Text::toT(u.onlineUser->getIdentity().getNick()), WinUtil::m_ChatTextSystem, SETTING(HUB_BOLD_TABS));
				}	
			}
		} else if(t.first == REMOVE_USER) {
			const UserTask& u = static_cast<UserTask&>(*t.second);
			removeUser(u.onlineUser);

			if (!u.onlineUser->isHidden() && client->get(HubSettings::ShowJoins) || (client->get(HubSettings::FavShowJoins) && FavoriteManager::getInstance()->isFavoriteUser(u.onlineUser->getUser()))) {
				addLine(Text::toT("*** " + STRING(PARTS) + " " + u.onlineUser->getIdentity().getNick()), WinUtil::m_ChatTextSystem, SETTING(HUB_BOLD_TABS));
			}
		}
	}
	
	if(resort && showUsers) {
		ctrlUsers.resort();
		resort = false;
	}

	ctrlUsers.SetRedraw(TRUE);
}

void HubFrame::onPassword() {
	if(client->getPassword().size() > 0) {
		client->password(client->getPassword());
		addStatus(TSTRING(STORED_PASSWORD_SENT), WinUtil::m_ChatTextSystem);
	} else {
		if(!SETTING(PROMPT_PASSWORD)) {
			ctrlMessage.SetWindowText(_T("/password "));
			ctrlMessage.SetFocus();
			ctrlMessage.SetSel(10, 10);
			waitingForPW = true;
		} else {
			LineDlg linePwd;
			linePwd.title = CTSTRING(ENTER_PASSWORD);
			linePwd.description = CTSTRING(ENTER_PASSWORD);
			linePwd.password = true;
			if(linePwd.DoModal(m_hWnd) == IDOK) {
				client->setPassword(Text::fromT(linePwd.line));
				client->password(Text::fromT(linePwd.line));
				waitingForPW = false;
			} else {
				client->disconnect(true);
			}
		}
	}
}

void HubFrame::onUpdateTabIcons() {
	switch(countType) {
	case Client::COUNT_OP:
		setIcon(HubOpIcon);
		break;
	case Client::COUNT_REGISTERED:
		setIcon(HubRegIcon);
		break;
	case Client::COUNT_NORMAL:
		setIcon(HubRegIcon);
		break;
	default:
		break;
	}
}

void HubFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
	
	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[6];
		ctrlStatus.GetClientRect(sr);

		w[0] = sr.right - statusSizes[0] - statusSizes[1] - statusSizes[2] - statusSizes[3] - 16;
		w[1] = w[0] + statusSizes[0];
		w[2] = w[1] + statusSizes[1];
		w[3] = w[2] + statusSizes[2];
		w[4] = w[3] + statusSizes[3];
		w[5] = w[4] + 16;
		
		ctrlStatus.SetParts(6, w);

		ctrlTooltips.SetMaxTipWidth(w[0]);

		// Strange, can't get the correct width of the last field...
		ctrlStatus.GetRect(4, sr);
		sr.left = sr.right;
		sr.right = sr.left + 16;
		ctrlShowUsers.MoveWindow(sr);
	}
		
	int h = WinUtil::fontHeight + 4;

	const int maxLines = resizePressed && (SETTING(MAX_RESIZE_LINES) <= 1) ? 2 : SETTING(MAX_RESIZE_LINES);

	if((maxLines != 1) && lineCount != 0) {
		if(lineCount < maxLines) {
			h = WinUtil::fontHeight * lineCount + 4;
		} else {
			h = WinUtil::fontHeight * maxLines + 4;
		}
	} 

	CRect rc = rect;
	rc.bottom -= h + 10;
	if(!showUsers) {
		if(GetSinglePaneMode() == SPLIT_PANE_NONE)
			SetSinglePaneMode(SPLIT_PANE_LEFT);
	} else {
		if(GetSinglePaneMode() != SPLIT_PANE_NONE)
			SetSinglePaneMode(SPLIT_PANE_NONE);
	}
	SetSplitterRect(rc);

	int buttonsize = 0;
	
	if(ctrlEmoticons.IsWindow())
		buttonsize +=26;

	if(ctrlMagnet.IsWindow())
		buttonsize += 26;

	if(ctrlResize.IsWindow())
		buttonsize += 26;

	rc = rect;
	rc.bottom -= 2;
	rc.top = rc.bottom - h - 5;
	rc.left +=2;
	rc.right -= (showUsers ? 320 : 0) + buttonsize;
	ctrlMessage.MoveWindow(rc);

//ApexDC
	if(h != (WinUtil::fontHeight + 4)) {
		rc.bottom -= h - (WinUtil::fontHeight + 4);
	}
//end

	if(ctrlResize.IsWindow()) {
		//resize lines button
		rc.left = rc.right + 2;
		rc.right += 24;
		ctrlResize.MoveWindow(rc);
	}

	if(ctrlEmoticons.IsWindow()) {
		rc.left = rc.right + 2;
		rc.right += 24;
		ctrlEmoticons.MoveWindow(rc);
	}
	
	if(ctrlMagnet.IsWindow()){
		//magnet button
		rc.left = rc.right + 2;
		rc.right += 24;
		ctrlMagnet.MoveWindow(rc);
	}

	if(showUsers){
		rc.left = rc.right + 2;
		rc.right = rc.left + 116;
		filter.getFilterBox().MoveWindow(rc);

		rc.left = rc.right + 4;
		rc.right = rc.left + 96;
		rc.top = rc.top + 0;
		rc.bottom = rc.bottom + 120;
		filter.getFilterColumnBox().MoveWindow(rc);

		rc.left = rc.right + 4;
		rc.right = rc.left + 96;
		rc.top = rc.top + 0;
		rc.bottom = rc.bottom + 120;
		filter.getFilterMethodBox().MoveWindow(rc);

	} else {
		rc.left = 0;
		rc.right = 0;
		filter.getFilterBox().MoveWindow(rc);
		filter.getFilterColumnBox().MoveWindow(rc);
		filter.getFilterMethodBox().MoveWindow(rc);
	}
}

LRESULT HubFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		if(shutdown || forceClose ||  WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_HUB_CLOSING, TSTRING(REALLY_CLOSE))) {
			RecentHubEntry* r = FavoriteManager::getInstance()->getRecentHubEntry(Text::fromT(server));
			if(r) {
				TCHAR buf[256];
				this->GetWindowText(buf, 255);
				r->setName(Text::fromT(buf));
				r->setUsers(Util::toString(client->getUserCount()));
				r->setShared(Util::toString(client->getAvailable()));
				FavoriteManager::getInstance()->updateRecent(r);
			}
			DeleteObject(hEmoticonBmp);
			DestroyIcon(HubOpIcon);
			DestroyIcon(HubRegIcon);
			DestroyIcon(HubIcon);

			SettingsManager::getInstance()->removeListener(this);
			FavoriteManager::getInstance()->removeListener(this);
			client->removeListener(this);
			client->disconnect(true);

			closed = true;
			PostMessage(WM_CLOSE);
			return 0;
		}
	} else {
		SettingsManager::getInstance()->set(SettingsManager::GET_USER_INFO, showUsers);
		FavoriteManager::getInstance()->removeUserCommand(Text::fromT(server));

		clearUserList();
		clearTaskList();
				
		string tmp, tmp2, tmp3;
		ctrlUsers.saveHeaderOrder(tmp, tmp2, tmp3);

		FavoriteHubEntry *fhe = FavoriteManager::getInstance()->getFavoriteHubEntry(Text::fromT(server));
		if(fhe != NULL) {
			CRect rc;
			if(!IsIconic()){
				//Get position of window
				GetWindowRect(&rc);
				
				//convert the position so it's relative to main window
				::ScreenToClient(GetParent(), &rc.TopLeft());
				::ScreenToClient(GetParent(), &rc.BottomRight());
				
				//save the position
				fhe->setBottom((uint16_t)(rc.bottom > 0 ? rc.bottom : 0));
				fhe->setTop((uint16_t)(rc.top > 0 ? rc.top : 0));
				fhe->setLeft((uint16_t)(rc.left > 0 ? rc.left : 0));
				fhe->setRight((uint16_t)(rc.right > 0 ? rc.right : 0));
			}

			fhe->setChatUserSplit(m_nProportionalPos);
			fhe->setUserListState(showUsers);
			fhe->setHeaderOrder(tmp);
			fhe->setHeaderWidths(tmp2);
			fhe->setHeaderVisible(tmp3);
			
			FavoriteManager::getInstance()->save();
		} else {
			SettingsManager::getInstance()->set(SettingsManager::HUBFRAME_ORDER, tmp);
			SettingsManager::getInstance()->set(SettingsManager::HUBFRAME_WIDTHS, tmp2);
			SettingsManager::getInstance()->set(SettingsManager::HUBFRAME_VISIBLE, tmp3);
		}
		bHandled = FALSE;
	}
	return 0;
}

void HubFrame::clearUserList() {
	for(auto& u: ctrlUsers)
		u.dec();

	ctrlUsers.DeleteAllItems();
}

void HubFrame::clearTaskList() {
	tasks.clear();
}

LRESULT HubFrame::onLButton(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	HWND focus = GetFocus();
	bHandled = false;
	if(focus == ctrlClient.m_hWnd) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		const int i = ctrlClient.CharFromPos(pt);
		int line = ctrlClient.LineFromChar(i);
		const int c = i - ctrlClient.LineIndex(line);
		const int len = ctrlClient.LineLength(i);
		if(len < 3) {
			return 0;
		}

		TCHAR* buf = new TCHAR[len+1];
		ctrlClient.GetLine(line, buf, len+1);
		tstring x = tstring(buf, len);
		delete[] buf;

		string::size_type start = x.find_last_of(_T(" <\t\r\n"), c);

		if(start == string::npos)
			start = 0;
		else
			start++;
					

		string::size_type end = x.find_first_of(_T(" >\t"), start+1);

			if(end == string::npos) // get EOL as well
				end = x.length();
			else if(end == start + 1)
				return 0;

			// Nickname click, let's see if we can find one like it in the name list...
			tstring nick = x.substr(start, end - start);
			OnlineUserPtr ui = client->findUser(Text::fromT(nick));
			if(ui) {
				bHandled = true;
				if (wParam & MK_CONTROL) { // MK_CONTROL = 0x0008
					PrivateFrame::openWindow(HintedUser(ui->getUser(), client->getHubUrl()), Util::emptyStringT, client);
				} else if (wParam & MK_SHIFT) {
					try {
						QueueManager::getInstance()->addList(HintedUser(ui->getUser(), client->getHubUrl()), QueueItem::FLAG_CLIENT_VIEW);
					} catch(const Exception& e) {
						addStatus(Text::toT(e.getError()), WinUtil::m_ChatTextSystem);
					}
				} else if(ui->getUser() != ClientManager::getInstance()->getMe()) {
					switch(SETTING(CHAT_DBLCLICK)) {
					case 0: {
						int items = ctrlUsers.GetItemCount();
						int pos = -1;
						ctrlUsers.SetRedraw(FALSE);
						for(int i = 0; i < items; ++i) {
							if(ctrlUsers.getItemData(i) == ui)
								pos = i;
							ctrlUsers.SetItemState(i, (i == pos) ? LVIS_SELECTED | LVIS_FOCUSED : 0, LVIS_SELECTED | LVIS_FOCUSED);
						}
						ctrlUsers.SetRedraw(TRUE);
						ctrlUsers.EnsureVisible(pos, FALSE);
					    break;
					}    
					case 1: {
					     tstring sUser = ui->getText(OnlineUser::COLUMN_NICK);
					     int iSelBegin, iSelEnd;
					     ctrlMessage.GetSel(iSelBegin, iSelEnd);

					     if((iSelBegin == 0) && (iSelEnd == 0)) {
							sUser += _T(": ");
							if(ctrlMessage.GetWindowTextLength() == 0) {   
			                    ctrlMessage.SetWindowText(sUser.c_str());
			                    ctrlMessage.SetFocus();
			                    ctrlMessage.SetSel(ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength());
							} else {
			                    ctrlMessage.ReplaceSel(sUser.c_str());
								ctrlMessage.SetFocus();
					        }
					     } else {
					          sUser += _T(" ");
					          ctrlMessage.ReplaceSel(sUser.c_str());
					          ctrlMessage.SetFocus();
					     }
					     break;
					}
					case 2:
						ui->pm();
					    break;
					case 3:
					    ui->getList();
					    break;
					case 4:
					    ui->matchQueue();
					    break;
					case 5:
					    ui->grant();
					    break;
					case 6:
					    ui->addFav();
					    break;
				}
			}
		}
	}
	return 0;
}

void HubFrame::addLine(const tstring& aLine) {
	addLine(Identity(NULL, 0), aLine, WinUtil::m_ChatTextGeneral );
}

void HubFrame::addLine(const tstring& aLine, CHARFORMAT2& cf, bool bUseEmo/* = true*/) {
    addLine(Identity(NULL, 0), aLine, cf, bUseEmo);
}

void HubFrame::addLine(const Identity& i, const tstring& aLine, CHARFORMAT2& cf, bool bUseEmo/* = true*/) {
//	ctrlClient.AdjustTextSize();

	if(client->get(HubSettings::LogMainChat)) {
		ParamMap params;
		params["message"] = Text::fromT(aLine);
		client->getHubIdentity().getParams(params, "hub", false);
		params["hubURL"] = client->getHubUrl();
		client->getMyIdentity().getParams(params, "my", true);
		LOG(LogManager::CHAT, params);
	}

	bool notify = ctrlClient.AppendChat(i, Text::toT(client->get(HubSettings::Nick)), timeStamps ? Text::toT("[" + Util::getShortTimeString() + "] ") : Util::emptyStringT, aLine + _T('\n'), cf, bUseEmo);
	if(notify)
		setNotify();

	if (SETTING(BOLD_HUB)) {
		setDirty();
	}

}

LRESULT HubFrame::onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 
	tabMenuShown = true;
	OMenu tabMenu, copyHubMenu;

	copyHubMenu.CreatePopupMenu();
	copyHubMenu.InsertSeparatorFirst(TSTRING(COPY));
	copyHubMenu.AppendMenu(MF_STRING, IDC_COPY_HUBNAME, CTSTRING(HUB_NAME));
	copyHubMenu.AppendMenu(MF_STRING, IDC_COPY_HUBADDRESS, CTSTRING(HUB_ADDRESS));

	tabMenu.CreatePopupMenu();
	tabMenu.InsertSeparatorFirst(Text::toT(!client->getHubName().empty() ? (client->getHubName().size() > 50 ? (client->getHubName().substr(0, 50) + "...") : client->getHubName()) : client->getHubUrl()));	
	if(SETTING(LOG_MAIN_CHAT)) {
		tabMenu.AppendMenu(MF_STRING, IDC_OPEN_HUB_LOG, CTSTRING(OPEN_HUB_LOG));
		tabMenu.AppendMenu(MF_SEPARATOR);
		tabMenu.AppendMenu(MF_STRING, IDC_HISTORY, CTSTRING(VIEW_HISTORY));
		tabMenu.AppendMenu(MF_SEPARATOR);
	}

	tabMenu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CTSTRING(CLEAR_CHAT));
	if(client->get(HubSettings::ChatNotify)) 
		tabMenu.AppendMenu(MF_CHECKED, IDC_NOTIFY, CTSTRING(NOTIFY));
	else
		tabMenu.AppendMenu(MF_UNCHECKED, IDC_NOTIFY, CTSTRING(NOTIFY));

	tabMenu.AppendMenu(MF_SEPARATOR);
	tabMenu.AppendMenu(MF_STRING, IDC_ADD_AS_FAVORITE, CTSTRING(ADD_TO_FAVORITES));
	tabMenu.AppendMenu(MF_STRING, ID_FILE_RECONNECT, CTSTRING(MENU_RECONNECT));
	tabMenu.AppendMenu(MF_SEPARATOR);

	auto p = ShareManager::getInstance()->getShareProfile(client->getShareProfile());

	tabMenu.AppendMenu(MF_STRING, IDC_BROWSE_OWN_LIST, CTSTRING_F(OPEN_HUB_FILELIST, Text::toT(p->getPlainName())));
	tabMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)copyHubMenu, CTSTRING(COPY));
	prepareMenu(tabMenu, ::UserCommand::CONTEXT_HUB, client->getHubUrl());
	tabMenu.AppendMenu(MF_SEPARATOR);
	tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));
	
	if(!client->isConnected())
		tabMenu.EnableMenuItem((UINT)(HMENU)copyHubMenu, MF_GRAYED);
	else
		tabMenu.EnableMenuItem((UINT)(HMENU)copyHubMenu, MF_ENABLED);
	
	tabMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
	return TRUE;
}

LRESULT HubFrame::onOpenMyList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	DirectoryListingManager::getInstance()->openOwnList(client->getShareProfile());
	return 0;
}

LRESULT HubFrame::onSetNotify(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	client->changeBoolHubSetting(HubSettings::ChatNotify);
	return 0;
}


LRESULT HubFrame::onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	HDC hDC = (HDC)wParam;
	::SetBkColor(hDC, WinUtil::bgColor);
	::SetTextColor(hDC, WinUtil::textColor);
	return (LRESULT)WinUtil::bgBrush;
}
	
LRESULT HubFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	CRect rc;            // client area of window 
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click
	tabMenuShown = false;
	OMenu Mnu;
	Mnu.CreatePopupMenu();

	ctrlUsers.GetHeader().GetWindowRect(&rc);
		
	if(PtInRect(&rc, pt) && showUsers) {
		ctrlUsers.showMenu(pt);
		return TRUE;
	}
		
	if(reinterpret_cast<HWND>(wParam) == ctrlUsers && showUsers && (ctrlUsers.GetSelectedCount() > 0)) {
		ctrlClient.setSelectedUser(Util::emptyStringT);
		if ( ctrlUsers.GetSelectedCount() == 1 ) {
			if(pt.x == -1 && pt.y == -1) {
				WinUtil::getContextMenuPos(ctrlUsers, pt);
			}
		}

		if(PreparePopupMenu(&ctrlUsers, Mnu)) {
			prepareMenu(Mnu, ::UserCommand::CONTEXT_USER, client->getHubUrl());
			Mnu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
		}
	}
	bHandled = FALSE;
	return 0; 
}

void HubFrame::runUserCommand(::UserCommand& uc) {
	if(!WinUtil::getUCParams(m_hWnd, uc, ucLineParams))
		return;

	auto ucParams = ucLineParams;

	client->getMyIdentity().getParams(ucParams, "my", true);
	client->getHubIdentity().getParams(ucParams, "hub", false);

	if(tabMenuShown) {
		client->sendUserCmd(uc, ucParams);
	} else {
		int sel = -1;
		while((sel = ctrlUsers.GetNextItem(sel, LVNI_SELECTED)) != -1) {
			const OnlineUserPtr u = ctrlUsers.getItemData(sel);
			if(u->getUser()->isOnline()) {
				auto tmp = ucParams;
				u->getIdentity().getParams(tmp, "user", true);
				client->sendUserCmd(uc, tmp);
			}
		}
	}
}

void HubFrame::onTab() {
	if(ctrlMessage.GetWindowTextLength() == 0) {
		handleTab(WinUtil::isShift());
		return;
	}
		
	HWND focus = GetFocus();
	if( (focus == ctrlMessage.m_hWnd) && !WinUtil::isShift() ) 
	{
		tstring text;
		text.resize(ctrlMessage.GetWindowTextLength());

		ctrlMessage.GetWindowText(&text[0], text.size() + 1);

		string::size_type textStart = text.find_last_of(_T(" \n\t"));

		if(complete.empty()) {
			if(textStart != string::npos) {
				complete = text.substr(textStart + 1);
			} else {
				complete = text;
			}
			if(complete.empty()) {
				// Still empty, no text entered...
				ctrlUsers.SetFocus();
				return;
			}
			int y = ctrlUsers.GetItemCount();

			for(int x = 0; x < y; ++x)
				ctrlUsers.SetItemState(x, 0, LVNI_FOCUSED | LVNI_SELECTED);
		}

		if(textStart == string::npos)
			textStart = 0;
		else
			textStart++;

		int start = ctrlUsers.GetNextItem(-1, LVNI_FOCUSED) + 1;
		int i = start;
		int j = ctrlUsers.GetItemCount();

		bool firstPass = i < j;
		if(!firstPass)
			i = 0;
		while(firstPass || (!firstPass && i < start)) {
			const OnlineUserPtr ui = ctrlUsers.getItemData(i);
			const tstring& nick = ui->getText(OnlineUser::COLUMN_NICK);
			bool found = (strnicmp(nick, complete, complete.length()) == 0);
			tstring::size_type x = 0;
			if(!found) {
				// Check if there's one or more [ISP] tags to ignore...
				tstring::size_type y = 0;
				while(nick[y] == _T('[')) {
					x = nick.find(_T(']'), y);
					if(x != string::npos) {
						if(strnicmp(nick.c_str() + x + 1, complete.c_str(), complete.length()) == 0) {
							found = true;
							break;
						}
					} else {
						break;
					}
					y = x + 1; // assuming that nick[y] == '\0' is legal
				}
			}
			if(found) {
				if((start - 1) != -1) {
					ctrlUsers.SetItemState(start - 1, 0, LVNI_SELECTED | LVNI_FOCUSED);
				}
				ctrlUsers.SetItemState(i, LVNI_FOCUSED | LVNI_SELECTED, LVNI_FOCUSED | LVNI_SELECTED);
				ctrlUsers.EnsureVisible(i, FALSE);
				ctrlMessage.SetSel(textStart, ctrlMessage.GetWindowTextLength(), TRUE);
				ctrlMessage.ReplaceSel(nick.c_str());
				return;
			}
			i++;
			if(i == j) {
				firstPass = false;
				i = 0;
			}
		}
	}
}

LRESULT HubFrame::onFileReconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	// We want to have it fresh
	ignoreList.clear();

	
	client->reconnect();
	return 0;
}

LRESULT HubFrame::onShowUsers(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	bHandled = FALSE;
	if(wParam == BST_CHECKED) {
		showUsers = true;
		client->refreshUserList(true);
	} else {
		showUsers = false;
		clearUserList();
	}

	SettingsManager::getInstance()->set(SettingsManager::GET_USER_INFO, showUsers);

	UpdateLayout(FALSE);
	return 0;
}

LRESULT HubFrame::onFollow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!redirect.empty()) {
		if(ClientManager::getInstance()->isConnected(Text::fromT(redirect))) {
			addStatus(TSTRING(REDIRECT_ALREADY_CONNECTED), WinUtil::m_ChatTextServer);
			return 0;
		}
		
		dcassert(frames.find(server) != frames.end());
		dcassert(frames[server] == this);
		frames.erase(server);
		server = redirect;
		frames[server] = this;

		// the client is dead, long live the client!
		client->removeListener(this);
		clearUserList();
		ClientManager::getInstance()->putClient(client);
		clearTaskList();
		client = ClientManager::getInstance()->createClient(Text::fromT(server));

		ctrlClient.setClient(client);

		RecentHubEntry r;
		r.setName("*");
		r.setDescription("***");
		r.setUsers("*");
		r.setShared("*");
		r.setServer(Text::fromT(redirect));
		FavoriteManager::getInstance()->addRecent(r);

		client->addListener(this);
		client->connect();
	}
	return 0;
}

LRESULT HubFrame::onEnterUsers(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {
	int item = ctrlUsers.GetNextItem(-1, LVNI_FOCUSED);
	if(item != -1) {
		try {
			QueueManager::getInstance()->addList(HintedUser((ctrlUsers.getItemData(item))->getUser(), client->getHubUrl()), QueueItem::FLAG_CLIENT_VIEW);
		} catch(const Exception& e) {
			addStatus(Text::toT(e.getError()));
		}
	}
	return 0;
}

LRESULT HubFrame::onGetToolTip(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMTTDISPINFO* nm = (NMTTDISPINFO*)pnmh;
	lastLines.clear();
	for(auto& i: lastLinesList) {
		lastLines += i;
		lastLines += _T("\r\n");
	}

	if(lastLines.size() > 2) {
		lastLines.erase(lastLines.size() - 2);
	}

	nm->lpszText = const_cast<TCHAR*>(lastLines.c_str());

	bHandled = FALSE;
	return 0;
}

void HubFrame::addStatus(const tstring& aLine, CHARFORMAT2& cf, bool inChat /* = true */) {
	tstring line = _T("[") + Text::toT(Util::getShortTimeString()) + _T("] ") + aLine;
	TCHAR* sLine = (TCHAR*)line.c_str();

   	if(line.size() > 512) {
		sLine[512] = NULL;
	}

	if(SETTING(HUB_BOLD_TABS))
		setDirty();


	ctrlStatus.SetText(0, sLine);
	while(lastLinesList.size() + 1 > MAX_CLIENT_LINES)
		lastLinesList.pop_front();
	lastLinesList.push_back(sLine);

	if (SETTING(BOLD_HUB)) {
		setDirty();
	}
	
	if(SETTING(STATUS_IN_CHAT) && inChat) {
		addLine(_T("*** ") + aLine, cf, SETTING(HUB_BOLD_TABS));
	}
	if(SETTING(LOG_STATUS_MESSAGES)) {
		ParamMap params;
		client->getHubIdentity().getParams(params, "hub", false);
		params["hubURL"] = client->getHubUrl();
		client->getMyIdentity().getParams(params, "my", true);
		params["message"] = Text::fromT(aLine);
		LOG(LogManager::STATUS, params);
	}
}

void HubFrame::resortUsers() {
	for(auto f: frames | map_values)
		f->resortForFavsFirst(true);
}

void HubFrame::closeDisconnected() {
	for(auto f: frames | map_values) {
		if (!f->client->isConnected()) {
			f->forceClose = true;
			f->PostMessage(WM_CLOSE);
		}
	}
}

void HubFrame::updateFonts() {
	for(auto f: frames | map_values) {
		f->setFonts();
		f->UpdateLayout();
	}
}

void HubFrame::reconnectDisconnected() {
	for(auto f: frames | map_values) {
		if (!(f->client->isConnected())) {
			f->client->disconnect(false); 
			f->clearUserList();
			f->client->connect(); 
		}
	}
}

void HubFrame::on(FavoriteManagerListener::UserAdded, const FavoriteUser& /*aUser*/) noexcept {
	resortForFavsFirst();
}
void HubFrame::on(FavoriteManagerListener::UserRemoved, const FavoriteUser& /*aUser*/) noexcept {
	resortForFavsFirst();
}

void HubFrame::resortForFavsFirst(bool justDoIt /* = false */) {
	if(justDoIt || SETTING(SORT_FAVUSERS_FIRST)) {
		resort = true;
		callAsync([this] { execTasks(); });
	}
}

LRESULT HubFrame::onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
	//updateUsers = true;
	return 0;
}

LRESULT HubFrame::onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if(updateUsers) {
		updateUsers = false;
		callAsync([this] { execTasks(); });
	}

	/*auto prevSelCount = selCount;
	selCount = users->countSelected();
	if(statusDirty || selCount != prevSelCount) {
		statusDirty = false;
		updateStatus();
	}*/

	if(statusDirty) {
		statusDirty = false;
		updateStatusBar();
	}
	return 0;
}

void HubFrame::updateStatusBar() { 
	size_t AllUsers = client->getUserCount();
	size_t ShownUsers = ctrlUsers.GetItemCount();

	tstring text[3];

	if(AllUsers != ShownUsers) {
		text[0] = Util::toStringW(ShownUsers) + _T("/") + Util::toStringW(AllUsers) + _T(" ") + TSTRING(HUB_USERS);
	} else {
		text[0] = Util::toStringW(AllUsers) + _T(" ") + TSTRING(HUB_USERS);
	}
	int64_t available = client->getAvailable();
	text[1] = Util::formatBytesW(available);

	if(AllUsers > 0)
		text[2] = Util::formatBytesW(available / AllUsers) + _T("/") + TSTRING(USER);

	bool update = false;
	for(int i = 0; i < 3; i++) {
		int size = WinUtil::getTextWidth(text[i], ctrlStatus.m_hWnd);
		if(size != statusSizes[i + 1]) {
			statusSizes[i + 1] = size;
			update = true;
		}
		ctrlStatus.SetText(i + 2, text[i].c_str());
	}

	if(update)
		UpdateLayout();
}

void HubFrame::on(ClientListener::SetIcons, const Client*, int aCountType) noexcept { 
	if(countType != aCountType){
		countType = aCountType;
		callAsync([=] { onUpdateTabIcons(); });
	}
}

void HubFrame::on(Connecting, const Client*) noexcept { 
	callAsync([=] {
		if(SETTING(SEARCH_PASSIVE) && client->isActive()) {
			addLine(TSTRING(ANTI_PASSIVE_SEARCH), WinUtil::m_ChatTextSystem);
		}

		addStatus(Text::toT(STRING(CONNECTING_TO) + " " + client->getHubUrl() + " ..."), WinUtil::m_ChatTextServer);
		setWindowTitle(client->getHubUrl());
	});
}
void HubFrame::on(Connected, const Client*) noexcept { 
	callAsync([=] { onConnected(); });
}
void HubFrame::on(UserUpdated, const Client*, const OnlineUserPtr& user) noexcept {
	auto task = new UserTask(user);
	callAsync([this, task] {
		tasks.add(UPDATE_USER_JOIN, unique_ptr<Task>(task));
		updateUsers = true;
	});
}
void HubFrame::on(UsersUpdated, const Client*, const OnlineUserList& aList) noexcept {
	for(auto& i: aList) {
		auto task = new UserTask(i);
		callAsync([this, task] { tasks.add(UPDATE_USER, unique_ptr<Task>(task)); });
	}
	callAsync([this] { updateUsers = true; });
}

void HubFrame::on(ClientListener::UserRemoved, const Client*, const OnlineUserPtr& user) noexcept {
	auto task = new UserTask(user);
	callAsync([this, task] {
		tasks.add(REMOVE_USER, unique_ptr<Task>(task));
		updateUsers = true;
	});
}

void HubFrame::on(Redirect, const Client*, const string& line) noexcept { 
	if(ClientManager::getInstance()->isConnected(line)) {
		callAsync([=] { addStatus(TSTRING(REDIRECT_ALREADY_CONNECTED), WinUtil::m_ChatTextServer); });
		return;
	}

	redirect = Text::toT(line);
	bool isFO = FavoriteManager::getInstance()->isFailOverUrl(client->getFavToken(), client->getHubUrl());

	if(SETTING(AUTO_FOLLOW) && !isFO) {
		PostMessage(WM_COMMAND, IDC_FOLLOW, 0);
	} else {
		string msg;
		if (isFO) {
			msg = STRING_F(REDIRECT_FAILOVER, line);
		} else {
			msg = STRING(PRESS_FOLLOW) + " " + line;
		}

		callAsync([=] { addStatus(Text::toT(msg), WinUtil::m_ChatTextServer); });
	}
}
void HubFrame::on(Failed, const string&, const string& line) noexcept {
	callAsync([=] {
		addStatus(Text::toT(line), WinUtil::m_ChatTextServer);
		onDisconnected();
	});
}
void HubFrame::on(GetPassword, const Client*) noexcept { 
	callAsync([=] {
		onPassword();
	});
}
void HubFrame::on(HubUpdated, const Client*) noexcept {
	string hubName;
	if(client->isTrusted()) {
		hubName = "[S] ";
	} else if(client->isSecure()) {
		hubName = "[U] ";
	}
	
	hubName += client->getHubName();
	if(!client->getHubDescription().empty()) {
		hubName += " - " + client->getHubDescription();
		cachedHubname = client->getHubDescription();
	}
	if(wentoffline && !cachedHubname.empty())
		hubName += " - " + cachedHubname;

	hubName += " (" + client->getHubUrl() + ")";

#ifdef _DEBUG
	string version = client->getHubIdentity().get("VE");
	if(!version.empty()) {
		hubName += " - " + version;
	}
#endif

	callAsync([=] {
		setWindowTitle(hubName);
	});
}
void HubFrame::on(Message, const Client*, const ChatMessage& message) noexcept {
	callAsync([=] {
		if(message.to && message.replyTo) {
			onPrivateMessage(message);
		} else {
			onChatMessage(message);
		}
	});
}	

void HubFrame::on(StatusMessage, const Client*, const string& line, int statusFlags) {
	callAsync([=] { 
		if(SETTING(BOLD_HUB_TABS_ON_KICK) && (statusFlags & ClientListener::FLAG_IS_SPAM)){
			setDirty();
		}
		addStatus(Text::toT(Text::toDOS(line)), WinUtil::m_ChatTextServer, !SETTING(FILTER_MESSAGES) || !(statusFlags & ClientListener::FLAG_IS_SPAM));
	});

}
void HubFrame::on(NickTaken, const Client*) noexcept {
	callAsync([=] { addStatus(TSTRING(NICK_TAKEN), WinUtil::m_ChatTextServer); });
}
void HubFrame::on(SearchFlood, const Client*, const string& line) noexcept {
	callAsync([=] { addStatus(TSTRING(SEARCH_SPAM_FROM) + _T(" ") + Text::toT(line), WinUtil::m_ChatTextServer); });
}

void HubFrame::on(HubTopic, const Client*, const string& line) noexcept {
	callAsync([=] { addStatus(TSTRING(HUB_TOPIC) + _T("\t") + Text::toT(line), WinUtil::m_ChatTextServer); });
}
void HubFrame::on(AddLine, const Client*, const string& line) noexcept {
	callAsync([=] { addStatus(Text::toT(line), WinUtil::m_ChatTextServer); });
}

void HubFrame::openLinksInTopic() {
	StringList urls;
	
	boost::regex linkReg(AirUtil::getLinkUrl());
	AirUtil::getRegexMatches(client->getHubDescription(), urls, linkReg);

	for(auto& url: urls) {
		Util::sanitizeUrl(url);
		WinUtil::openLink(Text::toT(url));
	}
}

void HubFrame::updateUserList(OnlineUserPtr ui) {
	
	//single update?
	//avoid refreshing the whole list and just update the current item
	//instead
	if(ui) {
		if(ui->isHidden()) {
			return;
		}
		if(filter.empty()) {
			if(ctrlUsers.findItem(ui.get()) == -1) {
				ui->inc();
				ctrlUsers.insertItem(ui.get(), UserInfoBase::getImage(ui->getIdentity(), client));
			}
		} else {
			auto filterPrep = filter.prepare();
			auto filterInfoF = [this, &ui](int column) { return Text::fromT(ui->getText(column)); };

			if(filter.match(filterPrep, filterInfoF)) {
				if(ctrlUsers.findItem(ui.get()) == -1) {
					ui->inc();
					ctrlUsers.insertItem(ui.get(), UserInfoBase::getImage(ui->getIdentity(), client));
				}
			} else {
				int i = ctrlUsers.findItem(ui.get());
				if(i != -1) {
					ctrlUsers.DeleteItem(i);
					ui->dec();
				}
			}
		}
	} else {
		ctrlUsers.SetRedraw(FALSE);
		clearUserList();

		OnlineUserList l;
		client->getUserList(l);

		if(filter.empty()) {
			for(const auto& ui: l){
				if(!ui->isHidden()) {
					ui->inc();
					ctrlUsers.insertItem(ui.get(), UserInfoBase::getImage(ui->getIdentity(), client));
				}
			}
		} else {
			auto i = l.begin();
			auto filterPrep = filter.prepare();
			auto filterInfoF = [this, &i](int column) { return Text::fromT((*i)->getText(column)); };

			for(; i != l.end(); ++i){
				auto ui = *i;
				if(!ui->isHidden() && (filter.empty() || filter.match(filterPrep, filterInfoF))) {
					ui->inc();
					ctrlUsers.insertItem(ui.get(), UserInfoBase::getImage(ui->getIdentity(), client));
				}
			}
		}
		ctrlUsers.SetRedraw(TRUE);
	}

	statusDirty = true;
}
void HubFrame::handleTab(bool reverse) {
	HWND focus = GetFocus();

	if(reverse) {
		if(focus == filter.getFilterMethodBox().m_hWnd) {
			filter.getFilterColumnBox().SetFocus();
		} else if(focus == filter.getFilterColumnBox().m_hWnd) {
			filter.getFilterBox().SetFocus();
		} else if(focus == filter.getFilterBox().m_hWnd) {
			ctrlMessage.SetFocus();
		} else if(focus == ctrlMessage.m_hWnd) {
			ctrlUsers.SetFocus();
		} else if(focus == ctrlUsers.m_hWnd) {
			ctrlClient.SetFocus();
		} else if(focus == ctrlClient.m_hWnd) {
			filter.getFilterMethodBox().SetFocus();
		}
	} else {
		if(focus == ctrlClient.m_hWnd) {
			ctrlUsers.SetFocus();
		} else if(focus == ctrlUsers.m_hWnd) {
			ctrlMessage.SetFocus();
		} else if(focus == ctrlMessage.m_hWnd) {
			filter.getFilterBox().SetFocus();
		} else if(focus == filter.getFilterBox().m_hWnd) {
			filter.getFilterColumnBox().SetFocus();
		} else if(focus == filter.getFilterColumnBox().m_hWnd) {
			filter.getFilterMethodBox().SetFocus();
		} else if(focus == filter.getFilterMethodBox().m_hWnd) {
			ctrlClient.SetFocus();
		}
	}
}

//void HubFrame::addClientLine(const tstring& aLine, CHARFORMAT2& cf, bool inChat /* = true */) {
/*	tstring line = _T("[") + Text::toT(Util::getShortTimeString()) + _T("] ") + aLine;

	ctrlStatus.SetText(0, line.c_str());
	while(lastLinesList.size() + 1 > MAX_CLIENT_LINES)
		lastLinesList.erase(lastLinesList.begin());
	lastLinesList.push_back(line);
	
	if (SETTING(BOLD_HUB)) {
		setDirty();
	}
	
	if(SETTING(STATUS_IN_CHAT) && inChat) {
		addLine(_T("*** ") + aLine, cf);
	}
}
*/
bool HubFrame::PreparePopupMenu(CWindow* /*pCtrl*/, OMenu& menu ) {
	if (copyMenu.m_hMenu != NULL) {
		copyMenu.DestroyMenu();
		copyMenu.m_hMenu = NULL;
	}

	copyMenu.CreatePopupMenu();
	copyMenu.InsertSeparatorFirst(TSTRING(COPY));

	for(int j=0; j < OnlineUser::COLUMN_LAST; j++) {
		copyMenu.AppendMenu(MF_STRING, IDC_COPY + j, CTSTRING_I(columnNames[j]));
	}
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_USER_ALL,  CTSTRING(INFO));

	size_t count = ctrlUsers.GetSelectedCount();
	bool isMe = false;

	if(count == 1) {
		tstring sNick = Text::toT(((OnlineUser*)ctrlUsers.getItemData(ctrlUsers.GetNextItem(-1, LVNI_SELECTED)))->getIdentity().getNick());
	    isMe = (sNick == Text::toT(client->getMyNick()));

		menu.InsertSeparatorFirst(sNick);

		if(SETTING(LOG_PRIVATE_CHAT)) {
			menu.AppendMenu(MF_STRING, IDC_OPEN_USER_LOG,  CTSTRING(OPEN_USER_LOG));
			menu.AppendMenu(MF_SEPARATOR);
			menu.AppendMenu(MF_STRING, IDC_USER_HISTORY,  CTSTRING(VIEW_HISTORY));
			menu.AppendMenu(MF_SEPARATOR);
		}
	} else {
		menu.InsertSeparatorFirst(Util::toStringW(count) + _T(" ") + TSTRING(HUB_USERS));
	}

	if(!isMe) {
		menu.AppendMenu(MF_STRING, IDC_PUBLIC_MESSAGE, CTSTRING(SEND_PUBLIC_MESSAGE));
		appendUserItems(menu);
		menu.AppendMenu(MF_SEPARATOR);

		if(count == 1) {
			const OnlineUserPtr ou = ctrlUsers.getItemData(ctrlUsers.GetNextItem(-1, LVNI_SELECTED));
			if (client->isOp() || !ou->getIdentity().isOp()) {
				if(ignoreList.find(ou->getUser()) == ignoreList.end()) {
					menu.AppendMenu(MF_STRING, IDC_IGNORE, CTSTRING(IGNORE_USER));
				} else {    
					menu.AppendMenu(MF_STRING, IDC_UNIGNORE, CTSTRING(UNIGNORE_USER));
				}
				menu.AppendMenu(MF_SEPARATOR);
			}
		}
	}
	
	menu.AppendMenu(MF_POPUP, (UINT)(HMENU)copyMenu, CTSTRING(COPY));
   

	/*if(AirUtil::isAdcHub(client->getHubUrl()))
		menu.SetMenuDefaultItem(IDC_BROWSELIST);
	else
		menu.SetMenuDefaultItem( IDC_GETLIST );*/

	return true;
}

LRESULT HubFrame::onSelectUser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlClient.getSelectedUser().empty()) {
		// No nick selected
		return 0;
	}

	int pos = ctrlUsers.findItem(ctrlClient.getSelectedUser());
	if ( pos == -1 ) {
		// User not found is list
		return 0;
	}

	int items = ctrlUsers.GetItemCount();
	ctrlUsers.SetRedraw(FALSE);
	for(int i = 0; i < items; ++i) {
		ctrlUsers.SetItemState(i, (i == pos) ? LVIS_SELECTED | LVIS_FOCUSED : 0, LVIS_SELECTED | LVIS_FOCUSED);
	}
	ctrlUsers.SetRedraw(TRUE);
	ctrlUsers.EnsureVisible(pos, FALSE);

	return 0;
}

LRESULT HubFrame::onPrivateMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		PrivateFrame::openWindow(HintedUser(ctrlUsers.getItemData(i)->getUser(), client->getHubUrl()), Util::emptyStringT, client);
	}

	return 0;
}

LRESULT HubFrame::onPublicMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	tstring sUsers = Util::emptyStringT;

	if(!client->isConnected())
		return 0;

	if(ctrlClient.getSelectedUser().empty()) {
		while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
			if (!sUsers.empty())
				sUsers += _T(", ");
			sUsers += Text::toT(((OnlineUser*)ctrlUsers.getItemData(i))->getIdentity().getNick());
		}
	} else {
		sUsers = ctrlClient.getSelectedUser();
	}

	int iSelBegin, iSelEnd;
	ctrlMessage.GetSel( iSelBegin, iSelEnd );

	if ( ( iSelBegin == 0 ) && ( iSelEnd == 0 ) ) {
		sUsers += _T(": ");
		if (ctrlMessage.GetWindowTextLength() == 0) {	
			ctrlMessage.SetWindowText(sUsers.c_str());
			ctrlMessage.SetFocus();
			ctrlMessage.SetSel( ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength() );
		} else {
			ctrlMessage.ReplaceSel( sUsers.c_str() );
			ctrlMessage.SetFocus();
		}
	} else {
		sUsers += _T(" ");
		ctrlMessage.ReplaceSel( sUsers.c_str() );
		ctrlMessage.SetFocus();
	}
	return 0;
}

LRESULT HubFrame::onOpenUserLog(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {	
	ParamMap params;
	OnlineUserPtr ui = nullptr;

	int i = -1;
	if((i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ui = ctrlUsers.getItemData(i);
	}

	if(!ui) 
		return 0;

	string file = ui->getLogPath();
	if(Util::fileExists(file)) {
		WinUtil::viewLog(file, wID == IDC_USER_HISTORY);
	} else {
		MessageBox(CTSTRING(NO_LOG_FOR_USER),CTSTRING(NO_LOG_FOR_USER), MB_OK );	  
	}
	return 0;
}

string HubFrame::getLogPath(bool status) const {
	ParamMap params;
	params["hubNI"] = [this] { return client->getHubName(); };
	params["hubURL"] = [this] { return client->getHubUrl(); };
	params["myNI"] = [this] { return client->getMyNick(); };
	return LogManager::getInstance()->getPath(status ? LogManager::STATUS : LogManager::CHAT, params);
}

LRESULT HubFrame::onOpenHubLog(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	string filename = getLogPath(false);
	if(Util::fileExists(filename)){
		WinUtil::viewLog(filename, wID == IDC_HISTORY);
	} else {
		MessageBox(CTSTRING(NO_LOG_FOR_HUB),CTSTRING(NO_LOG_FOR_HUB), MB_OK );	  
	}
	return 0;
}

LRESULT HubFrame::onStyleChange(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	bHandled = FALSE;
	if((wParam & MK_LBUTTON) && ::GetCapture() == m_hWnd) {
		UpdateLayout(FALSE);
	}
	return 0;
}

LRESULT HubFrame::onStyleChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	bHandled = FALSE;
	UpdateLayout(FALSE);
	return 0;
}

void HubFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	bool needRedraw = false;
	ctrlUsers.SetImageList(ResourceLoader::getUserImages(), LVSIL_SMALL);
	//ctrlUsers.Invalidate();
	if(ctrlUsers.GetBkColor() != WinUtil::bgColor) {
		needRedraw = true;
		ctrlClient.SetBackgroundColor(WinUtil::bgColor);
		ctrlUsers.SetBkColor(WinUtil::bgColor);
		ctrlUsers.SetTextBkColor(WinUtil::bgColor);
		ctrlUsers.setFlickerFree(WinUtil::bgBrush);
	}
	if(ctrlUsers.GetTextColor() != WinUtil::textColor) {
		needRedraw = true;
		ctrlUsers.SetTextColor(WinUtil::textColor);
	}
	if(ctrlUsers.GetFont() != WinUtil::listViewFont){
		ctrlUsers.SetFont(WinUtil::listViewFont);
		needRedraw = true;
	}

	if(needRedraw)
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

LRESULT HubFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT: {
			OnlineUser* ui = (OnlineUser*)cd->nmcd.lItemlParam;
			// tstring user = ui->getText(OnlineUser::COLUMN_NICK); 
			if (FavoriteManager::getInstance()->isFavoriteUser(ui->getUser())) {
				cd->clrText = SETTING(FAVORITE_COLOR);
			} else if (UploadManager::getInstance()->hasReservedSlot(ui->getUser())) {
				cd->clrText = SETTING(RESERVED_SLOT_COLOR);
			} else if (ignoreList.find(ui->getUser()) != ignoreList.end()) {
				cd->clrText = SETTING(IGNORED_COLOR);
			} else if(ui->getIdentity().isOp()) {
				cd->clrText = SETTING(OP_COLOR);
			} else if(!ui->getIdentity().isTcpActive(client)) {
				cd->clrText = SETTING(PASIVE_COLOR);
			} else {
				cd->clrText = SETTING(NORMAL_COLOUR);
			}
			if( SETTING(USE_HIGHLIGHT) ) {
				
				ColorList *cList = HighlightManager::getInstance()->getList();
				for(ColorIter i = cList->begin(); i != cList->end(); ++i) {
					ColorSettings* cs = &(*i);
					string str;
					if(cs->getContext() == HighlightManager::CONTEXT_NICKLIST) {
						tstring match = ui->getText(cs->getMatchColumn());
						if(match.empty()) continue;
						if(cs->usingRegexp()) {
							try {
								//have to have $Re:
								if(boost::regex_search(match.begin(), match.end(), cs->regexp)){
									if(cs->getHasFgColor()) cd->clrText = cs->getFgColor();
									break;
								}
							}catch(...) {}
						} else {
							if (Wildcard::patternMatch(Text::utf8ToAcp(Text::fromT(match)), Text::utf8ToAcp(Text::fromT(cs->getMatch())), '|')){
								if(cs->getHasFgColor()) cd->clrText = cs->getFgColor();
								break;
								}
							}
						}
					}
				}
			
			return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
		}

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {
		if(SETTING(GET_USER_COUNTRY) && (ctrlUsers.findColumn(cd->iSubItem) == OnlineUser::COLUMN_IP4 || ctrlUsers.findColumn(cd->iSubItem) == OnlineUser::COLUMN_IP6)) {
			CRect rc;
			OnlineUser* ou = (OnlineUser*)cd->nmcd.lItemlParam;
			ctrlUsers.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, rc);

			SetTextColor(cd->nmcd.hdc, cd->clrText);
			DrawThemeBackground(GetWindowTheme(ctrlUsers.m_hWnd), cd->nmcd.hdc, LVP_LISTITEM, 3, &rc, &rc );

			TCHAR buf[256];
			ctrlUsers.GetItemText((int)cd->nmcd.dwItemSpec, cd->iSubItem, buf, 255);
			buf[255] = 0;
			if(_tcslen(buf) > 0) {
				rc.left += 2;
				LONG top = rc.top + (rc.Height() - 15)/2;
				if((top - rc.top) < 2)
					top = rc.top + 1;

				POINT p = { rc.left, top };

				string ip = ou->getIdentity().getIp();
				uint8_t flagIndex = 0;
				if (!ip.empty()) {
					// Only attempt to grab a country mapping if we actually have an IP address
					string tmpCountry = GeoManager::getInstance()->getCountry(ip);
					if(!tmpCountry.empty()) {
						flagIndex = Localization::getFlagIndexByCode(tmpCountry.c_str());
					}
				}

				ResourceLoader::flagImages.Draw(cd->nmcd.hdc, flagIndex, p, LVSIL_SMALL);
				top = rc.top + (rc.Height() - WinUtil::getTextHeight(cd->nmcd.hdc) - 1)/2;
				::ExtTextOut(cd->nmcd.hdc, rc.left + 30, top + 1, ETO_CLIPPED, rc, buf, _tcslen(buf), NULL);
				return CDRF_SKIPDEFAULT;
			}
		}		
	}

	default:
		return CDRF_DODEFAULT;
	}
}

LRESULT HubFrame::onKeyDownUsers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMLVKEYDOWN* l = (NMLVKEYDOWN*)pnmh;
	if(l->wVKey == VK_TAB) {
		onTab();
	} else if(WinUtil::isCtrl()) {
		int i = -1;
		switch(l->wVKey) {
			case 'M':
				while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
					OnlineUserPtr ou = ctrlUsers.getItemData(i);
					if(ou->getUser() != ClientManager::getInstance()->getMe())
					{
						ou->pm();
					}
				}				
				break;
			// TODO: add others
		}
	}
	return 0;
}

LRESULT HubFrame::onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlClient.SetWindowText(Util::emptyStringT.c_str());
	return 0;
}

void HubFrame::setFonts() {
	/* 
	Pretty brave attemp to switch font on an open window, hope it dont cause any trouble.
	Reset the fonts. This will reset the charformats in the window too :( 
		they will apply again with new text..
		*/
	ctrlClient.SetRedraw(FALSE);
	ctrlClient.SetSelAll();

	ctrlClient.SetFont(WinUtil::font, FALSE);
	ctrlMessage.SetFont(WinUtil::font, FALSE);
	
	ctrlClient.SetSelectionCharFormat(WinUtil::m_ChatTextLog);
	ctrlClient.SetSelNone();
	ctrlClient.SetRedraw(TRUE);

	addStatus(_T("New Font & TextStyles Applied, TextMatching colors will apply after this line"), WinUtil::m_ChatTextSystem);
}

LRESULT HubFrame::onIgnore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/) {
	int i=-1;
	if(client->isConnected()) {
		while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
			ignoreList.insert(((OnlineUser*)ctrlUsers.getItemData(i))->getUser());
			IgnoreManager::getInstance()->storeIgnore(((OnlineUser*)ctrlUsers.getItemData(i))->getUser());
		}
	}
	return 0;
}

LRESULT HubFrame::onUnignore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/) {
	int i=-1;
	if(client->isConnected()) {
		while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
			ignoreList.erase(((OnlineUser*)ctrlUsers.getItemData(i))->getUser());
				IgnoreManager::getInstance()->removeIgnore(((OnlineUser*)ctrlUsers.getItemData(i))->getUser());
		}
	}
	return 0;
}