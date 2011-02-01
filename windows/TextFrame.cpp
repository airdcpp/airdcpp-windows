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

#include "TextFrame.h"
#include "WinUtil.h"
#include "../client/File.h"
#include "../client/StringTokenizer.h"
#include "../client/LogManager.h"

//#define MAX_TEXT_LEN 32768

void TextFrame::openWindow(const tstring& aFileName, bool Openlog, bool History) {
	TextFrame* frame = new TextFrame(aFileName, Openlog, History );
	frame->CreateEx(WinUtil::mdiClient);
}

LRESULT TextFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	ctrlPad.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY, WS_EX_CLIENTEDGE, IDC_CLIENT);

	ctrlPad.SetAutoURLDetect(false);
	ctrlPad.SetEventMask(ctrlPad.GetEventMask() | ENM_LINK);
	ctrlPad.Subclass();
	ctrlPad.LimitText(0);
	ctrlPad.SetFont(WinUtil::font);
	ctrlPad.SetBackgroundColor(WinUtil::bgColor); 
	string tmp;
	try {
		File f(Text::fromT(file), File::READ, File::OPEN);
		
		if(history) {

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

		} else if(openlog) {
			//if openlog just add the whole text
			tmp = f.read();
			ctrlPad.SetWindowText(Text::toT(tmp).c_str());
		
		} else if(!openlog && !history) {
		
		tmp = f.read();
		Text::toUtf8(tmp);
		 //add the line endings in nfo
		string::size_type i = 0;
		while((i = tmp.find('\n', i)) != string::npos) {
			if(i == 0 || tmp[i-1] != '\r') {
				tmp.insert(i, 1, '\r');
				i++;
			}
			i++;
		}
		
		ctrlPad.AppendText(Identity(NULL, 0), _T("- "), _T(""), Text::toT(tmp) + _T('\n'), WinUtil::m_ChatTextGeneral, false);
		}
		
		
		SetWindowText(Text::toT(Util::getFileName(Text::fromT(file))).c_str());
		f.close();
	} catch(const FileException& e) {
		ctrlPad.SetWindowText(Text::toT(Util::getFileName(Text::fromT(file)) + ": " + e.getError()).c_str());
	}
	
	
	WinUtil::SetIcon(m_hWnd, _T("systemlog.ico"));
	
	bHandled = FALSE;
	return 1;
}


LRESULT TextFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
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

void TextFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw() {
	RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

/**
 * @file
 * $Id: TextFrame.cpp 292 2007-06-13 12:15:07Z bigmuscle $
 */
