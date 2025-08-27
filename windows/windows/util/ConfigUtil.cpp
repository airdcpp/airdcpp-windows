

/*
* Copyright (C) 2011-2025 AirDC++ Project
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
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

#include <windows/stdafx.h>

#include <windows/util/ConfigUtil.h>

#include <windows/dialog/BrowseDlg.h>
#include <windows/util/WinUtil.h>

#include <airdcpp/util/text/Text.h>
#include <airdcpp/util/Util.h>

#include <api/common/SettingUtils.h>
#include <web-server/JsonUtil.h>
#include <web-server/WebServerManager.h>


namespace wingui {
using namespace webserver;

shared_ptr<ConfigUtil::ConfigItem> ConfigUtil::getConfigItem(ExtensionSettingItem& aSetting) {
	auto aType = aSetting.type;

	if (aType == ApiSettingItem::TYPE_BOOLEAN)
		return make_shared<ConfigUtil::BoolConfigItem>(aSetting);

	if (aType == ApiSettingItem::TYPE_NUMBER) {
		if (!aSetting.getEnumOptions().empty()) {
			return make_shared<ConfigUtil::EnumConfigItem>(aSetting);
		}

		return make_shared<ConfigUtil::IntConfigItem>(aSetting);
	}

	if (aType == ApiSettingItem::TYPE_FILE_PATH || aType == ApiSettingItem::TYPE_DIRECTORY_PATH || aType == ApiSettingItem::TYPE_EXISTING_FILE_PATH)
		return make_shared<ConfigUtil::BrowseConfigItem>(aSetting);

	if (aType == ApiSettingItem::TYPE_TEXT)
		return make_shared<ConfigUtil::MultilineStringConfigItem>(aSetting);

	if (ApiSettingItem::isString(aType)) {
		if (!aSetting.getEnumOptions().empty()) {
			return make_shared<ConfigUtil::EnumConfigItem>(aSetting);
		}

		return make_shared<ConfigUtil::StringConfigItem>(aSetting);
	}

	return make_shared<ConfigUtil::WebConfigItem>(aSetting);
}

ConfigUtil::ConfigItem::ConfigItem(ExtensionSettingItem& aSetting, int aFlags) : setting(aSetting), flags(aFlags) {

}

void ConfigUtil::ConfigItem::Create(HWND m_hWnd) {
	RECT rcDefault = { 0,0,0,0 };
	if (!(flags & FLAG_DISABLE_LABEL)) {
		ctrlLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | SS_LEFT, NULL);
		ctrlLabel.SetFont(WinUtil::systemFont);
		ctrlLabel.SetWindowText(Text::toT(getLabel()).c_str());
	}

	if (!(flags & FLAG_DISABLE_HELP) && !setting.getHelpStr().empty()) {
		ctrlHelp.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | SS_LEFT, NULL);
		ctrlHelp.SetFont(WinUtil::systemFont);
		ctrlHelp.SetWindowText(Text::toT(setting.getHelpStr()).c_str());
	}

	Create(m_hWnd, rcDefault);
}

void ConfigUtil::ConfigItem::onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam) {
	if ((HWND)lParam == ctrlHelp.m_hWnd) {
		HDC hDC = (HDC)wParam;
		::SetTextColor(hDC, RGB(104, 104, 104)); // grey
	}
}

int ConfigUtil::ConfigItem::calculateTextRows(const tstring& aText, HWND m_hWndControl, int aMaxWidth) noexcept {
	const auto width = WinUtil::getTextWidth(aText, m_hWndControl);
	if (width <= aMaxWidth) {
		return 1;
	}

	const auto rows = std::ceil(static_cast<double>(width) / static_cast<double>(aMaxWidth));
	return rows;
}

int ConfigUtil::ConfigItem::getParentRightEdge(HWND m_hWnd) {
	CRect rc;
	GetClientRect(m_hWnd, &rc);
	return rc.right;
}

tstring ConfigUtil::ConfigItem::valueToString() noexcept {
	if (setting.getValue().is_null()) {
		return Util::emptyStringT;
	}

	return Text::toT(setting.str());
}

CRect ConfigUtil::ConfigItem::calculateItemPosition(HWND m_hWnd, int aPrevConfigBottomMargin, int aConfigSpacing) {
	CRect rc;
	rc.left = MARGIN_LEFT;
	rc.top = aPrevConfigBottomMargin + aConfigSpacing;
	rc.right = getParentRightEdge(m_hWnd) - MARGIN_LEFT;
	rc.bottom = rc.top + WinUtil::getTextHeight(m_hWnd, WinUtil::systemFont) + 2;
	return rc;
}


int ConfigUtil::ConfigItem::updateLayout(HWND m_hWnd, int aPrevConfigBottomMargin, int aConfigSpacing) {
	auto rc = calculateItemPosition(m_hWnd, aPrevConfigBottomMargin, aConfigSpacing);

	// Label
	if (!(flags & FLAG_DISABLE_LABEL)) {
		addLabel(m_hWnd, rc);
	}

	// Component
	updateLayout(m_hWnd, rc);

	// Help text
	if (!(flags & FLAG_DISABLE_HELP) && !setting.getHelpStr().empty()) {
		addHelpText(m_hWnd, rc);
	}

	return rc.bottom;
}

void ConfigUtil::ConfigItem::addLabel(HWND /*m_hWnd*/, CRect& rect_) noexcept {
	rect_.right = rect_.left + min(WinUtil::getTextWidth(Text::toT(getLabel()), ctrlLabel.m_hWnd) + 1, MAX_TEXT_WIDTH);
	ctrlLabel.MoveWindow(rect_);
}

void ConfigUtil::ConfigItem::addHelpText(HWND m_hWnd, CRect& rect_) noexcept {
	rect_.left = MARGIN_LEFT;
	rect_.top = rect_.bottom + 2;
	rect_.right = rect_.left + MAX_TEXT_WIDTH;

	const auto rows = calculateTextRows(Text::toT(setting.getHelpStr()), ctrlHelp.m_hWnd);
	rect_.bottom = rect_.top + max(WinUtil::getTextHeight(m_hWnd, WinUtil::systemFont), 17) * rows;

	ctrlHelp.MoveWindow(rect_);
}

bool ConfigUtil::StringConfigItem::write() {
	auto val = SettingUtils::validateValue(Text::fromT(WinUtil::getEditText(ctrlEdit)), setting, nullptr);
	setting.setValue(val);
	return true;
}

void ConfigUtil::StringConfigItem::Create(HWND m_hWnd, RECT rcDefault) {
	ctrlEdit.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);
	ctrlEdit.SetFont(WinUtil::systemFont);
	ctrlEdit.SetWindowText(Text::toT(JsonUtil::parseValue<string>(setting.name, setting.getValue())).c_str());
	ctrlEdit.SetWindowLongPtr(GWL_EXSTYLE, ctrlEdit.GetWindowLongPtr(GWL_EXSTYLE) & ~WS_EX_NOPARENTNOTIFY);

}

void ConfigUtil::StringConfigItem::updateLayout(HWND m_hWnd, CRect& rc) {
	//CEdit
	rc.top = rc.bottom + 2;
	rc.bottom = rc.top + max(WinUtil::getTextHeight(m_hWnd, WinUtil::systemFont) + 5, 22);
	rc.right = rc.left + MAX_TEXT_WIDTH;
	ctrlEdit.MoveWindow(rc);
}

bool ConfigUtil::StringConfigItem::handleClick(HWND m_hWnd) {
	if (m_hWnd == ctrlEdit.m_hWnd) {
		return true;
	}

	return false;
}

void ConfigUtil::MultilineStringConfigItem::Create(HWND m_hWnd, RECT rcDefault) {
	ctrlEdit.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL | ES_AUTOVSCROLL, WS_EX_CLIENTEDGE);
	ctrlEdit.SetFont(WinUtil::systemFont);
	ctrlEdit.SetWindowText(Text::toT(JsonUtil::parseValue<string>(setting.name, setting.getValue())).c_str());
	ctrlEdit.SetWindowLongPtr(GWL_EXSTYLE, ctrlEdit.GetWindowLongPtr(GWL_EXSTYLE) & ~WS_EX_NOPARENTNOTIFY);
}

void ConfigUtil::MultilineStringConfigItem::updateLayout(HWND m_hWnd, CRect& rc) {
	//CEdit
	rc.top = rc.bottom + 2;
	const int rows = 4;
	rc.bottom = rc.top + (max(WinUtil::getTextHeight(m_hWnd, WinUtil::systemFont), 17) * rows) + 5;
	rc.right = rc.left + MAX_TEXT_WIDTH;
	ctrlEdit.MoveWindow(rc);
}


bool ConfigUtil::BoolConfigItem::write() {
	auto val = SettingUtils::validateValue((ctrlCheck.GetCheck() == 1), setting, nullptr);
	setting.setValue(val);
	return true;
}

void ConfigUtil::BoolConfigItem::Create(HWND m_hWnd, RECT rcDefault) {
	ctrlCheck.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_AUTOCHECKBOX, NULL);
	ctrlCheck.SetWindowLongPtr(GWL_EXSTYLE, ctrlCheck.GetWindowLongPtr(GWL_EXSTYLE) & ~WS_EX_NOPARENTNOTIFY);
	ctrlCheck.SetFont(WinUtil::systemFont);
	ctrlCheck.SetWindowText(Text::toT(getLabel()).c_str());

	ctrlCheck.SetCheck(JsonUtil::parseValue<bool>(setting.name, setting.getValue()));
}

void ConfigUtil::BoolConfigItem::updateLayout(HWND /*m_hWnd*/, CRect& rc) {
	ctrlCheck.MoveWindow(rc);
}

tstring ConfigUtil::BoolConfigItem::valueToString() noexcept {
	if (setting.getValue().is_null()) {
		return Util::emptyStringT;
	}

	return setting.boolean() ? TSTRING(ENABLED) : TSTRING(DISABLED);
}

bool ConfigUtil::BoolConfigItem::handleClick(HWND m_hWnd) {
	if (m_hWnd == ctrlCheck.m_hWnd) {
		return true;
	}
	return false;
}

void ConfigUtil::BrowseConfigItem::Create(HWND m_hWnd, RECT rcDefault) {
	StringConfigItem::Create(m_hWnd, rcDefault);
	ctrlButton.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL);
	ctrlButton.SetFont(WinUtil::systemFont);
	ctrlButton.SetWindowText(CTSTRING(BROWSE));

}

void ConfigUtil::BrowseConfigItem::updateLayout(HWND m_hWnd, CRect& rc) {
	StringConfigItem::updateLayout(m_hWnd, rc);

	auto prevConfigBottomMargin = rc.bottom;

	ctrlEdit.GetWindowRect(&rc);
	ctrlEdit.GetParent().ScreenToClient(&rc);

	// Path field
	rc.right -= (buttonWidth + 2); //resize CEdit for placing the button
	ctrlEdit.MoveWindow(rc);

	// Browse button
	rc.left = rc.right + 2;
	rc.right = rc.left + buttonWidth;
	rc.bottom = prevConfigBottomMargin;
	ctrlButton.MoveWindow(rc);
}


bool ConfigUtil::BrowseConfigItem::handleClick(HWND m_hWnd) {
	const auto getDialogType = [](const ExtensionSettingItem& aSetting) {
		switch (aSetting.type) {
			case ApiSettingItem::TYPE_DIRECTORY_PATH:
				return BrowseDlg::DIALOG_SELECT_FOLDER;
			case ApiSettingItem::TYPE_EXISTING_FILE_PATH:
				return BrowseDlg::DIALOG_OPEN_FILE;
			case ApiSettingItem::TYPE_FILE_PATH:
				return BrowseDlg::DIALOG_SAVE_FILE;
			default: {
				dcassert(0);
				return BrowseDlg::DIALOG_OPEN_FILE;
			}
		}
	};

	if (m_hWnd == ctrlButton.m_hWnd) {
		BrowseDlg dlg(ctrlButton.GetParent().m_hWnd, BrowseDlg::TYPE_GENERIC, getDialogType(setting));
		dlg.setPath(Text::toT(setting.getValue()), true);

		tstring target;
		if (!dlg.show(target)) {
			return false;
		}

		ctrlEdit.SetWindowTextW(target.c_str());
		return true;
	}
	return false;
}

bool ConfigUtil::IntConfigItem::write() {
	auto val = SettingUtils::validateValue(Util::toInt(Text::fromT(WinUtil::getEditText(ctrlEdit))), setting, nullptr);
	setting.setValue(val);
	return true;
}

void ConfigUtil::IntConfigItem::Create(HWND m_hWnd, RECT rcDefault) {
	ctrlEdit.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_NUMBER | ES_RIGHT, WS_EX_CLIENTEDGE);
	ctrlEdit.SetFont(WinUtil::systemFont);
	ctrlEdit.SetWindowText(Text::toT(Util::toString(JsonUtil::parseValue<int>(setting.name, setting.getValue()))).c_str());
	ctrlEdit.SetWindowLongPtr(GWL_EXSTYLE, ctrlEdit.GetWindowLongPtr(GWL_EXSTYLE) & ~WS_EX_NOPARENTNOTIFY);

	spin.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | UDS_SETBUDDYINT | UDS_ALIGNRIGHT, NULL);
	spin.SetBuddy(ctrlEdit.m_hWnd);
	auto limits = setting.getMinMax();
	spin.SetRange32(limits.min, limits.max);
}

void ConfigUtil::IntConfigItem::updateLayout(HWND m_hWnd, CRect& rc) {
	auto labelWidth = WinUtil::getTextWidth(Text::toT(getLabel()), ctrlLabel.m_hWnd);
	auto valueWidth = max(WinUtil::getTextWidth(Text::toT(Util::toString(setting.getMinMax().max)), ctrlEdit.m_hWnd), 30);
	auto spinWidth = 20;

	// Adjust the label if the text is really long so that the value will fit
	if (labelWidth + valueWidth > MAX_TEXT_WIDTH) {
		rc.right = rc.left + MAX_TEXT_WIDTH - valueWidth - spinWidth;

		const auto rows = calculateTextRows(Text::toT(getLabel()), ctrlLabel.m_hWnd, MAX_TEXT_WIDTH - valueWidth - spinWidth);
		rc.bottom = rc.top + WinUtil::getTextHeight(m_hWnd, WinUtil::systemFont) * rows;

		ctrlLabel.MoveWindow(rc);
	}

	//CEdit
	rc.right = rc.left + MAX_TEXT_WIDTH - spinWidth;
	rc.left = rc.right - valueWidth;
	int height = max(WinUtil::getTextHeight(m_hWnd, WinUtil::systemFont) + 5, 22);
	rc.top = rc.bottom - (rc.bottom - rc.top) / 2 - height / 2;
	rc.bottom = rc.top + height;
	ctrlEdit.MoveWindow(rc);

	//spin
	rc.left = rc.right - 1;
	rc.right += spinWidth;
	spin.MoveWindow(rc);
}

bool ConfigUtil::IntConfigItem::handleClick(HWND m_hWnd) {
	if (m_hWnd == ctrlEdit.m_hWnd) {
		return true;
	}
	return false;
}


// ENUMS
void ConfigUtil::EnumConfigItem::Create(HWND m_hWnd, RECT rcDefault) {
	ctrlSelect.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST | ES_RIGHT, WS_EX_CLIENTEDGE);
	ctrlSelect.SetFont(WinUtil::systemFont);

	{
		const auto& options = setting.getEnumOptions();
		for (const auto& option : options) {
			ctrlSelect.AddString(Text::toT(option.text).c_str());
		}

		auto curSel = find_if(options.begin(), options.end(), [this](const auto& aOption) {
			return aOption.id == setting.getValue();
		});

		if (curSel != options.end()) {
			ctrlSelect.SetCurSel(std::distance(options.begin(), curSel));
		}
	}

}

void ConfigUtil::EnumConfigItem::updateLayout(HWND m_hWnd, CRect& rc) {
	//CComboBox
	rc.top = rc.bottom + 2;
	rc.bottom = rc.top + max(WinUtil::getTextHeight(m_hWnd, WinUtil::systemFont) + 5, 22);

	rc.right = rc.left + MAX_TEXT_WIDTH;

	ctrlSelect.MoveWindow(rc);
}

tstring ConfigUtil::EnumConfigItem::valueToString() noexcept {
	const auto& options = setting.getEnumOptions();
	auto curSel = find_if(options.begin(), options.end(), [this](const auto& aOption) {
		return aOption.id == setting.getValue();
	});

	if (curSel != options.end()) {
		return Text::toT(curSel->text);
	}

	return Util::emptyStringT;
}

bool ConfigUtil::EnumConfigItem::handleClick(HWND m_hWnd) {
	if (m_hWnd == ctrlSelect.m_hWnd) {
		return true;
	}

	return false;
}

bool ConfigUtil::EnumConfigItem::write() {
	auto selIndex = ctrlSelect.GetCurSel();
	const auto selectValue = selIndex == -1 ? JsonUtil::emptyJson : setting.getEnumOptions().at(ctrlSelect.GetCurSel()).id;

	auto val = SettingUtils::validateValue(selectValue, setting, nullptr);
	setting.setValue(val);
	return true;
}


// LISTS
void ConfigUtil::WebConfigItem::Create(HWND m_hWnd, RECT rcDefault) {
	ctrlValue.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | /*WS_CLIPSIBLINGS | WS_CLIPCHILDREN |*/ SS_LEFT, NULL);
	ctrlValue.SetFont(WinUtil::systemFont);
	ctrlValue.SetWindowText(CTSTRING(EXTENSION_WEB_CFG_DESC));

	url.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | SS_LEFT, NULL);
	url.SetHyperLinkExtendedStyle(HLINK_UNDERLINEHOVER);
	url.SetHyperLink(Text::toT(WebServerManager::getInstance()->getLocalServerHttpUrl() + "/settings/extensions").c_str());
	url.SetLabel(CTSTRING(OPEN_IN_BROWSER));
}

void ConfigUtil::WebConfigItem::updateLayout(HWND m_hWnd, CRect& rc) {
	// Value
	rc.top = rc.bottom + 2;

	rc.right = rc.left + MAX_TEXT_WIDTH;

	const auto rows = calculateTextRows(TSTRING(EXTENSION_WEB_CFG_DESC), ctrlValue.m_hWnd);
	rc.bottom = rc.top + max(WinUtil::getTextHeight(m_hWnd, WinUtil::systemFont), 17) * rows;

	ctrlValue.MoveWindow(rc);

	// URL
	rc.top = rc.bottom + 2;
	rc.right = rc.left + max(WinUtil::getTextWidth(TSTRING(OPEN_IN_BROWSER), url.m_hWnd), 30);
	rc.bottom = rc.top + max(WinUtil::getTextHeight(m_hWnd, WinUtil::systemFont), 17) * 2;

	url.MoveWindow(rc);
}

bool ConfigUtil::WebConfigItem::handleClick(HWND m_hWnd) {
	if (m_hWnd == ctrlValue.m_hWnd || m_hWnd == url.m_hWnd) {
		return true;
	}

	return false;
}

bool ConfigUtil::WebConfigItem::write() {
	return true;
}

void ConfigUtil::WebConfigItem::onCtlColor(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if ((HWND)lParam == ctrlValue.m_hWnd) {
		HDC hDC = (HDC)wParam;
		::SetTextColor(hDC, RGB(104, 104, 104)); // grey
	}

	ConfigItem::onCtlColor(uMsg, wParam, lParam);
}
}