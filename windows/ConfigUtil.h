

/*
* Copyright (C) 2011-2017 AirDC++ Project
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

#ifndef CONFIG_UTIL_H
#define CONFIG_UTIL_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "stdafx.h"

#include "WinUtil.h"


struct ConfigIem {
	ConfigIem(const string& aName, const string& aId, int aType) : label(aName), id(aId), type(aType) {}
	GETSET(string, id, Id);
	GETSET(string, label, Label);
	int type;

	virtual int Create(HWND m_hWnd, int prevConfigBottomMargin, int configSpacing) = 0;
	virtual bool handleClick(HWND m_hWnd) = 0;

};

struct StringConfigItem : public ConfigIem {

	StringConfigItem(const string& aName, const string& aId, int aType) : ConfigIem(aName, aId, aType) {}

	string getValueString() {
		return Text::fromT(WinUtil::getEditText(ctrlEdit));
	}
	void setLabel() {
		ctrlLabel.SetWindowText(Text::toT(getLabel()).c_str());
	}

	int Create(HWND m_hWnd, int prevConfigBottomMargin, int configSpacing) {
		RECT rcDefault = { 0,0,0,0 };
		ctrlLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_AUTOHSCROLL, NULL);
		ctrlLabel.SetFont(WinUtil::systemFont);
		ctrlLabel.SetWindowText(Text::toT(getLabel()).c_str());

		ctrlEdit.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);
		ctrlEdit.SetFont(WinUtil::systemFont);

		CRect rc;

		//CStatic
		rc.left = 30;
		rc.top = prevConfigBottomMargin + configSpacing;
		rc.right = 500;
		rc.bottom = rc.top + WinUtil::getTextHeight(m_hWnd, WinUtil::systemFont);
		ctrlLabel.MoveWindow(rc);

		//CEdit
		rc.top = rc.bottom + 2;
		rc.bottom = rc.top + 25;

		ctrlEdit.MoveWindow(rc);

		return rc.bottom;

	}

	bool handleClick(HWND m_hWnd) {
		if (m_hWnd == ctrlEdit.m_hWnd) {
			return true;
		}
		return false;
	}


	CStatic ctrlLabel;
	CEdit ctrlEdit;
};

struct BoolConfigItem : public ConfigIem {

	BoolConfigItem(const string& aName, const string& aId, int aType) : ConfigIem(aName, aId, aType) {}

	bool getValueBool() {
		return ctrlCheck.GetCheck() == 1;
	}
	void setLabel() {
		ctrlCheck.SetWindowText(Text::toT(getLabel()).c_str());
	}

	int Create(HWND m_hWnd, int prevConfigBottomMargin, int configSpacing) {
		RECT rcDefault = { 0,0,0,0 };

		ctrlCheck.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_AUTOCHECKBOX, NULL);
		ctrlCheck.SetFont(WinUtil::systemFont);
		setLabel();

		CRect rc;
		rc.left = 30;
		rc.top = prevConfigBottomMargin + configSpacing;
		rc.right = 500;
		rc.bottom = rc.top + WinUtil::getTextHeight(m_hWnd, WinUtil::systemFont) +2;
		ctrlCheck.MoveWindow(rc);

		return rc.bottom;

	}

	bool handleClick(HWND m_hWnd) {
		if (m_hWnd == ctrlCheck.m_hWnd) {
			return true;
		}
		return false;
	}

	CButton ctrlCheck;
};

struct BrowseConfigItem : public StringConfigItem {

	BrowseConfigItem(const string& aName, const string& aId, int aType) : StringConfigItem(aName, aId, aType) {}

	int Create(HWND m_hWnd, int prevConfigBottomMargin, int configSpacing) {

		prevConfigBottomMargin = StringConfigItem::Create(m_hWnd, prevConfigBottomMargin, configSpacing);
		RECT rcDefault = { 0,0,0,0 };

		ctrlButton.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL);
		ctrlButton.SetFont(WinUtil::systemFont);
		ctrlButton.SetWindowText(CTSTRING(BROWSE));

		CRect rc;
		ctrlEdit.GetWindowRect(&rc);
		ctrlEdit.GetParent().ScreenToClient(&rc);
		rc.right -= (buttonWidth + 2);
		ctrlEdit.MoveWindow(rc);

		rc.left = rc.right + 2;
		rc.right = rc.left + buttonWidth;
		rc.bottom = prevConfigBottomMargin;
		ctrlButton.MoveWindow(rc);

		return prevConfigBottomMargin;

	}

	bool handleClick(HWND m_hWnd) {
		if (m_hWnd == ctrlButton.m_hWnd) {
			ctrlButton.SetWindowText(Text::toT("Clicked " + Util::toString(++clickCounter) + " times").c_str());
			return true;
		}
		return false;
	}
	CButton ctrlButton;
	int buttonWidth = 80;

	int clickCounter = 0; //for testing...
};



#endif