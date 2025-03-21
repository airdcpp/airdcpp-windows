/* 
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#include <windows/stdafx.h>
#include <windows/Resource.h>

#include <windows/frames/private_chat/PrivateFrame.h>
#include <windows/util/WinUtil.h>
#include <windows/MainFrm.h>
#include <windows/frames/hub/HubFrame.h>
#include <windows/util/FormatUtil.h>
#include <windows/util/ActionUtil.h>

#include <airdcpp/hub/Client.h>
#include <airdcpp/hub/ClientManager.h>
#include <airdcpp/favorites/FavoriteUserManager.h>
#include <airdcpp/events/LogManager.h>
#include <airdcpp/util/PathUtil.h>
#include <airdcpp/private_chat/PrivateChatManager.h>
#include <airdcpp/core/localization/ResourceManager.h>
#include <airdcpp/favorites/ReservedSlotManager.h>
#include <airdcpp/util/Util.h>


namespace wingui {
PrivateFrame::FrameMap PrivateFrame::frames;
string PrivateFrame::id = "PM";

void PrivateFrame::openWindow(const HintedUser& aReplyTo, bool aMessageReceived) {
	if (!PrivateChatManager::getInstance()->getChat(aReplyTo)) {
		PrivateChatManager::getInstance()->addChat(aReplyTo, false);
		return;
	}

	auto frame = frames.find(aReplyTo.user);
	if (frame == frames.cend()) {
		auto p = new PrivateFrame(aReplyTo);
		if (aMessageReceived && SETTING(POPUNDER_PM)) {
			WinUtil::hiddenCreateEx(p);
		} else {
			p->CreateEx(WinUtil::mdiClient);
		}

		frame = frames.emplace(aReplyTo.user, p).first;

		if (aMessageReceived)
			p->handleNotifications(true, Util::emptyStringT);

	} else {
		frame->second->activate();
	}

	if (!aMessageReceived) {
		frame->second->activate();
	}
}

bool PrivateFrame::getWindowParams(HWND hWnd, StringMap& params) {
	auto f = ranges::find_if(frames | views::values, [hWnd](const PrivateFrame* h) { return hWnd == h->m_hWnd; }).base();
	if (f != frames.end()) {
		params["id"] = PrivateFrame::id;
		params["CID"] = f->first->getCID().toBase32();
		params["url"] = f->second->getHubUrl();
		FavoriteUserManager::getInstance()->addSavedUser(f->first);
		return true;
	}
	return false;
}

bool PrivateFrame::parseWindowParams(StringMap& params) {
	if (params["id"] == PrivateFrame::id) {
		string cid = params["CID"];
		string hubUrl = params["url"];
		auto u = ClientManager::getInstance()->getUser(CID(cid));
		if (u) {
			PrivateChatManager::getInstance()->addChat(HintedUser(u, hubUrl), false);
		}
		return true;
	}
	return false;
}


PrivateFrame::PrivateFrame(const HintedUser& replyTo_) :
created(false), closed(false), curCommandPosition(0),
	ctrlHubSelContainer(WC_COMBOBOXEX, this, HUB_SEL_MAP),
	ctrlMessageContainer(WC_EDIT, this, EDIT_MESSAGE_MAP),
	ctrlClientContainer(WC_EDIT, this, EDIT_MESSAGE_MAP),
	ctrlStatusContainer(STATUSCLASSNAME, this, STATUS_MSG_MAP),
	UserInfoBaseHandler(false, true), isTyping(false), userTyping(false), windowLoaded(false)
{
	chat = PrivateChatManager::getInstance()->getChat(replyTo_);
	ctrlClient.setClient(chat->getClient());
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
	
	CToolInfo ti_tool(TTF_SUBCLASS, ctrlStatus.m_hWnd, STATUS_TEXT + POPUP_UID, 0, LPSTR_TEXTCALLBACK);
	ctrlTooltips.AddTool(&ti_tool);

	CToolInfo ti_tool1(TTF_SUBCLASS, ctrlStatus.m_hWnd, STATUS_CC + POPUP_UID, 0, LPSTR_TEXTCALLBACK);
	ctrlTooltips.AddTool(&ti_tool1);

	CToolInfo ti_tool2(TTF_SUBCLASS, ctrlStatus.m_hWnd, STATUS_AWAY + POPUP_UID, 0, LPSTR_TEXTCALLBACK);
	ctrlTooltips.AddTool(&ti_tool2);
	
	CToolInfo ti_tool3(TTF_SUBCLASS, ctrlStatus.m_hWnd, STATUS_COUNTRY + POPUP_UID, 0, LPSTR_TEXTCALLBACK);
	ctrlTooltips.AddTool(&ti_tool3);

	ctrlClientContainer.SubclassWindow(ctrlClient.m_hWnd);
	ctrlMessageContainer.SubclassWindow(ctrlMessage.m_hWnd);
	ctrlStatusContainer.SubclassWindow(ctrlStatus.m_hWnd);
	created = true;

	SettingsManager::getInstance()->addListener(this);

	callAsync([this] {
		// The frame must have been created before these operations

		{
			// Postpone caching of new messages received during this operation to avoid
			// them being displayed twice

			RLock l(chat->getCache().getCS());
			chat->addListener(this);

			// Append messages that were received while the frame was being created
			for (const auto& message : chat->getCache().getMessagesUnsafe()) {
				if (message.type == Message::TYPE_CHAT) {
					onChatMessage(message.chatMessage);
				} else if (message.logMessage->getType() == LogMessage::Type::HISTORY) {
					addMessage(message, WinUtil::m_ChatTextLog);
				} else {
					onStatusMessage(message.logMessage);
				}
			}
		}

		updateOnlineStatus();  
		windowLoaded = true;
	});

	ctrlClient.setContextMenuHandler([this](auto& menu_, const auto& aHighlight) {
		// Older highlights may not be in the message cache so extensions wouldn't be able to access them...
		if (aHighlight && chat->getCache().findMessageHighlight(aHighlight->getToken())) {
			vector<MessageHighlightToken> tokens = { aHighlight->getToken() };
			EXT_CONTEXT_MENU_ENTITY(menu_, PrivateChatMessageHighlight, tokens, chat);
		}
	});

	WinUtil::SetIcon(m_hWnd, (getUser() && getUser()->isSet(User::BOT)) ? IDI_BOT : IDR_PRIVATE);

	::SetTimer(m_hWnd, 0, 1000, 0);
	
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
	if(windowLoaded)
		chat->setRead();
	return 0;
}

void PrivateFrame::updatePMInfo(uint8_t aType) {

	switch (aType) {
	case PrivateChat::MSG_SEEN: {
		tstring msg = WinUtil::formatTimestamp() + _T("*** ") + TSTRING(MESSAGE_SEEN) + _T(" ***");
		if (!userTyping)
			addStatus(msg, GET_ICON(IDI_SEEN, 16));
		else
			lastStatus = { msg, GET_ICON(IDI_SEEN, 16) };
		break;
	}
	case PrivateChat::TYPING_ON:
		//setStatusText to prevent saving lastStatus
		userTyping = true;
		setStatusText(_T(" *** ") + TSTRING(USER_TYPING) + _T(" ***"), GET_ICON(IDI_TYPING, 16));
		break;

	case PrivateChat::TYPING_OFF:
		//Restore the previous status
		userTyping = false;
		addStatus(lastStatus.first, lastStatus.second);
		break;

	case PrivateChat::QUIT:
		userTyping = false;
		setStatusText(_T(" *** ") + TSTRING(USER_CLOSED_WINDOW) + _T(" ***"), LogMessage::SEV_INFO);
		break;
	}
}

LRESULT PrivateFrame::onStatusBarClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
	if (!getUser()->isOnline() || getUser()->isNMDC() || getUser()->isSet(User::BOT))
		return 0;
	
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	CRect rect;
	ctrlStatus.GetRect(STATUS_CC, rect);
	if (PtInRect(&rect, pt)) {
		if (ccReady())
			closeCC();
		else
			chat->startCC();
	}
	bHandled = TRUE;
	return 0;
}

LRESULT PrivateFrame::onGetToolTip(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/) {

	LPNMTTDISPINFO pDispInfo = (LPNMTTDISPINFO)pnmh;
	pDispInfo->szText[0] = 0;
	if (idCtrl == STATUS_TEXT + POPUP_UID) {
		lastLines.clear();
		for (const auto& i : lastLinesList) {
			lastLines += i;
			lastLines += _T("\r\n");
		}

		if (lastLines.size() > 2) {
			lastLines.erase(lastLines.size() - 2);
		}

		pDispInfo->lpszText = const_cast<TCHAR*>(lastLines.c_str());
		return 0;
	}

	if (idCtrl == STATUS_CC + POPUP_UID) {
		if (getUser()->isNMDC() || getUser()->isSet(User::BOT))
			return 0;

		lastCCPMError = Text::toT(chat->getLastCCPMError());
		pDispInfo->lpszText = !chat->allowCCPM() ? const_cast<TCHAR*>(lastCCPMError.c_str()) :
			ccReady() ? (LPWSTR)CTSTRING(DISCONNECT_CCPM) : (LPWSTR)CTSTRING(START_CCPM);
	} else if (idCtrl == STATUS_AWAY + POPUP_UID) {
		pDispInfo->lpszText = !chat->isOnline() ? (LPWSTR)CTSTRING(USER_OFFLINE) : userAway ? (LPWSTR)CTSTRING(AWAY) : (LPWSTR)CTSTRING(ONLINE);
	} else if (idCtrl == STATUS_COUNTRY + POPUP_UID) {
		pDispInfo->lpszText = const_cast<TCHAR*>(countryPopup.c_str());
	}

	return 0;
}


LRESULT PrivateFrame::OnRelayMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	LPMSG pMsg = (LPMSG)lParam;
	if(ctrlTooltips.m_hWnd != NULL && pMsg->message >= WM_MOUSEFIRST && pMsg->message <= WM_MOUSELAST)
		ctrlTooltips.RelayEvent(pMsg);
	return 0;
}

LRESULT PrivateFrame::onEditChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled) {
	if (hWndCtl == ctrlMessage.m_hWnd) {
		chat->setRead();
	}
	bHandled = FALSE;
	return 0;
}

LRESULT PrivateFrame::onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {

	//Check if the user left text in the output but is idle or is looking at other windows and paused writing.
	LASTINPUTINFO info = { sizeof(LASTINPUTINFO) };
	bool inactive = GetLastInputInfo(&info) && (::GetTickCount() - info.dwTime) > 1 * 60 * 1000;
	inactive = inactive || GetFocus() != ctrlMessage.m_hWnd && GetFocus() != ctrlClient.m_hWnd;
	
	if (!inactive && !isTyping && ctrlMessage.GetWindowTextLength() > 0) {
		isTyping = true;
		chat->setTypingState(true);
	}
	else if ((inactive && isTyping) || isTyping && ctrlMessage.GetWindowTextLength() == 0) {
		isTyping = false;
		chat->setTypingState(false);
	}

	bHandled = TRUE;
	return 0;
}

LRESULT PrivateFrame::onHubChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
	auto selUrl = hubs[ctrlHubSel.GetCurSel()].hubUrl;
	if (chat->getHubUrl() == selUrl)
		return 0;

	chat->setHubUrl(selUrl);
	chat->statusMessage(STRING_F(MESSAGES_SENT_THROUGH, hubs[ctrlHubSel.GetCurSel()].hubName), LogMessage::SEV_INFO, LogMessage::Type::SERVER);
	bHandled = FALSE;
	return 0;
}

const UserPtr& PrivateFrame::getUser() const {
	return chat->getUser(); 
}

const string& PrivateFrame::getHubUrl() const {
	return chat->getHubUrl(); 
}

void PrivateFrame::updateOnlineStatus() {
	if (!chat->isOnline()) {
		setDisconnected(true);
		showHubSelection(false);
		updateTabIcon(true);
	} else {
		setDisconnected(false);
		updateTabIcon(false);

		if (!ccReady() && !getUser()->isNMDC() && !getUser()->isSet(User::BOT)) {
			hubs = ClientManager::getInstance()->getUserInfoList(getUser());
			while (ctrlHubSel.GetCount()) {
				ctrlHubSel.DeleteString(0);
			}

			fillHubSelection();
			showHubSelection(true);
		} else {
			showHubSelection(false);
		}
	}

	auto nicks = FormatUtil::getNicks(chat->getHintedUser());
	auto hubNames = chat->isOnline() ? FormatUtil::getHubNames(chat->getHintedUser()) : TSTRING(OFFLINE);

	ctrlClient.setClient(chat->isOnline() ? chat->getClient() : nullptr);
	updateStatusBar();

	SetWindowText((nicks + _T(" - ") + hubNames).c_str());
}

int hubToImage(const string& aHubUrl) {
	auto c = ClientManager::getInstance()->findClient(aHubUrl);
	if (!c) {
		return 0;
	}

	const auto ident = c->getMyIdentity();
	if (ident.isOp())
		return 2;
	
	return ident.isRegistered() ? 1 : 0;
}

void PrivateFrame::fillHubSelection() {
	for (const auto& hubInfo: hubs) {
		auto img = hubToImage(hubInfo.hubUrl);
		auto i = ctrlHubSel.AddItem(Text::toT(hubInfo.hubName).c_str(), img, img, 0);

		if (hubInfo.hubUrl == getHubUrl()) {
			ctrlHubSel.SetCurSel(i);
		}
	}
}

void PrivateFrame::showHubSelection(bool show) {
	ctrlHubSel.ShowWindow(show);
	ctrlHubSel.EnableWindow(show);
}

void PrivateFrame::handleNotifications(bool windowOpened, const tstring& aMessage) {
	
	auto handleBeep = [=] {
		if (!SETTING(SOUNDS_DISABLED)) {
			if (SETTING(BEEPFILE).empty()) {
				MessageBeep(MB_OK);
			} else {
				WinUtil::playSound(Text::toT(SETTING(BEEPFILE)));
			}
		}
	};
	
	if (windowOpened) {
		//Don't notify if we're going to display the same notification when handling the incoming message (prevents doing it twice)
		if(!SETTING(FLASH_WINDOW_ON_PM) && SETTING(FLASH_WINDOW_ON_NEW_PM))
			WinUtil::FlashWindow();

		if (!SETTING(POPUP_PM) && SETTING(POPUP_NEW_PM))
			WinUtil::showPopup(FormatUtil::getNicks(chat->getHintedUser()) + _T(" - ") + FormatUtil::getHubNames(chat->getHintedUser()), TSTRING(PRIVATE_MESSAGE));

		if (!SETTING(PRIVATE_MESSAGE_BEEP) && SETTING(PRIVATE_MESSAGE_BEEP_OPEN))
			handleBeep();

	} else {

		if (SETTING(FLASH_WINDOW_ON_PM)) 
			WinUtil::FlashWindow();
		
		if (SETTING(POPUP_PM)) {
			if (!aMessage.empty() && SETTING(PM_PREVIEW)) {
				tstring message = aMessage.substr(0, 250);
				WinUtil::showPopup(message.c_str(), CTSTRING(PRIVATE_MESSAGE));
			} else {
				WinUtil::showPopup(FormatUtil::getNicks(chat->getHintedUser()) +
					_T(" - ") + FormatUtil::getHubNames(chat->getHintedUser()), TSTRING(PRIVATE_MESSAGE));
			}
		}

		if (SETTING(PRIVATE_MESSAGE_BEEP)) 
			handleBeep();

		if (!getUser()->isSet(User::BOT))
			MainFrame::getMainFrame()->onChatMessage(true);
	}
}

bool PrivateFrame::checkFrameCommand(const tstring& aCmd, const tstring& /*aParam*/, tstring& /*message_*/, tstring& status_, bool& /*thirdPerson*/) {
	if (stricmp(aCmd.c_str(), _T("grant")) == 0) {
		FavoriteUserManager::getInstance()->getReservedSlots().reserveSlot(HintedUser(chat->getHintedUser()), 600);
		addClientLine(TSTRING(SLOT_GRANTED), LogMessage::SEV_INFO);
	} else if (stricmp(aCmd.c_str(), _T("close")) == 0) {
		PostMessage(WM_CLOSE);
	} else if ((stricmp(aCmd.c_str(), _T("favorite")) == 0) || (stricmp(aCmd.c_str(), _T("fav")) == 0)) {
		FavoriteUserManager::getInstance()->addFavoriteUser(chat->getHintedUser());
		addClientLine(TSTRING(FAVORITE_USER_ADDED), LogMessage::SEV_INFO);
	} else if (stricmp(aCmd.c_str(), _T("getlist")) == 0) {
		handleGetList();
	} else if (stricmp(aCmd.c_str(), _T("log")) == 0) {
		ActionUtil::openFile(Text::toT(chat->getLogPath()));
	} else if (Util::stricmp(aCmd.c_str(), _T("direct")) == 0 || Util::stricmp(aCmd.c_str(), _T("encrypted")) == 0) {
		chat->startCC();
	} else if (Util::stricmp(aCmd.c_str(), _T("disconnect")) == 0) {
		closeCC();
	} else if (stricmp(aCmd.c_str(), _T("help")) == 0) {
		status_ = _T("*** ") + ChatFrameBase::commands + _T("Additional commands for private message tabs: /getlist, /grant, /favorite, /direct or /encrypted");
	} else {
		return false;
	}

	return true;
}

bool PrivateFrame::sendMessageHooked(const OutgoingChatMessage& aMessage, string& error_) {
	return chat->sendMessageHooked(aMessage, error_);
}

LRESULT PrivateFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		PrivateChatManager::getInstance()->removeChat(getUser());
		return 0;
	} else {
		chat->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);

		frames.erase(getUser());

		bHandled = FALSE;
		return 0;
	}
}

void PrivateFrame::closeCC(bool silent) {
	if (ccReady()) {
		if (!silent) { 
			chat->statusMessage(STRING(CCPM_DISCONNECTING),LogMessage::SEV_INFO, LogMessage::Type::SERVER);
		}

		chat->closeCC(false, true);
	}
}

bool PrivateFrame::ccReady() const {
	return chat->ccReady();
}

// Messages
void PrivateFrame::addClientLine(const tstring& aLine, uint8_t aSeverity) {
	if (!created) {
		CreateEx(WinUtil::mdiClient);
	}
	addStatus(WinUtil::formatMessageWithTimestamp(aLine), ResourceLoader::getSeverityIcon(aSeverity));
	if (SETTING(BOLD_PM)) {
		setDirty();
	}
}

void PrivateFrame::onChatMessage(const ChatMessagePtr& aMessage) noexcept {
	auto myPM = aMessage->getReplyTo()->getUser() == ClientManager::getInstance()->getMe();
	addMessage(Message(aMessage));

	if (!myPM) {
		addStatus(WinUtil::formatMessageWithTimestamp(TSTRING(LAST_MESSAGE_RECEIVED)), ResourceLoader::getSeverityIcon(LogMessage::SEV_INFO));
		handleNotifications(false, Text::toT(aMessage->format()));
	} else if (!userTyping) {
		addStatus(WinUtil::formatMessageWithTimestamp(TSTRING(LAST_MESSAGE_SENT)), ResourceLoader::getSeverityIcon(LogMessage::SEV_INFO));
	}
}

void PrivateFrame::onStatusMessage(const LogMessagePtr& aMessage) noexcept {
	addStatusMessage(aMessage);
	// addStatusLine(Text::toT(aMessage->getText()), aMessage->getSeverity());
}

void PrivateFrame::addStatusMessage(const LogMessagePtr& aMessage) {
	if (SETTING(STATUS_IN_CHAT)) {
		addMessage(Message(aMessage), getStatusMessageStyle(aMessage));
	}

	auto status = Text::toT(aMessage->getText()) + _T(" ***");
	addClientLine(status, aMessage->getSeverity());
}

void PrivateFrame::addStatus(const tstring& aLine, HICON aIcon) {
	if (lastLinesList.empty() || (lastLinesList.back() != aLine)) {
		while (lastLinesList.size() + 1 > 5)
			lastLinesList.pop_front();
		lastLinesList.push_back(aLine);
	}

	lastStatus = { aLine, aIcon };
	ctrlStatus.SetText(STATUS_TEXT, aLine.c_str(), SBT_NOTABPARSING);
	ctrlStatus.SetIcon(STATUS_TEXT, aIcon);
}

void PrivateFrame::addPrivateLine(const tstring& aLine) {
	auto message = std::make_shared<LogMessage>(Text::fromT(aLine), LogMessage::SEV_INFO, LogMessage::Type::PRIVATE, Util::emptyString);
	addMessage(Message(message), WinUtil::m_ChatTextPrivate);
}

void PrivateFrame::addMessage(const Message& aMessage, CHARFORMAT2& cf) {
	auto notify = ctrlClient.AppendMessage(aMessage, cf);
	if (notify) {
		setNotify();
	}

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


	if (getUser()->isOnline()) {
		tabMenu.appendSeparator();
		OMenu* copyMenu = tabMenu.createSubMenu(TSTRING(COPY), true);

		for (int j = 0; j < UserUtil::COLUMN_LAST; j++) {
			copyMenu->AppendMenu(MF_STRING, IDC_COPY + j, CTSTRING_I(HubFrame::columnNames[j]));
		}
	}
	
	prepareMenu(tabMenu, UserCommand::CONTEXT_USER, ClientManager::getInstance()->getHubUrls(getUser()->getCID()));
	if(!(tabMenu.GetMenuState(tabMenu.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR)) {	
		tabMenu.AppendMenu(MF_SEPARATOR);
	}


	EXT_CONTEXT_MENU(tabMenu, PrivateChat, vector<CID>({ getUser()->getCID() }));

	tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));

	tabMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt);
	return TRUE;
}

LRESULT PrivateFrame::onCopyUserInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring sCopy;

	const OnlineUserPtr ou = ClientManager::getInstance()->findOnlineUser(HintedUser(getUser(), getHubUrl()));
	if (ou) {
		sCopy = UserUtil::getUserText(ou, static_cast<uint8_t>(wID - IDC_COPY), true);
	}

	if (!sCopy.empty())
		WinUtil::setClipboard(sCopy);

	return 0;
}

void PrivateFrame::runUserCommand(UserCommand& uc) {
	if (!ActionUtil::getUCParams(m_hWnd, uc, ucLineParams))
		return;

	auto ucParams = ucLineParams;

	UserCommandManager::getInstance()->userCommand(chat->getHintedUser(), uc, ucParams, true);
}

void PrivateFrame::updateStatusBar() {
	tstring tmp = Util::emptyStringT;
	if (ccReady()) {
		tmp = _T(" ") + TSTRING(SEND_PM_VIA) + _T(": ") + TSTRING(DIRECT_ENCRYPTED_CHANNEL);
		ctrlStatus.SetIcon(STATUS_CC, GET_ICON(IDI_SECURE, 16));
	} else if(chat->isOnline() && !getUser()->isNMDC() && !getUser()->isSet(User::BOT)) {
		tmp = _T(" ") + TSTRING(SEND_PM_VIA);
		ctrlStatus.SetIcon(STATUS_CC, chat->allowCCPM() ? ResourceLoader::iSecureGray : ResourceLoader::iCCPMUnSupported);
	} else {
		ctrlStatus.SetIcon(STATUS_CC, NULL);
	}
	setCountryFlag();
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
			ctrlStatus.GetRect(STATUS_TEXT, r);
			ctrlTooltips.SetToolRect(ctrlStatus.m_hWnd, STATUS_TEXT + POPUP_UID, r);

			ctrlStatus.GetRect(STATUS_CC, r);
			ctrlTooltips.SetToolRect(ctrlStatus.m_hWnd, STATUS_CC + POPUP_UID, r);

			ctrlStatus.GetRect(STATUS_AWAY, r);
			ctrlTooltips.SetToolRect(ctrlStatus.m_hWnd, STATUS_AWAY + POPUP_UID, r);

			ctrlStatus.GetRect(STATUS_COUNTRY, r);
			ctrlTooltips.SetToolRect(ctrlStatus.m_hWnd, STATUS_COUNTRY + POPUP_UID, r);
		};

		CRect sr;
		ctrlStatus.GetClientRect(sr);

		int w[STATUS_LAST];
		int desclen = WinUtil::getStatusTextWidth(TSTRING(SEND_PM_VIA), ctrlStatus.m_hWnd) +2;
		int desclen2 = WinUtil::getStatusTextWidth(TSTRING(DIRECT_ENCRYPTED_CHANNEL), ctrlStatus.m_hWnd) +2;
		const int status_away_size = 22;
		const int status_country_size = 32;

		if (desclen2 < 190) //Make sure the HubSel Combo will fit too.
			desclen2 = 190;

		w[STATUS_TEXT] = sr.right - desclen - desclen2 - 22 - status_away_size - status_country_size -8;
		w[STATUS_CC] = w[STATUS_TEXT] + 22;
		w[STATUS_HUBSEL] = w[STATUS_CC] + desclen + desclen2;
		w[STATUS_AWAY] = w[STATUS_HUBSEL] + status_away_size;
		w[STATUS_COUNTRY] = w[STATUS_AWAY] + status_country_size +8;
		ctrlStatus.SetParts(STATUS_LAST, w);

		ctrlTooltips.SetMaxTipWidth(w[STATUS_TEXT]);
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

	if (ctrlSendMessage.IsWindow())
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

	if (ctrlSendMessage.IsWindow()) {
		rc.left = rc.right + 2;
		rc.right += 24;
		ctrlSendMessage.MoveWindow(rc);
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
		bool userBot = getUser() && getUser()->isSet(User::BOT);
		setIcon(userBot ? GET_ICON(IDI_BOT_OFF, 16) : GET_ICON(IDR_PRIVATE_OFF, 16));
		setAway();
		return;
	}
	OnlineUserPtr ou = ClientManager::getInstance()->findOnlineUser(chat->getHintedUser());
	if (ou) {
		//Image list GetIcon results in a new icon handle each time, manage it with CIcon
		tabIcon = ResourceLoader::getUserImages().GetIcon(UserUtil::getUserImageIndex(ou));
		setIcon(tabIcon);
		userAway = !getUser()->isSet(User::BOT) && ou->getIdentity().isAway();
	}
	setAway();
}

void PrivateFrame::setCountryFlag() {
	if (ctrlStatus.GetIcon(STATUS_COUNTRY) != NULL) //Only need to set once.
		return;

	OnlineUserPtr ou = ClientManager::getInstance()->findOnlineUser(chat->getHintedUser());
	if (ou && getUser() && !getUser()->isSet(User::BOT)) {
		auto countryInfo = FormatUtil::toCountryInfo(ou->getIdentity().getTcpConnectIp());
		if (!countryInfo.text.empty()) {
			countryPopup = countryInfo.text;
			ctrlStatus.SetIcon(STATUS_COUNTRY, ResourceLoader::flagImages.GetIcon(countryInfo.flagIndex));
		}
	}
}

void PrivateFrame::setAway() {
	if (getUser()->isSet(User::BOT))
		return;

	if (!getUser()->isOnline()) {
		ctrlStatus.SetIcon(STATUS_AWAY, GET_ICON(IDR_PRIVATE_OFF, 16));
	} else {
		ctrlStatus.SetIcon(STATUS_AWAY, userAway ? GET_ICON(IDI_USER_AWAY, 16) : GET_ICON(IDI_USER_BASE, 16));
	}
}

LRESULT PrivateFrame::onOpenUserLog(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {	
	string file = chat->getLogPath();
	if(PathUtil::fileExists(file)) {
		ActionUtil::viewLog(file, wID == IDC_USER_HISTORY);
	} else {
		MessageBox(CTSTRING(NO_LOG_FOR_USER), CTSTRING(NO_LOG_FOR_USER), MB_OK );	  
	}	

	return 0;
}

LRESULT PrivateFrame::onPublicMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	if(!chat->isOnline())
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

void PrivateFrame::on(PrivateChatListener::UserUpdated, PrivateChat*) noexcept{
	callAsync([this] { updateOnlineStatus(); });
}

void PrivateFrame::on(PrivateChatListener::PMStatus, PrivateChat*, uint8_t aType) noexcept{
	callAsync([this, aType] {
		updatePMInfo(aType);
	});
}

void PrivateFrame::on(PrivateChatListener::PrivateMessage, PrivateChat*, const ChatMessagePtr& aMessage) noexcept{
	callAsync([=] {
		onChatMessage(aMessage);
	});
}

void PrivateFrame::activate() noexcept {
	callAsync([this] {
		if (::IsIconic(m_hWnd))
			::ShowWindow(m_hWnd, SW_RESTORE);
		MDIActivate(m_hWnd);
	});
}

void PrivateFrame::on(PrivateChatListener::StatusMessage, PrivateChat*, const LogMessagePtr& aMessage, const string& aOwner) noexcept {
	if (!aOwner.empty() && aOwner != WinUtil::ownerId) {
		return;
	}

	callAsync([=] {
		onStatusMessage(aMessage);
	});
}

void PrivateFrame::on(PrivateChatListener::Close, PrivateChat*) noexcept {
	callAsync([&] {
		closed = true;
		PostMessage(WM_CLOSE);
		});
}
}