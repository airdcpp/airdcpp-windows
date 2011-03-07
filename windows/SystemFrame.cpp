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

#include "SystemFrame.h"
#include "WinUtil.h"
#include "TextFrame.h"
#include "../client/File.h"
#include "../client/LogManager.h"
#include "../client/ShareManager.h"

LRESULT SystemFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	ctrlPad.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL, WS_EX_CLIENTEDGE);
	
	ctrlPad.SetReadOnly(TRUE);
	ctrlPad.SetFont(WinUtil::font);
	ctrlPad.SetBackgroundColor(WinUtil::bgColor); 
	ctrlPad.SetDefaultCharFormat(WinUtil::m_ChatTextGeneral);
	ctrlPad.LimitText(0);
	ctrlClientContainer.SubclassWindow(ctrlPad.m_hWnd);

	//might miss some messages
	deque<pair<time_t, string> > oldMessages = LogManager::getInstance()->getLastLogs();
	LogManager::getInstance()->addListener(this);

	for(deque<pair<time_t, string> >::iterator i = oldMessages.begin(); i != oldMessages.end(); ++i) {
		addLine(i->first, Text::toT(i->second));
	}


	tabMenu = CreatePopupMenu();
	if(BOOLSETTING(LOG_SYSTEM)) {
		tabMenu.AppendMenu(MF_STRING, IDC_OPEN_SYSTEM_LOG, CTSTRING(OPEN_SYSTEM_LOG));
		tabMenu.AppendMenu(MF_SEPARATOR);
	}
	tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));
	
	SettingsManager::getInstance()->addListener(this);
	WinUtil::SetIcon(m_hWnd, _T("systemlog.ico"));
	bHandled = FALSE;
	return 1;
}

LRESULT SystemFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
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
		
		bHandled = WinUtil::parseDBLClick(x, start, end);
	}
	return 0;
}

LRESULT SystemFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	auto_ptr<pair<time_t, tstring> > msg((pair<time_t, tstring>*)wParam);
	
	addLine(msg->first, msg->second);
	setDirty();
	return 0;
}

void SystemFrame::addLine(time_t t, const tstring& msg) {
	//dont know if should go over 64kb
	if(ctrlPad.GetWindowTextLength() > (128*1024)) {
		ctrlPad.SetRedraw(FALSE);
		ctrlPad.SetSel(0, ctrlPad.LineIndex(ctrlPad.LineFromChar(2000)));
		ctrlPad.ReplaceSel(_T(""));
		ctrlPad.SetRedraw(TRUE);
	}

	ctrlPad.AppendText((Text::toT("\r\n[" + Util::getTimeStamp(t) + "] ") + msg).c_str());
	
}

LRESULT SystemFrame::onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 
	tabMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
	return TRUE;
}

LRESULT SystemFrame::onSystemLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
tstring filename = Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatTime(SETTING(LOG_FILE_SYSTEM), time(NULL))));
	if(Util::fileExists(Text::fromT(filename))){
			if(BOOLSETTING(OPEN_LOGS_INTERNAL) == false) {
			ShellExecute(NULL, NULL, filename.c_str(), NULL, NULL, SW_SHOWNORMAL);
		} else {
			TextFrame::openWindow(filename, true, false);
		}
	} else {
		MessageBox(CTSTRING(NO_LOG_FOR_HUB),CTSTRING(NO_LOG_FOR_HUB), MB_OK );	  
	} 
	
	return 0; 
}

void SystemFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw() {
    PostMessage(WM_REFRESH_SETTINGS);
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
		menu.AppendMenu(MF_STRING, IDC_SEARCH, CTSTRING(SEARCH));
		
		menu.AppendMenu(MF_SEPARATOR);
		menu.AppendMenu(MF_STRING, IDC_OPEN_FOLDER, CTSTRING(OPEN_FOLDER));

		menu.AppendMenu(MF_SEPARATOR);
		if(BOOLSETTING(LOG_SYSTEM)) {
		menu.AppendMenu(MF_STRING, IDC_OPEN_SYSTEM_LOG, CTSTRING(OPEN_SYSTEM_LOG));
		menu.AppendMenu(MF_SEPARATOR);
		}
		menu.AppendMenu(MF_STRING, ID_EDIT_SELECT_ALL, CTSTRING(SELECT_ALL));
		menu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CTSTRING(CLEAR));
	
	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

	return 0;

}
LRESULT SystemFrame::OnRButtonDown(POINT pt) {
	selLine.clear();
	selPath.clear();
	FileName.clear();

	selLine = LineFromPos(pt);
	selPath = getPath(selLine);
	FileName = getFile(selPath);
	
	return 0;
}
tstring SystemFrame::getFile(tstring path){
	tstring file = Util::emptyStringT;
	
	if(!path.empty()) {

		int end = path.length();
		
		file = path.substr(path.find_last_of(PATH_SEPARATOR) +1, end);
		
		if(file.empty()) //we only have a path, return the last dir if dont have a file?
			return Util::getLastDir(selPath);

		end = file.find_first_of(_T(".")) + 4; //shouldnt have double dots in filenames?
		
		if(end == tstring::npos)
			return Util::getLastDir(selPath); //take the last dir if dont have a file?


		file = file.substr(0, end);
	}

	return file;
}
tstring SystemFrame::getPath(tstring line) {
	
	tstring::size_type start = line.find_first_of(PATH_SEPARATOR); 
	tstring::size_type end = line.length();
	
	if(start == tstring::npos)   //no path, return
		return Util::emptyStringT;

	start = line.rfind(_T(" "), start) +1; //find the drive
	
	if(start == tstring::npos) //this shouldnt happen.
		return Util::emptyStringT;

		
	tstring path = line.substr(start, end);
	
	//LogManager::getInstance()->message(Text::fromT(path));
	
	/* Yes ofc regex match would be better, just need to know how to make a match :) 

			tstring path;
			try{
			
			boost::wregex reg;
			reg.assign(_T(^([a-zA-Z]\:|\\\\[^\/\\:*?"<>|]+\\[^\/\\:*?"<>|]+)(\\[^\/\\:*?"<>|]+)+(\.[^\/\\:*?"<>|]+)$));
			boost::match_results<tstring::const_iterator> result;
			
			if(boost::regex_search(line, result, reg, boost::match_default)) {
				path = path.substr(result.position(), result.length());
			}
			}catch(...) {
			
			}
			*/

	if(!path.empty())
		return path;
	else
		return Util::emptyStringT;
}

tstring SystemFrame::LineFromPos(const POINT& p) const {
	int iCharPos = ctrlPad.CharFromPos(p);
	int len = ctrlPad.LineLength(iCharPos);

	if(len < 3) {
		return Util::emptyStringT;
	}

	tstring tmp;
	tmp.resize(len);

	ctrlPad.GetLine(ctrlPad.LineFromChar(iCharPos), &tmp[0], len);

	return tmp;
}

LRESULT SystemFrame::onOpenFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring tmp = Util::getFilePath(selPath); //need to pick up the path here if we have a missing file, they dont exist :)
	if(Util::fileExists(Text::fromT(tmp)))
		WinUtil::openFolder(tmp);

	return 0;
}
LRESULT SystemFrame::onEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlPad.Copy();
	return 0;
}
LRESULT SystemFrame::onEditSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlPad.SetSelAll();
	return 0;
}

LRESULT SystemFrame::onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlPad.SetWindowText(_T(""));
	return 0;
}

LRESULT SystemFrame::onSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	

	CHARRANGE cr;
	ctrlPad.GetSel(cr);
	if(cr.cpMax != cr.cpMin) {
		TCHAR *buf = new TCHAR[cr.cpMax - cr.cpMin + 1];
		ctrlPad.GetSelText(buf);
		searchTerm = Util::replace(buf, _T("\r"), _T("\r\n"));
		delete[] buf;
		} else if(!FileName.empty()) {
			searchTerm = FileName;

	}

	WinUtil::search(searchTerm, 0, false);
	searchTerm = Util::emptyStringT;
	return 0;
}