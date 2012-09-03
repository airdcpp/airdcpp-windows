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

#include "PrivateFrame.h"
#include "SearchFrm.h"
#include "WinUtil.h"
#include "MainFrm.h"
#include "EmoticonsManager.h"
#include "TextFrame.h"

#include "../client/Client.h"
#include "../client/ClientManager.h"
#include "../client/Util.h"
#include "../client/LogManager.h"
#include "../client/UploadManager.h"
#include "../client/ShareManager.h"
#include "../client/FavoriteManager.h"
#include "../client/QueueManager.h"
#include "../client/StringTokenizer.h"

#include <boost/range/algorithm/for_each.hpp>

PrivateFrame::FrameMap PrivateFrame::frames;
tstring pSelectedLine = Util::emptyStringT;
tstring pSelectedURL = Util::emptyStringT;

extern EmoticonsManager* emoticonsManager;

PrivateFrame::PrivateFrame(const HintedUser& replyTo_, Client* c) : replyTo(replyTo_),
	created(false), closed(false), online(true), curCommandPosition(0), hubName(Util::emptyStringT), 
	ctrlMessageContainer(WC_EDIT, this, PM_MESSAGE_MAP),
	ctrlHubSelContainer(WC_COMBOBOX, this, HUB_SEL_MAP),
	ctrlClientContainer(WC_EDIT, this, PM_MESSAGE_MAP), menuItems(0)
{
	ctrlClient.setClient(c);
}

LRESULT PrivateFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	
	ctrlClient.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY, WS_EX_CLIENTEDGE | WS_EX_ACCEPTFILES, IDC_CLIENT);
	
	ctrlClientContainer.SubclassWindow(ctrlClient.m_hWnd);
	ctrlClient.Subclass();
	ctrlClient.LimitText(0);
	ctrlClient.SetFont(WinUtil::font);
	
	ctrlClient.SetBackgroundColor( SETTING(BACKGROUND_COLOR) ); 
	ctrlClient.SetAutoURLDetect(false);
	ctrlClient.SetEventMask( ctrlClient.GetEventMask() | ENM_LINK );


	ctrlMessage.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_AUTOHSCROLL | ES_MULTILINE | ES_AUTOVSCROLL, WS_EX_CLIENTEDGE);

	ctrlHubSel.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE, IDC_HUB);
	ctrlHubSelContainer.SubclassWindow(ctrlHubSel.m_hWnd);
	ctrlHubSel.SetFont(WinUtil::font);

	ctrlHubSelDesc.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | SS_RIGHT | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	ctrlHubSelDesc.SetFont(WinUtil::font);
	
	ctrlMessageContainer.SubclassWindow(ctrlMessage.m_hWnd);

	ctrlMessage.SetFont(WinUtil::font);
	ctrlMessage.SetLimitText(9999);
	lineCount = 1; //ApexDC

	ctrlEmoticons.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_FLAT | BS_BITMAP | BS_CENTER, 0, IDC_EMOT);

	hEmoticonBmp.LoadFromResource(IDR_EMOTICON, _T("PNG"), _Module.get_m_hInst());
  	ctrlEmoticons.SetBitmap(hEmoticonBmp);

	addSpeakerTask();
	created = true;

	ClientManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	readLog();
	WinUtil::SetIcon(m_hWnd, IDR_PRIVATE);
	bHandled = FALSE;
	return 1;
}

void PrivateFrame::addSpeakerTask() {
	delayEvents.addEvent(replyTo.user->getCID(), [this] { runSpeakerTask(); }, 1000);
}

void PrivateFrame::runSpeakerTask() {
	PostMessage(WM_SPEAKER, USER_UPDATED);
}

LRESULT PrivateFrame::onHubChanged(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled) {
	auto hp = hubs[ctrlHubSel.GetCurSel()];
	changeClient();

	updateOnlineStatus();
	addStatusLine(CTSTRING_F(MESSAGES_SENT_THROUGH, Text::toT(hp.second)));

	bHandled = FALSE;
	return 0;
}

void PrivateFrame::on(ClientManagerListener::UserUpdated, const OnlineUser& aUser) noexcept {
	if(aUser.getUser() == replyTo.user) {
		ctrlClient.setClient(const_cast<Client*>(&aUser.getClient()));
		addSpeakerTask();
	}
}

void PrivateFrame::on(ClientManagerListener::UserConnected, const OnlineUser& aUser) noexcept {
	if(aUser.getUser() == replyTo.user)
		addSpeakerTask();
}

void PrivateFrame::on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) noexcept {
	if(aUser == replyTo.user) {
		ctrlClient.setClient(nullptr);
		addSpeakerTask();
	}
}

void PrivateFrame::addStatusLine(const tstring& aLine) {
	tstring status = _T(" *** ") + aLine + _T(" ***");
	if(BOOLSETTING(STATUS_IN_CHAT)) {
		addLine(status, WinUtil::m_ChatTextServer);
	} else {
		addClientLine(status);
	}
}

void PrivateFrame::changeClient() {
	auto hp = hubs[ctrlHubSel.GetCurSel()];
	replyTo.hint = move(hp.first);
	if(replyTo.hint.empty())
		replyTo.hint = initialHub;

	ctrlClient.setClient(ClientManager::getInstance()->getClient(replyTo.hint));
}

void PrivateFrame::updateOnlineStatus() {
	const CID& cid = replyTo.user->getCID();
	const string& hint = replyTo.hint;

	pair<tstring, bool> hubNames = WinUtil::getHubNames(cid, hint);

	hubName = move(hubNames.first);

	//setIcon(online ? IDI_PRIVATE : IDI_PRIVATE_OFF);

	auto oldSel = ctrlHubSel.GetStyle() & WS_VISIBLE ? ctrlHubSel.GetCurSel() : 0;
	StringPair oldHubPair;
	if (!hubs.empty())
		oldHubPair = hubs[oldSel]; // cache the old hub name

	hubs = ClientManager::getInstance()->getHubs(cid, hint);
	while (ctrlHubSel.GetCount()) {
		ctrlHubSel.DeleteString(0);
	}

	//General things
	if(hubNames.second) {	
		setDisconnected(false);
		if(!online) {
			addStatusLine(TSTRING(USER_WENT_ONLINE) + _T(" [") + WinUtil::getNicks(replyTo.user->getCID(), replyTo.hint) + _T(" - ") + hubName + _T("]"));
		}
	} else {
		setDisconnected(true);
		addStatusLine(TSTRING(USER_WENT_OFFLINE) + _T(" [") + Text::toT(oldHubPair.second) + _T("]"));
		ctrlClient.setClient(nullptr);
	}

	online = hubNames.second;
	SetWindowText((WinUtil::getNicks(replyTo.user->getCID(), replyTo.hint) + _T(" - ") + hubName).c_str());

	//ADC related changes
	if(online && !replyTo.user->isNMDC() && !hubs.empty()) {
		if(!(ctrlHubSel.GetStyle() & WS_VISIBLE)) {
			showHubSelection(true);
		}

		boost::for_each(hubs, [&](const StringPair &hub) {
			auto idx = ctrlHubSel.AddString(Text::toT(hub.second).c_str());
			if(hub.first == replyTo.hint) {
				ctrlHubSel.SetCurSel(idx);
			}
		});

		if(ctrlHubSel.GetCurSel() == -1) {
			ctrlHubSel.SetCurSel(0);
			changeClient();
			addStatusLine(CTSTRING_F(USER_OFFLINE_PM_CHANGE, Text::toT(oldHubPair.second) % Text::toT(hubs[0].second)));
		} else if (oldSel >= 0 && oldSel != ctrlHubSel.GetCurSel()) {
			changeClient();
			addStatusLine(CTSTRING_F(MESSAGES_SENT_THROUGH_REMOTE, Text::toT(hubs[ctrlHubSel.GetCurSel()].second)));
		}
	} else {
		showHubSelection(false);
	}
}

void PrivateFrame::showHubSelection(bool show) {
	ctrlHubSel.ShowWindow(show);
	ctrlHubSel.EnableWindow(show);

	ctrlHubSelDesc.ShowWindow(show);
	ctrlHubSelDesc.EnableWindow(show);

	UpdateLayout();
}

void PrivateFrame::gotMessage(const Identity& from, const UserPtr& to, const UserPtr& replyTo, const tstring& aMessage, Client* c) {
	PrivateFrame* p = nullptr;
	bool myPM = replyTo == ClientManager::getInstance()->getMe();
	const UserPtr& user = myPM ? to : replyTo;
	
	FrameIter i = frames.find(user);
	if(i == frames.end()) {
		if(frames.size() > 200) return;
		p = new PrivateFrame(HintedUser(user, c->getHubUrl()), c);
		frames[user] = p;
		
		p->addLine(from, aMessage);

		if(Util::getAway()) {
			if(!(BOOLSETTING(NO_AWAYMSG_TO_BOTS) && user->isSet(User::BOT))) 
			{
				ParamMap params;
				from.getParams(params, "user", false);
				p->sendMessage(Text::toT(Util::getAwayMessage(params)));
			}
		}

		if(BOOLSETTING(FLASH_WINDOW_ON_NEW_PM)) {
			WinUtil::FlashWindow();
		}
		if(BOOLSETTING(POPUP_NEW_PM)) {
			pair<tstring, bool> hubs = WinUtil::getHubNames(replyTo, c->getHubUrl());
			if(BOOLSETTING(PM_PREVIEW)) {
				tstring message = aMessage.substr(0, 250);
				MainFrame::getMainFrame()->ShowBalloonTip(message.c_str(), CTSTRING(PRIVATE_MESSAGE));
			} else {
				MainFrame::getMainFrame()->ShowBalloonTip(WinUtil::getNicks(replyTo, c->getHubUrl()) + _T(" - ") + hubs.first, TSTRING(PRIVATE_MESSAGE));
			}
		}

		if((BOOLSETTING(PRIVATE_MESSAGE_BEEP) || BOOLSETTING(PRIVATE_MESSAGE_BEEP_OPEN)) && (!BOOLSETTING(SOUNDS_DISABLED))) {
			if (SETTING(BEEPFILE).empty()) {
				MessageBeep(MB_OK);
			} else {
				::PlaySound(Text::toT(SETTING(BEEPFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
			}
		}
	} else {
		if(!myPM) {

			if(BOOLSETTING(FLASH_WINDOW_ON_PM) && !BOOLSETTING(FLASH_WINDOW_ON_NEW_PM)) {
				WinUtil::FlashWindow();
			}

			if(BOOLSETTING(POPUP_PM)) {
				if(BOOLSETTING(PM_PREVIEW)) {
					tstring message = aMessage.substr(0, 250);
					MainFrame::getMainFrame()->ShowBalloonTip(message.c_str(), CTSTRING(PRIVATE_MESSAGE));
				} else {
					pair<tstring, bool> hubs = WinUtil::getHubNames(replyTo, c->getHubUrl());
					MainFrame::getMainFrame()->ShowBalloonTip(WinUtil::getNicks(replyTo, c->getHubUrl()) + _T(" - ") + hubs.first, TSTRING(PRIVATE_MESSAGE));
				}
			}

			if((BOOLSETTING(PRIVATE_MESSAGE_BEEP)) && (!BOOLSETTING(SOUNDS_DISABLED))) {
				if (SETTING(BEEPFILE).empty()) {
					MessageBeep(MB_OK);
				} else {
					::PlaySound(Text::toT(SETTING(BEEPFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
				}
			}
			i->second->updateFrameOnlineStatus(HintedUser(user, c->getHubUrl()), c);
		}
		i->second->addLine(from, aMessage);
	}
}

void PrivateFrame::openWindow(const HintedUser& replyTo, const tstring& msg, Client* c) {
	PrivateFrame* p = NULL;
	FrameIter i = frames.find(replyTo);
	if(i == frames.end()) {
		if(frames.size() > 200) return;
		p = new PrivateFrame(replyTo, c);
		frames[replyTo] = p;
		p->CreateEx(WinUtil::mdiClient);
	} else {
		p = i->second;
		p->updateFrameOnlineStatus(replyTo, c); 
		if(::IsIconic(p->m_hWnd))
			::ShowWindow(p->m_hWnd, SW_RESTORE);
		p->MDIActivate(p->m_hWnd);
	}
	if(!msg.empty())
		p->sendMessage(msg);
}
/*
 update the re used frame to the correct hub, 
 so it doesnt appear offline while user is sending us messages with another hub :P
 should we change to same hub with the user even if its not offline?
*/
void PrivateFrame::updateFrameOnlineStatus(const HintedUser& newUser, Client* c) {

	if(!replyTo.user->isNMDC() && replyTo.hint != newUser.hint) {
		replyTo.hint = newUser.hint;
		ctrlClient.setClient(c);
		updateOnlineStatus();
	}
}

LRESULT PrivateFrame::onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	if (uMsg != WM_KEYDOWN && uMsg != WM_CUT && uMsg != WM_PASTE) {
		switch(wParam) {
			case VK_RETURN:
				if( WinUtil::isShift() || WinUtil::isCtrl() ||  WinUtil::isAlt() ) {
					bHandled = FALSE;
				}
				break;
		case VK_TAB:
				bHandled = TRUE;
  				break;
  			default:
  				bHandled = FALSE;
				break;
		}
		if ((uMsg == WM_CHAR) && (GetFocus() == ctrlMessage.m_hWnd) && (wParam != VK_RETURN) && (wParam != VK_TAB) && (wParam != VK_BACK)) {
			if ((!SETTING(SOUND_TYPING_NOTIFY).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
				PlaySound(Text::toT(SETTING(SOUND_TYPING_NOTIFY)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
		}
		return 0;
	}
	
	switch(wParam) {
		case VK_TAB:
		{
			if(GetFocus() == ctrlMessage.m_hWnd)
			{
				ctrlClient.SetFocus();
			}
			else
			{
				ctrlMessage.SetFocus();
			}
		}	
	}
	
	if((GetFocus() == ctrlMessage.m_hWnd) && (GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000) && (wParam == 'A')){
				ctrlMessage.SetSelAll();
				return 0;
	 }
	// don't handle these keys unless the user is entering a message
	if (GetFocus() != ctrlMessage.m_hWnd) {
		bHandled = FALSE;
		return 0;
	}	
	
	switch(wParam) {
		case VK_RETURN:
			if( (GetKeyState(VK_SHIFT) & 0x8000) || 
				(GetKeyState(VK_CONTROL) & 0x8000) || 
				(GetKeyState(VK_MENU) & 0x8000) ) {
				bHandled = FALSE;
			} else {
				if(uMsg == WM_KEYDOWN) {
					onEnter();
				}
			}
			break;
		case VK_UP:
			if ((GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
				//scroll up in chat command history
				//currently beyond the last command?
				if (curCommandPosition > 0) {
					//check whether current command needs to be saved
					if (curCommandPosition == prevCommands.size()) {
						currentCommand.resize(ctrlMessage.GetWindowTextLength());
						ctrlMessage.GetWindowText(&currentCommand[0], ctrlMessage.GetWindowTextLength() + 1);
					}
					//replace current chat buffer with current command
					ctrlMessage.SetWindowText(prevCommands[--curCommandPosition].c_str());
				}
			} else {
				bHandled = FALSE;
			}
			break;
		case VK_DOWN:
			if ((GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
				//scroll down in chat command history
				//currently beyond the last command?
				if (curCommandPosition + 1 < prevCommands.size()) {
					//replace current chat buffer with current command
					ctrlMessage.SetWindowText(prevCommands[++curCommandPosition].c_str());
				} else if (curCommandPosition + 1 == prevCommands.size()) {
					//revert to last saved, unfinished command
					ctrlMessage.SetWindowText(currentCommand.c_str());
					++curCommandPosition;
				}
			} else {
				bHandled = FALSE;
			}
			break;
		case VK_HOME:
			if (!prevCommands.empty() && (GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
				curCommandPosition = 0;
				currentCommand.resize(ctrlMessage.GetWindowTextLength());
				ctrlMessage.GetWindowText(&currentCommand[0], ctrlMessage.GetWindowTextLength() + 1);
				ctrlMessage.SetWindowText(prevCommands[curCommandPosition].c_str());
			} else {
				bHandled = FALSE;
			}
			break;
		case VK_END:
			if ((GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
				curCommandPosition = prevCommands.size();
				ctrlMessage.SetWindowText(currentCommand.c_str());
			} else {
				bHandled = FALSE;
			}
			break;
 //ApexDC
		case VK_PRIOR: // page up
			ctrlClient.SendMessage(WM_VSCROLL, SB_PAGEUP);
			break;
		case VK_NEXT: // page down
			ctrlClient.SendMessage(WM_VSCROLL, SB_PAGEDOWN);
			break;
//End
		default:
			bHandled = FALSE;
//ApexDC
		}

	// Kinda ugly, but oh well... will clean it later, maybe...
	if(SETTING(MAX_RESIZE_LINES) != 1) {
		int newLineCount = ctrlMessage.GetLineCount();
		int start, end;
		ctrlMessage.GetSel(start, end);
		if(wParam == VK_DELETE || uMsg == WM_CUT ||  uMsg == WM_PASTE || (wParam == VK_BACK && (start != end))) {
			if(uMsg == WM_PASTE) {
				// We don't need to hadle these
				if(!IsClipboardFormatAvailable(CF_TEXT))
					return 0;
				if(!::OpenClipboard(WinUtil::mainWnd))
					return 0;
			}
			tstring buf;
			string::size_type i;
			buf.resize(ctrlMessage.GetWindowTextLength() + 1);
			buf.resize(ctrlMessage.GetWindowText(&buf[0], buf.size()));
			buf = buf.substr(start, end);
			while((i = buf.find(_T("\n"))) != string::npos) {
				buf.erase(i, 1);
				newLineCount--;
			}
			if(uMsg == WM_PASTE) {
				HGLOBAL hglb = GetClipboardData(CF_TEXT); 
				if(hglb != NULL) { 
					char* lptstr = (char*)GlobalLock(hglb); 
					if(lptstr != NULL) {
						string tmp(lptstr);
						// why I have to do this this way?
						for(string::size_type i = 0; i < tmp.size(); ++i) {
							if(tmp[i] == '\n') {
								newLineCount++;
								if(newLineCount >= SETTING(MAX_RESIZE_LINES))
									break;
							}
						}
						GlobalUnlock(hglb);
					}
				}
				CloseClipboard();
			}
		} else if(wParam == VK_BACK) {
			POINT cpt;
			GetCaretPos(&cpt);
			int charIndex = ctrlMessage.CharFromPos(cpt);
			charIndex = LOWORD(charIndex) - ctrlMessage.LineIndex(ctrlMessage.LineFromChar(charIndex));
			if(charIndex <= 0) {
				newLineCount -= 1;
			}
		} else if(WinUtil::isCtrl() && wParam == VK_RETURN) {
			newLineCount += 1;
		} 
		if(newLineCount != lineCount) {
			if(lineCount >= SETTING(MAX_RESIZE_LINES) && newLineCount >= SETTING(MAX_RESIZE_LINES)) {
				lineCount = newLineCount;
			} else {
				lineCount = newLineCount;
				UpdateLayout(FALSE);
			}
		}
//End
	}
	return 0;
}

void PrivateFrame::onEnter()
{ 
	
	bool resetText = true;

	if(ctrlMessage.GetWindowTextLength() > 0) {
		tstring s;
		s.resize(ctrlMessage.GetWindowTextLength());
		
		ctrlMessage.GetWindowText(&s[0], s.size() + 1);

		// save command in history, reset current buffer pointer to the newest command
		curCommandPosition = prevCommands.size();		//this places it one position beyond a legal subscript
		if (!curCommandPosition || curCommandPosition > 0 && prevCommands[curCommandPosition - 1] != s) {
			++curCommandPosition;
			prevCommands.push_back(s);
		}
		currentCommand = Util::emptyStringT;

		// Process special commands
		if(s[0] == '/') {
			tstring m = s;
			tstring param;
			tstring message;
			tstring status;
			bool thirdPerson = false;
			if(WinUtil::checkCommand(s, param, message, status, thirdPerson)) {
				if(!message.empty()) {
					sendMessage(message, thirdPerson);
				}
				if(!status.empty()) {
					addClientLine(status);
				}
			} else if((stricmp(s.c_str(), _T("clear")) == 0) || (stricmp(s.c_str(), _T("cls")) == 0)) {
				ctrlClient.SetWindowText(_T(""));
			} else if(stricmp(s.c_str(), _T("grant")) == 0) {
				UploadManager::getInstance()->reserveSlot(HintedUser(replyTo, replyTo.hint), 600);
				addClientLine(TSTRING(SLOT_GRANTED));
			} else if(stricmp(s.c_str(), _T("close")) == 0) {
				PostMessage(WM_CLOSE);
			} else if((stricmp(s.c_str(), _T("favorite")) == 0) || (stricmp(s.c_str(), _T("fav")) == 0)) {
				FavoriteManager::getInstance()->addFavoriteUser(replyTo);
				addClientLine(TSTRING(FAVORITE_USER_ADDED));
			} else if(stricmp(s.c_str(), _T("getlist")) == 0) {
				BOOL bTmp;
				onGetList(0,0,0,bTmp);
			} else if(stricmp(s.c_str(), _T("log")) == 0) {
				ParamMap params;
				const CID& cid = replyTo.user->getCID();
				const string& hint = replyTo.hint;
	
				params["hubNI"] = Util::toString(ClientManager::getInstance()->getHubNames(cid, hint));
				params["hubURL"] = Util::toString(ClientManager::getInstance()->getHubUrls(cid, hint));
				params["userCID"] = cid.toBase32(); 
				params["userNI"] = ClientManager::getInstance()->getNicks(cid, hint)[0];
				params["myCID"] = ClientManager::getInstance()->getMe()->getCID().toBase32();
				WinUtil::openFile(Text::toT(LogManager::getInstance()->getPath(LogManager::PM, params)));
			} else if(stricmp(s.c_str(), _T("stats")) == 0) {
				sendMessage(Text::toT(WinUtil::generateStats()));
			} else if(stricmp(s.c_str(), _T("speed")) == 0) {
					addLine (WinUtil::Speedinfo(), WinUtil::m_ChatTextSystem);
			} else if(stricmp(s.c_str(), _T("info")) == 0) {
					addLine (WinUtil::UselessInfo(), WinUtil::m_ChatTextSystem);
			} else if((stricmp(s.c_str(), _T("disks")) == 0) || (stricmp(s.c_str(), _T("di")) == 0)){
					addLine (WinUtil::diskInfo(), WinUtil::m_ChatTextSystem);
			} else if(stricmp(s.c_str(), _T("df")) == 0) {
					addLine (WinUtil::DiskSpaceInfo(), WinUtil::m_ChatTextSystem);
			} else if(stricmp(s.c_str(), _T("dfs")) == 0) {
					sendMessage(WinUtil::DiskSpaceInfo());
			} else if(stricmp(s.c_str(), _T("uptime")) == 0) {
					sendMessage(Text::toT(WinUtil::uptimeInfo()));

			} else if(stricmp(s.c_str(), _T("help")) == 0) {
				addLine(_T("*** ") + WinUtil::commands + _T(", /getlist, /clear, /grant, /close, /favorite, /winamp"), WinUtil::m_ChatTextSystem);
			} else {
				if(replyTo.user->isOnline()) {
					if (BOOLSETTING(SEND_UNKNOWN_COMMANDS)) {
						sendMessage(tstring(m));
					} else {
						addClientLine(TSTRING(UNKNOWN_COMMAND) + _T(" ") + m);
					}
				} else {
					ctrlStatus.SetText(0, CTSTRING(USER_WENT_OFFLINE));
					resetText = false;
				}
			}
		} else {
			if(replyTo.user->isOnline()) {
				sendMessage(s);
			} else {
				ctrlStatus.SetText(0, CTSTRING(USER_WENT_OFFLINE));
				resetText = false;
			}
		}
		if(resetText)
			ctrlMessage.SetWindowText(_T(""));
	} 
	
}

void PrivateFrame::sendMessage(const tstring& msg, bool thirdPerson) {
	ClientManager::getInstance()->privateMessage(replyTo, Text::fromT(msg), thirdPerson);
}

LRESULT PrivateFrame::onMagnet(UINT uMsg, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/){
	tstring* tmp = (tstring*)lParam;
	/*
	if the sender is not a bot, assume the user sending the magnet is sharing it,
	if he isnt, too bad.. we just get file not available..
	*/
	
	if(!replyTo.user->isSet(User::BOT))
		WinUtil::parseMagnetUri(*tmp, false, replyTo.user, replyTo.hint);
	else
		WinUtil::parseMagnetUri(*tmp);

	delete tmp;

	return 0;
} 


LRESULT PrivateFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		hEmoticonBmp.Destroy();
		ClientManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		FrameIter i = frames.find(replyTo);
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
	params["hubNI"] = [&] { return Util::toString(ClientManager::getInstance()->getHubNames(cid, hint)); };
	params["hubURL"] = [&] { return Util::toString(ClientManager::getInstance()->getHubUrls(cid, hint)); };
	params["userCID"] = [&cid] { return cid.toBase32(); };
	params["userNI"] = [&] { return ClientManager::getInstance()->getNicks(cid, hint)[0]; };
	params["myCID"] = [] { return ClientManager::getInstance()->getMe()->getCID().toBase32(); };
}

void PrivateFrame::addLine(const Identity& from, const tstring& aLine, CHARFORMAT2& cf) {
	if(!created) {
		if(BOOLSETTING(POPUNDER_PM))
			WinUtil::hiddenCreateEx(this);
		else
			CreateEx(WinUtil::mdiClient);
	}

	CRect r;
	ctrlClient.GetClientRect(r);

	if(BOOLSETTING(LOG_PRIVATE_CHAT)) {
		ParamMap params;
		params["message"] = [&aLine] { return Text::fromT(aLine); };
		fillLogParams(params);
		LOG(LogManager::PM, params);
	}

	if(BOOLSETTING(TIME_STAMPS)) {
		ctrlClient.AppendText(from, Text::toT(SETTING(NICK)), Text::toT("[" + Util::getShortTimeString() + "] "), aLine + _T('\n'), cf);
	} else {
		ctrlClient.AppendText(from, Text::toT(SETTING(NICK)), _T(""), aLine + _T('\n'), cf);
	}
	addClientLine(TSTRING(LAST_CHANGE) + _T(" ") + Text::toT(Util::getTimeString()));

	if (BOOLSETTING(BOLD_PM)) {
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

	tabMenu.InsertSeparatorFirst(Text::toT(ClientManager::getInstance()->getNicks(replyTo.user->getCID(), replyTo.hint)[0]));
	if(BOOLSETTING(LOG_PRIVATE_CHAT)) {
		tabMenu.AppendMenu(MF_STRING, IDC_OPEN_USER_LOG,  CTSTRING(OPEN_USER_LOG));
		tabMenu.AppendMenu(MF_SEPARATOR);
		tabMenu.AppendMenu(MF_STRING, IDC_USER_HISTORY,  CTSTRING(VIEW_HISTORY));
		tabMenu.AppendMenu(MF_SEPARATOR);
	}
	tabMenu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CTSTRING(CLEAR_CHAT));
	tabMenu.AppendMenu(MF_SEPARATOR);
	tabMenu.AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
	tabMenu.AppendMenu(MF_STRING, IDC_BROWSELIST, CTSTRING(BROWSE_FILE_LIST));
	tabMenu.AppendMenu(MF_STRING, IDC_MATCH_QUEUE, CTSTRING(MATCH_QUEUE));
	tabMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)WinUtil::grantMenu, CTSTRING(GRANT_SLOTS_MENU));
	tabMenu.AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));

	prepareMenu(tabMenu, UserCommand::CONTEXT_USER, ClientManager::getInstance()->getHubUrls(replyTo.user->getCID(), replyTo.hint));
	if(!(tabMenu.GetMenuState(tabMenu.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR)) {	
		tabMenu.AppendMenu(MF_SEPARATOR);
	}
	tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));

	tabMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
	return TRUE;
}

void PrivateFrame::runUserCommand(UserCommand& uc) {

	if(!WinUtil::getUCParams(m_hWnd, uc, ucLineParams))
		return;

	auto ucParams = ucLineParams;

	ClientManager::getInstance()->userCommand(replyTo, uc, ucParams, true);
}

LRESULT PrivateFrame::onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	try {
		QueueManager::getInstance()->addList(HintedUser(replyTo, replyTo.hint), QueueItem::FLAG_CLIENT_VIEW);
	} catch(const Exception& e) {
		addClientLine(Text::toT(e.getError()));
	}
	return 0;
}

LRESULT PrivateFrame::onBrowseList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	try {
		QueueManager::getInstance()->addList(HintedUser(replyTo, replyTo.hint), QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_PARTIAL_LIST);
	} catch(const Exception& e) {
		addClientLine(Text::toT(e.getError()));
	}
	return 0;
}

LRESULT PrivateFrame::onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	try {
		QueueManager::getInstance()->addList(HintedUser(replyTo, replyTo.hint), QueueItem::FLAG_MATCH_QUEUE);
	} catch(const Exception& e) {
		addClientLine(Text::toT(e.getError()));
	}
	return 0;
}

LRESULT PrivateFrame::onGrantSlot(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	uint64_t time = 0;
	switch(wID) {
		case IDC_GRANTSLOT:			time = 600; break;
		case IDC_GRANTSLOT_DAY:		time = 3600; break;
		case IDC_GRANTSLOT_HOUR:	time = 24*3600; break;
		case IDC_GRANTSLOT_WEEK:	time = 7*24*3600; break;
		case IDC_UNGRANTSLOT:		time = 0; break;
	}
	
	if(time > 0)
		UploadManager::getInstance()->reserveSlot(HintedUser(replyTo, replyTo.hint), time);
	else
		UploadManager::getInstance()->unreserveSlot(replyTo);

	return 0;
}

LRESULT PrivateFrame::onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	FavoriteManager::getInstance()->addFavoriteUser(replyTo);
	return 0;
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
		//if (ctrlHubSelDesc.IsWindow()) {
			int w[STATUS_LAST];
			tstring tmp = TSTRING(SEND_PM_VIA);
		
			w[STATUS_TEXT] = sr.right - 166 - WinUtil::getTextWidth(tmp, ctrlHubSelDesc.m_hWnd) - 20;
			w[STATUS_HUBSEL_DESC] = w[0] + WinUtil::getTextWidth(tmp, ctrlHubSelDesc.m_hWnd) + 20;
			w[STATUS_HUBSEL_CTRL] = w[1] + 150;

			ctrlHubSelDesc.SetWindowTextW(tmp.c_str());

			sr.top = (WinUtil::getTextHeight(m_hWnd, WinUtil::font) / 2);
			sr.left = w[STATUS_HUBSEL_DESC-1] + 10;
			sr.right = w[STATUS_HUBSEL_DESC] - 10;
			ctrlHubSelDesc.MoveWindow(sr);

			sr.top = 2;
			sr.left = w[STATUS_HUBSEL_CTRL-1];
			sr.right = w[STATUS_HUBSEL_CTRL];
			ctrlHubSel.MoveWindow(sr);

			ctrlStatus.SetParts(STATUS_LAST, w);
		} else {
			int w[1];
			w[0] = sr.right - 16;
			ctrlStatus.SetParts(1, w);
		}
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
	ctrlClient.MoveWindow(rc);
	
	rc = rect;
	rc.bottom -= 2;
	rc.top = rc.bottom - h - 5;
	rc.left +=2;
	rc.right -= 2 + 24;
	ctrlMessage.MoveWindow(rc);

	 //ApexDC	
	if(h != (WinUtil::fontHeight + 4)) {
		rc.bottom -= h - (WinUtil::fontHeight + 4);
	}
	rc.left = rc.right + 2;
  	rc.right += 24;
  	 
  	ctrlEmoticons.MoveWindow(rc);
}

LRESULT PrivateFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if(reinterpret_cast<HWND>(wParam) == ctrlEmoticons) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click
		menuItems = 0;

		if(emoMenu != NULL)
			emoMenu.DestroyMenu();

		emoMenu.CreatePopupMenu();
		emoMenu.InsertSeparatorFirst(_T("Emoticons Pack"));
		emoMenu.AppendMenu(MF_STRING, IDC_EMOMENU, _T("Disabled"));
		
		if (SETTING(EMOTICONS_FILE) == "Disabled")
			emoMenu.CheckMenuItem( IDC_EMOMENU, MF_BYCOMMAND | MF_CHECKED );
		
		// nacteme seznam emoticon packu (vsechny *.xml v adresari EmoPacks)
		WIN32_FIND_DATA data;
		HANDLE hFind;
		hFind = FindFirstFile(Text::toT(Util::getPath(Util::PATH_EMOPACKS) + "*.xml").c_str(), &data);
		if(hFind != INVALID_HANDLE_VALUE) {
			do {
				tstring name = data.cFileName;
				tstring::size_type i = name.rfind('.');
				name = name.substr(0, i);

				menuItems++;
				emoMenu.AppendMenu(MF_STRING, IDC_EMOMENU + menuItems, name.c_str());
				if(name == Text::toT(SETTING(EMOTICONS_FILE))) emoMenu.CheckMenuItem( IDC_EMOMENU + menuItems, MF_BYCOMMAND | MF_CHECKED );
			} while(FindNextFile(hFind, &data));
			FindClose(hFind);
		}
		
		if(menuItems>0)
			emoMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

		emoMenu.RemoveFirstItem();
	}

	return 0;
}

string PrivateFrame::getLogPath() const {
	ParamMap params;
	fillLogParams(params);
	return LogManager::getInstance()->getPath(LogManager::PM, params);
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
			ctrlClient.AppendText(Identity(NULL, 0), _T("- "), _T(""), Text::toT(lines[i]) + _T('\n'), WinUtil::m_ChatTextLog, true);
		}
		f.close();
	} catch(const FileException&){
	}
}

void PrivateFrame::closeAll(){
	for(FrameIter i = frames.begin(); i != frames.end(); ++i)
		i->second->PostMessage(WM_CLOSE, 0, 0);
}

void PrivateFrame::closeAllOffline() {
	for(FrameIter i = frames.begin(); i != frames.end(); ++i) {
		if(!i->first->isOnline())
			i->second->PostMessage(WM_CLOSE, 0, 0);
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

LRESULT PrivateFrame::onEmoPackChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TCHAR buf[256];
	emoMenu.GetMenuString(wID, buf, 256, MF_BYCOMMAND);
	if (buf!=Text::toT(SETTING(EMOTICONS_FILE))) {
		SettingsManager::getInstance()->set(SettingsManager::EMOTICONS_FILE, Text::fromT(buf));
		emoticonsManager->Unload();
		emoticonsManager->Load();
	}
	return 0;
}

LRESULT PrivateFrame::onEmoticons(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled) {
	if (hWndCtl != ctrlEmoticons.m_hWnd) {
		bHandled = false;
        return 0;
    }
 
	EmoticonsDlg dlg;
	ctrlEmoticons.GetWindowRect(dlg.pos);
	dlg.DoModal(m_hWnd);
	if (!dlg.result.empty()) {
		TCHAR* message = new TCHAR[ctrlMessage.GetWindowTextLength()+1];
		ctrlMessage.GetWindowText(message, ctrlMessage.GetWindowTextLength()+1);
		tstring s(message, ctrlMessage.GetWindowTextLength());
		delete[] message;
		
		ctrlMessage.SetWindowText((s+dlg.result).c_str());
		ctrlMessage.SetFocus();
		ctrlMessage.SetSel( ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength() );
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
LRESULT PrivateFrame::onDropFiles(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/){
	HDROP drop = (HDROP)wParam;
	tstring buf;
	buf.resize(MAX_PATH);

	UINT nrFiles;
	
	nrFiles = DragQueryFile(drop, (UINT)-1, NULL, 0);
	
	for(UINT i = 0; i < nrFiles; ++i){
		if(DragQueryFile(drop, i, &buf[0], MAX_PATH)){
			if(!PathIsDirectory(&buf[0])){
				addMagnet(buf);
			}
		}
	}

	DragFinish(drop);

	return 0;
}
void PrivateFrame::addMagnet(const tstring& path) {
	string magnetlink = Util::emptyString;

	TTHValue TTH;
	int64_t size = 0;
	boost::scoped_array<char> buf(new char[512 * 1024]);

	try {
		File f(Text::fromT(path), File::READ, File::OPEN);
		TigerTree tth(TigerTree::calcBlockSize(f.getSize(), 1));

		if(f.getSize() > 0) {
			size_t n = 512*1024;
			while( (n = f.read(&buf[0], n)) > 0) {
				tth.update(&buf[0], n);
				n = 512*1024;
			}
		} else {
			tth.update("", 0);
		}
		tth.finalize();

		TTH = tth.getRoot();
		size = f.getSize();
		magnetlink = "magnet:?xt=urn:tree:tiger:"+ TTH.toBase32() +"&xl="+Util::toString(size)+"&dn="+Text::fromT(Util::getFileName(path));
		f.close();
	}catch(...) { }

	if(!magnetlink.empty()){
		if(ShareManager::getInstance()->addTempShare(replyTo.user->getCID().toBase32(), TTH, Text::fromT(path), size, !replyTo.user->isNMDC()))
			ctrlMessage.SetWindowText(Text::toT(magnetlink).c_str());
		else
			MessageBox(_T("File is not shared and temporary shares are not supported with NMDC hubs!"), _T("NMDC hub not supported!"), MB_ICONWARNING | MB_OK);
	}
}

/**
 * @file
 * $Id: PrivateFrame.cpp 476 2010-01-25 21:43:12Z bigmuscle $
 */
