/* 
 * Copyright (C) 2001-2019 Jacek Sieka, arnetheduck on gmail point com
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

#include <airdcpp/File.h>
#include <airdcpp/LogManager.h>
#include <airdcpp/StringTokenizer.h>
#include <airdcpp/ViewFileManager.h>

//#define MAX_TEXT_LEN 32768
string TextFrame::id = "TextFrame";
TextFrame::FrameMap TextFrame::frames;

string TextFrame::readFile(const string& aFilePath) noexcept {
	string text;
	try {
		//check the size
		auto aSize = File::getSize(aFilePath);
		if (aSize == 0) {
			throw FileException(STRING(CANT_OPEN_EMPTY_FILE));
		} else if (aSize > Util::convertSize(50, Util::MB)) {
			throw FileException(STRING_F(VIEWED_FILE_TOO_BIG, aFilePath % Util::formatBytes(aSize)));
		}

		File f(aFilePath, File::READ, File::OPEN);
		text = f.read();
	}
	catch (const FileException& e) {
		LogManager::getInstance()->message(aFilePath + ": " + e.getError().c_str(), LogMessage::SEV_NOTIFY);
	}
	return text;
}

void TextFrame::openFile(const string& aFilePath) {
	string aText = TextFrame::readFile(aFilePath);
	if (aText.empty())
		return;

	auto frm = new TextFrame(Util::getFileName(aFilePath), aText);
	frm->setNfo(Util::getFileExt(aFilePath) == ".nfo");
	frm->openWindow();
}

void TextFrame::openFile(const ViewFilePtr& aFile) {
	string aText = TextFrame::readFile(aFile->getPath());
	if (aText.empty())
		return;

	auto frm = new TextFrame(aFile->getFileName(), aText, aFile);
	frm->setNfo(Util::getFileExt(aFile->getPath()) == ".nfo");
	frm->openWindow();
}

void TextFrame::viewText(const string& aTitle, const string& aText, bool aFormatText, bool aUseEmo) {
	if (aText.empty())
		return;

	auto frm = new TextFrame(aTitle, aText);
	frm->setUseTextFormatting(aFormatText);
	frm->setUseEmoticons(aUseEmo);
	frm->openWindow();
}

void TextFrame::openWindow() {
	if (SETTING(POPUNDER_TEXT)) {
		WinUtil::hiddenCreateEx(this);
	} else {
		this->CreateEx(WinUtil::mdiClient);
	}
	frames.emplace(this->m_hWnd, this);
}

TextFrame::TextFrame(const string& aTitle, const string& aText, const ViewFilePtr& aFile/*nullptr*/) : title(Text::toT(aTitle)), text(aText) {

	viewFile = aFile;
	SettingsManager::getInstance()->addListener(this);

	if(viewFile)
		ViewFileManager::getInstance()->addListener(this);
}

void TextFrame::on(ViewFileManagerListener::FileClosed, const ViewFilePtr& aFile) noexcept {
	if (aFile != viewFile) {
		return;
	}

	PostMessage(WM_CLOSE, 0, 0);
}

void TextFrame::on(ViewFileManagerListener::FileRead, const ViewFilePtr& aFile) noexcept {
	if (aFile != viewFile) {
		return;
	}
}

LRESULT TextFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	ctrlPad.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY, WS_EX_CLIENTEDGE, IDC_CLIENT);

	ctrlPad.Subclass();
	ctrlPad.LimitText(0);

	if (getNfo()) {
		setViewModeNfo();
	} else {
		ctrlPad.SetFont(WinUtil::font);
		ctrlPad.SetBackgroundColor(WinUtil::bgColor);
		ctrlPad.SetDefaultCharFormat(WinUtil::m_ChatTextGeneral);
	}

	if (getUseTextFormatting()) {
		ctrlPad.setFormatPaths(true);
		ctrlPad.setFormatLinks(true);
		ctrlPad.setFormatReleases(true);
		tstring aText = Text::toT(text);
		ctrlPad.AppendText(aText, getUseEmoticons());
	} else {
		ctrlPad.SetWindowText(Text::toT(text).c_str());
	}

	if (!getAutoScroll()) {
		//set scroll position to top
		ctrlPad.setAutoScrollToEnd(false);
		ctrlPad.SetSel(0, 0);
	}

	SetWindowText(title.c_str());
	
	CRect rc(SETTING(TEXT_LEFT), SETTING(TEXT_TOP), SETTING(TEXT_RIGHT), SETTING(TEXT_BOTTOM));
	if(! (rc.top == 0 && rc.bottom == 0 && rc.left == 0 && rc.right == 0) )
		MoveWindow(rc, TRUE);
	
	WinUtil::SetIcon(m_hWnd, IDI_TEXT);
	
	bHandled = FALSE;
	return 1;
}

void TextFrame::setViewModeNfo() {

	text = Text::toDOS(text);
	text = Text::toUtf8(text);

	//edit text style, disable dwEffects, bold, italic etc. looks really bad with bold font.
	CHARFORMAT2 cf;
	cf.cbSize = 9;  //use fixed size for testing.
	cf.dwEffects = 0;
	cf.dwMask = CFM_BACKCOLOR | CFM_COLOR;
	cf.crBackColor = SETTING(BACKGROUND_COLOR);
	cf.crTextColor = SETTING(TEXT_COLOR);
	cf.bCharSet = OEM_CHARSET;

	//We need to disable auto font, otherwise it will mess up our new font.
	LRESULT lres = ::SendMessage(ctrlPad.m_hWnd, EM_GETLANGOPTIONS, 0, 0);
	lres &= ~IMF_AUTOFONT;
	::SendMessage(ctrlPad.m_hWnd, EM_SETLANGOPTIONS, 0, lres);

	ctrlPad.SetFont(WinUtil::OEMFont);
	//set the colors...
	ctrlPad.SetBackgroundColor(WinUtil::bgColor);
	ctrlPad.SetDefaultCharFormat(cf);

	setUseTextFormatting(true);
	setAutoScroll(false);

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

	if (viewFile) {
		ViewFileManager::getInstance()->removeFile(viewFile->getTTH());
		ViewFileManager::getInstance()->removeListener(this);
	}

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