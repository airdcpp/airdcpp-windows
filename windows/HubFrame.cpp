/*
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "HubFrame.h"
#include "LineDlg.h"
#include "SearchFrm.h"
#include "PrivateFrame.h"
#include "TextFrame.h"

#include "../client/DirectoryListingManager.h"
#include "../client/ChatMessage.h"
#include "../client/QueueManager.h"
#include "../client/ShareManager.h"
#include "../client/Util.h"
#include "../client/StringTokenizer.h"
#include "../client/FavoriteManager.h"
#include "../client/LogManager.h"
#include "../client/AdcCommand.h"
#include "../client/SettingsManager.h"
#include "../client/ConnectionManager.h" 
#include "../client/NmdcHub.h"
#include "../client/Wildcards.h"
#include "../client/ColorSettings.h"
#include "../client/HighlightManager.h"
#include "../client/ConnectivityManager.h"
#include "../client/Localization.h"
#include "../client/GeoManager.h"

HubFrame::FrameMap HubFrame::frames;
HubFrame::IgnoreMap HubFrame::ignoreList;

int HubFrame::columnSizes[] = { 100, 75, 75, 75, 100, 75, 130, 100, 50, 40, 40, 40, 40, 300 };
int HubFrame::columnIndexes[] = { OnlineUser::COLUMN_NICK, OnlineUser::COLUMN_SHARED, OnlineUser::COLUMN_EXACT_SHARED,
	OnlineUser::COLUMN_DESCRIPTION, OnlineUser::COLUMN_TAG,	OnlineUser::COLUMN_ULSPEED, OnlineUser::COLUMN_DLSPEED, OnlineUser::COLUMN_IP, OnlineUser::COLUMN_EMAIL,
	OnlineUser::COLUMN_VERSION, OnlineUser::COLUMN_MODE, OnlineUser::COLUMN_HUBS, OnlineUser::COLUMN_SLOTS, OnlineUser::COLUMN_CID };
ResourceManager::Strings HubFrame::columnNames[] = { ResourceManager::NICK, ResourceManager::SHARED, ResourceManager::EXACT_SHARED, 
	ResourceManager::DESCRIPTION, ResourceManager::TAG, ResourceManager::SETCZDC_UPLOAD_SPEED, ResourceManager::SETCZDC_DOWNLOAD_SPEED, ResourceManager::IP_BARE, ResourceManager::EMAIL,
	ResourceManager::VERSION, ResourceManager::MODE, ResourceManager::HUBS, ResourceManager::SLOTS, ResourceManager::CID };

LRESULT HubFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	CToolInfo ti(TTF_SUBCLASS, ctrlStatus.m_hWnd);

	ctrlClient.setClient(client);
	init(m_hWnd, rcDefault);
	ctrlMessageContainer.SubclassWindow(ctrlMessage.m_hWnd);
	ctrlClientContainer.SubclassWindow(ctrlClient.m_hWnd);

	ctrlFilter.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);

	ctrlFilterContainer.SubclassWindow(ctrlFilter.m_hWnd);
	ctrlFilter.SetFont(WinUtil::font);
	WinUtil::addCue(ctrlFilter.m_hWnd, CTSTRING(FILTER_DOTS), TRUE);

	ctrlFilterSel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL |
		WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);

	ctrlFilterSelContainer.SubclassWindow(ctrlFilterSel.m_hWnd);
	ctrlFilterSel.SetFont(WinUtil::font);

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

	const FavoriteHubEntry *fhe = FavoriteManager::getInstance()->getFavoriteHubEntry(Text::fromT(server));
	if(fhe) {
		WinUtil::splitTokens(columnIndexes, fhe->getHeaderOrder(), OnlineUser::COLUMN_LAST);
		WinUtil::splitTokens(columnSizes, fhe->getHeaderWidths(), OnlineUser::COLUMN_LAST);
	} else {
		WinUtil::splitTokens(columnIndexes, SETTING(HUBFRAME_ORDER), OnlineUser::COLUMN_LAST);
		WinUtil::splitTokens(columnSizes, SETTING(HUBFRAME_WIDTHS), OnlineUser::COLUMN_LAST);                           
	}
    	
	for(uint8_t j = 0; j < OnlineUser::COLUMN_LAST; ++j) {
		int fmt = (j == OnlineUser::COLUMN_SHARED || j == OnlineUser::COLUMN_EXACT_SHARED || j == OnlineUser::COLUMN_SLOTS) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlUsers.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
		ctrlFilterSel.AddString(CTSTRING_I(columnNames[j]));
	}
	
	ctrlUsers.setColumnOrderArray(OnlineUser::COLUMN_LAST, columnIndexes);
	ctrlFilterSel.AddString(CTSTRING(ANY));
	ctrlFilterSel.SetCurSel(0);
	
	if(fhe) {
		ctrlUsers.setVisible(fhe->getHeaderVisible());
    } else {
	    ctrlUsers.setVisible(SETTING(HUBFRAME_VISIBLE));
    }
	
	ctrlUsers.SetBkColor(WinUtil::bgColor);
	ctrlUsers.SetTextBkColor(WinUtil::bgColor);
	ctrlUsers.SetTextColor(WinUtil::textColor);
	ctrlUsers.setFlickerFree(WinUtil::bgBrush);
	ctrlUsers.setSortColumn(OnlineUser::COLUMN_NICK);
	ctrlUsers.SetImageList(WinUtil::userImages, LVSIL_SMALL);



	showJoins = BOOLSETTING(SHOW_JOINS);
	favShowJoins = BOOLSETTING(FAV_SHOW_JOINS);

	if(fhe){
		logMainChat = fhe->getHubLogMainchat();
		hubshowjoins = fhe->getHubShowJoins();
		showchaticon = fhe->getChatNotify();
	} else {
		logMainChat = false;
		hubshowjoins = false;
		showchaticon = false;
	}

	WinUtil::SetIcon(m_hWnd, IDI_HUB);

	HubOpIcon = WinUtil::createIcon(IDI_HUBOP, 16);
	HubRegIcon = WinUtil::createIcon(IDI_HUBREG, 16);
	HubIcon =  WinUtil::createIcon(IDI_HUB, 16);

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
    TimerManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

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
		waitingForPW(false), extraSort(false), server(aServer), closed(false), 
		showUsers(BOOLSETTING(GET_USER_INFO)), updateUsers(false), resort(false),
		timeStamps(BOOLSETTING(TIME_STAMPS)),
		hubchatusersplit(chatusersplit),
		ctrlShowUsersContainer(WC_BUTTON, this, SHOW_USERS),
		ctrlFilterContainer(WC_EDIT, this, FILTER_MESSAGE_MAP),
		ctrlFilterSelContainer(WC_COMBOBOX, this, FILTER_MESSAGE_MAP),
		ctrlMessageContainer(WC_EDIT, this, EDIT_MESSAGE_MAP),
		ctrlClientContainer(WC_EDIT, this, EDIT_MESSAGE_MAP),
		ChatFrameBase(this)
{
	client = ClientManager::getInstance()->createClient(Text::fromT(aServer));
	client->setShareProfile(aShareProfile);
	client->addListener(this);

	if(FavoriteManager::getInstance()->getFavoriteHubEntry(Text::fromT(server)) != NULL) {
		showUsers = userliststate;
	} else {
		showUsers = BOOLSETTING(GET_USER_INFO);
	}
		
	memset(statusSizes, 0, sizeof(statusSizes));
}

bool HubFrame::sendMessage(const tstring& aMessage, bool isThirdPerson) {
	client->hubMessage(Text::fromT(aMessage), isThirdPerson);
	return true;
}

bool HubFrame::checkFrameCommand(tstring& cmd, tstring& param, tstring& message, tstring& status, bool& thirdPerson) {	
	if(stricmp(cmd.c_str(), _T("join"))==0) {
		if(!param.empty()) {
			redirect = param;
			if(BOOLSETTING(JOIN_OPEN_NEW_WINDOW)) {
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
		showJoins = !showJoins;
		if(showJoins) {
			status = TSTRING(JOIN_SHOWING_ON);
		} else {
			status = TSTRING(JOIN_SHOWING_OFF);
		}
	} else if( stricmp(cmd.c_str(), _T("favshowjoins")) == 0 ) {
		favShowJoins = !favShowJoins;
		if(favShowJoins) {
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
			ignorelist += _T(" ") + Text::toT(ClientManager::getInstance()->getNicks((*i)->getCID(), Util::emptyString)[0]); // ignore user isn't hub dependent
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
	} else if(stricmp(cmd.c_str(), _T("me")) == 0) {
		//message = client->hubMessage(Text::fromT(s));
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
			aEntry.setNick(client->getMyNick());
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
	string Copy;

	int sel = -1;
	while((sel = ctrlUsers.GetNextItem(sel, LVNI_SELECTED)) != -1) {
		const OnlineUserPtr ou = ctrlUsers.getItemData(sel);

		Copy += "\r\nNick :              " + ou->getIdentity().getNick();
		Copy += "\r\nShared :          " + Util::formatBytes(ou->getIdentity().getBytesShared());
		Copy += "\r\nDescription :    " + ou->getIdentity().getDescription();
		Copy += "\r\nTag :               " + ou->getIdentity().getTag();
		Copy += "\r\nUpload Speed :   " + ou->getIdentity().getConnection();
		Copy += "\r\nDownload Speed :   " + ou->getIdentity().getDLSpeed();
		Copy += "\r\nIp :                  " + ou->getIdentity().getIp();
		Copy += "\r\nEmail :             " + ou->getIdentity().getEmail();
		Copy += "\r\nSlots :             " + ou->getIdentity().get("SL");

	}
	tstring sCopy = Text::toT(Copy);
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
		int pos = ctrlUsers.findItem(u.onlineUser.get());

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

LRESULT HubFrame::onSpeaker(UINT /*uMsg*/, WPARAM /* wParam */, LPARAM /* lParam */, BOOL& /*bHandled*/) {
	TaskQueue::List t;
	tasks.get(t);

	if(t.size() > 2) {
		ctrlUsers.SetRedraw(FALSE);
	}

	for(auto i = t.begin(); i != t.end(); ++i) {
		if(i->first == UPDATE_USER) {

			updateUser(static_cast<UserTask&>(*i->second));
			UserTask& u = static_cast<UserTask&>(*i->second);
			if(IgnoreManager::getInstance()->isIgnored(u.onlineUser->getIdentity().getNick())) {
				ignoreList.insert(u.onlineUser->getUser());
			} else if(!IgnoreManager::getInstance()->isIgnored(u.onlineUser->getIdentity().getNick()) && (ignoreList.find(u.onlineUser->getUser()) != ignoreList.end())) {
				ignoreList.erase(u.onlineUser->getUser());
			}
			updateUser(u);
		} else if(i->first == UPDATE_USER_JOIN) {
			UserTask& u = static_cast<UserTask&>(*i->second);
			if(updateUser(u)) {
				bool isFavorite = FavoriteManager::getInstance()->isFavoriteUser(u.onlineUser->getUser());
				if (isFavorite && (!SETTING(SOUND_FAVUSER).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
					PlaySound(Text::toT(SETTING(SOUND_FAVUSER)).c_str(), NULL, SND_FILENAME | SND_ASYNC);

				if(isFavorite && BOOLSETTING(POPUP_FAVORITE_CONNECTED)) {
					WinUtil::showPopup(Text::toT(u.onlineUser->getIdentity().getNick() + " - " + client->getHubName()), TSTRING(FAVUSER_ONLINE));
				}

				if (showJoins || (favShowJoins && isFavorite) || hubshowjoins) {
				 	addLine(_T("*** ") + TSTRING(JOINS) + Text::toT(u.onlineUser->getIdentity().getNick()), WinUtil::m_ChatTextSystem, BOOLSETTING(HUB_BOLD_TABS));
				}	
			}
		} else if(i->first == REMOVE_USER) {
			const UserTask& u = static_cast<UserTask&>(*i->second);
			removeUser(u.onlineUser);

			if (showJoins || hubshowjoins || (favShowJoins && FavoriteManager::getInstance()->isFavoriteUser(u.onlineUser->getUser()))) {
				addLine(Text::toT("*** " + STRING(PARTS) + " " + u.onlineUser->getIdentity().getNick()), WinUtil::m_ChatTextSystem, BOOLSETTING(HUB_BOLD_TABS));
			}
		} else if(i->first == CONNECTED) {
			addStatus(TSTRING(CONNECTED), WinUtil::m_ChatTextServer);
			wentoffline = false;
			setDisconnected(false);
			
			tstring text = Text::toT(client->getCipherName());
			ctrlStatus.SetText(1, text.c_str());
			statusSizes[0] = WinUtil::getTextWidth(text, ctrlStatus.m_hWnd);

			if(BOOLSETTING(POPUP_HUB_CONNECTED)) {
				WinUtil::showPopup(Text::toT(client->getAddress()), TSTRING(CONNECTED));
			}

			if ((!SETTING(SOUND_HUBCON).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
				PlaySound(Text::toT(SETTING(SOUND_HUBCON)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
		} else if(i->first == DISCONNECTED) {

			clearUserList();
//			setTabColor(RGB(255, 0, 0));
//			setIconState();
			setDisconnected(true);
			wentoffline = true;

			if ((!SETTING(SOUND_HUBDISCON).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
				PlaySound(Text::toT(SETTING(SOUND_HUBDISCON)).c_str(), NULL, SND_FILENAME | SND_ASYNC);

			if(BOOLSETTING(POPUP_HUB_DISCONNECTED)) {
				WinUtil::showPopup(Text::toT(client->getAddress()), TSTRING(DISCONNECTED));
			}
			

		} else if(i->first == ADD_CHAT_LINE) {
			if (!i->second)
				continue;

    		const MessageTask& msg = static_cast<MessageTask&>(*i->second);
			if(IgnoreManager::getInstance()->isIgnored(msg.from.getNick())) {
				ignoreList.insert(msg.from.getUser());
			} else if(!IgnoreManager::getInstance()->isIgnored(msg.from.getNick()) && (ignoreList.find(msg.from.getUser()) != ignoreList.end())) {
				ignoreList.erase(msg.from.getUser());
			}

			if(!msg.from.getUser() || (ignoreList.find(msg.from.getUser()) == ignoreList.end()) || (msg.from.isOp() && !client->isOp())) {
				addLine(msg.from, Text::toT(msg.str), WinUtil::m_ChatTextGeneral);
				if(showchaticon) {
					HWND hMainWnd = MainFrame::getMainFrame()->m_hWnd;
					::PostMessage(hMainWnd, WM_SPEAKER, MainFrame::SET_HUB_TRAY_ICON, NULL);
				}
			}
		} else if(i->first == ADD_STATUS_LINE) {
			const StatusTask& status = static_cast<StatusTask&>(*i->second);
			addStatus(Text::toT(status.str), WinUtil::m_ChatTextServer, status.inChat);
		} else if(i->first == SET_WINDOW_TITLE) {
			SetWindowText(Text::toT(static_cast<StatusTask&>(*i->second).str).c_str());
			//SetMDIFrameMenu();	
			MDIRefreshMenu();

		} else if(i->first == STATS) {
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
		} else if(i->first == GET_PASSWORD) {
			if(client->getPassword().size() > 0) {
				client->password(client->getPassword());
				addStatus(TSTRING(STORED_PASSWORD_SENT), WinUtil::m_ChatTextSystem);
			} else {
				if(!BOOLSETTING(PROMPT_PASSWORD)) {
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
		} else if(i->first == PRIVATE_MESSAGE) {
			const MessageTask& pm = static_cast<MessageTask&>(*i->second);
			tstring nick = Text::toT(pm.from.getNick());
			if(IgnoreManager::getInstance()->isIgnored(pm.from.getNick())) {
				ignoreList.insert(pm.from.getUser());
			} else if(!IgnoreManager::getInstance()->isIgnored(pm.from.getNick()) && (ignoreList.find(pm.from.getUser()) != ignoreList.end())) {
			   ignoreList.erase(pm.from.getUser());
			}

			if(!pm.from.getUser() || (ignoreList.find(pm.from.getUser()) == ignoreList.end()) ||
			  (pm.from.isOp() && !client->isOp())) {
				bool myPM = pm.replyTo == ClientManager::getInstance()->getMe();
				const UserPtr& user = myPM ? pm.to : pm.replyTo;
				if(pm.hub) {
					if(BOOLSETTING(IGNORE_HUB_PMS)) {
						addStatus(TSTRING(IGNORED_MESSAGE) + _T(" ") + Text::toT(pm.str), WinUtil::m_ChatTextSystem, false);
					} else if(BOOLSETTING(POPUP_HUB_PMS) || PrivateFrame::isOpen(user)) {
						PrivateFrame::gotMessage(pm.from, pm.to, pm.replyTo, Text::toT(pm.str), client);
					} else {
						addLine(TSTRING(PRIVATE_MESSAGE_FROM) + _T(" ") + nick + _T(": ") + Text::toT(pm.str), WinUtil::m_ChatTextPrivate);
					}
				} else if(pm.bot) {
					if(BOOLSETTING(IGNORE_BOT_PMS)) {
						addStatus(TSTRING(IGNORED_MESSAGE) + _T(" ") + Text::toT(pm.str), WinUtil::m_ChatTextPrivate, false);
					} else if(BOOLSETTING(POPUP_BOT_PMS) || PrivateFrame::isOpen(user)) {
						PrivateFrame::gotMessage(pm.from, pm.to, pm.replyTo, Text::toT(pm.str), client);
					} else {
						addLine(TSTRING(PRIVATE_MESSAGE_FROM) + _T(" ") + nick + _T(": ") + Text::toT(pm.str), WinUtil::m_ChatTextPrivate);
					}
				} else {
					if(BOOLSETTING(POPUP_PMS) || PrivateFrame::isOpen(user)) {
						PrivateFrame::gotMessage(pm.from, pm.to, pm.replyTo, Text::toT(pm.str), client);
					} else {
						addLine(TSTRING(PRIVATE_MESSAGE_FROM) + _T(" ") + nick + _T(": ") + Text::toT(pm.str), WinUtil::m_ChatTextPrivate);
					}

					HWND hMainWnd = MainFrame::getMainFrame()->m_hWnd;//GetTopLevelWindow();
					::PostMessage(hMainWnd, WM_SPEAKER, MainFrame::SET_PM_TRAY_ICON, NULL);
				}
			}
		}
	}
	
	if(resort && showUsers) {
		ctrlUsers.resort();
		resort = false;
	}

	if(t.size() > 2) {
		ctrlUsers.SetRedraw(TRUE);
	}
	
	return 0;
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

		ctrlLastLines.SetMaxTipWidth(w[0]);

		// Strange, can't get the correct width of the last field...
		ctrlStatus.GetRect(4, sr);
		sr.left = sr.right + 2;
		sr.right = sr.left + 16;
		ctrlShowUsers.MoveWindow(sr);
	}
		
	int h = WinUtil::fontHeight + 4;

//ApexDC	
	if(SETTING(MAX_RESIZE_LINES) != 1 && lineCount != 0) {
		if(lineCount < SETTING(MAX_RESIZE_LINES)) {
			h = WinUtil::fontHeight * lineCount + 4;
		} else {
			h = WinUtil::fontHeight * SETTING(MAX_RESIZE_LINES) + 4;
		}
	}
//End

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

	int buttonsize = 24 +2;
	if(ctrlMagnet.IsWindow())
		buttonsize = buttonsize *2;

	rc = rect;
	rc.bottom -= 2;
	rc.top = rc.bottom - h - 5;
	rc.left +=2;
	rc.right -= (showUsers ? 200 : 0) + buttonsize;
	ctrlMessage.MoveWindow(rc);

//ApexDC
	if(h != (WinUtil::fontHeight + 4)) {
		rc.bottom -= h - (WinUtil::fontHeight + 4);
	}
//end

	rc.left = rc.right + 2;
	rc.right += 24;
	ctrlEmoticons.MoveWindow(rc);
	
	if(ctrlMagnet.IsWindow()){
		//magnet button
		rc.left = rc.right + 2;
		rc.right += 24;
		ctrlMagnet.MoveWindow(rc);
	}

	if(showUsers){
		rc.left = rc.right + 2;
		rc.right = rc.left + 116;
		ctrlFilter.MoveWindow(rc);

		rc.left = rc.right + 4;
		rc.right = rc.left + 76;
		rc.top = rc.top + 0;
		rc.bottom = rc.bottom + 120;
		ctrlFilterSel.MoveWindow(rc);
	} else {
		rc.left = 0;
		rc.right = 0;
		ctrlFilter.MoveWindow(rc);
		ctrlFilterSel.MoveWindow(rc);
	}
}

LRESULT HubFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
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
		TimerManager::getInstance()->removeListener(this);
		FavoriteManager::getInstance()->removeListener(this);
		client->removeListener(this);
		client->disconnect(true);

		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
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
		return 0;
	}
}

void HubFrame::clearUserList() {
	for(CtrlUsers::iterator i = ctrlUsers.begin(); i != ctrlUsers.end(); i++) {
		(*i).dec();
	}
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

		int i = ctrlClient.CharFromPos(pt);
		int line = ctrlClient.LineFromChar(i);
		int c = i - ctrlClient.LineIndex(line);
		int len = ctrlClient.LineLength(i);
		if(len < 3) {
			return 0;
		}

		TCHAR* buf = new TCHAR[len+1];
		ctrlClient.GetLine(line, buf, len+1);
		tstring x = tstring(buf, len-1);
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

	if(BOOLSETTING(LOG_MAIN_CHAT) | logMainChat) {
		ParamMap params;
		params["message"] = Text::fromT(aLine);
		client->getHubIdentity().getParams(params, "hub", false);
		params["hubURL"] = client->getHubUrl();
		client->getMyIdentity().getParams(params, "my", true);
		LOG(LogManager::CHAT, params);
	}

	if(timeStamps) {
		ctrlClient.AppendText(i, Text::toT(client->getCurrentNick()), Text::toT("[" + Util::getShortTimeString() + "] "), aLine + _T('\n'), cf, bUseEmo);
	} else {
		ctrlClient.AppendText(i, Text::toT(client->getCurrentNick()), Util::emptyStringT, aLine + _T('\n'), cf, bUseEmo);
	}
	if (BOOLSETTING(BOLD_HUB)) {
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
	if(BOOLSETTING(LOG_MAIN_CHAT)) {
		tabMenu.AppendMenu(MF_STRING, IDC_OPEN_HUB_LOG, CTSTRING(OPEN_HUB_LOG));
		tabMenu.AppendMenu(MF_SEPARATOR);
		tabMenu.AppendMenu(MF_STRING, IDC_HISTORY, CTSTRING(VIEW_HISTORY));
		tabMenu.AppendMenu(MF_SEPARATOR);
	}

	tabMenu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CTSTRING(CLEAR_CHAT));
	if(showchaticon) 
		tabMenu.AppendMenu(MF_CHECKED, IDC_NOTIFY, CTSTRING(NOTIFY));
	else
		tabMenu.AppendMenu(MF_UNCHECKED, IDC_NOTIFY, CTSTRING(NOTIFY));

	tabMenu.AppendMenu(MF_SEPARATOR);
	tabMenu.AppendMenu(MF_STRING, IDC_ADD_AS_FAVORITE, CTSTRING(ADD_TO_FAVORITES));
	tabMenu.AppendMenu(MF_STRING, ID_FILE_RECONNECT, CTSTRING(MENU_RECONNECT));
	tabMenu.AppendMenu(MF_SEPARATOR);
	tabMenu.AppendMenu(MF_STRING, IDC_OPEN_MY_LIST, CTSTRING(MENU_OPEN_HUB_OWN_LIST));
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
	//DirectoryListingFrame::openWindow(Text::toT(client->getShareProfile()), Text::toT(Util::emptyString), HintedUser(ClientManager::getInstance()->getMe(), client->getHubUrl()), 0, true);
	DirectoryListingManager::getInstance()->openOwnList(client->getShareProfile());
	return 0;
}

LRESULT HubFrame::onSetNotify(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	FavoriteHubEntry *fhe = FavoriteManager::getInstance()->getFavoriteHubEntry(Text::fromT(server));

	if(showchaticon){
		if(fhe)
			fhe->setChatNotify(false);

		showchaticon = false;
	}else{
		if(fhe)
			fhe->setChatNotify(true);

		showchaticon = true;
	}
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

LRESULT HubFrame::onGetToolTip(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMTTDISPINFO* nm = (NMTTDISPINFO*)pnmh;
	lastLines.clear();
	for(TStringIter i = lastLinesList.begin(); i != lastLinesList.end(); ++i) {
		lastLines += *i;
		lastLines += _T("\r\n");
	}
	if(lastLines.size() > 2) {
		lastLines.erase(lastLines.size() - 2);
	}
	nm->lpszText = const_cast<TCHAR*>(lastLines.c_str());

	return 0;
}

void HubFrame::addStatus(const tstring& aLine, CHARFORMAT2& cf, bool inChat /* = true */) {
	tstring line = _T("[") + Text::toT(Util::getShortTimeString()) + _T("] ") + aLine;
	TCHAR* sLine = (TCHAR*)line.c_str();

   	if(line.size() > 512) {
		sLine[512] = NULL;
	}

	if(BOOLSETTING(HUB_BOLD_TABS))
		setDirty();


	ctrlStatus.SetText(0, sLine);
	while(lastLinesList.size() + 1 > MAX_CLIENT_LINES)
		lastLinesList.erase(lastLinesList.begin());
	lastLinesList.push_back(sLine);

	if (BOOLSETTING(BOLD_HUB)) {
		setDirty();
	}
	
	if(BOOLSETTING(STATUS_IN_CHAT) && inChat) {
		addLine(_T("*** ") + aLine, cf, BOOLSETTING(HUB_BOLD_TABS));
	}
	if(BOOLSETTING(LOG_STATUS_MESSAGES)) {
		ParamMap params;
		client->getHubIdentity().getParams(params, "hub", false);
		params["hubURL"] = client->getHubUrl();
		client->getMyIdentity().getParams(params, "my", true);
		params["message"] = Text::fromT(aLine);
		LOG(LogManager::STATUS, params);
	}
}

void HubFrame::resortUsers() {
	for(FrameIter i = frames.begin(); i != frames.end(); ++i)
		i->second->resortForFavsFirst(true);
}

void HubFrame::closeDisconnected() {
	for(FrameIter i=frames.begin(); i!= frames.end(); ++i) {
		if (!(i->second->client->isConnected())) {
			i->second->PostMessage(WM_CLOSE);
		}
	}
}

void HubFrame::updateFonts() {
	for(FrameIter i=frames.begin(); i!= frames.end(); ++i) {
		i->second->setFonts();
		i->second->UpdateLayout();
	}
}

void HubFrame::reconnectDisconnected() {
	for(FrameIter i=frames.begin(); i!= frames.end(); ++i) {
		if (!(i->second->client->isConnected())) {
			i->second->client->disconnect(false); 
			i->second->clearUserList();
			i->second->client->connect(); 
		}
	}
};

void HubFrame::on(FavoriteManagerListener::UserAdded, const FavoriteUser& /*aUser*/) noexcept {
	resortForFavsFirst();
}
void HubFrame::on(FavoriteManagerListener::UserRemoved, const FavoriteUser& /*aUser*/) noexcept {
	resortForFavsFirst();
}

void HubFrame::resortForFavsFirst(bool justDoIt /* = false */) {
	if(justDoIt || BOOLSETTING(SORT_FAVUSERS_FIRST)) {
		resort = true;
		PostMessage(WM_SPEAKER);
	}
}

void HubFrame::on(Second, uint64_t /*aTick*/) noexcept {
	if(updateUsers) {
		updateStatusBar();
		updateUsers = false;
		PostMessage(WM_SPEAKER);
	}
}

void HubFrame::on(ClientListener::SetIcons, const Client*, int status) noexcept { 
	if(client->isReady() == true) {
		if(status == 2) {	
			setIcon(HubOpIcon);
		} else if(status == 1) {
			setIcon(HubRegIcon);
		} else {
			setIcon(HubIcon);
		}
	}
}

void HubFrame::on(Connecting, const Client*) noexcept { 
	if(BOOLSETTING(SEARCH_PASSIVE) && ClientManager::getInstance()->isActive(client->getHubUrl())) {
		addLine(TSTRING(ANTI_PASSIVE_SEARCH), WinUtil::m_ChatTextSystem);
	}
	speak(ADD_STATUS_LINE, STRING(CONNECTING_TO) + " " + client->getHubUrl() + " ...");
	speak(SET_WINDOW_TITLE, client->getHubUrl());
}
void HubFrame::on(Connected, const Client*) noexcept { 
	speak(CONNECTED);
}
void HubFrame::on(UserUpdated, const Client*, const OnlineUserPtr& user) noexcept {
	speak(UPDATE_USER_JOIN, user);
}
void HubFrame::on(UsersUpdated, const Client*, const OnlineUserList& aList) noexcept {
	for(OnlineUserList::const_iterator i = aList.begin(); i != aList.end(); ++i) {
		tasks.add(UPDATE_USER, unique_ptr<Task>(new UserTask(*i)));
	}
	updateUsers = true;
}

void HubFrame::on(ClientListener::UserRemoved, const Client*, const OnlineUserPtr& user) noexcept {
	speak(REMOVE_USER, user);
}

void HubFrame::on(Redirect, const Client*, const string& line) noexcept { 
	if(ClientManager::getInstance()->isConnected(line)) {
		speak(ADD_STATUS_LINE, STRING(REDIRECT_ALREADY_CONNECTED));
		return;
	}

	redirect = Text::toT(line);
	if(BOOLSETTING(AUTO_FOLLOW)) {
		PostMessage(WM_COMMAND, IDC_FOLLOW, 0);
	} else {
		speak(ADD_STATUS_LINE, STRING(PRESS_FOLLOW) + " " + line);
	}
}
void HubFrame::on(Failed, const Client*, const string& line) noexcept { 
	speak(ADD_STATUS_LINE, line); 
	speak(DISCONNECTED); 
}
void HubFrame::on(GetPassword, const Client*) noexcept { 
	speak(GET_PASSWORD);
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
	speak(SET_WINDOW_TITLE, hubName);
}
void HubFrame::on(Message, const Client*, const ChatMessage& message) noexcept {
	if(message.to && message.replyTo) {
		speak(PRIVATE_MESSAGE, message.from, message.to, message.replyTo, message.format());
	} else {
		speak(ADD_CHAT_LINE, message.from->getIdentity(), message.format());
	}
}	

void HubFrame::on(StatusMessage, const Client*, const string& line, int statusFlags) {
	//Messy, but kind of the shortest way not to override params
	if(BOOLSETTING(BOLD_HUB_TABS_ON_KICK) && (statusFlags & ClientListener::FLAG_IS_SPAM)){
		setDirty();
	}
	speak(ADD_STATUS_LINE, Text::toDOS(line), !BOOLSETTING(FILTER_MESSAGES) || !(statusFlags & ClientListener::FLAG_IS_SPAM));
}
void HubFrame::on(NickTaken, const Client*) noexcept {
	speak(ADD_STATUS_LINE, STRING(NICK_TAKEN));
}
void HubFrame::on(SearchFlood, const Client*, const string& line) noexcept {
	speak(ADD_STATUS_LINE, STRING(SEARCH_SPAM_FROM) + " " + line);
}

void HubFrame::on(HubTopic, const Client*, const string& line) noexcept {
	speak(ADD_STATUS_LINE, STRING(HUB_TOPIC) + "\t" + line);
}
void HubFrame::on(AddLine, const Client*, const string& line) noexcept {
	speak(ADD_STATUS_LINE, line);
}
void HubFrame::openLinksInTopic() {
	int length = GetWindowTextLength();
	TCHAR* buf = new TCHAR[length + 1];
	
	GetWindowText(buf, length);
	
	tstring topic = buf;
	delete[] buf;

	int pos = -1;
	TStringList urls;
	
	while( (pos = topic.find(_T("http://"), pos+1)) != string::npos ){
		int pos2 = topic.find(_T(" "), pos+1);
		urls.push_back(topic.substr(pos, pos2-pos));
	}
	pos = -1;
	while( (pos = topic.find(_T("www."), pos+1)) != string::npos ) {
		if(topic[pos-1] != _T('/')) {
			int pos2 = topic.find(_T(" "), pos+1);
			urls.push_back(topic.substr(pos, pos2-pos));
		}
	}

	pos = -1;
	while( (pos = topic.find(_T("https://"), pos+1)) != string::npos ){
		int pos2 = topic.find(_T(" "), pos+1);
		urls.push_back(topic.substr(pos, pos2-pos));
	}

	pos = -1;
	while( (pos = topic.find(_T("mms://"), pos+1)) != string::npos ){
		int pos2 = topic.find(_T(" "), pos+1);
		urls.push_back(topic.substr(pos, pos2-pos));
	}

	pos = -1;
	while( (pos = topic.find(_T("ftp://"), pos+1)) != string::npos ){
		int pos2 = topic.find(_T(" "), pos+1);
		urls.push_back(topic.substr(pos, pos2-pos));
	}

	for( TStringIter i = urls.begin(); i != urls.end(); ++i ) {
		WinUtil::openLink((*i));
	}
}

LRESULT HubFrame::onFilterChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	if(uMsg == WM_CHAR && wParam == VK_TAB) {
		handleTab(WinUtil::isShift());
		return 0;
	}
	
	if(!BOOLSETTING(FILTER_ENTER) || (wParam == VK_RETURN)) {
		TCHAR *buf = new TCHAR[ctrlFilter.GetWindowTextLength()+1];
		ctrlFilter.GetWindowText(buf, ctrlFilter.GetWindowTextLength()+1);
		filter = buf;
		delete[] buf;
	
		updateUserList();
		updateUsers = true;
	}

	bHandled = FALSE;

	return 0;
}

LRESULT HubFrame::onSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
	TCHAR *buf = new TCHAR[ctrlFilter.GetWindowTextLength()+1];
	ctrlFilter.GetWindowText(buf, ctrlFilter.GetWindowTextLength()+1);
	filter = buf;
	delete[] buf;
	
	updateUserList();
	updateUsers = true;

	bHandled = FALSE;

	return 0;
}

bool HubFrame::parseFilter(FilterModes& mode, int64_t& size) {
	tstring::size_type start = (tstring::size_type)tstring::npos;
	tstring::size_type end = (tstring::size_type)tstring::npos;
	int64_t multiplier = 1;
	
	if(filter.empty()) {
		return false;
	}
	if(filter.compare(0, 2, _T(">=")) == 0) {
		mode = GREATER_EQUAL;
		start = 2;
	} else if(filter.compare(0, 2, _T("<=")) == 0) {
		mode = LESS_EQUAL;
		start = 2;
	} else if(filter.compare(0, 2, _T("==")) == 0) {
		mode = EQUAL;
		start = 2;
	} else if(filter.compare(0, 2, _T("!=")) == 0) {
		mode = NOT_EQUAL;
		start = 2;
	} else if(filter[0] == _T('<')) {
		mode = LESS;
		start = 1;
	} else if(filter[0] == _T('>')) {
		mode = GREATER;
		start = 1;
	} else if(filter[0] == _T('=')) {
		mode = EQUAL;
		start = 1;
	}

	if(start == tstring::npos)
		return false;
	if(filter.length() <= start)
		return false;

	if((end = Util::findSubString(filter, _T("TiB"))) != tstring::npos) {
		multiplier = 1024LL * 1024LL * 1024LL * 1024LL;
	} else if((end = Util::findSubString(filter, _T("GiB"))) != tstring::npos) {
		multiplier = 1024*1024*1024;
	} else if((end = Util::findSubString(filter, _T("MiB"))) != tstring::npos) {
		multiplier = 1024*1024;
	} else if((end = Util::findSubString(filter, _T("KiB"))) != tstring::npos) {
		multiplier = 1024;
	} else if((end = Util::findSubString(filter, _T("TB"))) != tstring::npos) {
		multiplier = 1000LL * 1000LL * 1000LL * 1000LL;
	} else if((end = Util::findSubString(filter, _T("GB"))) != tstring::npos) {
		multiplier = 1000*1000*1000;
	} else if((end = Util::findSubString(filter, _T("MB"))) != tstring::npos) {
		multiplier = 1000*1000;
	} else if((end = Util::findSubString(filter, _T("kB"))) != tstring::npos) {
		multiplier = 1000;
	} else if((end = Util::findSubString(filter, _T("B"))) != tstring::npos) {
		multiplier = 1;
	}

	if(end == tstring::npos) {
		end = filter.length();
	}
	
	tstring tmpSize = filter.substr(start, end-start);
	size = static_cast<int64_t>(Util::toDouble(Text::fromT(tmpSize)) * multiplier);
	
	return true;
}

void HubFrame::updateUserList(OnlineUserPtr ui) {
	int64_t size = -1;
	FilterModes mode = NONE;
	
	//single update?
	//avoid refreshing the whole list and just update the current item
	//instead
	if(ui != NULL) {
		if(ui->isHidden()) {
			return;
		}
		if(filter.empty()) {
			if(ctrlUsers.findItem(ui.get()) == -1) {
				ui->inc();
				ctrlUsers.insertItem(ui.get(), UserInfoBase::getImage(ui->getIdentity(), client));
			}
		} else {
			int sel = ctrlFilterSel.GetCurSel();
			bool doSizeCompare = sel == OnlineUser::COLUMN_SHARED && parseFilter(mode, size);
			boost::wregex reg(filter, boost::regex_constants::icase);

			if(matchFilter(*ui, sel, reg, doSizeCompare, mode, size)) {
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
			for(OnlineUserList::const_iterator i = l.begin(); i != l.end(); ++i){
				const OnlineUserPtr& ui = *i;
				if(!ui->isHidden()) {
					ui->inc();
					ctrlUsers.insertItem(ui.get(), UserInfoBase::getImage(ui->getIdentity(), client));
				}
			}
		} else {
			int sel = ctrlFilterSel.GetCurSel();
			bool doSizeCompare = sel == OnlineUser::COLUMN_SHARED && parseFilter(mode, size);
			boost::wregex reg(filter, boost::regex_constants::icase);

			for(OnlineUserList::const_iterator i = l.begin(); i != l.end(); ++i) {
				const OnlineUserPtr& ui = *i;
				if(!ui->isHidden() && matchFilter(*ui, sel, reg, doSizeCompare, mode, size)) {
					ui->inc();
					ctrlUsers.insertItem(ui.get(), UserInfoBase::getImage(ui->getIdentity(), client));
				}
			}
		}
		ctrlUsers.SetRedraw(TRUE);
	}
}

void HubFrame::handleTab(bool reverse) {
	HWND focus = GetFocus();

	if(reverse) {
		if(focus == ctrlFilterSel.m_hWnd) {
			ctrlFilter.SetFocus();
		} else if(focus == ctrlFilter.m_hWnd) {
			ctrlMessage.SetFocus();
		} else if(focus == ctrlMessage.m_hWnd) {
			ctrlUsers.SetFocus();
		} else if(focus == ctrlUsers.m_hWnd) {
			ctrlClient.SetFocus();
		} else if(focus == ctrlClient.m_hWnd) {
			ctrlFilterSel.SetFocus();
		}
	} else {
		if(focus == ctrlClient.m_hWnd) {
			ctrlUsers.SetFocus();
		} else if(focus == ctrlUsers.m_hWnd) {
			ctrlMessage.SetFocus();
		} else if(focus == ctrlMessage.m_hWnd) {
			ctrlFilter.SetFocus();
		} else if(focus == ctrlFilter.m_hWnd) {
			ctrlFilterSel.SetFocus();
		} else if(focus == ctrlFilterSel.m_hWnd) {
			ctrlClient.SetFocus();
		}
	}
}

bool HubFrame::matchFilter(const OnlineUser& ui, int sel, const boost::wregex aReg, bool doSizeCompare, FilterModes mode, int64_t size) {
	if(filter.empty())
		return true;

	bool insert = false;
	if(doSizeCompare) {
		switch(mode) {
			case EQUAL: insert = (size == ui.getIdentity().getBytesShared()); break;
			case GREATER_EQUAL: insert = (size <=  ui.getIdentity().getBytesShared()); break;
			case LESS_EQUAL: insert = (size >=  ui.getIdentity().getBytesShared()); break;
			case GREATER: insert = (size < ui.getIdentity().getBytesShared()); break;
			case LESS: insert = (size > ui.getIdentity().getBytesShared()); break;
			case NOT_EQUAL: insert = (size != ui.getIdentity().getBytesShared()); break;
			case NONE: ; break;
		}
	} else {
		try {
			if(sel >= OnlineUser::COLUMN_LAST) {
				for(uint8_t i = OnlineUser::COLUMN_FIRST; i < OnlineUser::COLUMN_LAST; ++i) {
					tstring s = ui.getText(i);
					if(boost::regex_search(s.begin(), s.end(), aReg)) {
						insert = true;
						break;
					}
				}
			} else {
				tstring s = ui.getText(static_cast<uint8_t>(sel));
				if(boost::regex_search(s.begin(), s.end(), aReg))
					insert = true;
			}
		} catch(...) {
			insert = true;
		}
	}

	return insert;
}

//void HubFrame::addClientLine(const tstring& aLine, CHARFORMAT2& cf, bool inChat /* = true */) {
/*	tstring line = _T("[") + Text::toT(Util::getShortTimeString()) + _T("] ") + aLine;

	ctrlStatus.SetText(0, line.c_str());
	while(lastLinesList.size() + 1 > MAX_CLIENT_LINES)
		lastLinesList.erase(lastLinesList.begin());
	lastLinesList.push_back(line);
	
	if (BOOLSETTING(BOLD_HUB)) {
		setDirty();
	}
	
	if(BOOLSETTING(STATUS_IN_CHAT) && inChat) {
		addLine(_T("*** ") + aLine, cf);
	}
}
*/
bool HubFrame::PreparePopupMenu(CWindow *pCtrl, OMenu& menu ) {
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

		if(BOOLSETTING(LOG_PRIVATE_CHAT)) {
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
   

	if(AirUtil::isAdcHub(client->getHubUrl()))
		menu.SetMenuDefaultItem(IDC_BROWSELIST);
	else
		menu.SetMenuDefaultItem( IDC_GETLIST );

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
	OnlineUserPtr ui = NULL;

	int i = -1;
	if((i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ui = ctrlUsers.getItemData(i);
	}

	if(ui == NULL) return 0;

	params["userNI"] = ui->getIdentity().getNick();
	params["hubNI"] = client->getHubName();
	params["myNI"] = client->getMyNick();
	params["userCID"] = ui->getUser()->getCID().toBase32();
	params["hubURL"] = client->getHubUrl();
	string file = LogManager::getInstance()->getPath(LogManager::PM, params);
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
	ctrlUsers.SetImageList(WinUtil::userImages, LVSIL_SMALL);
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
			if( BOOLSETTING(USE_HIGHLIGHT) ) {
				
				ColorList *cList = HighlightManager::getInstance()->getList();
				for(ColorIter i = cList->begin(); i != cList->end(); ++i) {
					ColorSettings* cs = &(*i);
					string str;
					if(cs->getContext() == HighlightManager::CONTEXT_NICKLIST) {
						if(cs->usingRegexp()) {
							try {
								//have to have $Re:
								tstring nick = Text::toT(ui->getIdentity().getNick());
								if(boost::regex_search(nick.begin(), nick.end(), cs->regexp)){
									if(cs->getHasFgColor()) cd->clrText = cs->getFgColor();
									break;
								}
							}catch(...) {}
						} else {
							if (Wildcard::patternMatch(Text::utf8ToAcp(ui->getIdentity().getNick()), Text::utf8ToAcp(Text::fromT(cs->getMatch())), '|')){
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
		if(BOOLSETTING(GET_USER_COUNTRY) && (ctrlUsers.findColumn(cd->iSubItem) == OnlineUser::COLUMN_IP)) {
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

				WinUtil::flagImages.Draw(cd->nmcd.hdc, flagIndex, p, LVSIL_SMALL);
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
	ctrlClient.SetFont(WinUtil::font, FALSE);
	ctrlMessage.SetFont(WinUtil::font, FALSE);
	ctrlFilter.SetFont(WinUtil::font, FALSE);
	ctrlFilterSel.SetFont(WinUtil::font, FALSE);
		
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