/*
* Copyright (C) 2012-2016 AirDC++ Project
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
#include "WinUtil.h"
#include "RssFeedsPage.h"
#include "BrowseDlg.h"

#include <airdcpp/StringTokenizer.h>
#include <airdcpp/ResourceManager.h>

#define setMinMax(x, y, z) \
	updown.Attach(GetDlgItem(x)); \
	updown.SetRange32(y, z); \
	updown.Detach();

#define ATTACH(id, var) var.Attach(GetDlgItem(id))

RssFeedsPage::RssFeedsPage(const string& aName, RSSPtr aFeed) : name(aName), feedItem(aFeed) { 
	loading = true;
}

RssFeedsPage::~RssFeedsPage() {
}

LRESULT RssFeedsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	ATTACH(IDC_RSS_URL, ctrlUrl);
	::SetWindowText(GetDlgItem(IDC_RSS_URL_TEXT), CTSTRING(LINK));

	ATTACH(IDC_RSS_NAME, ctrlName);
	::SetWindowText(GetDlgItem(IDC_RSS_NAME_TEXT), CTSTRING(NAME));

	ATTACH(IDC_RSS_INTERVAL, ctrlInterval);
	::SetWindowText(GetDlgItem(IDC_INTERVAL_TEXT), CTSTRING(MINIMUM_UPDATE_INTERVAL_MIN));
	setMinMax(IDC_RSS_INT_SPIN, 10, 999);
	ctrlInterval.SetWindowText(_T("60"));

	::SetWindowText(GetDlgItem(IDCANCEL), CTSTRING(CANCEL));
	::SetWindowText(GetDlgItem(IDC_RSS_GROUP_TEXT), CTSTRING(RSS_CONFIG));
	::SetWindowText(GetDlgItem(IDC_RSS_ENABLE), CTSTRING(ENABLE_RSS));

	ctrlUrl.SetWindowText(Text::toT(feedItem->getUrl()).c_str());
	ctrlName.SetWindowText(Text::toT(feedItem->getFeedName()).c_str());
	ctrlInterval.SetWindowText(Util::toStringW(feedItem->getUpdateInterval()).c_str());
	CheckDlgButton(IDC_RSS_ENABLE, feedItem->getEnable() ? TRUE : FALSE);


	CenterWindow(GetParent());
	SetWindowText(CTSTRING(RSS_CONFIG));
	loading = false;
	fixControls();

	return TRUE;
}

LRESULT RssFeedsPage::onIntervalChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/) {
	if (loading)
		return 0;

	int value = Util::toInt(Text::fromT(WinUtil::getEditText(ctrlInterval)));
	if (value == 0) {
		value = 30; // use 30 minutes as default interval
	}
	else if (value < 10) {
		value = 10;
	}
	ctrlInterval.SetWindowText(Text::toT(Util::toString(value)).c_str());
	return 0;
}

bool RssFeedsPage::write() {

	auto feedname = Text::fromT(WinUtil::getEditText(ctrlName));
	auto url = Text::fromT(WinUtil::getEditText(ctrlUrl));
	auto interval = Util::toInt(Text::fromT(WinUtil::getEditText(ctrlInterval)));
	auto enabled = IsDlgButtonChecked(IDC_RSS_ENABLE) ? true : false;

	if (url.empty() || feedname.empty()) {
		MessageBox(_T("URL and Name must not be empty"));
		return false;
	}

	auto feed = RSSManager::getInstance()->getFeedByName(feedname);
	if (feed && feed != feedItem) {
		MessageBox(_T("An item with the same Name already exists"));
		return false;
	}
	feed = RSSManager::getInstance()->getFeedByUrl(url);
	if (feed && feed != feedItem) {
		MessageBox(_T("An item with the same Url already exists"));
		return false;
	}

	RSSManager::getInstance()->updateFeedItem(feedItem, url, feedname, interval, enabled);
	
	return true;
}


LRESULT RssFeedsPage::onEnable(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (loading)
		return 0;
	fixControls();
	return 0;
}


void RssFeedsPage::fixControls() {
	BOOL enabled = IsDlgButtonChecked(IDC_RSS_ENABLE);
	::EnableWindow(GetDlgItem(IDC_RSS_INTERVAL), enabled);
	::EnableWindow(GetDlgItem(IDC_RSS_INTERVAL_TEXT), enabled);
	::EnableWindow(GetDlgItem(IDC_RSS_INT_SPIN), enabled);

}