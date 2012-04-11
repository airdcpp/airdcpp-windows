/*
 * Copyright (C) 2012-2013 AirDC++ 
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

#include "NfoViewer.h"
#include "WinUtil.h"
#include "../client/File.h"

void NfoViewer::openWindow(const tstring& aFileName) {
	NfoViewer* frame = new NfoViewer(aFileName);
	frame->CreateEx(WinUtil::mdiClient);
}

LRESULT NfoViewer::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	ctrlNfo.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY, WS_EX_CLIENTEDGE, IDC_NFO);

	ctrlNfo.SetAutoURLDetect(true);
	ctrlNfo.SetEventMask(ctrlNfo.GetEventMask() | ENM_LINK);
	ctrlNfo.LimitText(0);

	string tmp;
	try {

		File f(Text::fromT(file), File::READ, File::OPEN);
		
		tmp = Text::toDOS(f.read());
		tmp = Text::toUtf8(tmp);

		//add the line endings in nfo
		string::size_type i = 0;
		while((i = tmp.find('\n', i)) != string::npos) {
			if(i == 0 || tmp[i-1] != '\r') {
				tmp.insert(i, 1, '\r');
				i++;
			}
			i++;
		}

		//edit text style, disable dwEffects, bold, italic etc. looks really bad with bold font.
		CHARFORMAT2 cf;
		cf.cbSize = 9;  //use fixed size for testing.
		cf.dwEffects = 0;
		cf.dwMask = CFM_BACKCOLOR | CFM_COLOR;
		cf.crBackColor = SETTING(BACKGROUND_COLOR);
		cf.crTextColor = SETTING(TEXT_COLOR);
		cf.bCharSet = OEM_CHARSET;

		//We need to disable autofont, otherwise it will mess up our new font.
		LRESULT lres = ::SendMessage(ctrlNfo.m_hWnd, EM_GETLANGOPTIONS, 0, 0);
		lres &= ~IMF_AUTOFONT;
		::SendMessage(ctrlNfo.m_hWnd, EM_SETLANGOPTIONS, 0, lres);
		
		ctrlNfo.SetFont(WinUtil::OEMFont);
		//set the colors...
		ctrlNfo.SetBackgroundColor(WinUtil::bgColor); 
		ctrlNfo.SetDefaultCharFormat(cf);
		
		ctrlNfo.SetWindowText(Text::toT(tmp).c_str()); 
		
		SetWindowText(Text::toT(Util::getFileName(Text::fromT(file))).c_str());
		f.close();
	} catch(const FileException& e) {
		ctrlNfo.SetWindowText(Text::toT(Util::getFileName(Text::fromT(file)) + ": " + e.getError()).c_str());
	}
	//todo own positions?
	CRect rc(SETTING(TEXT_LEFT), SETTING(TEXT_TOP), SETTING(TEXT_RIGHT), SETTING(TEXT_BOTTOM));
	if(! (rc.top == 0 && rc.bottom == 0 && rc.left == 0 && rc.right == 0) )
		MoveWindow(rc, TRUE);
	
	WinUtil::SetIcon(m_hWnd, _T("systemlog.ico"));
	
	bHandled = FALSE;
	return 1;
}

LRESULT NfoViewer::onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	SelectedWord.clear();

	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click

	if(pt.x == -1 && pt.y == -1) {
		CRect erc;
		ctrlNfo.GetRect(&erc);
		pt.x = erc.Width() / 2;
		pt.y = erc.Height() / 2;
		ClientToScreen(&pt);
	}

	POINT ptCl = pt;
	ctrlNfo.ScreenToClient(&ptCl); 
		
	tstring x;
	tstring::size_type start = (tstring::size_type)WinUtil::textUnderCursor(ptCl, ctrlNfo, x);
	tstring::size_type end = x.find(_T(" "), start);

	if(end == tstring::npos)
		end = x.length();
		
	SelectedWord = x.substr(start, end-start);

	CMenu menu;
	menu.CreatePopupMenu();
	menu.AppendMenu(MF_STRING, ID_EDIT_COPY, CTSTRING(COPY));
	menu.AppendMenu(MF_STRING, IDC_SEARCH, CTSTRING(SEARCH));
	
	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

	return 0;

}

LRESULT NfoViewer::onEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CHARRANGE cr;
	ctrlNfo.GetSel(cr);
	if(cr.cpMax != cr.cpMin)
		ctrlNfo.Copy();
	else
		WinUtil::setClipboard(SelectedWord);
	
	return 0;
}
LRESULT NfoViewer::onSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CHARRANGE cr;
	ctrlNfo.GetSel(cr);
	if(cr.cpMax != cr.cpMin) {
		tstring tmp;
		TCHAR *buf = new TCHAR[cr.cpMax - cr.cpMin + 1];
		ctrlNfo.GetSelText(buf);
		tmp = Util::replace(buf, _T("\r"), _T("\r\n"));
		delete[] buf;
		WinUtil::searchAny(tmp);
	} else {
		WinUtil::searchAny(SelectedWord);
	}
	return 0;
}

LRESULT NfoViewer::onClientEnLink(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	ENLINK* enlink = (ENLINK*)pnmh;
	switch(enlink->msg) {
		case WM_LBUTTONUP:
		{
			tstring url;
			url.resize((enlink->chrg.cpMax - enlink->chrg.cpMin) + 1);
			ctrlNfo.GetTextRange(enlink->chrg.cpMin, enlink->chrg.cpMax, &url[0]);
			WinUtil::openLink(url);
			break;
		}
		case WM_RBUTTONUP:
		{
			ctrlNfo.SetSel(enlink->chrg.cpMin, enlink->chrg.cpMax);
			InvalidateRect(NULL);
			break;
		}
	}

	return 0;
}

LRESULT NfoViewer::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	CRect rc;
	if(!IsIconic()){
		//Get position of window
		GetWindowRect(&rc);
				
		//convert the position so it's relative to main window
		::ScreenToClient(GetParent(), &rc.TopLeft());
		::ScreenToClient(GetParent(), &rc.BottomRight());
				
		//save the position, todo own positions?
		SettingsManager::getInstance()->set(SettingsManager::TEXT_BOTTOM, (rc.bottom > 0 ? rc.bottom : 0));
		SettingsManager::getInstance()->set(SettingsManager::TEXT_TOP, (rc.top > 0 ? rc.top : 0));
		SettingsManager::getInstance()->set(SettingsManager::TEXT_LEFT, (rc.left > 0 ? rc.left : 0));
		SettingsManager::getInstance()->set(SettingsManager::TEXT_RIGHT, (rc.right > 0 ? rc.right : 0));
	}
	SettingsManager::getInstance()->removeListener(this);
	bHandled = FALSE;
	return 0;
}

void NfoViewer::UpdateLayout(BOOL /*bResizeBars*/ /* = TRUE */)
{
	CRect rc;
	GetClientRect(rc);
	
	rc.bottom -= 1;
	rc.top += 1;
	rc.left +=1;
	rc.right -=1;
	ctrlNfo.MoveWindow(rc);
}

void NfoViewer::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

