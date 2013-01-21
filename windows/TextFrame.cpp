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

#include "TextFrame.h"
#include "WinUtil.h"
#include "../client/File.h"
#include "../client/StringTokenizer.h"

//#define MAX_TEXT_LEN 32768

void TextFrame::openWindow(const tstring& aFileName, Type aType) {
	TextFrame* frame = new TextFrame(aFileName, aType);
	if(SETTING(POPUNDER_TEXT) && aType == NORMAL)
		WinUtil::hiddenCreateEx(frame);
	else
		frame->CreateEx(WinUtil::mdiClient);
}

void TextFrame::openWindow(const tstring& aTitle, const tstring& aText, Type aType) {
	TextFrame* frame = new TextFrame(aTitle, aType, aText);
	//if(SETTING(POPUNDER_TEXT))
		WinUtil::hiddenCreateEx(frame);
	//else
	//	frame->CreateEx(WinUtil::mdiClient);
}

TextFrame::TextFrame(const tstring& fileName, Type aType, const tstring& aText /*empty*/) : file(fileName), textType(aType), text(aText) {
	SettingsManager::getInstance()->addListener(this);
}

LRESULT TextFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	ctrlPad.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY, WS_EX_CLIENTEDGE, IDC_CLIENT);

	ctrlPad.Subclass();
	ctrlPad.LimitText(0);

	string tmp;
	try {
		if(textType < NORMAL) {
			ctrlPad.SetFont(WinUtil::font);
			ctrlPad.SetBackgroundColor(WinUtil::bgColor); 
			ctrlPad.SetDefaultCharFormat(WinUtil::m_ChatTextGeneral);

			if (textType == HISTORY) {
				ctrlPad.setFormatLinks(true);
				ctrlPad.setFormatReleases(true);
				File f(Text::fromT(file), File::READ, File::OPEN);
				int64_t size = f.getSize();
				if(size > 64*1024) {
					f.setPos(size - 64*1024);
				}
			
				tmp = f.read(64*1024);
				StringList lines;
				lines = StringTokenizer<string>(tmp, "\r\n").getTokens();
				long totalLines = lines.size();
				int i = totalLines > (SETTING(LOG_LINES) +1) ? totalLines - SETTING(LOG_LINES) : 0;

				for(; i < totalLines; ++i){
					ctrlPad.AppendText(Identity(NULL, 0), _T("- "), _T(""), Text::toT(lines[i]) + _T('\n'), WinUtil::m_ChatTextGeneral, true);
				}
				SetWindowText(Text::toT(Util::getFileName(Text::fromT(file))).c_str());
			} else if (textType == LOG) {
				ctrlPad.setFormatPaths(true);
				ctrlPad.setFormatLinks(true);
				ctrlPad.setFormatReleases(true);
				File f(Text::fromT(file), File::READ, File::OPEN);
				//if openlog just add the whole text
				tmp = f.read();
				ctrlPad.SetWindowText(Text::toT(tmp).c_str());
				SetWindowText(Text::toT(Util::getFileName(Text::fromT(file))).c_str());
			} else if (textType == REPORT) {
				ctrlPad.SetRedraw(FALSE);
				ctrlPad.setFormatPaths(true);
				ctrlPad.SetWindowText(text.c_str());
				ctrlPad.FormatEmoticonsAndLinks(text, 0, false);

				ctrlPad.setAutoScrollToEnd(false);
				ctrlPad.SetSel(0, 0); //set scroll position to top

				SetWindowText(file.c_str());
				ctrlPad.SetRedraw(TRUE);
			}
		} else if(textType == NORMAL) {
			File f(Text::fromT(file), File::READ, File::OPEN);
			tmp = Text::toDOS(f.read());
			tmp = Text::toUtf8(tmp);

			tstring::size_type j = 0; 
			while((j = tmp.find("\r", j)) != string::npos)
				tmp.erase(j, 1);

			//edit text style, disable dwEffects, bold, italic etc. looks really bad with bold font.
			CHARFORMAT2 cf;
			cf.cbSize = 9;  //use fixed size for testing.
			cf.dwEffects = 0;
			cf.dwMask = CFM_BACKCOLOR | CFM_COLOR;
			cf.crBackColor = SETTING(BACKGROUND_COLOR);
			cf.crTextColor = SETTING(TEXT_COLOR);
			cf.bCharSet = OEM_CHARSET;

			//We need to disable autofont, otherwise it will mess up our new font.
			LRESULT lres = ::SendMessage(ctrlPad.m_hWnd, EM_GETLANGOPTIONS, 0, 0);
			lres &= ~IMF_AUTOFONT;
			::SendMessage(ctrlPad.m_hWnd, EM_SETLANGOPTIONS, 0, lres);
		
			ctrlPad.SetFont(WinUtil::OEMFont);
			//set the colors...
			ctrlPad.SetBackgroundColor(WinUtil::bgColor); 
			ctrlPad.SetDefaultCharFormat(cf);
		
			//ctrlPad.SetTextEx((LPCTSTR)tmp.c_str(), ST_SELECTION, CP_UTF8);
			tstring msg = Text::toT(tmp);
			ctrlPad.SetWindowText(msg.c_str());
			ctrlPad.setFormatLinks(true);
			ctrlPad.setFormatReleases(true);

			ctrlPad.FormatEmoticonsAndLinks(msg, 0, false);
			ctrlPad.setAutoScrollToEnd(false);
			ctrlPad.SetSel(0, 0); //set scroll position to top
			SetWindowText(Text::toT(Util::getFileName(Text::fromT(file))).c_str());
		}
		//f.close();
	} catch(const FileException& e) {
		ctrlPad.SetWindowText(Text::toT(Util::getFileName(Text::fromT(file)) + ": " + e.getError()).c_str());
	}
	
	CRect rc(SETTING(TEXT_LEFT), SETTING(TEXT_TOP), SETTING(TEXT_RIGHT), SETTING(TEXT_BOTTOM));
	if(! (rc.top == 0 && rc.bottom == 0 && rc.left == 0 && rc.right == 0) )
		MoveWindow(rc, TRUE);
	
	WinUtil::SetIcon(m_hWnd, IDI_TEXT);
	
	bHandled = FALSE;
	return 1;
}


LRESULT TextFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	CRect rc;
	if(!IsIconic()){
		//Get position of window
		GetWindowRect(&rc);
				
		//convert the position so it's relative to main window
		::ScreenToClient(GetParent(), &rc.TopLeft());
		::ScreenToClient(GetParent(), &rc.BottomRight());
				
		//save the position
		SettingsManager::getInstance()->set(SettingsManager::TEXT_BOTTOM, (rc.bottom > 0 ? rc.bottom : 0));
		SettingsManager::getInstance()->set(SettingsManager::TEXT_TOP, (rc.top > 0 ? rc.top : 0));
		SettingsManager::getInstance()->set(SettingsManager::TEXT_LEFT, (rc.left > 0 ? rc.left : 0));
		SettingsManager::getInstance()->set(SettingsManager::TEXT_RIGHT, (rc.right > 0 ? rc.right : 0));
	}
	SettingsManager::getInstance()->removeListener(this);
	bHandled = FALSE;
	return 0;
}

void TextFrame::UpdateLayout(BOOL /*bResizeBars*/ /* = TRUE */)
{
	CRect rc;

	GetClientRect(rc);
	
	rc.bottom -= 1;
	rc.top += 1;
	rc.left +=1;
	rc.right -=1;
	ctrlPad.MoveWindow(rc);
}

	
LRESULT TextFrame::onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	HWND hWnd = (HWND)lParam;
	HDC hDC = (HDC)wParam;
	if(hWnd == ctrlPad.m_hWnd) {
		::SetBkColor(hDC, WinUtil::bgColor);
		::SetTextColor(hDC, WinUtil::textColor);
		return (LRESULT)WinUtil::bgBrush;
	}
	bHandled = FALSE;
	return FALSE;
}
	
LRESULT TextFrame::OnFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ctrlPad.SetFocus();
	return 0;
}

void TextFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}