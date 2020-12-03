/*
 * Copyright (C) 2001-2021 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(TEXT_FRAME_H)
#define TEXT_FRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatTabCtrl.h"
#include "RichTextBox.h"
#include "atlstr.h"

#include <airdcpp/ViewFileManagerListener.h>
#include <airdcpp/SimpleXML.h>

class TextFrame : public MDITabChildWindowImpl<TextFrame>, private SettingsManagerListener, private ViewFileManagerListener
{
public:
	enum class FileType {
		NFO,
		LOG,
		OTHER,
	};

	static void openFile(const string& aFilePath);
	static void openFile(const ViewFilePtr& aFile);
	static TextFrame* viewText(const string& aTitle, const string& aText, FileType aType, const ViewFilePtr& aFile = nullptr);
	static FileType parseFileType(const string& aName) noexcept;

	static bool getWindowParams(HWND hWnd, StringMap&/*params*/) {
		auto f = frames.find(hWnd);
		if (f != frames.end()) {
			//params["id"] = TextFrame::id;
			return true;
		}
		return false;
	}

	DECLARE_FRAME_WND_CLASS_EX(_T("TextFrame"), IDR_NOTEPAD, 0, COLOR_3DFACE);

	TextFrame(const string& aTitle, const string& aText, FileType aType, const ViewFilePtr& aFile);
	~TextFrame() { }
	
	typedef MDITabChildWindowImpl<TextFrame> baseClass;
	BEGIN_MSG_MAP(TextFrame)
		MESSAGE_HANDLER(WM_LBUTTONUP, onClientEnLink)
		MESSAGE_HANDLER(WM_RBUTTONUP, onClientEnLink)
		MESSAGE_HANDLER(WM_SETFOCUS, OnFocus)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		CHAIN_MSG_MAP(baseClass)
		
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClientEnLink(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) { return ctrlPad.onClientEnLink(uMsg, wParam, lParam, bHandled); }
	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}
	
	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	void UpdateLayout(BOOL bResizeBars = TRUE);

	static string id;

private:

	typedef map<HWND, TextFrame*> FrameMap;
	static FrameMap frames;

	void openWindow();
	static string readFile(const string& aFilePath) noexcept;

	tstring title;
	string text;
	RichTextBox ctrlPad;
	const FileType type;

	bool useTextFormatting() const noexcept;
	bool useEmoticons() const noexcept;

	IGETSET(bool, autoScroll, AutoScroll, true);

	void setViewModeNfo();

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;
	void on(ViewFileManagerListener::FileClosed, const ViewFilePtr& aFile) noexcept;
	void on(ViewFileManagerListener::FileRead, const ViewFilePtr& aFile) noexcept;

	ViewFilePtr viewFile = nullptr;
};

#endif // !defined(TEXT_FRAME_H)