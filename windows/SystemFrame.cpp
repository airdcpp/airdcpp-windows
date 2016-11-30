/*
 * Copyright (C) 2001-2016 Jacek Sieka, arnetheduck on gmail point com
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

#include "SystemFrame.h"
#include "WinUtil.h"
#include "TextFrame.h"
#include "MainFrm.h"
#include "ResourceLoader.h"

#include <airdcpp/AirUtil.h>
#include <airdcpp/File.h>
#include <airdcpp/LogManager.h>
#include <airdcpp/ShareManager.h>

#include <airdcpp/version.h>
#include <airdcpp/format.h>
#include <airdcpp/modules/AutoSearchManager.h>

#define ICON_SIZE 16

LRESULT SystemFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	ctrlPad.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		WS_VSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY, WS_EX_CLIENTEDGE);

	ctrlPad.SetFont(WinUtil::font);
	ctrlPad.SetBackgroundColor(WinUtil::bgColor); 
	ctrlPad.SetDefaultCharFormat(WinUtil::m_ChatTextGeneral);
	ctrlPad.LimitText(96*1024); //now that we have icons we might want to limit even lower, the ram usage grows when many icons in view.
	ctrlClientContainer.SubclassWindow(ctrlPad.m_hWnd);
	textHeight = WinUtil::getTextHeight(ctrlPad.m_hWnd, WinUtil::font);
	
	if(!hbInfo)
		hbInfo = ResourceLoader::getBitmapFromIcon(IDI_INFO, WinUtil::bgColor,  ICON_SIZE, ICON_SIZE);
	if(!hbWarning)
		hbWarning = ResourceLoader::getBitmapFromIcon(IDI_IWARNING, WinUtil::bgColor,  ICON_SIZE, ICON_SIZE);
	if(!hbError)
		hbError = ResourceLoader::getBitmapFromIcon(IDI_IERROR, WinUtil::bgColor, ICON_SIZE, ICON_SIZE);

	reg.assign(_T("((?<=\\s)(([A-Za-z0-9]:)|(\\\\))(\\\\[^\\\\:]+)(\\\\([^\\s:])([^\\\\:])*)*((\\.[a-z0-9]{2,10})|(\\\\))(?=(\\s|$|:|,)))"));

	//might miss some messages
	auto oldMessages = LogManager::getInstance()->getCache().getLogMessages();
	LogManager::getInstance()->addListener(this);

	for (const auto& i: oldMessages) {
		addLine(i);
	}

	tabMenu = CreatePopupMenu();
	if(SETTING(LOG_SYSTEM)) {
		tabMenu.AppendMenu(MF_STRING, IDC_OPEN_SYSTEM_LOG, CTSTRING(OPEN_SYSTEM_LOG));
		tabMenu.AppendMenu(MF_SEPARATOR);
	}
	tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));

	CRect rc(SETTING(SYSLOG_LEFT), SETTING(SYSLOG_TOP), SETTING(SYSLOG_RIGHT), SETTING(SYSLOG_BOTTOM));
	if(! (rc.top == 0 && rc.bottom == 0 && rc.left == 0 && rc.right == 0) )
		MoveWindow(rc, TRUE);

	SettingsManager::getInstance()->addListener(this);
	WinUtil::SetIcon(m_hWnd, IDI_LOGS);
	bHandled = FALSE;
	return 1;
}

LRESULT SystemFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	CRect rc;
	if(!IsIconic()){
		//Get position of window
		GetWindowRect(&rc);
				
		//convert the position so it's relative to main window
		::ScreenToClient(GetParent(), &rc.TopLeft());
		::ScreenToClient(GetParent(), &rc.BottomRight());
				
		//save the position
		SettingsManager::getInstance()->set(SettingsManager::SYSLOG_BOTTOM, (rc.bottom > 0 ? rc.bottom : 0));
		SettingsManager::getInstance()->set(SettingsManager::SYSLOG_TOP, (rc.top > 0 ? rc.top : 0));
		SettingsManager::getInstance()->set(SettingsManager::SYSLOG_LEFT, (rc.left > 0 ? rc.left : 0));
		SettingsManager::getInstance()->set(SettingsManager::SYSLOG_RIGHT, (rc.right > 0 ? rc.right : 0));
	}

	ctrlPad.SetWindowText(_T(""));

	LogManager::getInstance()->removeListener(this);
	SettingsManager::getInstance()->removeListener(this);
	bHandled = FALSE;
	WinUtil::setButtonPressed(IDC_SYSTEM_LOG, false);
	return 0;
	
}

void SystemFrame::UpdateLayout(BOOL /*bResizeBars*/ /* = TRUE */)
{
	CRect rc;

	GetClientRect(rc);
	
	rc.bottom -= 1;
	rc.top += 1;
	rc.left +=1;
	rc.right -=1;
	ctrlPad.MoveWindow(rc);
	
}

LRESULT SystemFrame::onLButton(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
	HWND focus = GetFocus();
	bHandled = false;
	if(focus == ctrlPad.m_hWnd) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		tstring x;
		tstring::size_type start = (tstring::size_type)WinUtil::textUnderCursor(pt, ctrlPad, x);
		tstring::size_type end = x.find(_T(" "), start);

		if(end == tstring::npos)
			end = x.length();
		
		bHandled = WinUtil::parseDBLClick(x.substr(start, end-start));
	}
	return 0;
}

LRESULT SystemFrame::onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	TaskQueue::List tl;
	messages.get(tl);

	for(auto& t: tl) {
		if(t.first == ADD_LINE) {
			MessageTask& msg = static_cast<MessageTask&>(*t.second);
			addLine(msg.data);
		}
	}
	setDirty();
	return 0;
}

void SystemFrame::addLine(const LogMessagePtr& aMessageData) {
	ctrlPad.SetRedraw(FALSE);
	
	POINT pt = { 0 };
	bool scroll = !lButtonDown && scrollIsAtEnd();
	ctrlPad.GetScrollPos(&pt);
	int curPos = ctrlPad.CharFromPos(pt); //current scroll position by character pos

	LONG SavedBegin, SavedEnd;
	LONG Begin = 0; 
	LONG End = 0;

	ctrlPad.GetSel(SavedBegin, SavedEnd);

	End = Begin = ctrlPad.GetTextLengthEx(GTL_NUMCHARS);


	tstring Text = Text::toT(aMessageData->getText()) + _T(" \r\n");
	tstring time = Text::toT(" [" + Util::getTimeStamp(aMessageData->getTime()) + "] ");
	tstring line = time + Text;

	LONG limitText = ctrlPad.GetLimitText();
	LONG TextLength = End + line.size();

	if((TextLength +1) > limitText) {
		dcdebug("textlength %s \n", Util::toString(TextLength).c_str());
		LONG RemoveEnd = 0;
		RemoveEnd = ctrlPad.LineIndex(ctrlPad.LineFromChar(2000));
		End = Begin -=RemoveEnd;
		SavedBegin -= RemoveEnd;
		SavedEnd -= RemoveEnd;

		//fix the scroll position if text was removed from the start
		POINT p = { 0 };
		curPos -= RemoveEnd;
		p = ctrlPad.PosFromChar(curPos);
		pt.y = p.y;

		ctrlPad.SetSel(0, RemoveEnd);
		ctrlPad.ReplaceSel(_T(""));
	}

	ctrlPad.AppendText(line.c_str());
	
	End += time.size() -1;
	ctrlPad.SetSel(Begin, End);
	ctrlPad.SetSelectionCharFormat(WinUtil::m_TextStyleTimestamp);

	if (aMessageData->getSeverity() == LogMessage::SEV_ERROR) {
		ctrlPad.SetSel(End, End+Text.length()-1);
		CHARFORMAT2 ec = WinUtil::m_ChatTextGeneral;
		ec.crTextColor = SETTING(ERROR_COLOR);
		ctrlPad.SetSelectionCharFormat(ec);
	}

	Colorize(Text, End+1); //timestamps should always be timestamps right?

	ctrlPad.SetSel(Begin, Begin);

	switch(aMessageData->getSeverity()) {

	case LogMessage::SEV_INFO:
		CImageDataObject::InsertBitmap(ctrlPad.GetOleInterface(),hbInfo, false);
		break;
	case LogMessage::SEV_WARNING:
		CImageDataObject::InsertBitmap(ctrlPad.GetOleInterface(), hbWarning, false);
		break;
	case LogMessage::SEV_ERROR:
		CImageDataObject::InsertBitmap(ctrlPad.GetOleInterface(), hbError, false);
		if(!errorNotified && !getActive()) { 
			setIcon(ResourceLoader::getSeverityIcon(LogMessage::SEV_ERROR));
			errorNotified = true;
		}
		break;
	default:
		break;
	}
	
	ctrlPad.SetSel(SavedBegin, SavedEnd); //restore the user selection

	if(scroll/* && (SavedBegin == SavedEnd)*/) { 
		scrollToEnd();
	} else {
		ctrlPad.SetScrollPos(&pt);
	}
	
	ctrlPad.SetRedraw(TRUE);
	ctrlPad.InvalidateRect(NULL);
}

void SystemFrame::Colorize(const tstring& line, LONG Begin){

	tstring::const_iterator start = line.begin();
	tstring::const_iterator end = line.end();
	boost::match_results<tstring::const_iterator> result;
	int pos=0;

	while(boost::regex_search(start, end, result, reg, boost::match_default)) {
		ctrlPad.SetSel(pos + Begin + result.position(), pos + Begin + result.position() + result.length());
		ctrlPad.SetSelectionCharFormat(WinUtil::m_ChatTextServer);
		start = result[0].second;
		pos=pos+result.position() + result.length();
	}
}

void SystemFrame::scrollToEnd() {
	ctrlPad.PostMessage(EM_SCROLL, SB_BOTTOM, 0);
}

bool SystemFrame::scrollIsAtEnd() {
	SCROLLINFO si = { 0 };
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	ctrlPad.GetScrollInfo(SB_VERT, &si);
	const int tmp = si.nMax - (int)si.nPage - textHeight;
	return si.nPage == 0 || (si.nPos >= tmp) && (si.nTrackPos >= tmp);
}


LRESULT SystemFrame::onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 
	tabMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
	return TRUE;
	
}

LRESULT SystemFrame::onSystemLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	string filename = LogManager::getInstance()->getPath(LogManager::SYSTEM);
	if(Util::fileExists(filename)){
		WinUtil::viewLog(filename);
	} else {
		WinUtil::showMessageBox(TSTRING(NO_LOG_EXISTS));
	}
	
	return 0; 
}

void SystemFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
    PostMessage(WM_REFRESH_SETTINGS);
}

void SystemFrame::on(Message, const LogMessagePtr& aMessageData) noexcept {
	if (aMessageData->getSeverity() == LogMessage::SEV_NOTIFY) {
		return;
	}

	speak(ADD_LINE, aMessageData);
}

void SystemFrame::on(LogManagerListener::MessagesRead) noexcept {

}

LRESULT SystemFrame::onRefreshSettings(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	return 0;
}

LRESULT SystemFrame::onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click

	if(pt.x == -1 && pt.y == -1) {
		CRect erc;
		ctrlPad.GetRect(&erc);
		pt.x = erc.Width() / 2;
		pt.y = erc.Height() / 2;
		ClientToScreen(&pt);
	}

	POINT ptCl = pt;
	ScreenToClient(&ptCl); 
	OnRButtonDown(ptCl); 

	OMenu menu;
	menu.CreatePopupMenu();

	menu.AppendMenu(MF_STRING, ID_EDIT_COPY, CTSTRING(COPY));

	menu.AppendMenu(MF_SEPARATOR);
	if(SETTING(LOG_SYSTEM)) {
		menu.AppendMenu(MF_STRING, IDC_OPEN_SYSTEM_LOG, CTSTRING(OPEN_SYSTEM_LOG));
		menu.AppendMenu(MF_SEPARATOR);
	}
	menu.AppendMenu(MF_STRING, ID_EDIT_SELECT_ALL, CTSTRING(SELECT_ALL));
	menu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CTSTRING(CLEAR_MESSAGES));

	if (!selWord.empty()) {
		menu.AppendMenu(MF_STRING, IDC_COPY_DIR, CTSTRING(COPY_DIRECTORY));
		menu.AppendMenu(MF_SEPARATOR);
		menu.AppendMenu(MF_STRING, IDC_SEARCHDIR, CTSTRING(SEARCH_DIRECTORY));
		menu.AppendMenu(MF_STRING, IDC_ADD_AUTO_SEARCH_DIR, CTSTRING(ADD_AUTO_SEARCH_DIR));
		if (selWord[selWord.length() - 1] != PATH_SEPARATOR) {
			menu.AppendMenu(MF_STRING, IDC_SEARCH, CTSTRING(SEARCH_FILENAME));
			auto path = Text::fromT(selWord);
			if (Util::fileExists(path)) {
				menu.AppendMenu(MF_STRING, IDC_DELETE_FILE, CTSTRING(DELETE_FILE));
			} else {
				menu.AppendMenu(MF_STRING, IDC_ADD_AUTO_SEARCH_FILE, CTSTRING(ADD_AUTO_SEARCH_FILE));
				menu.AppendMenu(MF_SEPARATOR);
			}
		}

		menu.AppendMenu(MF_STRING, IDC_OPEN_FOLDER, CTSTRING(OPEN_FOLDER));
	}
	
	menu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
	return 0;

}
LRESULT SystemFrame::OnRButtonDown(POINT pt) {
	selWord.clear();

	selWord = WordFromPos(pt);
	return 0;
}

tstring SystemFrame::WordFromPos(const POINT& p) {

	int iCharPos = ctrlPad.CharFromPos(p);
	int line = ctrlPad.LineFromChar(iCharPos);
	int len = ctrlPad.LineLength(iCharPos);
	if(len < 3)
		return Util::emptyStringT;

	int Begin = ctrlPad.LineIndex(line);
	int c = iCharPos - Begin;

	tstring x;
	x.resize(len+1);
	x.resize(ctrlPad.GetLine(line, &x[0], len+1));

	tstring::const_iterator start = x.begin();
	tstring::const_iterator end = x.end();
	boost::match_results<tstring::const_iterator> result;
	int pos=0;

	while(boost::regex_search(start, end, result, reg, boost::match_default)) {
		if (pos + result.position() <= c && pos + result.position() + result.length() >= c) {
			ctrlPad.SetSel(pos + Begin + result.position(), pos + Begin + result.position() + result.length());
			return x.substr(pos + result.position(), result.length());
		}
		start = result[0].second;
		pos=pos+result.position() + result.length();
	}

	/* No path found, try to get the current selection */
	tstring ret;
	CHARRANGE cr;
	ctrlPad.GetSel(cr);
	if(cr.cpMax != cr.cpMin) {
		TCHAR *buf = new TCHAR[cr.cpMax - cr.cpMin + 1];
		ctrlPad.GetSelText(buf);
		ret = Util::replaceT(buf, _T("\r"), _T("\r\n"));
		delete[] buf;
	}

	return ret;
}

LRESULT SystemFrame::onSize(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if(wParam != SIZE_MINIMIZED) { 
		if((HIWORD(lParam) > 0)/* && scrollIsAtEnd()*/) 
			scrollToEnd();

		if(errorNotified) {
			setIcon(GET_ICON(IDI_LOGS, 16));
			errorNotified = false;
		}
	} 

	bHandled = FALSE;
	return 0;
}

LRESULT SystemFrame::onOpenFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring tmp = Util::getFilePath(selWord); //need to pick up the path here if we have a missing file, they dont exist :)
	if(Util::fileExists(Text::fromT(tmp)))
		WinUtil::openFolder(tmp);

	ctrlPad.SetSelNone();
	return 0;
}

LRESULT SystemFrame::onDeleteFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	string path = Text::fromT(selWord);
	string msg = STRING_F(DELETE_FILE_CONFIRM, path);
	if(WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_FILE_DELETIONS, Text::toT(msg).c_str())) {
		MainFrame::getMainFrame()->addThreadedTask([=] { 
			if (File::deleteFileEx(path, 3))
				ShareManager::getInstance()->removeTempShare(path); 
		});
	}

	ctrlPad.SetSelNone();
	return 0;
}

LRESULT SystemFrame::onAddAutoSearchFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	string targetPath;
	if (selWord.find(PATH_SEPARATOR) != tstring::npos)
		targetPath = Util::getFilePath(Text::fromT(selWord));
	auto fileName = Util::getFileName(Text::fromT(selWord));

	AutoSearchManager::getInstance()->addAutoSearch(fileName, targetPath, false, AutoSearch::CHAT_DOWNLOAD);

	ctrlPad.SetSelNone();
	return 0;
}

LRESULT SystemFrame::onAddAutoSearchDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto targetPath = Util::getParentDir(Text::fromT(selWord), PATH_SEPARATOR, true);
	auto dirName = Util::getLastDir(selWord[selWord.length() - 1] != PATH_SEPARATOR ? Util::getFilePath(Text::fromT(selWord)) : Text::fromT(selWord));

	AutoSearchManager::getInstance()->addAutoSearch(dirName, targetPath, true, AutoSearch::CHAT_DOWNLOAD, true);

	ctrlPad.SetSelNone();
	return 0;
}

LRESULT SystemFrame::onEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlPad.Copy();
	return 0;
}

LRESULT SystemFrame::onCopyDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	WinUtil::setClipboard(Text::toT(AirUtil::getReleaseDirLocal(Text::fromT(selWord), true)));
	return 0;
}

LRESULT SystemFrame::onEditSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlPad.SetSelAll();
	return 0;
}

LRESULT SystemFrame::onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	LogManager::getInstance()->clearCache();
	ctrlPad.SetWindowText(_T(""));
	return 0;
}

LRESULT SystemFrame::onSearchFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	WinUtil::search(Util::getFileName(selWord));
	return 0;
}

LRESULT SystemFrame::onSearchDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	WinUtil::search(Text::toT(AirUtil::getReleaseDirLocal(Text::fromT(selWord), true)), true);
	return 0;
}