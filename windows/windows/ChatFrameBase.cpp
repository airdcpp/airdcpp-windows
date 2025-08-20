/*
 * Copyright (C) 2011-2024 AirDC++ Project
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

#include <windows/util/ActionUtil.h>
#include <windows/dialog/BrowseDlg.h>
#include <windows/ChatFrameBase.h>
#include <windows/ChatCommands.h>
#include <windows/EmoticonsDlg.h>
#include <windows/EmoticonsManager.h>
#include <windows/components/ExMessageBox.h>
#include <windows/HttpLinks.h>
#include <windows/MainFrm.h>
#include <windows/players/Players.h>
#include <windows/frames/search/SearchFrm.h>
#include <windows/util/WinUtil.h>

#include <airdcpp/modules/AutoSearchManager.h>

#include <airdcpp/hub/ClientManager.h>
#include <airdcpp/connectivity/ConnectivityManager.h>
#include <airdcpp/hash/HashManager.h>
#include <airdcpp/util/LinkUtil.h>
#include <airdcpp/events/LogManager.h>
#include <airdcpp/util/PathUtil.h>
#include <airdcpp/queue/QueueManager.h>
#include <airdcpp/settings/SettingsManager.h>
#include <airdcpp/share/profiles/ShareProfileManager.h>
#include <airdcpp/share/temp_share/TempShareManager.h>
#include <airdcpp/connection/ThrottleManager.h>
#include <airdcpp/core/update/UpdateManager.h>
#include <airdcpp/util/Util.h>

#include <airdcpp/core/version.h>

namespace wingui {

ChatFrameBase::ChatFrameBase() {
}

ChatFrameBase::~ChatFrameBase() { }

LRESULT ChatFrameBase::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	cancelHashing = true;
	tasks.wait();

	bHandled = FALSE;
	return 0;
}

LRESULT ChatFrameBase::OnForwardMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	LPMSG pMsg = (LPMSG)lParam;
	if((pMsg->message >= WM_MOUSEFIRST) && (pMsg->message <= WM_MOUSELAST))
		ctrlTooltips.RelayEvent(pMsg);
	return 0;
}

void ChatFrameBase::init(HWND /*m_hWnd*/, RECT aRcDefault) {
	ctrlClient.Create(m_hWnd, aRcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		WS_VSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY, WS_EX_CLIENTEDGE | WS_EX_ACCEPTFILES, IDC_CLIENT);


	ctrlClient.Subclass();
	ctrlClient.LimitText(1024 * 64 * 2);
	ctrlClient.SetFont(WinUtil::font);

	ctrlClient.setAllowClear(true);
	
	ctrlClient.SetBackgroundColor(WinUtil::bgColor);

	ctrlTooltips.Create(m_hWnd, aRcDefault, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON, WS_EX_TOPMOST);
	ctrlTooltips.SetDelayTime(TTDT_AUTOMATIC, 600);
	ctrlTooltips.Activate(TRUE);

	if(SETTING(SHOW_MULTILINE)){
		ctrlResize.Create(m_hWnd, aRcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_FLAT | BS_ICON | BS_CENTER, 0, IDC_RESIZE);
		ctrlResize.SetIcon(GET_ICON(IDI_EXPAND_UP, 16));
		ctrlResize.SetFont(WinUtil::font);
		ctrlTooltips.AddTool(ctrlResize.m_hWnd, CTSTRING(MULTILINE_INPUT));
	}

	if (SETTING(SHOW_SEND_MESSAGE)) {
		ctrlSendMessage.Create(m_hWnd, aRcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_FLAT | BS_ICON | BS_CENTER, 0, IDC_SEND_MESSAGE);
		ctrlSendMessage.SetIcon(GET_ICON(IDI_SEND_MESSAGE, 16));
		ctrlSendMessage.SetFont(WinUtil::font);
		ctrlTooltips.AddTool(ctrlSendMessage.m_hWnd, CTSTRING(SEND_MESSAGE));
	}


	ctrlMessage.Create(m_hWnd, aRcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL |
		ES_AUTOHSCROLL | ES_MULTILINE | ES_AUTOVSCROLL, WS_EX_CLIENTEDGE);
	ctrlMessage.SetFont(WinUtil::font);
	ctrlMessage.SetLimitText(32*1024);
	lineCount = 1; //ApexDC

	if(SETTING(SHOW_EMOTICON)){
		ctrlEmoticons.Create(m_hWnd, aRcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_FLAT | BS_BITMAP | BS_CENTER, 0, IDC_EMOT);
		ctrlEmoticons.SetIcon(GET_ICON(IDR_EMOTICON, 16));
		ctrlTooltips.AddTool(ctrlEmoticons.m_hWnd, CTSTRING(INSERT_EMOTICON));
	}

	if(SETTING(SHOW_MAGNET)) {
		ctrlMagnet.Create(m_hWnd, aRcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_FLAT | BS_ICON | BS_CENTER, 0, IDC_BMAGNET);
		ctrlMagnet.SetIcon(GET_ICON(IDI_SEND_FILE, 16));
		ctrlTooltips.AddTool(ctrlMagnet.m_hWnd, getSendFileTitle().c_str());
	}
}

const tstring& ChatFrameBase::getSendFileTitle() {
	bool tmp = (getUser() && (!getUser()->isSet(User::BOT) && !getUser()->isSet(User::NMDC)));
	return tmp ? TSTRING(SEND_FILE_PM) : TSTRING(SEND_FILE_HUB);
}

LRESULT ChatFrameBase::onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!complete.empty() && wParam != VK_TAB && uMsg == WM_KEYDOWN)
		complete.clear();

	if (uMsg != WM_KEYDOWN && uMsg != WM_CUT && uMsg != WM_PASTE) { //ApexDC
		switch(wParam) {
			case VK_RETURN:
				if ((GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_SHIFT) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
					bHandled = FALSE;
				}
				break;
			case VK_TAB:
				bHandled = TRUE;
  				break;
			case VK_F16: //adds strange char when using ctrl+backspace
				bHandled = TRUE;
  				break;
  			default:
  				bHandled = FALSE;
				break;
		}

		 if ((uMsg == WM_CHAR) && (GetFocus() == ctrlMessage.m_hWnd) && (wParam != VK_RETURN) && (wParam != VK_TAB) && (wParam != VK_BACK)) {
			if ((!SETTING(SOUND_TYPING_NOTIFY).empty()) && (!SETTING(SOUNDS_DISABLED)))
				WinUtil::playSound(Text::toT(SETTING(SOUND_TYPING_NOTIFY)));
		}
		return 0;
	}

	if(wParam == VK_TAB) {
		onTab();
		return 0;
	} else if((GetFocus() == ctrlMessage.m_hWnd) && (GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000) && (wParam == 'A')){
		ctrlMessage.SetSelAll();
		return 0;
	} if(wParam == VK_F3) {
		ctrlClient.findText();
		return 0;
	} 

	// don't handle these keys unless the user is entering a message
	if (GetFocus() != ctrlMessage.m_hWnd) {
		bHandled = FALSE;
		return 0;
	}

	switch(wParam) {
		case VK_RETURN:
			if( (GetKeyState(VK_CONTROL) & 0x8000) || 
				(GetKeyState(VK_MENU) & 0x8000)  ||
				(GetKeyState(VK_SHIFT) & 0x8000)) {
					bHandled = FALSE;
				} else {
					if (resizePressed && !WinUtil::isShift()) { //Shift + Enter to send
						ctrlMessage.AppendText(_T("\r\n"));
					} else {
						handleSendMessage();
					}
				}
			break;
		case VK_UP:
			if ( (GetKeyState(VK_MENU) & 0x8000) ||	
				((SETTING(USE_CTRL_FOR_LINE_HISTORY) && WinUtil::isCtrl()) || 
				((!SETTING(USE_CTRL_FOR_LINE_HISTORY) && (ctrlMessage.GetWindowTextLength() == 0 || inHistory))))) {
				//scroll up in chat command history
				//currently beyond the last command?
				if (curCommandPosition > 0) {
					//check whether current command needs to be saved
					if (curCommandPosition == prevCommands.size()) {
						getLineText(currentCommand);
					}

					//replace current chat buffer with current command
					ctrlMessage.SetWindowText(prevCommands[--curCommandPosition].c_str());
					inHistory = true;
				}
				// move cursor to end of line
				ctrlMessage.SetSel(ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength());
			} else {
				bHandled = FALSE;
			}

			break;
		case VK_DOWN:
			if ( (GetKeyState(VK_MENU) & 0x8000) ||	
				((SETTING(USE_CTRL_FOR_LINE_HISTORY) && WinUtil::isCtrl()) ||
				((!SETTING(USE_CTRL_FOR_LINE_HISTORY) && (ctrlMessage.GetWindowTextLength() == 0 || inHistory))))) {
				//scroll down in chat command history

				//currently beyond the last command?
				if (curCommandPosition + 1 < prevCommands.size()) {
					//replace current chat buffer with current command
					ctrlMessage.SetWindowText(prevCommands[++curCommandPosition].c_str());
					inHistory = true;
				} else if (curCommandPosition + 1 == prevCommands.size()) {
					//revert to last saved, unfinished command

					ctrlMessage.SetWindowText(currentCommand.c_str());
					++curCommandPosition;
				}
				// move cursor to end of line
				ctrlMessage.SetSel(ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength());
			} else {
				bHandled = FALSE;
			}

			break;
		case VK_PRIOR: // page up
			ctrlClient.SendMessage(WM_VSCROLL, SB_PAGEUP);

			break;
		case VK_NEXT: // page down
			ctrlClient.SendMessage(WM_VSCROLL, SB_PAGEDOWN);

			break;
		case VK_HOME:
			if (!prevCommands.empty() && (GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
				curCommandPosition = 0;
				
				getLineText(currentCommand);

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
		default:
			inHistory = false;
			bHandled = FALSE;
//ApexDC
	}

	//reset the line history scrolling
	if (inHistory && (wParam != VK_UP && wParam != VK_DOWN)) {
		inHistory = false;
		curCommandPosition = prevCommands.size();
	}

	// Kinda ugly, but oh well... will clean it later, maybe...
	//if(SETTING(MAX_RESIZE_LINES) != 1) {
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
			getLineText(buf);
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
						for(i = 0; i < tmp.size(); ++i) {
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

			if (WinUtil::isCtrl()) {
				if (charIndex == 0)
					return 0;

				tstring s;
				getLineText(s);
				tstring newLine = s;

				//trim spaces before the word
				for (int i = charIndex-1; i >= 0; --i) {
					if (!isgraph(newLine[i])) {
						newLine.erase(i, 1);
					} else {
						break;
					}
				}

				if (s[charIndex-1] != _T('\n')) {
					//always stay on the old line here...
					auto p = newLine.find_last_of(_T("\n "), charIndex);
					if (p == tstring::npos) {
						p = 0;
					} else {
						p++;
					}

					newLine = newLine.erase(p, charIndex-p);
					charIndex = charIndex - (s.length()-newLine.length());
				} else {
					//remove the empty line
					charIndex--;
					newLineCount -= 1;
				}

				ctrlMessage.SetWindowText(newLine.c_str());
				ctrlMessage.SetFocus();
				ctrlMessage.SetSel(charIndex, charIndex);
			} else {
				charIndex = LOWORD(charIndex) - ctrlMessage.LineIndex(ctrlMessage.LineFromChar(charIndex));
				if(charIndex <= 0)
					newLineCount -= 1;
			}
		} else if((WinUtil::isCtrl() || WinUtil::isShift()) && wParam == VK_RETURN) {
			newLineCount += 1;
		}

		if((!resizePressed) && newLineCount != lineCount) {
			if(lineCount >= SETTING(MAX_RESIZE_LINES) && newLineCount >= SETTING(MAX_RESIZE_LINES)) {
				lineCount = newLineCount;
			} else {
				lineCount = newLineCount > 1 ? newLineCount : 1;
				UpdateLayout(FALSE);
			}
		}
//End
	//}
	return 0;
}

void ChatFrameBase::getLineText(tstring& s) {
	s.resize(ctrlMessage.GetWindowTextLength());
	ctrlMessage.GetWindowText(&s[0], ctrlMessage.GetWindowTextLength() + 1);
}

LRESULT ChatFrameBase::onSendMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
	handleSendMessage();
	ctrlMessage.SetFocus();
	bHandled = FALSE;
	return 0;
}

void ChatFrameBase::addStatusLine(const tstring& aStatus, LogMessage::Severity aSeverity, LogMessage::Type aType) {
	auto logMessage = std::make_shared<LogMessage>(Text::fromT(aStatus), aSeverity, aType, Util::emptyString);
	addStatusMessage(logMessage);
}

LRESULT ChatFrameBase::onResize(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
	resizePressed = !resizePressed;
	ctrlResize.SetIcon(resizePressed ? GET_ICON(IDI_EXPAND_DOWN, 16) : GET_ICON(IDI_EXPAND_UP, 16));

	//resize with the button even if user has set max lines for disabling the function otherwise.
	const int maxLines = SETTING(MAX_RESIZE_LINES) <= 1 ? 2 : SETTING(MAX_RESIZE_LINES);

	int newLineCount = resizePressed ? maxLines : 0;

	if(newLineCount != lineCount) {
		lineCount = newLineCount;
		UpdateLayout(FALSE);
	}

	bHandled = FALSE;
	return 0;
}

LRESULT ChatFrameBase::onDropFiles(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/){
	HDROP drop = (HDROP)wParam;
	tstring buf;
	buf.resize(MAX_PATH);

	UINT nrFiles;
	
	nrFiles = DragQueryFile(drop, (UINT)-1, NULL, 0);
	
	StringList paths;
	for(UINT i = 0; i < nrFiles; ++i){
		if(DragQueryFile(drop, i, &buf[0], MAX_PATH)){
			if(!PathIsDirectory(&buf[0])){
				paths.push_back(Text::fromT(buf).data());
			}
		}
	}
	DragFinish(drop);

	if (!paths.empty())
		addMagnet(paths);
	return 0;
}

LRESULT ChatFrameBase::onAddMagnet(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	if (!getClient())
		return 0;

	tstring file;

	BrowseDlg dlg(m_hWnd, BrowseDlg::TYPE_GENERIC, BrowseDlg::DIALOG_SELECT_FILE);
	if (dlg.show(file)) {
		addMagnet({ Text::fromT(file) });
	}

	return 0;
 }

void ChatFrameBase::addMagnet(const StringList& aPaths) {
	if(!getClient())
		return;

	//warn the user that nmdc share will be public access.
	if((getUser() && getUser()->isSet(User::NMDC))) {
		WinUtil::ShowMessageBox(SettingsManager::NMDC_MAGNET_WARN, CTSTRING(NMDC_MAGNET_WARNING));
	}

	setStatusText(aPaths.size() > 1 ? TSTRING_F(CREATING_MAGNET_FOR_X, aPaths.size()) : TSTRING_F(CREATING_MAGNET_FOR, Text::toT(aPaths.front())), LogMessage::SEV_INFO);
	tasks.run([=] {
		int64_t sizeLeft = 0;
		for(auto& path: aPaths)
			sizeLeft += File::getSize(path);

		tstring ret;
		int pos = 1;
		for (auto& path: aPaths) {
			TTHValue tth;
			auto size = File::getSize(path);
			try {
				HashManager::getInstance()->getFileTTH(path, size, true, tth, sizeLeft, cancelHashing, [=] (int64_t aTimeLeft, const string& aFileName) -> void {
					//update the statusbar
					if (aTimeLeft == 0)
						return;

					callAsync([=] {
						tstring status = TSTRING_F(HASHING_X_LEFT, Text::toT(aFileName) % Text::toT(Util::formatDuration(aTimeLeft, true)));
						if (aPaths.size() > 1) 
							status += _T(" (") + Text::toLower(TSTRING(FILE)) + _T(" ") + WinUtil::toStringW(pos) + _T("/") + WinUtil::toStringW(aPaths.size()) + _T(")");
						setStatusText(status, LogMessage::SEV_INFO);
					});
				});
			} catch (const Exception& e) {
				callAsync([=] { setStatusText(TSTRING_F(HASHING_FAILED_X, Text::toT(e.getError())), LogMessage::SEV_ERROR); });
				return;
			}

			if (cancelHashing) {
				return;
			}

			callAsync([=] {
				if (getClient()) {
					TempShareManager::getInstance()->addTempShare(tth, PathUtil::getFileName(path), path, size, getClient()->get(HubSettings::ShareProfile), ctrlClient.getTempShareUser());
				}
			});

			if (!ret.empty())
				ret += _T(" ");
			ret += Text::toT(ActionUtil::makeMagnet(tth, PathUtil::getFileName(path), size));
			pos++;
		}

		callAsync([=] {
			setStatusText(aPaths.size() > 1 ? TSTRING_F(MAGNET_CREATED_FOR_X, aPaths.size()) : TSTRING_F(MAGNET_CREATED_FOR, Text::toT(aPaths.front())), LogMessage::SEV_INFO);
			appendTextLine(ret, true);
		});
	});

}

void ChatFrameBase::setStatusText(const tstring& aLine, uint8_t sev) {
	ctrlStatus.SetText(0, WinUtil::formatMessageWithTimestamp(aLine).c_str(), SBT_NOTABPARSING);
	ctrlStatus.SetIcon(0, ResourceLoader::getSeverityIcon(sev));
}

void ChatFrameBase::setStatusText(const tstring& aLine, HICON aIcon) {
	ctrlStatus.SetText(0, WinUtil::formatMessageWithTimestamp(aLine).c_str(), SBT_NOTABPARSING);
	ctrlStatus.SetIcon(0, aIcon);
}


LRESULT ChatFrameBase::onWinampSpam(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring cmd, message, status;
	bool thirdPerson = false;
	if (SETTING(MEDIA_PLAYER) == 0) {
		cmd = _T("/winamp");
	} else if(SETTING(MEDIA_PLAYER) == 1) {
		cmd = _T("/itunes");
	} else if(SETTING(MEDIA_PLAYER) == 2) {
		cmd = _T("/mpc");
	} else if(SETTING(MEDIA_PLAYER) == 3) {
		cmd = _T("/wmp");
	} else if(SETTING(MEDIA_PLAYER) == 4) {
		cmd = _T("/spotify");
	} else {
		addStatusLine(CTSTRING(NO_MEDIA_SPAM), LogMessage::SEV_INFO, LogMessage::Type::SYSTEM);
		return 0;
	}
	if (checkCommand(cmd, message, status, thirdPerson)){
		if (!message.empty()) {
			sendFrameMessage(message, thirdPerson);
		}
		if (!status.empty()) {
			addStatusLine(status, LogMessage::SEV_INFO, LogMessage::Type::PRIVATE);
		}
	}
	return 0;
}

void ChatFrameBase::sendFrameMessage(const tstring& aMsg, bool aThirdPerson /*false*/) {
	if (!aMsg.empty()) {
		MainFrame::getMainFrame()->addThreadedTask([=] {
			string error;
			if (!sendMessageHooked(OutgoingChatMessage(Text::fromT(aMsg), this, WinUtil::ownerId, aThirdPerson), error) && !error.empty()) {
				callAsync([=] { 
					addStatusLine(Text::toT(error), LogMessage::SEV_ERROR, LogMessage::Type::SERVER); 
				});
			}
		});
	}
}

void ChatFrameBase::appendTextLine(const tstring& aText, bool addSpace) {
	tstring s;
	getLineText(s);
	int start, end;
	ctrlMessage.GetSel(start, end);
	tstring startSpace, endSpace = _T("");
	if (addSpace && s.length() > 0){
		if (start > 0 && !isspace(s[start - 1]))
			startSpace = _T(" ");
		if (start != static_cast<int>(s.length()) && !isspace(s[start]))
			endSpace = _T(" ");
	}

	ctrlMessage.InsertText(start, (startSpace + aText + endSpace).c_str());
	ctrlMessage.SetFocus();
	ctrlMessage.SetSel( ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength() );
}

LRESULT ChatFrameBase::onEmoticons(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled) {
	if (hWndCtl != ctrlEmoticons.m_hWnd) {
		bHandled = false;
		return 0;
	}

	EmoticonsDlg dlg;
	ctrlEmoticons.GetWindowRect(dlg.pos);
	dlg.DoModal(hWndCtl);
	if (!dlg.result.empty()) {
		appendTextLine(dlg.result, true);
	}
	return 0;
}

LRESULT ChatFrameBase::onEmoPackChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TCHAR buf[256];
	emoMenu.GetMenuString(wID, buf, 256, MF_BYCOMMAND);
	if (buf != Text::toT(SETTING(EMOTICONS_FILE))) {
		SettingsManager::getInstance()->set(SettingsManager::EMOTICONS_FILE, Text::fromT(buf));
		EmoticonsManager::getInstance()->Unload();
		EmoticonsManager::getInstance()->Load();
	}
	return 0;
}

LRESULT ChatFrameBase::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click
		
	if(reinterpret_cast<HWND>(wParam) == ctrlEmoticons) {
		if(emoMenu.m_hMenu) {
			emoMenu.DestroyMenu();
			emoMenu.m_hMenu = NULL;
		}
		emoMenu.CreatePopupMenu();
		menuItems = 0;
		emoMenu.InsertSeparatorFirst(TSTRING(EMOTICONS_PACK));
		emoMenu.AppendMenu(MF_STRING, IDC_EMOMENU, _T("Disabled"));
		if (SETTING(EMOTICONS_FILE)=="Disabled") emoMenu.CheckMenuItem( IDC_EMOMENU, MF_BYCOMMAND | MF_CHECKED );
		// nacteme seznam emoticon packu (vsechny *.xml v adresari EmoPacks)
		WIN32_FIND_DATA data;
		HANDLE hFind;
		hFind = FindFirstFile(Text::toT(WinUtil::getPath(WinUtil::PATH_EMOPACKS) + "*.xml").c_str(), &data);
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

		emoMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, ctrlClient.m_hWnd);
		bHandled = TRUE;
		return 1;
	}

	return 0; 
}

void ChatFrameBase::handleSendMessage() {
	tstring message;
	tstring status;
	tstring cmd;
	bool thirdPerson = false;
	bool isGuiCommand = false;

	if(ctrlMessage.GetWindowTextLength() > 0) {
		tstring s;
		getLineText(s);

		// save command in history, reset current buffer pointer to the newest command
		curCommandPosition = prevCommands.size();		//this places it one position beyond a legal subscript
		if (!curCommandPosition || curCommandPosition > 0 && prevCommands[curCommandPosition - 1] != s) {
			++curCommandPosition;
			prevCommands.push_back(s);
		}
		currentCommand = Util::emptyStringT;

		// Special command
		if (s[0] == _T('/')) {
			cmd = s;
			if (SETTING(CLIENT_COMMANDS)) {
				addStatusLine(_T("Client command: ") + s, LogMessage::SEV_INFO, LogMessage::Type::SYSTEM);
			}

			isGuiCommand = checkCommand(cmd, message, status, thirdPerson);
		} else {
			if (SETTING(SERVER_COMMANDS)) {
				if(s[0] == '!' || s[0] == '+' || s[0] == '-')
					status = _T("Server command: ") + s;
			}

			message = s;
		}
	} else {
		MessageBeep(MB_ICONEXCLAMATION);
		return;
	}

	//If status in chat is disabled the command result as status message wont display, so add it as private line.
	if (!status.empty()) {
		if (isGuiCommand) {
			addPrivateLine(status);
		} else {
			addStatusLine(status, LogMessage::SEV_INFO, LogMessage::Type::SYSTEM);
		}
	}

	if (!message.empty()) {
		sendFrameMessage(message, thirdPerson);
	}
	
	if (!cmd.empty() && (!isGuiCommand || Util::stricmp(cmd, _T("/help")) == 0)) {
		// Let the extensions handle it too
		sendFrameMessage(cmd, false);
	}

	ctrlMessage.SetWindowText(Util::emptyStringT.c_str());
}

static const TCHAR *msgs[] = { _T("\r\n-- I'm a happy AirDC++ user. You could be happy too.\r\n"),
_T("\r\n-- Is it Superman? No, it's AirDC++!\r\n"),
_T("\r\n-- My files are burning in my computer...download are way too fast!\r\n"),
_T("\r\n-- STOP!! My client is too fast, slow down with the writings!\r\n"),
_T("\r\n-- We are always behind the corner waiting to grab your nuts...I mean files!\r\n"),
_T("\r\n-- Knock, knock...we are here to take your files again, we needed backup files too!\r\n"),
_T("\r\n-- Why bother searching when AirDC++ can take care of everything?\r\n"),
_T("\r\n-- The only way to stop me from getting your files is to close DC++!\r\n"),
_T("\r\n-- We love your files soo much, so we try to get them over and over again...\r\n"),
_T("\r\n-- Let us thru the waiting line, we download faster than the lightning!\r\n"),
_T("\r\n-- Sometimes we download so fast that we accidentally get the whole person on the other side...\r\n"),
_T("\r\n-- Holy crap, my download speed is sooooo fast that it made a hole in the hard drive!\r\n"),
_T("\r\n-- Once you got access to it, don't let it go...\r\n"),
_T("\r\n-- Do you feel the wind? It's the download that goes too fast...\r\n"),
_T("\r\n-- No matter what, no matter where, it's always home if AirDC++ is there!\r\n"),
_T("\r\n-- Knock, knock...we are leaving back the trousers we accidentally downloaded...\r\n"),
_T("\r\n-- Are you blind? You have downloaded that movie 4 times already!\r\n"),
_T("\r\n-- My client has been in jail twice, has yours?\r\n"),
_T("\r\n-- Keep your downloads close, but keep your uploads even closer!\r\n")


};

#define MSGS 18

tstring ChatFrameBase::commands = Text::toT("\n\t\t\t\t\tHELP\n\
------------------------------------------------------------------------------------------------------------------------------------------------------------\n\
/refresh\t\t\t\t\tRefresh share\n\
/optimizedb\t\t\t\tRemove unused entries from the hash databases\n\
/verifydb\t\t\t\t\tOptimize and verify the integrity of the hash databases\n\
/stop\t\t\t\t\tStop SFV check\n\
/sharestats\t\t\t\tShow general share statistics (only visible to yourself)\n\
/dbstats\t\t\t\t\tShow techical statistics about the hash database backend (only visible to yourself)\n\
------------------------------------------------------------------------------------------------------------------------------------------------------------\n\
/search <string>\t\t\t\tSearch for...\n\
/whois [IP]\t\t\t\tFind info about user from the IP address\n\
------------------------------------------------------------------------------------------------------------------------------------------------------------\n\
/slots #\t\t\t\t\tUpload slots\n\
/extraslots #\t\t\t\tSet extra slots\n\
/smallfilesize #\t\t\t\tSet smallfile size\n\
/ts\t\t\t\t\tShow timestamp in mainchat\n\
/connection\t\t\t\tShow connection settings, IP & ports (only visible to yourself)\n\
/showjoins\t\t\t\tShow user joins in mainchat\n\
/shutdown\t\t\t\tSystem shutdown\n\
------------------------------------------------------------------------------------------------------------------------------------------------------------\n\
/AirDC++\t\t\t\t\tShow AirDC++ version in mainchat\n\
------------------------------------------------------------------------------------------------------------------------------------------------------------\n\
/away <msg>\t\t\t\tSet away message\n\
/winamp, /w\t\t\t\tShows Winamp spam in mainchat (as public message)\n\
/spotify, /s\t\t\t\tShows Spotify spam in mainchat (as public message)\n\
/itunes\t\t\t\t\tShows iTunes spam in mainchat (as public message)\n\
/wmp\t\t\t\t\tShows Windows Media Player spam in mainchat (as public message)\n\
/mpc\t\t\t\t\tShows Media Player Classic spam in mainchat (as public message)\n\
/clear,/c\t\t\t\t\tClears chat\n\
/speed\t\t\t\t\tShow download/upload speeds in mainchat (only visible to yourself)\n\
/stats\t\t\t\t\tShow stats in mainchat (as public message)\n\
/clientstats\t\t\t\tShow general statistics about the clients in all hubs (only visible to yourself)\n\
/prvstats\t\t\t\t\tView stats (only visible to yourself)\n\
/info\t\t\t\t\tView system info (only visible to yourself)\n\
/log system\t\t\t\tOpen system log\n\
/log downloads\t\t\t\tOpen downloads log\n\
/log uploads\t\t\t\tOpen uploads log\n\
/df\t\t\t\t\tShow disk space info (only visible to yourself)\n\
/dfs\t\t\t\t\tShow disk space info (as public message)\n\
/disks, /di\t\t\t\tShow detailed disk info about all mounted disks (only visible to yourself)\n\
/uptime\t\t\t\t\tShow uptime info (as public message)\n\
/topic\t\t\t\t\tShow topic\n\
/ctopic\t\t\t\t\tOpen link in topic\n\
/ratio, /r\t\t\t\t\tShow ratio in chat (as public message)\n\
/close\t\t\t\t\tClose the current tab\n\n");

string ChatFrameBase::getAwayMessage() {
	return ctrlClient.getClient() ? ctrlClient.getClient()->get(HubSettings::AwayMsg) : SETTING(DEFAULT_AWAY_MESSAGE);
}

CHARFORMAT2& ChatFrameBase::getStatusMessageStyle(const LogMessagePtr& aMessage) noexcept {
	switch (aMessage->getType()) {
	case dcpp::LogMessage::Type::SPAM:
	case dcpp::LogMessage::Type::SYSTEM:
		return WinUtil::m_ChatTextSystem;
	case dcpp::LogMessage::Type::PRIVATE:
		return WinUtil::m_ChatTextPrivate;
	case dcpp::LogMessage::Type::HISTORY:
		return WinUtil::m_ChatTextLog;
	case dcpp::LogMessage::Type::SERVER:
	default:
		return WinUtil::m_ChatTextServer;
	}
}

// An advanced function that will crash the client with the wanted recursion number for testing crash handling
// Generates nice, long lines in exception info
template<typename T, typename AllocT, typename Dummy1, typename Dummy2>
size_t crashWithRecursion(int aCurCount, int aMaxCount, Dummy1 d1, Dummy2 d2) {
	if (aCurCount == aMaxCount) {
		string* tmp = nullptr;
		return tmp->size();
	}

	return crashWithRecursion<T, AllocT, Dummy1, Dummy2>(aCurCount + 1, aMaxCount, d1, d2);
}

bool ChatFrameBase::checkCommand(const tstring& aCmd, tstring& message_, tstring& status_, bool& thirdPerson_) {
	auto cmd = aCmd;
	tstring param;

	{
		auto i = cmd.find(' ');
		if (i != string::npos) {
			param = cmd.substr(i + 1);
			cmd = cmd.substr(1, i - 1);
		} else {
			cmd = cmd.substr(1);
		}
	}

	if (stricmp(cmd.c_str(), _T("log")) == 0) {
		if (stricmp(param.c_str(), _T("system")) == 0) {
			ActionUtil::openFile(Text::toT(LogManager::getInstance()->getPath(LogManager::SYSTEM)));
		} else if (stricmp(param.c_str(), _T("downloads")) == 0) {
			ActionUtil::openFile(Text::toT(LogManager::getInstance()->getPath(LogManager::DOWNLOAD)));
		} else if (stricmp(param.c_str(), _T("uploads")) == 0) {
			ActionUtil::openFile(Text::toT(LogManager::getInstance()->getPath(LogManager::UPLOAD)));
		} else {
			return false;
		}
	} else if (stricmp(cmd.c_str(), _T("me")) == 0) {
		message_ = param;
		thirdPerson_ = true;
		return true;
	} else if ((stricmp(cmd.c_str(), _T("ratio")) == 0) || (stricmp(cmd.c_str(), _T("r")) == 0)) {
		char ratio[512];
		//thirdPerson = true;
		snprintf(ratio, sizeof(ratio), "Ratio: %s (Uploaded: %s | Downloaded: %s)",
			(SETTING(TOTAL_DOWNLOAD) > 0) ? Util::toString(((double)SETTING(TOTAL_UPLOAD)) / ((double)SETTING(TOTAL_DOWNLOAD))).c_str() : "inf.",
			Util::formatBytes(SETTING(TOTAL_UPLOAD)).c_str(), Util::formatBytes(SETTING(TOTAL_DOWNLOAD)).c_str());
		message_ = Text::toT(ratio);

		return true;
	} else if (stricmp(cmd.c_str(), _T("refresh")) == 0) {
		//refresh path
		try {
			if (!param.empty()) {
				if (stricmp(param.c_str(), _T("incoming")) == 0) {
					ShareManager::getInstance()->refresh(ShareRefreshType::REFRESH_INCOMING, ShareRefreshPriority::MANUAL);
				} else {
					auto refreshQueueInfo = ShareManager::getInstance()->refreshVirtualName(Text::fromT(param), ShareRefreshPriority::MANUAL);
					if (!refreshQueueInfo) {
						status_ = TSTRING(DIRECTORY_NOT_FOUND);
					}
				}
			} else {
				ShareManager::getInstance()->refresh(ShareRefreshType::REFRESH_ALL, ShareRefreshPriority::MANUAL);
			}
		} catch (const ShareException& e) {
			status_ = Text::toT(e.getError());
		}

		return true;
	} else if (stricmp(cmd.c_str(), _T("slots")) == 0) {
		int j = Util::toInt(Text::fromT(param));
		if (j > 0) {
			SettingsManager::getInstance()->set(SettingsManager::UPLOAD_SLOTS, j);
			status_ = TSTRING(SLOTS_SET);
			ClientManager::getInstance()->myInfoUpdated();
		} else {
			status_ = TSTRING(INVALID_NUMBER_OF_SLOTS);
		}
	} else if (stricmp(cmd.c_str(), _T("search")) == 0) {
		if (!param.empty()) {
			SearchFrame::openWindow(param);
		} else {
			status_ = TSTRING(SPECIFY_SEARCH_STRING);
		}
	} else if ((stricmp(cmd.c_str(), _T("airdc++")) == 0) || (stricmp(cmd.c_str(), _T("++")) == 0)) {
		message_ = msgs[GET_TICK() % MSGS] + Text::toT("-- " + Text::fromT(HttpLinks::homepage) + "  <" + shortVersionString + ">");
	} else if (stricmp(cmd.c_str(), _T("calcprio")) == 0) {
		QueueManager::getInstance()->calculateBundlePriorities(true);
	} else if (stricmp(cmd.c_str(), _T("generatelist")) == 0) {
		MainFrame::getMainFrame()->addThreadedTask([this] { ShareManager::getInstance()->generateOwnList(0); });
	} else if (stricmp(cmd.c_str(), _T("as")) == 0) {
		//AutoSearchManager::getInstance()->runSearches();
	} else if (stricmp(cmd.c_str(), _T("clientstats")) == 0) {
		status_ = Text::toT(ChatCommands::hubStats());
	} else if (stricmp(cmd.c_str(), _T("compact")) == 0) {
		MainFrame::getMainFrame()->addThreadedTask([this] { HashManager::getInstance()->compact(); });
	} else if (stricmp(cmd.c_str(), _T("setlistdirty")) == 0) {
		auto profiles = ShareManager::getInstance()->getProfiles();
		ProfileTokenSet pts;
		for (auto& sp : profiles)
			pts.insert(sp->getToken());

		ShareManager::getInstance()->getProfileMgr().setProfilesDirty(pts, true);
	} else if (stricmp(cmd.c_str(), _T("away")) == 0) {
		auto am = ActivityManager::getInstance();

		if (am->isAway()) {
			am->setAway(AWAY_OFF);
			status_ = TSTRING(AWAY_MODE_OFF);
		} else {
			am->setAway(AWAY_MANUAL);
			ParamMap sm;
			status_ = TSTRING(AWAY_MODE_ON) + _T(" ") + Text::toT(am->getAwayMessage(getAwayMessage(), sm));
		}
	} else if (stricmp(cmd.c_str(), _T("u")) == 0) {
		if (!param.empty()) {
			ActionUtil::openLink(Text::toT(LinkUtil::encodeURI(Text::fromT(param))));
		}
	} else if (stricmp(cmd.c_str(), _T("optimizedb")) == 0) {
		HashManager::getInstance()->startMaintenance(false);
	} else if (stricmp(cmd.c_str(), _T("verifydb")) == 0) {
		HashManager::getInstance()->startMaintenance(true);
	} else if (stricmp(cmd.c_str(), _T("shutdown")) == 0) {
		MainFrame::setShutDown(!(MainFrame::getShutDown()));
		if (MainFrame::getShutDown()) {
			status_ = TSTRING(SHUTDOWN_ON);
		} else {
			status_ = TSTRING(SHUTDOWN_OFF);
		}
	} else if ((stricmp(cmd.c_str(), _T("disks")) == 0) || (stricmp(cmd.c_str(), _T("di")) == 0)) {
		status_ = ChatCommands::diskInfo();
	} else if (stricmp(cmd.c_str(), _T("stats")) == 0) {
		message_ = Text::toT(ChatCommands::generateStats());
	} else if (stricmp(cmd.c_str(), _T("prvstats")) == 0) {
		status_ = Text::toT(ChatCommands::generateStats());
	} else if (stricmp(cmd.c_str(), _T("dbstats")) == 0) {
		status_ = _T("Collecing statistics, please wait... (this may take a few minutes with large databases)");
		tasks.run([this] {
			auto text = Text::toT(HashManager::getInstance()->getDbStats());
			callAsync([=] { addStatusLine(text, LogMessage::SEV_INFO, LogMessage::Type::SYSTEM); });
		});
	} else if (stricmp(cmd.c_str(), _T("sharestats")) == 0) {
		status_ = Text::toT(ChatCommands::shareStats());
	} else if (stricmp(cmd.c_str(), _T("speed")) == 0) {
		status_ = ChatCommands::Speedinfo();
	} else if (stricmp(cmd.c_str(), _T("info")) == 0) {
		status_ = ChatCommands::UselessInfo();
	} else if (stricmp(cmd.c_str(), _T("df")) == 0) {
		status_ = ChatCommands::DiskSpaceInfo();

	} else if (stricmp(cmd.c_str(), _T("version")) == 0) {
		status_ = Text::toT(ChatCommands::ClientVersionInfo());
	} else if (stricmp(cmd.c_str(), _T("dfs")) == 0) {
		message_ = ChatCommands::DiskSpaceInfo();
	} else if (stricmp(cmd.c_str(), _T("uptime")) == 0) {
		message_ = Text::toT(ChatCommands::uptimeInfo());
	} else if (stricmp(cmd.c_str(), _T("f")) == 0) {
		ctrlClient.findText();
	} else if (stricmp(cmd.c_str(), _T("whois")) == 0) {
		ctrlClient.handleSearchIP(Text::toT(LinkUtil::encodeURI(Text::fromT(param))));
	} else if ((stricmp(cmd.c_str(), _T("clear")) == 0) || (stricmp(cmd.c_str(), _T("cls")) == 0)) {
		ctrlClient.handleEditClearAll();
	} else if (Util::stricmp(cmd.c_str(), _T("conn")) == 0 || Util::stricmp(cmd.c_str(), _T("connection")) == 0) {
		status_ = Text::toT(ConnectivityManager::getInstance()->getInformation());
	} else if (stricmp(cmd.c_str(), _T("extraslots")) == 0) {
		int j = Util::toInt(Text::fromT(param));
		if (j > 0) {
			SettingsManager::getInstance()->set(SettingsManager::EXTRA_SLOTS, j);
			status_ = TSTRING(EXTRA_SLOTS_SET);
		} else {
			status_ = TSTRING(INVALID_NUMBER_OF_SLOTS);
		}
	} else if (stricmp(cmd.c_str(), _T("smallfilesize")) == 0) {
		int j = Util::toInt(Text::fromT(param));
		if (j >= 64) {
			SettingsManager::getInstance()->set(SettingsManager::SET_MINISLOT_SIZE, j);
			status_ = TSTRING(SMALL_FILE_SIZE_SET);
		} else {
			status_ = TSTRING(INVALID_SIZE);
		}
	} else if (Util::stricmp(cmd.c_str(), _T("upload")) == 0) {
		auto value = Util::toInt(Text::fromT(param));
		ThrottleManager::setSetting(ThrottleManager::getCurSetting(SettingsManager::MAX_UPLOAD_SPEED_MAIN), value);
		status_ = value ? CTSTRING_F(UPLOAD_LIMIT_SET_TO, value) : CTSTRING(UPLOAD_LIMIT_DISABLED);
	} else if (Util::stricmp(cmd.c_str(), _T("download")) == 0) {
		auto value = Util::toInt(Text::fromT(param));
		ThrottleManager::setSetting(ThrottleManager::getCurSetting(SettingsManager::MAX_DOWNLOAD_SPEED_MAIN), value);
		status_ = value ? CTSTRING_F(DOWNLOAD_LIMIT_SET_TO, value) : CTSTRING(DOWNLOAD_LIMIT_DISABLED);
	} else if (stricmp(cmd.c_str(), _T("wmp")) == 0) { // Mediaplayer Support
		string spam = Players::getWMPSpam(FindWindow(_T("WMPlayerApp"), NULL));
		if (!spam.empty()) {
			if (spam != "no_media") {
				message_ = Text::toT(spam);
			} else {
				status_ = _T("You have no media playing in Windows Media Player");
			}
		} else {
			status_ = _T("Supported version of Windows Media Player is not running");
		}

	} else if ((stricmp(cmd.c_str(), _T("spotify")) == 0) || (stricmp(cmd.c_str(), _T("s")) == 0)) {
		string spam = Players::getSpotifySpam(FindWindow(_T("SpotifyMainWindow"), NULL));
		if (!spam.empty()) {
			if (spam != "no_media") {
				message_ = Text::toT(spam);
			} else {
				status_ = _T("You have no media playing in Spotify");
			}
		} else {
			status_ = _T("Supported version of Spotify is not running");
		}

	} else if (stricmp(cmd.c_str(), _T("itunes")) == 0) {
		string spam = Players::getItunesSpam(FindWindow(_T("iTunes"), _T("iTunes")));
		if (!spam.empty()) {
			if (spam != "no_media") {
				message_ = Text::toT(spam);
			} else {
				status_ = _T("You have no media playing in iTunes");
			}
		} else {
			status_ = _T("Supported version of iTunes is not running");
		}
	} else if (stricmp(cmd.c_str(), _T("mpc")) == 0) {
		string spam = Players::getMPCSpam();
		if (!spam.empty()) {
			message_ = Text::toT(spam);
		} else {
			status_ = _T("Supported version of Media Player Classic is not running");
		}
	} else if ((stricmp(cmd.c_str(), _T("winamp")) == 0) || (stricmp(cmd.c_str(), _T("w")) == 0)) {
		string spam = Players::getWinAmpSpam();
		if (!spam.empty()) {
			message_ = Text::toT(spam);
		} else {
			status_ = _T("Supported version of Winamp is not running");
		}
	} else if (stricmp(cmd.c_str(), _T("crashtest")) == 0) {
		if (param == _T("terminate")) {
			terminate();
		}

		int recursionCount = Util::toInt(Text::fromT(param));
		crashWithRecursion<string, string::allocator_type, string, string>(0, recursionCount > 0 ? recursionCount : 30, "1", "2");
	} else if (stricmp(cmd.c_str(), _T("aspopnext")) == 0) {
		AutoSearchManager::getInstance()->maybePopSearchItem(GET_TICK(), true);
	} else {
		return checkFrameCommand(cmd, param, message_, status_, thirdPerson_);
	}

	//check if /me was added by the command
	auto i = message_.find(' ');
	if (i != string::npos && message_.substr(1, i - 1) == _T("me")) {
		message_ = message_.substr(i+1);
		thirdPerson_ = true;
	}

	return true;
}
}