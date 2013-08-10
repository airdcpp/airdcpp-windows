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

#include <boost/range/algorithm/for_each.hpp>

PrivateFrame::FrameMap PrivateFrame::frames;

PrivateFrame::PrivateFrame(const HintedUser& replyTo_, Client* c) : replyTo(replyTo_),
	created(false), closed(false), online(true), curCommandPosition(0), 
	ctrlHubSelContainer(WC_COMBOBOX, this, HUB_SEL_MAP),
	ctrlMessageContainer(WC_EDIT, this, EDIT_MESSAGE_MAP),
	ctrlClientContainer(WC_EDIT, this, EDIT_MESSAGE_MAP),
	UserInfoBaseHandler(false, true)
{
	ctrlClient.setClient(c);
	ctrlClient.setPmUser(replyTo_.user);
}

LRESULT PrivateFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	ctrlHubSel.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE, IDC_HUB);
	ctrlHubSelContainer.SubclassWindow(ctrlHubSel.m_hWnd);
	ctrlHubSel.SetFont(WinUtil::systemFont);

	init(m_hWnd, rcDefault);
	ctrlClientContainer.SubclassWindow(ctrlClient.m_hWnd);
	ctrlMessageContainer.SubclassWindow(ctrlMessage.m_hWnd);
	
	bool userBot = replyTo.user && replyTo.user->isSet(User::BOT);
	userOffline = userBot ? ResourceLoader::loadIcon(IDI_BOT_OFF) : ResourceLoader::loadIcon(IDR_PRIVATE_OFF);
	userOnline = userBot ? ResourceLoader::loadIcon(IDI_BOT) : ResourceLoader::loadIcon(IDR_PRIVATE);
	created = true;

	ClientManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	readLog();

	WinUtil::SetIcon(m_hWnd, userBot ? IDI_BOT : IDR_PRIVATE);

	//add the updateonlinestatus in the wnd message queue so the frame and tab can finish creating first.
	runSpeakerTask();

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
	
void PrivateFrame::addClientLine(const tstring& aLine) {
	if(!created) {
		CreateEx(WinUtil::mdiClient);
		//updateOnlineStatus();
	}
	setStatusText(aLine);
	if (SETTING(BOLD_PM)) {
		setDirty();
	}
}

LRESULT PrivateFrame::OnRelayMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	LPMSG pMsg = (LPMSG)lParam;
	if(ctrlTooltips.m_hWnd != NULL && pMsg->message >= WM_MOUSEFIRST && pMsg->message <= WM_MOUSELAST)
		ctrlTooltips.RelayEvent(pMsg);
	return 0;
}

void PrivateFrame::addSpeakerTask(bool addDelay) {
	if (addDelay)
		delayEvents.addEvent(replyTo.user->getCID(), [this] { runSpeakerTask(); }, 1000);
	else
		runSpeakerTask();
}

void PrivateFrame::runSpeakerTask() {
	callAsync([this] { updateOnlineStatus(); });
}

LRESULT PrivateFrame::onHubChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
	auto hp = hubs[ctrlHubSel.GetCurSel()];
	changeClient();

	updateOnlineStatus();
	addStatusLine(CTSTRING_F(MESSAGES_SENT_THROUGH, Text::toT(hp.second)));

	bHandled = FALSE;
	return 0;
}

void PrivateFrame::on(ClientManagerListener::UserConnected, const OnlineUser& aUser, bool) noexcept {
	if(aUser.getUser() == replyTo.user) {
		addSpeakerTask(true); //delay this to possible show more nicks & hubs in the connect message :]
	}
}

void PrivateFrame::on(ClientManagerListener::UserDisconnected, const UserPtr& aUser, bool wentOffline) noexcept {
	if(aUser == replyTo.user) {
		ctrlClient.setClient(nullptr);
		addSpeakerTask(wentOffline ? false : true);
	}
}

void PrivateFrame::addStatusLine(const tstring& aLine) {
	tstring status = _T(" *** ") + aLine + _T(" ***");
	if(SETTING(STATUS_IN_CHAT)) {
		addLine(status, WinUtil::m_ChatTextServer);
	} else {
		addClientLine(status);
	}
}

void PrivateFrame::changeClient() {
	replyTo.hint = hubs[ctrlHubSel.GetCurSel()].first;
	ctrlClient.setClient(ClientManager::getInstance()->getClient(replyTo.hint));
}

void PrivateFrame::updateOnlineStatus() {
	const CID& cid = replyTo.user->getCID();
	const string& hint = replyTo.hint;

	dcassert(!hint.empty());

	//get the hub and online status
	auto hubsInfoNew = move(WinUtil::getHubNames(cid));
	if (!hubsInfoNew.second && !online) {
		//nothing to update... probably a delayed event
		return;
	}

	auto tmp = WinUtil::getHubNames(cid);

	auto oldSel = ctrlHubSel.GetStyle() & WS_VISIBLE ? ctrlHubSel.GetCurSel() : 0;
	StringPair oldHubPair;
	if (!hubs.empty())
		oldHubPair = hubs[oldSel]; // cache the old hub name

	hubs = ClientManager::getInstance()->getHubs(cid);
	while (ctrlHubSel.GetCount()) {
		ctrlHubSel.DeleteString(0);
	}

	//General things
	if(hubsInfoNew.second) {
		//the user is online

		hubNames = WinUtil::getHubNames(replyTo);
		nicks = WinUtil::getNicks(HintedUser(replyTo, hint));
		setDisconnected(false);

		if(!online) {
			addStatusLine(TSTRING(USER_WENT_ONLINE) + _T(" [") + nicks + _T(" - ") + hubNames + _T("]"));
			setIcon(userOnline);
		}
	} else {
		setDisconnected(true);
		setIcon(userOffline);
		addStatusLine(TSTRING(USER_WENT_OFFLINE) + _T(" [") + hubNames + _T("]"));
		ctrlClient.setClient(nullptr);
	}

	//ADC related changes
	if(hubsInfoNew.second && !replyTo.user->isNMDC() && !hubs.empty()) {
		if(!(ctrlHubSel.GetStyle() & WS_VISIBLE)) {
			showHubSelection(true);
		}

		for (const auto& hub: hubs) {
			auto idx = ctrlHubSel.AddString(Text::toT(hub.second).c_str());
			if(hub.first == hint) {
				ctrlHubSel.SetCurSel(idx);
			}
		}

		if(ctrlHubSel.GetCurSel() == -1) {
			//the hub was not found
			ctrlHubSel.SetCurSel(0);
			changeClient();
			if (!online) //the user came online but not in the previous hub
				addStatusLine(CTSTRING_F(MESSAGES_SENT_THROUGH, Text::toT(hubs[ctrlHubSel.GetCurSel()].second)));
			else
				addStatusLine(CTSTRING_F(USER_OFFLINE_PM_CHANGE, Text::toT(oldHubPair.second) % Text::toT(hubs[0].second)));
		} else if (!oldHubPair.first.empty() && oldHubPair.first != hint) {
			addStatusLine(CTSTRING_F(MESSAGES_SENT_THROUGH_REMOTE, Text::toT(hubs[ctrlHubSel.GetCurSel()].second)));
		} else if (!ctrlClient.getClient()) {
			changeClient();
		}
	} else {
		showHubSelection(false);
	}

	online = hubsInfoNew.second;
	SetWindowText((nicks + _T(" - ") + hubNames).c_str());
}

void PrivateFrame::showHubSelection(bool show) {
	ctrlHubSel.ShowWindow(show);
	ctrlHubSel.EnableWindow(show);

	UpdateLayout();
}

void PrivateFrame::gotMessage(const Identity& from, const UserPtr& to, const UserPtr& replyTo, const tstring& aMessage, Client* c) {
	PrivateFrame* p = nullptr;
	bool myPM = replyTo == ClientManager::getInstance()->getMe();
	const UserPtr& user = myPM ? to : replyTo;
	
	auto hintedUser = HintedUser(user, c->getHubUrl());

	auto i = frames.find(user);
	if(i == frames.end()) {
		if(frames.size() > 200) return;

		p = new PrivateFrame(hintedUser, c);
		frames[user] = p;
		
		p->addLine(from, aMessage);

		if(AirUtil::getAway()) {
			if(!(SETTING(NO_AWAYMSG_TO_BOTS) && user->isSet(User::BOT))) 
			{
				ParamMap params;
				from.getParams(params, "user", false);

				string error;
				p->sendMessage(Text::toT(AirUtil::getAwayMessage(p->getAwayMessage(), params)), error);
			}
		}

		if(SETTING(FLASH_WINDOW_ON_NEW_PM)) {
			WinUtil::FlashWindow();
		}

		if(SETTING(POPUP_NEW_PM)) {
			if(SETTING(PM_PREVIEW)) {
				tstring message = aMessage.substr(0, 250);
				WinUtil::showPopup(message.c_str(), CTSTRING(PRIVATE_MESSAGE));
			} else {
				WinUtil::showPopup(WinUtil::getNicks(hintedUser) + _T(" - ") + p->hubNames, TSTRING(PRIVATE_MESSAGE));
			}
		}

		if((SETTING(PRIVATE_MESSAGE_BEEP) || SETTING(PRIVATE_MESSAGE_BEEP_OPEN)) && (!SETTING(SOUNDS_DISABLED))) {
			if (SETTING(BEEPFILE).empty()) {
				MessageBeep(MB_OK);
			} else {
				WinUtil::playSound(Text::toT(SETTING(BEEPFILE)));
			}
		}
	} else {
		if(!myPM) {
			i->second->updateFrameOnlineStatus(HintedUser(user, c->getHubUrl()), c);
			if(SETTING(FLASH_WINDOW_ON_PM) && !SETTING(FLASH_WINDOW_ON_NEW_PM)) {
				WinUtil::FlashWindow();
			}

			if(SETTING(POPUP_PM)) {
				if(SETTING(PM_PREVIEW)) {
					tstring message = aMessage.substr(0, 250);
					WinUtil::showPopup(message.c_str(), CTSTRING(PRIVATE_MESSAGE));
				} else {
					WinUtil::showPopup(WinUtil::getNicks(hintedUser) + _T(" - ") + i->second->hubNames, TSTRING(PRIVATE_MESSAGE));
				}
			}

			if((SETTING(PRIVATE_MESSAGE_BEEP)) && (!SETTING(SOUNDS_DISABLED))) {
				if (SETTING(BEEPFILE).empty()) {
					MessageBeep(MB_OK);
				} else {
					WinUtil::playSound(Text::toT(SETTING(BEEPFILE)));
				}
			}
		}
		i->second->addLine(from, aMessage);
	}
}

void PrivateFrame::openWindow(const HintedUser& replyTo, const tstring& msg, Client* c) {
	PrivateFrame* p = nullptr;
	auto i = frames.find(replyTo);
	if(i == frames.end()) {
		if(frames.size() > 200) return;
		p = new PrivateFrame(replyTo, c);
		frames[replyTo] = p;
		p->CreateEx(WinUtil::mdiClient);
		//p->updateOnlineStatus();
	} else {
		p = i->second;
		p->updateFrameOnlineStatus(replyTo, c); 
		if(::IsIconic(p->m_hWnd))
			::ShowWindow(p->m_hWnd, SW_RESTORE);
		p->MDIActivate(p->m_hWnd);
	}

	p->sendFrameMessage(msg);
}
/*
 update the re used frame to the correct hub, 
 so it doesnt appear offline while user is sending us messages with another hub :P
*/
void PrivateFrame::updateFrameOnlineStatus(const HintedUser& newUser, Client* c) {

	if(!replyTo.user->isNMDC() && replyTo.hint != newUser.hint) {
		replyTo.hint = newUser.hint;
		ctrlClient.setClient(c);
		updateOnlineStatus();
	}
}

bool PrivateFrame::checkFrameCommand(tstring& cmd, tstring& /*param*/, tstring& /*message*/, tstring& status, bool& /*thirdPerson*/) { 
	if(stricmp(cmd.c_str(), _T("grant")) == 0) {
		UploadManager::getInstance()->reserveSlot(HintedUser(replyTo), 600);
		addClientLine(TSTRING(SLOT_GRANTED));
	} else if(stricmp(cmd.c_str(), _T("close")) == 0) {
		PostMessage(WM_CLOSE);
	} else if((stricmp(cmd.c_str(), _T("favorite")) == 0) || (stricmp(cmd.c_str(), _T("fav")) == 0)) {
		FavoriteManager::getInstance()->addFavoriteUser(replyTo);
		addClientLine(TSTRING(FAVORITE_USER_ADDED));
	} else if(stricmp(cmd.c_str(), _T("getlist")) == 0) {
		handleGetList();
	} else if(stricmp(cmd.c_str(), _T("log")) == 0) {
		WinUtil::openFile(Text::toT(getLogPath()));
	} else if(stricmp(cmd.c_str(), _T("help")) == 0) {
		status = _T("*** ") + ChatFrameBase::commands + _T("Additional commands for private message tabs: /getlist, /grant, /favorite");
	} else {
		return false;
	}

	return true;
}

bool PrivateFrame::sendMessage(const tstring& msg, string& error_, bool thirdPerson) {
	if(replyTo.user->isOnline()) {
		return ClientManager::getInstance()->privateMessage(replyTo, Text::fromT(msg), error_, thirdPerson);
	}
	error_ = STRING(USER_OFFLINE);
	return false;
}

LRESULT PrivateFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		LogManager::getInstance()->removePmCache(replyTo.user);
		ClientManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		closed = true;
		DestroyIcon(userOnline);
		DestroyIcon(userOffline);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		auto i = frames.find(replyTo);
		if(i != frames.end())
			frames.erase(i);

		bHandled = FALSE;
		return 0;
	}
}

void PrivateFrame::addLine(const tstring& aLine, CHARFORMAT2& cf) {
	Identity i = Identity(NULL, 0);
    addLine(i, aLine, cf);
}

void PrivateFrame::addLine(const Identity& from, const tstring& aLine) {
	addLine(from, aLine, WinUtil::m_ChatTextGeneral );
}

void PrivateFrame::fillLogParams(ParamMap& params) const {
	const CID& cid = replyTo.user->getCID();
	const string& hint = replyTo.hint;
	params["hubNI"] = [&] { return Text::fromT(hubNames); };
	params["hubURL"] = [&] { return hint; };
	params["userCID"] = [&cid] { return cid.toBase32(); };
	params["userNI"] = [&] { return ClientManager::getInstance()->getNick(replyTo.user, hint); };
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
		LogManager::getInstance()->log(replyTo.user, params);
	}

	auto myNick = Text::toT(ctrlClient.getClient() ? ctrlClient.getClient()->get(HubSettings::Nick) : SETTING(NICK));
	bool notify = ctrlClient.AppendChat(from, myNick, SETTING(TIME_STAMPS) ? Text::toT("[" + Util::getShortTimeString() + "] ") : _T(""), aLine + _T('\n'), cf);
	addClientLine(TSTRING(LAST_CHANGE) + _T(" ") + Text::toT(Util::getTimeString()));

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

	tabMenu.InsertSeparatorFirst(Text::toT(ClientManager::getInstance()->getNick(replyTo.user, replyTo.hint, true)));
	if(SETTING(LOG_PRIVATE_CHAT)) {
		tabMenu.AppendMenu(MF_STRING, IDC_OPEN_USER_LOG,  CTSTRING(OPEN_USER_LOG));
		tabMenu.AppendMenu(MF_SEPARATOR);
		tabMenu.AppendMenu(MF_STRING, IDC_USER_HISTORY,  CTSTRING(VIEW_HISTORY));
		tabMenu.AppendMenu(MF_SEPARATOR);
	}
	tabMenu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CTSTRING(CLEAR_CHAT));
	appendUserItems(tabMenu, true, replyTo.user);

	prepareMenu(tabMenu, UserCommand::CONTEXT_USER, ClientManager::getInstance()->getHubUrls(replyTo.user->getCID()));
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

	ClientManager::getInstance()->userCommand(replyTo, uc, ucParams, true);
}

void PrivateFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);

	if(ctrlStatus.IsWindow()) {
		CRect sr;
		ctrlStatus.GetClientRect(sr);

		if (ctrlHubSel.GetStyle() & WS_VISIBLE) {
			int w[STATUS_LAST];
			tstring tmp = _T(" ") + TSTRING(SEND_PM_VIA);
			ctrlStatus.SetText(STATUS_HUBSEL, tmp.c_str());

			int desclen = WinUtil::getTextWidth(tmp, ctrlStatus.m_hWnd);
			w[STATUS_TEXT] = sr.right - 165 - desclen;
			w[STATUS_HUBSEL] = w[0] + desclen + 165;

			sr.top = 1;
			sr.left = w[STATUS_HUBSEL-1] + desclen + 10;
			sr.right = sr.left + 150;
			ctrlHubSel.MoveWindow(sr);

			ctrlStatus.SetParts(STATUS_LAST, w);
		} else {
			int w[1];
			w[0] = sr.right - 16;
			ctrlStatus.SetParts(1, w);
		}
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

string PrivateFrame::getLogPath() const {
	ParamMap params;
	fillLogParams(params);
	return LogManager::getInstance()->getPath(replyTo.user, params);
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

void PrivateFrame::closeAll(){
	for(auto f: frames | map_values)
		f->PostMessage(WM_CLOSE, 0, 0);
}

void PrivateFrame::closeAllOffline() {
	for(auto& fp: frames) {
		if(!fp.first->isOnline())
			fp.second->PostMessage(WM_CLOSE, 0, 0);
	}
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

void PrivateFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	ctrlClient.SetBackgroundColor(WinUtil::bgColor);
	RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
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