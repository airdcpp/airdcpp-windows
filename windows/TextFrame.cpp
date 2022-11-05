/* 
 * Copyright (C) 2001-2022 Jacek Sieka, arnetheduck on gmail point com
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


string TextFrame::id = "TextFrame";
TextFrame::FrameMap TextFrame::frames;

#define MAX_FORMAT_TEXT_LEN 32768
#define MAX_SIZE_MB 50

string TextFrame::readFile(const string& aFilePath) noexcept {
	string text;
	try {
		//check the size
		auto aSize = File::getSize(aFilePath);
		if (aSize == 0) {
			throw FileException(STRING(CANT_OPEN_EMPTY_FILE));
		} else if (aSize > Util::convertSize(MAX_SIZE_MB, Util::MB)) {
			throw FileException(STRING_F(VIEWED_FILE_TOO_BIG, aFilePath % Util::formatBytes(aSize)));
		}

		File f(aFilePath, File::READ, File::OPEN);
		text = f.read();
	} catch (const FileException& e) {
		ViewFileManager::log(aFilePath + ": " + e.getError().c_str(), LogMessage::SEV_NOTIFY);
	}

	return text;
}

TextFrame::FileType TextFrame::parseFileType(const string& aName) noexcept {
	auto ext = Util::getFileExt(aName);
	if (ext == ".nfo") {
		return FileType::NFO;
	} else if (ext == ".log") {
		return FileType::LOG;
	}

	return FileType::OTHER;
}

void TextFrame::openFile(const string& aFilePath) {
	auto text = TextFrame::readFile(aFilePath);
	viewText(Util::getFileName(aFilePath), text, parseFileType(aFilePath), nullptr);
}

void TextFrame::openFile(const ViewFilePtr& aFile) {
	auto text = TextFrame::readFile(aFile->getPath());
	viewText(aFile->getFileName(), text, parseFileType(aFile->getPath()), aFile);
}

TextFrame* TextFrame::viewText(const string& aTitle, const string& aText, FileType aType, const ViewFilePtr& aViewFile) {
	if (aText.empty()) {
		return nullptr;
	}

	auto frm = new TextFrame(aTitle, aText, aType, aViewFile);
	frm->openWindow();
	return frm;
}

void TextFrame::openWindow() {
	if (SETTING(POPUNDER_TEXT)) {
		WinUtil::hiddenCreateEx(this);
	} else {
		this->CreateEx(WinUtil::mdiClient);
	}
	frames.emplace(this->m_hWnd, this);
}

TextFrame::TextFrame(const string& aTitle, const string& aText, FileType aType, const ViewFilePtr& aFile/*nullptr*/) : title(Text::toT(aTitle)), text(aText), viewFile(aFile), type(aType) {
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

bool TextFrame::useTextFormatting() const noexcept {
	return text.length() < MAX_FORMAT_TEXT_LEN;
}

bool TextFrame::useEmoticons() const noexcept {
	return type != FileType::NFO;
}

LRESULT TextFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	ctrlPad.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY, WS_EX_CLIENTEDGE, IDC_CLIENT);

	ctrlPad.Subclass();
	ctrlPad.LimitText(0);

	if (type == FileType::NFO) {
		setViewModeNfo();
	} else {
		ctrlPad.SetFont(WinUtil::font);
		ctrlPad.SetBackgroundColor(WinUtil::bgColor);
		ctrlPad.SetDefaultCharFormat(WinUtil::m_ChatTextGeneral);
	}

	if (useTextFormatting()) {
		text = Message::unifyLineEndings(text);

		MessageHighlight::SortedList highlights;
		MessageHighlight::parseLinkHighlights(text, highlights, nullptr);
		MessageHighlight::parseReleaseHighlights(text, highlights);
		RichTextBox::parsePathHighlights(text, highlights);

		ctrlPad.SetText(text, highlights, useEmoticons());
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