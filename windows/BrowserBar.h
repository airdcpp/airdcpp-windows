/*
* Copyright (C) 2011-2014 AirDC++ Project
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

#ifndef BROWSERBAR_H
#define BROWSERBAR_H

#include "stdafx.h"
#define HISTORY_MSG_MAP 9

struct tButton {
	int id, image;
	ResourceManager::Strings tooltip;
};

static const tButton TBButtons[] = {
	{ IDC_BACK, 2, ResourceManager::BACK },
	{ IDC_FORWARD, 1, ResourceManager::FORWARD },
	{ IDC_UP, 0, ResourceManager::LEVEL_UP },
};

template<class ParentT>
class BrowserBar : boost::noncopyable {
	typedef std::function<void(const string&, bool)> HandleHistoryFunction;
	typedef std::function<void()> HandleUPFuntion;

public:
	BrowserBar(ParentT* aParent, HandleHistoryFunction aHistoryF, HandleUPFuntion aHandleUP) : 
		ParentW(aParent), handleHistory(aHistoryF), handleUP(aHandleUP), historyIndex(1),
		pathContainer(WC_COMBOBOX, aParent, HISTORY_MSG_MAP) {}
	virtual ~BrowserBar() {}

	BEGIN_MSG_MAP(BrowserBar)
		COMMAND_ID_HANDLER(IDC_BACK, onTBButton)
		COMMAND_ID_HANDLER(IDC_FORWARD, onTBButton)
		COMMAND_ID_HANDLER(IDC_UP, onTBButton)
		ALT_MSG_MAP(HISTORY_MSG_MAP)
			COMMAND_CODE_HANDLER(CBN_SELCHANGE, onPathChange)
	END_MSG_MAP()

	void Init() {
		//path field
		RECT rc = { 0, 0, 0, 0 };
		ctrlPath.Create(ParentW->m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
			WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL, 0);
		ctrlPath.SetFont(WinUtil::systemFont);
		pathContainer.SubclassWindow(ctrlPath.m_hWnd);

		//Create a toolbar
		ctrlToolbar.Create(ParentW->m_hWnd, NULL, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST, 0, ATL_IDW_TOOLBAR);
		ctrlToolbar.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS);
		ctrlToolbar.SetImageList(ResourceLoader::getArrowImages());
		ctrlToolbar.SetButtonStructSize();
		addCmdBarButtons();
		ctrlToolbar.AutoSize();

		ParentW->CreateSimpleReBar(WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | RBS_VARHEIGHT | RBS_AUTOSIZE | CCS_NODIVIDER);
		ParentW->AddSimpleReBarBand(ctrlToolbar.m_hWnd, NULL, FALSE, NULL, TRUE);
		ParentW->AddSimpleReBarBand(ctrlPath.m_hWnd, NULL, FALSE, 300, FALSE);
	}

	LRESULT onTBButton(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		switch (wID)
		{
		case IDC_BACK:
			back();
			break;
		case IDC_FORWARD:
			forward();
			break;
		case IDC_UP:
			handleUP();
			break;
		default:
			break;
		}
		return 0;
	}

	LRESULT onPathChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
		handleHistory(history[ctrlPath.GetCurSel()], false);
		bHandled = FALSE;
		return 0;
	}

	void addCmdBarButtons() {
		TBBUTTON nTB;
		memzero(&nTB, sizeof(TBBUTTON));

		int buttonsCount = sizeof(TBButtons) / sizeof(TBButtons[0]);
		for (int i = 0; i < buttonsCount; i++){
			nTB.iBitmap = TBButtons[i].image;
			nTB.idCommand = TBButtons[i].id;
			nTB.fsState = TBSTATE_ENABLED;
			nTB.fsStyle = TBSTYLE_AUTOSIZE;
			nTB.iString = ctrlToolbar.AddStrings(CTSTRING_I((ResourceManager::Strings)TBButtons[i].tooltip));
			ctrlToolbar.AddButtons(1, &nTB);
		}
	}

	void addHistory(const string& aPath) {
		if (history.size() >= historyIndex)
			history.erase(history.begin() + historyIndex, history.end());
		while (history.size() > 25)
			history.pop_front();
		history.emplace_back(aPath);
		historyIndex = history.size();
		updateHistoryCombo();
	}

	void updateHistoryCombo() {
		ctrlPath.ResetContent();
		for (auto& i : history) {
			ctrlPath.AddString(Text::toT(i).c_str());
		}
		setCurSel();
	}

	void setCurSel() {
		ctrlPath.SetCurSel(historyIndex - 1);
	}

	string getCurSel() {
		if (historyIndex >= history.size())
			return Util::emptyString;

		return history[historyIndex];

	}

	void back() {
		if (history.size() > 1 && historyIndex > 1) {
			historyIndex--;
			handleHistory(history[historyIndex - 1], true);
			setCurSel();
		}
	}

	void forward() {
		if (history.size() > 1 && historyIndex < history.size()) {
			historyIndex++;
			handleHistory(history[historyIndex - 1], true);
			setCurSel();
		}
	}

	void clearHistory() {
		history.clear();
		historyIndex = 1;
	}
	void changeWindowState(bool enable) {
		ctrlToolbar.EnableButton(IDC_UP, enable);
		ctrlToolbar.EnableButton(IDC_FORWARD, enable);
		ctrlToolbar.EnableButton(IDC_BACK, enable);
		ctrlPath.EnableWindow(enable);
	}

private:

	HandleHistoryFunction handleHistory;
	HandleUPFuntion handleUP;
	ParentT* ParentW;
	CToolBarCtrl ctrlToolbar;
	CComboBox ctrlPath;
	CContainedWindow pathContainer;

	deque<string> history;
	size_t historyIndex;

};
#endif