

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
#include <api/common/SettingUtils.h>
#include "ConfigUtil.h"


shared_ptr<ConfigUtil::ConfigIem> ConfigUtil::getConfigItem(const string& aName, const string& aId, int aType) {
	if (aType == webserver::ApiSettingItem::TYPE_STRING)
		return make_shared<ConfigUtil::StringConfigItem>(aName, aId, aType);

	if (aType == webserver::ApiSettingItem::TYPE_BOOLEAN)
		return make_shared<ConfigUtil::BoolConfigItem>(aName, aId, aType);

	if (aType == webserver::ApiSettingItem::TYPE_FILE_PATH || aType == webserver::ApiSettingItem::TYPE_DIRECTORY_PATH)
		return make_shared<ConfigUtil::BrowseConfigItem>(aName, aId, aType);

	return nullptr;
}

int ConfigUtil::ConfigIem::getParentRightEdge(HWND m_hWnd) {
	CRect rc;
	GetClientRect(m_hWnd, &rc);
	return rc.right;
}

CRect ConfigUtil::ConfigIem::calculateItemPosition(HWND m_hWnd, int prevConfigBottomMargin, int configSpacing) {
	CRect rc;
	rc.left = 20;
	rc.top = prevConfigBottomMargin + configSpacing;
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
	ctrlEdit.SetWindowLongPtr(GWL_EXSTYLE, ctrlEdit.GetWindowLongPtr(GWL_EXSTYLE) & ~WS_EX_NOPARENTNOTIFY);

}

int ConfigUtil::StringConfigItem::updateLayout(HWND m_hWnd, int prevConfigBottomMargin, int configSpacing) {
	//CStatic
	CRect rc = calculateItemPosition(m_hWnd, prevConfigBottomMargin, configSpacing);
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
}

int ConfigUtil::BoolConfigItem::updateLayout(HWND m_hWnd, int prevConfigBottomMargin, int configSpacing) {
	CRect rc = calculateItemPosition(m_hWnd, prevConfigBottomMargin, configSpacing);
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

int ConfigUtil::BrowseConfigItem::updateLayout(HWND m_hWnd, int prevConfigBottomMargin, int configSpacing) {
	prevConfigBottomMargin = StringConfigItem::updateLayout(m_hWnd, prevConfigBottomMargin, configSpacing);

	CRect rc;
	ctrlEdit.GetWindowRect(&rc);
	ctrlEdit.GetParent().ScreenToClient(&rc);
	rc.right -= (buttonWidth + 2); //resize CEdit for placing the button
	ctrlEdit.MoveWindow(rc);

	rc.left = rc.right + 2;
	rc.right = rc.left + buttonWidth;
	rc.bottom = prevConfigBottomMargin;
	ctrlButton.MoveWindow(rc);

	return prevConfigBottomMargin;
}


bool ConfigUtil::BrowseConfigItem::handleClick(HWND m_hWnd) {
	if (m_hWnd == ctrlButton.m_hWnd) {
		ctrlButton.SetWindowText(Text::toT("Clicked " + Util::toString(++clickCounter) + " times").c_str());
		return true;
	}
	return false;
}
