/* 
 * Copyright (C) 2001-2014 Jacek Sieka, arnetheduck on gmail point com
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

#include "PrivateFrame.h"
#include "WinUtil.h"
#include "MainFrm.h"

#include "../client/Client.h"
#include "../client/ClientManager.h"
#include "../client/Util.h"
#include "../client/LogManager.h"
#include "../client/UploadManager.h"
#include "../client/FavoriteManager.h"
#include "../client/StringTokenizer.h"
#include "../client/ResourceManager.h"
#include "../client/Adchub.h"

bool PrivateFrame::gotMessage(const ChatMessage& aMessage, Client* c) {
	bool myPM = aMessage.replyTo->getUser() == ClientManager::getInstance()->getMe();
	const UserPtr& user = myPM ? aMessage.to->getUser() : aMessage.replyTo->getUser();

	auto chat = MessageManager::getInstance()->getChat(user);
	if (chat) {
		chat->Message(aMessage);
		return true;
	}

	auto hintedUser = HintedUser(user, c->getHubUrl());
	PrivateFrame* p = new PrivateFrame(hintedUser, c);
	auto text = Text::toT(aMessage.format());
	p->addLine(aMessage.from->getIdentity(), text);

	if (!myPM) {
		p->handleNotifications(true, text, aMessage.from->getIdentity());
	}
	return true;
}

void PrivateFrame::openWindow(const HintedUser& replyTo, const tstring& msg, Client* c) {
	auto chat = MessageManager::getInstance()->getChat(replyTo.user);
	
	if (chat) {
		chat->Activate(Text::fromT(msg), c);
	} else {
		PrivateFrame* p = new PrivateFrame(replyTo, c);
		p->CreateEx(WinUtil::mdiClient);
		p->sendFrameMessage(msg);
	}
}


PrivateFrame::PrivateFrame(const HintedUser& replyTo_, Client* c) :
created(false), closed(false), online(replyTo_.user->isOnline()), curCommandPosition(0),
	ctrlHubSelContainer(WC_COMBOBOXEX, this, HUB_SEL_MAP),
	ctrlMessageContainer(WC_EDIT, this, EDIT_MESSAGE_MAP),
	ctrlClientContainer(WC_EDIT, this, EDIT_MESSAGE_MAP),
	ctrlStatusContainer(STATUSCLASSNAME, this, STATUS_MSG_MAP),
	UserInfoBaseHandler(false, true), hasUnSeenMessages(false), isTyping(false), userTyping(false)
{
	chat = MessageManager::getInstance()->addChat(replyTo_);
	ctrlClient.setClient(c);
	ctrlClient.setPmUser(replyTo_.user);
}

LRESULT PrivateFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	RECT r = { 0, 0, 0, 150 };
	ctrlHubSel.Create(ctrlStatus.m_hWnd, r, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST , 0, IDC_HUB);

	ctrlHubSel.SetFont(WinUtil::systemFont);
	ctrlHubSelContainer.SubclassWindow(ctrlHubSel.m_hWnd);
	ctrlHubSel.SetImageList(ResourceLoader::getHubImages());
	
	init(m_hWnd, rcDefault);
	
	CToolInfo ti_tool(TTF_SUBCLASS, ctrlStatus.m_hWnd, STATUS_CC + POPUP_UID, 0, LPSTR_TEXTCALLBACK);
	ctrlTooltips.AddTool(&ti_tool);

	ctrlClientContainer.SubclassWindow(ctrlClient.m_hWnd);
	ctrlMessageContainer.SubclassWindow(ctrlMessage.m_hWnd);
	ctrlStatusContainer.SubclassWindow(ctrlStatus.m_hWnd);
	
	bool userBot = getUser() && getUser()->isSet(User::BOT);
	userOffline = userBot ? ResourceLoader::loadIcon(IDI_BOT_OFF) : ResourceLoader::loadIcon(IDR_PRIVATE_OFF);

	iCCReady = ResourceLoader::loadIcon(IDI_SECURE, 16);
	iStartCC = ResourceLoader::convertGrayscaleIcon(ResourceLoader::loadIcon(IDI_SECURE, 16));
	iNoCCPM = ResourceLoader::mergeIcons(iStartCC, ResourceLoader::loadIcon(IDI_USER_NOCONNECT, 16), 16); //Temp, Todo: find own icon for this!

	created = true;

	ClientManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);
	chat->addListener(this);

	readLog();

	WinUtil::SetIcon(m_hWnd, userBot ? IDI_BOT : IDR_PRIVATE);

	//add the updateonlinestatus in the wnd message queue so the frame and tab can finish creating first.
	chat->checkAlwaysCCPM();
	callAsync([this] { updateOnlineStatus(); });

	bHandled = FALSE;
	return 1;
}
	
LRESULT PrivateFrame::onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	HWND hWnd = (HWND)lParam;
	HDC hDC = (HDC)wParam;
	if(hWnd == ctrlClient.m_hWnd || hWnd == ctrlMessage.m_hWnd) {
		::SetBkColor(hDC, WinUtil::bgColor);
		::SetTextColor(hDC, WinUtil::textColor);
		return (LRESULT)WinUtil::bgBrush;
	}

	bHandled = FALSE;
	return FALSE;
}

LRESULT PrivateFrame::onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ctrlMessage.SetFocus();
	return 0;
}

LRESULT PrivateFrame::onFocusMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	bHandled = FALSE;
	sendSeen();
	return 0;
}
	
void PrivateFrame::addClientLine(const tstring& aLine, uint8_t severity) {
	if(!created) {
		CreateEx(WinUtil::mdiClient);
	}
	addStatus(aLine, ResourceLoader::getSeverityIcon(severity));
	if (SETTING(BOLD_PM)) {
		setDirty();
	}
}

void PrivateFrame::addStatus(const tstring& aLine, const CIcon& aIcon) {
	lastStatus = { aLine, aIcon };
	setStatusText(aLine, aIcon);
}

void PrivateFrame::updatePMInfo(uint8_t aType) {
	if (!created) {
		CreateEx(WinUtil::mdiClient);
	}

	switch (aType) {
	case PrivateChat::MSG_SEEN:
		if (!userTyping)
			addStatus(_T("*** Message seen ***"), ResourceLoader::loadIcon(IDI_ONLINE, 16));
		else
			lastStatus = { _T("*** Message seen ***"), ResourceLoader::loadIcon(IDI_ONLINE, 16) };
		break;
	case PrivateChat::TYPING_ON:
		//setStatusText to prevent saving lastStatus
		userTyping = true;
		setStatusText(_T("*** The user is typing...***"), ResourceLoader::loadIcon(IDR_TRAY_PM, 16));
		break;
	case PrivateChat::TYPING_OFF:
		//Restore the previous status
		userTyping = false;
		if (lastStatus.first.empty())
			addClientLine(TSTRING(LAST_CHANGE) + _T(" ") + Text::toT(Util::getTimeString()), LogManager::LOG_INFO);
		else
			addStatus(lastStatus.first, lastStatus.second);
		break;
	}

	if (SETTING(BOLD_PM)) {
		setDirty();
	}
}

LRESULT PrivateFrame::onStatusBarClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
	if (!getUser()->isOnline() || getUser()->isNMDC())
		return 0;
	
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	CRect rect;
	ctrlStatus.GetRect(STATUS_CC, rect);
	if (PtInRect(&rect, pt)) {
		if (ccReady())
			closeCC();
		else
			chat->StartCC();
	}
	bHandled = TRUE;
	return 0;
}

LRESULT PrivateFrame::onGetToolTip(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	if (getUser()->isNMDC())
		return 0;
	
	LPNMTTDISPINFO pDispInfo = (LPNMTTDISPINFO)pnmh;
	pDispInfo->szText[0] = 0;
	if (idCtrl == STATUS_CC + POPUP_UID) {
		lastCCPMError = Text::toT(chat->getLastCCPMError());
		pDispInfo->lpszText = !chat->getSupportsCCPM() ? const_cast<TCHAR*>(lastCCPMError.c_str()):
			ccReady() ? (LPWSTR)CTSTRING(DISCONNECT_CCPM) : (LPWSTR)CTSTRING(START_CCPM);
	}
	return 0;
}


LRESULT PrivateFrame::OnRelayMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	LPMSG pMsg = (LPMSG)lParam;
	if(ctrlTooltips.m_hWnd != NULL && pMsg->message >= WM_MOUSEFIRST && pMsg->message <= WM_MOUSELAST)
		ctrlTooltips.RelayEvent(pMsg);
	return 0;
}

LRESULT PrivateFrame::onHubChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
	changeClient();
	updateOnlineStatus(true);

	bHandled = FALSE;
	return 0;
}

void PrivateFrame::addStatusLine(const tstring& aLine, uint8_t severity) {
	tstring status = _T(" *** ") + aLine + _T(" ***");
	if (SETTING(STATUS_IN_CHAT)) {
		addLine(status, WinUtil::m_ChatTextServer);
	}
	addClientLine(status, severity);
	
}

void PrivateFrame::changeClient() {
	chat->setHubUrl(hubs[ctrlHubSel.GetCurSel()].first);
	ctrlClient.setClient(chat->getClient());
}

void PrivateFrame::updateOnlineStatus(bool ownChange) {
	const CID& cid = getUser()->getCID();

	//get the hub and online status
	auto hubsInfoNew = move(WinUtil::getHubNames(cid));
	bool UserOnline = hubsInfoNew.second;
	if (!UserOnline && !online) {
		//nothing to update... probably a delayed event or we are opening the tab for the first time
		if (nicks.empty())
			nicks = WinUtil::getNicks(chat->getHintedUser());
		if (hubNames.empty())
			hubNames = TSTRING(OFFLINE);
		
		ctrlClient.setClient(nullptr);
		setDisconnected(true);
		showHubSelection(false);
		updateTabIcon(true);
	} else {
		auto oldSel = ctrlHubSel.GetStyle() & WS_VISIBLE ? ctrlHubSel.GetCurSel() : 0;
		StringPair oldHubPair;
		if (!hubs.empty() && oldSel != -1)
			oldHubPair = hubs[oldSel]; // cache the old hub name

		hubs = ClientManager::getInstance()->getHubs(cid);
		while (ctrlHubSel.GetCount()) {
			ctrlHubSel.DeleteString(0);
		}

		//General things
		if (UserOnline) {
			//the user is online
			hubNames = WinUtil::getHubNames(chat->getHintedUser());
			nicks = WinUtil::getNicks(chat->getHintedUser());
			setDisconnected(false);
			updateTabIcon(false);

			if (!online) {
				addStatusLine(TSTRING(USER_WENT_ONLINE) + _T(" [") + nicks + _T(" - ") + hubNames + _T("]"), LogManager::LOG_INFO);
			}
		} else {
			if (nicks.empty())
				nicks = WinUtil::getNicks(chat->getHintedUser());

			updateTabIcon(true);
			setDisconnected(true);
			ctrlClient.setClient(nullptr);
			addStatusLine(TSTRING(USER_WENT_OFFLINE) + _T(" [") + hubNames + _T("]"), LogManager::LOG_INFO);
		}

		//ADC related changes
		if (!ccReady() && UserOnline && !getUser()->isNMDC() && !hubs.empty()) {
			if (!(ctrlHubSel.GetStyle() & WS_VISIBLE)) {
				showHubSelection(true);
			}

			fillHubSelection();

			if (ownChange && ctrlHubSel.GetCurSel() != -1) {
				addStatusLine(CTSTRING_F(MESSAGES_SENT_THROUGH, Text::toT(hubs[ctrlHubSel.GetCurSel()].second)), LogManager::LOG_INFO);
			} else if (ctrlHubSel.GetCurSel() == -1) {
				//the hub was not found
				ctrlHubSel.SetCurSel(0);
				changeClient();
				if (!online) //the user came online but not in the previous hub
					addStatusLine(CTSTRING_F(MESSAGES_SENT_THROUGH, Text::toT(hubs[ctrlHubSel.GetCurSel()].second)), LogManager::LOG_WARNING);
				else
					addStatusLine(CTSTRING_F(USER_OFFLINE_PM_CHANGE, Text::toT(oldHubPair.second) % Text::toT(hubs[0].second)), LogManager::LOG_WARNING);
			}
			else if (!oldHubPair.first.empty() && oldHubPair.first != getHubUrl()) {
				addStatusLine(CTSTRING_F(MESSAGES_SENT_THROUGH_REMOTE, Text::toT(hubs[ctrlHubSel.GetCurSel()].second)), LogManager::LOG_WARNING);
			} else if (!ctrlClient.getClient()) {
				changeClient();
			}
		} else if (ccReady()) {
			fillHubSelection();
			//Must be a better way to detect hub changes than using the Combobox.
			if (ctrlHubSel.GetCurSel() == -1) {
				//the hub was not found
				ctrlHubSel.SetCurSel(0);
				changeClient();
			}
			if(ctrlHubSel.GetStyle() & WS_VISIBLE)
				showHubSelection(false);
		} else {
			showHubSelection(false);
		}

		online = UserOnline;
	}
	updateStatusBar();

	SetWindowText((nicks + _T(" - ") + hubNames).c_str());
}

void PrivateFrame::fillHubSelection() {
	auto* cm = ClientManager::getInstance();
	auto idents = cm->getIdentities(cm->getMe());

	for (const auto& hub : hubs) {
		auto me = idents.find(hub.first);
		int img = me == idents.end() ? 0 : me->second.isOp() ? 2 : me->second.isRegistered() ? 1 : 0;
		auto i = ctrlHubSel.AddItem(Text::toT(hub.second).c_str(), img, img, 0);
		if (hub.first == getHubUrl()) {
			ctrlHubSel.SetCurSel(i);
		}
	}
}
void PrivateFrame::showHubSelection(bool show) {
	ctrlHubSel.ShowWindow(show);
	ctrlHubSel.EnableWindow(show);
}

void PrivateFrame::handleNotifications(bool newWindow, const tstring& aMessage, const Identity& from) {
	hasUnSeenMessages = true;
	userTyping = false;
	addClientLine(TSTRING(LAST_CHANGE) + _T(" ") + Text::toT(Util::getTimeString()), LogManager::LOG_INFO);
	
	if (!getUser()->isSet(User::BOT))
		MainFrame::getMainFrame()->onChatMessage(true);

	if (newWindow && AirUtil::getAway() && (!(SETTING(NO_AWAYMSG_TO_BOTS) && getUser()->isSet(User::BOT))))
		{
			ParamMap params;
			from.getParams(params, "user", false);

			string error;
			sendMessage(Text::toT(AirUtil::getAwayMessage(getAwayMessage(), params)), error);
		}

	if ((SETTING(FLASH_WINDOW_ON_PM) && !SETTING(FLASH_WINDOW_ON_NEW_PM)) || 
		(newWindow && SETTING(FLASH_WINDOW_ON_NEW_PM))) {
		WinUtil::FlashWindow();
	}

	if ((newWindow && SETTING(POPUP_NEW_PM)) || SETTING(POPUP_PM)) {
		if (SETTING(PM_PREVIEW)) {
			tstring message = aMessage.substr(0, 250);
			WinUtil::showPopup(message.c_str(), CTSTRING(PRIVATE_MESSAGE));
		} else {
			WinUtil::showPopup(WinUtil::getNicks(chat->getHintedUser()) + _T(" - ") + hubNames, TSTRING(PRIVATE_MESSAGE));
		}
	}

	if ((SETTING(PRIVATE_MESSAGE_BEEP) || (newWindow && SETTING(PRIVATE_MESSAGE_BEEP_OPEN))) && (!SETTING(SOUNDS_DISABLED))) {
		if (SETTING(BEEPFILE).empty()) {
			MessageBeep(MB_OK);
		} else {
			WinUtil::playSound(Text::toT(SETTING(BEEPFILE)));
		}
	}
}

/*
 update the re used frame to the correct hub, 
 so it doesnt appear offline while user is sending us messages with another hub :P
*/
void PrivateFrame::checkClientChanged(Client* c, bool ownChange) {

	if(!getUser()->isNMDC() && getHubUrl() != c->getHubUrl()) {
		chat->setHubUrl(c->getHubUrl());
		ctrlClient.setClient(c);
		updateOnlineStatus(ownChange);
	}
}

bool PrivateFrame::checkFrameCommand(tstring& cmd, tstring& /*param*/, tstring& /*message*/, tstring& status, bool& /*thirdPerson*/) { 
	if(stricmp(cmd.c_str(), _T("grant")) == 0) {
		UploadManager::getInstance()->reserveSlot(HintedUser(chat->getHintedUser()), 600);
		addClientLine(TSTRING(SLOT_GRANTED), LogManager::LOG_INFO);
	} else if(stricmp(cmd.c_str(), _T("close")) == 0) {
		PostMessage(WM_CLOSE);
	} else if((stricmp(cmd.c_str(), _T("favorite")) == 0) || (stricmp(cmd.c_str(), _T("fav")) == 0)) {
		FavoriteManager::getInstance()->addFavoriteUser(chat->getHintedUser());
		addClientLine(TSTRING(FAVORITE_USER_ADDED), LogManager::LOG_INFO);
	} else if(stricmp(cmd.c_str(), _T("getlist")) == 0) {
		handleGetList();
	} else if(stricmp(cmd.c_str(), _T("log")) == 0) {
		WinUtil::openFile(Text::toT(getLogPath()));
	}
	else if (Util::stricmp(cmd.c_str(), _T("direct")) == 0 || Util::stricmp(cmd.c_str(), _T("encrypted")) == 0) {
		chat->StartCC();
	}
	else if (Util::stricmp(cmd.c_str(), _T("disconnect")) == 0) {
		closeCC();
	} else if(stricmp(cmd.c_str(), _T("help")) == 0) {
		status = _T("*** ") + ChatFrameBase::commands + _T("Additional commands for private message tabs: /getlist, /grant, /favorite");
	} else {
		return false;
	}

	return true;
}

bool PrivateFrame::sendMessage(const tstring& msg, string& error_, bool thirdPerson) {

	if (getUser()->isOnline()) {
		callAsync([=] { isTyping = false; });
		return chat->sendPrivateMessage(chat->getHintedUser(), Text::fromT(msg), error_, thirdPerson);
	}
	error_ = STRING(USER_OFFLINE);
	return false;
}

LRESULT PrivateFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		LogManager::getInstance()->removePmCache(getUser());
		ClientManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		chat->removeListener(this);
		MessageManager::getInstance()->removeChat(getUser());

		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		bHandled = FALSE;
		return 0;
	}
}

void PrivateFrame::closeCC(bool silent) {
	if (ccReady()) {
		if (!silent) { addStatusLine(TSTRING(CCPM_DISCONNECTING),LogManager::LOG_INFO); }
		ConnectionManager::getInstance()->disconnect(getUser(), CONNECTION_TYPE_PM);
	}
}

bool PrivateFrame::ccReady() const {
	return chat->ccReady();
}

void PrivateFrame::addLine(const tstring& aLine, CHARFORMAT2& cf) {
	Identity i = Identity(NULL, 0);
    addLine(i, aLine, cf);
}

void PrivateFrame::addLine(const Identity& from, const tstring& aLine) {
	addLine(from, aLine, WinUtil::m_ChatTextGeneral );
}

void PrivateFrame::fillLogParams(ParamMap& params) const {
	const CID& cid = getUser()->getCID();;
	params["hubNI"] = [&] { return Text::fromT(hubNames); };
	params["hubURL"] = [&] { return getHubUrl(); };
	params["userCID"] = [&cid] { return cid.toBase32(); };
	params["userNI"] = [&] { return ClientManager::getInstance()->getNick(getUser(), getHubUrl()); };
	params["myCID"] = [] { return ClientManager::getInstance()->getMe()->getCID().toBase32(); };
}

void PrivateFrame::addLine(const Identity& from, const tstring& aLine, CHARFORMAT2& cf) {
	if(!created) {
		if(SETTING(POPUNDER_PM))
			WinUtil::hiddenCreateEx(this);
		else
			CreateEx(WinUtil::mdiClient);
	}

	CRect r;
	ctrlClient.GetClientRect(r);

	if(SETTING(LOG_PRIVATE_CHAT)) {
		ParamMap params;
		params["message"] = [&aLine] { return Text::fromT(aLine); };
		fillLogParams(params);
		LogManager::getInstance()->log(getUser(), params);
	}

	auto myNick = Text::toT(ctrlClient.getClient() ? ctrlClient.getClient()->get(HubSettings::Nick) : SETTING(NICK));
	bool notify = ctrlClient.AppendChat(from, myNick, SETTING(TIME_STAMPS) ? Text::toT("[" + Util::getShortTimeString() + "] ") : _T(""), aLine + _T('\n'), cf);
	//addClientLine(TSTRING(LAST_CHANGE) + _T(" ") + Text::toT(Util::getTimeString()), LogManager::LOG_INFO);

	if(notify)
		setNotify();

	if (SETTING(BOLD_PM)) {
		setDirty();
	}
}

LRESULT PrivateFrame::onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlClient.SetWindowText(_T(""));
	return 0;
}

LRESULT PrivateFrame::onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 

	OMenu tabMenu;
	tabMenu.CreatePopupMenu();	

	tabMenu.InsertSeparatorFirst(Text::toT(ClientManager::getInstance()->getNick(getUser(), getHubUrl(), true)));
	if(SETTING(LOG_PRIVATE_CHAT)) {
		tabMenu.AppendMenu(MF_STRING, IDC_OPEN_USER_LOG,  CTSTRING(OPEN_USER_LOG));
		tabMenu.AppendMenu(MF_SEPARATOR);
		tabMenu.AppendMenu(MF_STRING, IDC_USER_HISTORY,  CTSTRING(VIEW_HISTORY));
		tabMenu.AppendMenu(MF_SEPARATOR);
	}
	tabMenu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CTSTRING(CLEAR_CHAT));
	appendUserItems(tabMenu, true, getUser());

	prepareMenu(tabMenu, UserCommand::CONTEXT_USER, ClientManager::getInstance()->getHubUrls(getUser()->getCID()));
	if(!(tabMenu.GetMenuState(tabMenu.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR)) {	
		tabMenu.AppendMenu(MF_SEPARATOR);
	}
	tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));

	tabMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt);
	return TRUE;
}

void PrivateFrame::runUserCommand(UserCommand& uc) {

	if(!WinUtil::getUCParams(m_hWnd, uc, ucLineParams))
		return;

	auto ucParams = ucLineParams;

	ClientManager::getInstance()->userCommand(chat->getHintedUser(), uc, ucParams, true);
}

void PrivateFrame::updateStatusBar() {
	tstring tmp = Util::emptyStringT;
	if(ctrlHubSel.GetStyle() & WS_VISIBLE) {
		tmp = _T(" ") + TSTRING(SEND_PM_VIA);
		ctrlStatus.SetIcon(STATUS_CC, chat->getSupportsCCPM() ? iStartCC : iNoCCPM);
	} else if(ccReady()){
		tmp = _T(" ") + TSTRING(SEND_PM_VIA) +_T(": ") + TSTRING(DIRECT_ENCRYPTED_CHANNEL);
		ctrlStatus.SetIcon(STATUS_CC, iCCReady);
	} else {
		ctrlStatus.SetIcon(STATUS_CC, NULL);
	}

	ctrlStatus.SetText(STATUS_HUBSEL, tmp.c_str());
}

void PrivateFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);

	if(ctrlStatus.IsWindow()) {
		
		auto setToolRect = [&] {
			CRect r;
			ctrlStatus.GetRect(STATUS_CC, r);
			ctrlTooltips.SetToolRect(ctrlStatus.m_hWnd, STATUS_CC + POPUP_UID, r);
		};

		CRect sr;
		ctrlStatus.GetClientRect(sr);

		int w[STATUS_LAST];
		int desclen = WinUtil::getTextWidth(TSTRING(SEND_PM_VIA), ctrlStatus.m_hWnd) +2;
		int desclen2 = WinUtil::getTextWidth(TSTRING(DIRECT_ENCRYPTED_CHANNEL), ctrlStatus.m_hWnd) +2;

		if (desclen2 < 190) //Make sure the HubSel Combo will fit too.
			desclen2 = 190;

		w[STATUS_TEXT] = sr.right - desclen - desclen2 - 30;
		w[STATUS_CC] = w[STATUS_TEXT] + 22;
		w[STATUS_HUBSEL] = w[STATUS_CC] + desclen + desclen2 +8;
		ctrlStatus.SetParts(STATUS_LAST, w);
		setToolRect();

		if (ctrlHubSel.IsWindow()){
			sr.top = 1;
			sr.left = w[STATUS_HUBSEL - 1] + desclen + 10;
			sr.right = sr.left + 170;
			ctrlHubSel.MoveWindow(sr);
		}
		updateStatusBar();
	}
	
	int h = WinUtil::fontHeight + 4;
	const int maxLines = resizePressed && SETTING(MAX_RESIZE_LINES) <= 1 ? 2 : SETTING(MAX_RESIZE_LINES);

	if((maxLines != 1) && lineCount != 0) {
		if(lineCount < maxLines) {
			h = WinUtil::fontHeight * lineCount + 4;
		} else {
			h = WinUtil::fontHeight * maxLines + 4;
		}
	} 

	CRect rc = rect;
	rc.bottom -= h + 10;
	ctrlClient.MoveWindow(rc);
	
	int buttonsize = 0;
	if(ctrlEmoticons.IsWindow() && SETTING(SHOW_EMOTICON))
		buttonsize +=26;

	if(ctrlMagnet.IsWindow())
		buttonsize += 26;

	if(ctrlResize.IsWindow())
		buttonsize += 26;

	rc = rect;
	rc.bottom -= 2;
	rc.top = rc.bottom - h - 5;
	rc.left +=2;
	rc.right -= buttonsize;
	ctrlMessage.MoveWindow(rc);

	 //ApexDC	
	if(h != (WinUtil::fontHeight + 4)) {
		rc.bottom -= h - (WinUtil::fontHeight + 4);
	}

	if(ctrlResize.IsWindow()) {
		//resize lines button
		rc.left = rc.right + 2;
		rc.right += 24;
		ctrlResize.MoveWindow(rc);
	}

	if(ctrlEmoticons.IsWindow()){
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
}

void PrivateFrame::updateTabIcon(bool offline) {
	if (offline) {
		setIcon(userOffline);
		return;
	}
	OnlineUserPtr ou = ClientManager::getInstance()->findOnlineUser(chat->getHintedUser());
	if (ou) {
		tabIcon = ResourceLoader::getUserImages().GetIcon(ou->getImageIndex());
		setIcon(tabIcon);
	}
}

string PrivateFrame::getLogPath() const {
	ParamMap params;
	fillLogParams(params);
	return LogManager::getInstance()->getPath(getUser(), params);
}

void PrivateFrame::readLog() {
	if (SETTING(SHOW_LAST_LINES_LOG) == 0) return;
	try {
		File f(getLogPath(), File::READ, File::OPEN);
		
		int64_t size = f.getSize();

		if(size > 32*1024) {
			f.setPos(size - 32*1024);
		}
		string buf = f.read(32*1024);
		StringList lines;

		if(strnicmp(buf.c_str(), "\xef\xbb\xbf", 3) == 0)
			lines = StringTokenizer<string>(buf.substr(3), "\r\n").getTokens();
		else
			lines = StringTokenizer<string>(buf, "\r\n").getTokens();

		int linesCount = lines.size();

		int i = linesCount > (SETTING(SHOW_LAST_LINES_LOG) + 1) ? linesCount - SETTING(SHOW_LAST_LINES_LOG) : 0;

		for(; i < linesCount; ++i){
			ctrlClient.AppendChat(Identity(NULL, 0), _T("- "), _T(""), Text::toT(lines[i]) + _T('\n'), WinUtil::m_ChatTextLog, true);
		}
		f.close();
	} catch(const FileException&){
	}
}

void PrivateFrame::sendSeen() {
	if (hasUnSeenMessages)
		chat->addPMInfo(PrivateChat::MSG_SEEN);

	hasUnSeenMessages = false;
}

void PrivateFrame::sendTyping(bool off) {
	chat->addPMInfo(off ? PrivateChat::TYPING_OFF : PrivateChat::TYPING_ON);
}


LRESULT PrivateFrame::onOpenUserLog(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {	
	string file = getLogPath();
	if(Util::fileExists(file)) {
		WinUtil::viewLog(file, wID == IDC_USER_HISTORY);
	} else {
		MessageBox(CTSTRING(NO_LOG_FOR_USER), CTSTRING(NO_LOG_FOR_USER), MB_OK );	  
	}	

	return 0;
}

LRESULT PrivateFrame::onPublicMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	if(!online)
		return 0;

	tstring sUsers = ctrlClient.getSelectedUser();

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

void PrivateFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept{
	ctrlClient.SetBackgroundColor(WinUtil::bgColor);
	RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

void PrivateFrame::on(PrivateChatListener::UserUpdated) noexcept{
	callAsync([this] { updateOnlineStatus(); });
}

void PrivateFrame::on(PrivateChatListener::CCPMStatusChanged, const string& aMessage) noexcept{
	callAsync([this, aMessage] {
		addStatusLine(Text::toT(aMessage), LogManager::LOG_INFO);
		updateOnlineStatus(true);
	});
}

void PrivateFrame::on(PrivateChatListener::StatusMessage, const string& aMessage, uint8_t sev) noexcept{
	callAsync([this, aMessage, sev] {
		addStatusLine(Text::toT(aMessage), sev);
	});
}

void PrivateFrame::on(PrivateChatListener::PMStatus, uint8_t aType) noexcept{
	callAsync([this, aType] {
		updatePMInfo(aType);
	});
}

void PrivateFrame::on(PrivateChatListener::PrivateMessage, const ChatMessage& aMessage) noexcept{
	callAsync([this, aMessage] {
		bool myPM = aMessage.replyTo->getUser() == ClientManager::getInstance()->getMe();

		auto text = Text::toT(aMessage.format());
		addLine(aMessage.from->getIdentity(), text);

		if (!myPM) {
			handleNotifications(false, text, aMessage.from->getIdentity());
			checkClientChanged(&aMessage.from->getClient(), false);
		} else if (!userTyping) {
			addClientLine(TSTRING(LAST_CHANGE) + _T(" ") + Text::toT(Util::getTimeString()), LogManager::LOG_INFO);
		}

	});
}

void PrivateFrame::on(PrivateChatListener::Activate, const string& msg, Client* c) noexcept{
	callAsync([this, msg, c] {
		checkClientChanged(c, true);
		if (::IsIconic(m_hWnd))
			::ShowWindow(m_hWnd, SW_RESTORE);
		MDIActivate(m_hWnd);
		sendFrameMessage(Text::toT(msg));
	});
}