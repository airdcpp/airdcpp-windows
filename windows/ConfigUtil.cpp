

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

#include "stdafx.h"

#include "WinUtil.h"
#include "ConfigUtil.h"


using namespace webserver;

shared_ptr<ConfigUtil::ConfigIem> ConfigUtil::getConfigItem(ServerSettingItem& aSetting) {
	auto aType = aSetting.type;

	if (aType == ApiSettingItem::TYPE_STRING)
		return make_shared<ConfigUtil::StringConfigItem>(aSetting);

	if (aType == ApiSettingItem::TYPE_BOOLEAN)
		return make_shared<ConfigUtil::BoolConfigItem>(aSetting);

	if (aType == ApiSettingItem::TYPE_NUMBER)
		return make_shared<ConfigUtil::IntConfigItem>(aSetting);

	if (aType == ApiSettingItem::TYPE_FILE_PATH || aType == ApiSettingItem::TYPE_DIRECTORY_PATH)
		return make_shared<ConfigUtil::BrowseConfigItem>(aSetting);

	return nullptr;
}

int ConfigUtil::ConfigIem::getParentRightEdge(HWND m_hWnd) {
	CRect rc;
	GetClientRect(m_hWnd, &rc);
	return rc.right;
}

CRect ConfigUtil::ConfigIem::calculateItemPosition(HWND m_hWnd, int aaPrevConfigBottomMargin, int aConfigSpacing) {
	CRect rc;
	rc.left = 20;
	rc.top = aaPrevConfigBottomMargin + aConfigSpacing;
	rc.right = getParentRightEdge(m_hWnd) -20;
	rc.bottom = rc.top + WinUtil::getTextHeight(m_hWnd, WinUtil::systemFont) + 2;
	return rc;
}

void ConfigUtil::StringConfigItem::setLabel() {
	ctrlLabel.SetWindowText(Text::toT(getLabel()).c_str());
}

void ConfigUtil::StringConfigItem::Create(HWND m_hWnd) {
	RECT rcDefault = { 0,0,0,0 };
	ctrlLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_AUTOHSCROLL, NULL);
	ctrlLabel.SetFont(WinUtil::systemFont);
	ctrlLabel.SetWindowText(Text::toT(getLabel()).c_str());

	ctrlEdit.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);
	ctrlEdit.SetFont(WinUtil::systemFont);
	ctrlEdit.SetWindowText(Text::toT(JsonUtil::parseValue<string>(setting.name, setting.getValue())).c_str());
	ctrlEdit.SetWindowLongPtr(GWL_EXSTYLE, ctrlEdit.GetWindowLongPtr(GWL_EXSTYLE) & ~WS_EX_NOPARENTNOTIFY);

}

int ConfigUtil::StringConfigItem::updateLayout(HWND m_hWnd, int aPrevConfigBottomMargin, int aConfigSpacing) {
	//CStatic
	CRect rc = calculateItemPosition(m_hWnd, aPrevConfigBottomMargin, aConfigSpacing);
	ctrlLabel.MoveWindow(rc);

	//CEdit
	rc.top = rc.bottom + 2;
	rc.bottom = rc.top + max(WinUtil::getTextHeight(m_hWnd, WinUtil::systemFont) + 5, 22);

	ctrlEdit.MoveWindow(rc);

	return rc.bottom;
}

bool ConfigUtil::StringConfigItem::handleClick(HWND m_hWnd) {
	if (m_hWnd == ctrlEdit.m_hWnd) {
		return true;
	}
	return false;
}


void ConfigUtil::BoolConfigItem::setLabel() {
	ctrlCheck.SetWindowText(Text::toT(getLabel()).c_str());
}

void ConfigUtil::BoolConfigItem::Create(HWND m_hWnd) {
	RECT rcDefault = { 0,0,0,0 };

	ctrlCheck.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_AUTOCHECKBOX, NULL);
	ctrlCheck.SetWindowLongPtr(GWL_EXSTYLE, ctrlCheck.GetWindowLongPtr(GWL_EXSTYLE) & ~WS_EX_NOPARENTNOTIFY);
	ctrlCheck.SetFont(WinUtil::systemFont);
	setLabel();

	ctrlCheck.SetCheck(JsonUtil::parseValue<bool>(setting.name, setting.getValue()));

}

int ConfigUtil::BoolConfigItem::updateLayout(HWND m_hWnd, int aPrevConfigBottomMargin, int aConfigSpacing) {
	CRect rc = calculateItemPosition(m_hWnd, aPrevConfigBottomMargin, aConfigSpacing);
	ctrlCheck.MoveWindow(rc);

	return rc.bottom;
}

bool ConfigUtil::BoolConfigItem::handleClick(HWND m_hWnd) {
	if (m_hWnd == ctrlCheck.m_hWnd) {
		return true;
	}
	return false;
}

void ConfigUtil::BrowseConfigItem::Create(HWND m_hWnd) {

	RECT rcDefault = { 0,0,0,0 };
	StringConfigItem::Create(m_hWnd);
	ctrlButton.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL);
	ctrlButton.SetFont(WinUtil::systemFont);
	ctrlButton.SetWindowText(CTSTRING(BROWSE));

}

int ConfigUtil::BrowseConfigItem::updateLayout(HWND m_hWnd, int aPrevConfigBottomMargin, int aConfigSpacing) {
	aPrevConfigBottomMargin = StringConfigItem::updateLayout(m_hWnd, aPrevConfigBottomMargin, aConfigSpacing);

	CRect rc;
	ctrlEdit.GetWindowRect(&rc);
	ctrlEdit.GetParent().ScreenToClient(&rc);
	rc.right -= (buttonWidth + 2); //resize CEdit for placing the button
	ctrlEdit.MoveWindow(rc);

	rc.left = rc.right + 2;
	rc.right = rc.left + buttonWidth;
	rc.bottom = aPrevConfigBottomMargin;
	ctrlButton.MoveWindow(rc);

	return aPrevConfigBottomMargin;
}


bool ConfigUtil::BrowseConfigItem::handleClick(HWND m_hWnd) {
	if (m_hWnd == ctrlButton.m_hWnd) {
		ctrlButton.SetWindowText(Text::toT("Clicked " + Util::toString(++clickCounter) + " times").c_str());
		return true;
	}
	return false;
}

void ConfigUtil::IntConfigItem::setLabel() {
	ctrlLabel.SetWindowText(Text::toT(getLabel()).c_str());
}

void ConfigUtil::IntConfigItem::Create(HWND m_hWnd) {
	RECT rcDefault = { 0,0,0,0 };
	ctrlLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | SS_RIGHT, NULL);
	ctrlLabel.SetFont(WinUtil::systemFont);
	ctrlLabel.SetWindowText(Text::toT(getLabel()).c_str());

	ctrlEdit.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_NUMBER, WS_EX_CLIENTEDGE);
	ctrlEdit.SetFont(WinUtil::systemFont);
	ctrlEdit.SetWindowText(Text::toT(Util::toString(JsonUtil::parseValue<int>(setting.name, setting.getValue()))).c_str());
	ctrlEdit.SetWindowLongPtr(GWL_EXSTYLE, ctrlEdit.GetWindowLongPtr(GWL_EXSTYLE) & ~WS_EX_NOPARENTNOTIFY);

	spin.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | UDS_SETBUDDYINT | UDS_ALIGNRIGHT, NULL);
	spin.SetBuddy(ctrlEdit.m_hWnd);
	auto limits = setting.getMinMax();
	spin.SetRange32(limits.min, limits.max);

}

int ConfigUtil::IntConfigItem::updateLayout(HWND m_hWnd, int aPrevConfigBottomMargin, int aConfigSpacing) {
	//CStatic
	CRect rc = calculateItemPosition(m_hWnd, aPrevConfigBottomMargin, aConfigSpacing);
	rc.right -= 200;
	ctrlLabel.MoveWindow(rc);

	//CEdit
	rc.left = rc.right + 2;
	rc.right += 30;
	int height = max(WinUtil::getTextHeight(m_hWnd, WinUtil::systemFont) + 5, 22);
	rc.top = rc.bottom - (rc.bottom - rc.top) / 2 - height / 2;
	rc.bottom = rc.top + height;
	 
	ctrlEdit.MoveWindow(rc);

	rc.left = rc.right - 1;
	rc.right += 20;
	spin.MoveWindow(rc);

	return rc.bottom;
}

bool ConfigUtil::IntConfigItem::handleClick(HWND m_hWnd) {
	if (m_hWnd == ctrlEdit.m_hWnd) {
		return true;
	}
	return false;
}