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

#define MAX_TEXT_LEN 32768

void TextFrame::openWindow(const tstring& aFileName) {
	TextFrame* frame = new TextFrame(aFileName);
	frame->CreateEx(WinUtil::mdiClient);
}

LRESULT TextFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	ctrlPad.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY, WS_EX_CLIENTEDGE);

	ctrlPad.LimitText(0);
	ctrlPad.SetFont(WinUtil::font);
	string tmp;
	try {
		tmp = File(Text::fromT(file), File::READ, File::OPEN).read();
		tmp = Text::toUtf8(tmp);
		string::size_type i = 0;
		while((i = tmp.find('\n', i)) != string::npos) {
			if(i == 0 || tmp[i-1] != '\r') {
				tmp.insert(i, 1, '\r');
				i++;
			}
			i++;
		}
		ctrlPad.SetWindowText(Text::toT(tmp).c_str());
		ctrlPad.EmptyUndoBuffer();
		SetWindowText(Text::toT(Util::getFileName(Text::fromT(file))).c_str());
	} catch(const FileException& e) {
		SetWindowText(Text::toT(Util::getFileName(Text::fromT(file)) + ": " + e.getError()).c_str());
	}
	
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
