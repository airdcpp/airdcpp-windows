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

RssFeedsPage::RssFeedsPage(const string& aName) : name(aName) { 
	loading = true;
}

RssFeedsPage::~RssFeedsPage() {
	ctrlRssList.Detach();
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

	ATTACH(IDC_RSS_LIST, ctrlRssList);

	CRect rc;
	ctrlRssList.GetClientRect(rc);
	//ctrlRssList.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT);
	ctrlRssList.InsertColumn(0, _T("Dummy"), LVCFMT_LEFT, (rc.Width() - 17), 0);

	::SetWindowText(GetDlgItem(IDCANCEL), CTSTRING(CANCEL));
	::SetWindowText(GetDlgItem(IDC_RSS_REMOVE), CTSTRING(REMOVE));
	::SetWindowText(GetDlgItem(IDC_RSS_ADD), CTSTRING(ADD));
	::SetWindowText(GetDlgItem(IDC_RSS_GROUP_TEXT), CTSTRING(RSS_CONFIG));
	::SetWindowText(GetDlgItem(IDC_RSS_UPDATE), CTSTRING(UPDATE));
	::SetWindowText(GetDlgItem(IDC_RSS_ENABLE), CTSTRING(ENABLE_RSS));

	::EnableWindow(GetDlgItem(IDC_RSS_REMOVE), false);
	::EnableWindow(GetDlgItem(IDC_RSS_UPDATE), false);

	{
		Lock l(RSSManager::getInstance()->getCS());
		auto tmpList = RSSManager::getInstance()->getRss();
		for (auto i : tmpList)
			rssList.emplace_back(i);
	}
	loading = false;
	fillList();

	ctrlRssList.SelectItem(0);
	CenterWindow(GetParent());
	SetWindowText(CTSTRING(RSS_CONFIG));

	return TRUE;
}

LRESULT RssFeedsPage::onSelectionChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	if (loading)
		return 0;

	NM_LISTVIEW* lv = (NM_LISTVIEW*)pnmh;
	::EnableWindow(GetDlgItem(IDC_RSS_REMOVE), (lv->uNewState & LVIS_FOCUSED));
	::EnableWindow(GetDlgItem(IDC_RSS_UPDATE), (lv->uNewState & LVIS_FOCUSED));
	loading = true;
	if (ctrlRssList.GetSelectedCount() == 1) {
		auto item = rssList.begin();
		advance(item, ctrlRssList.GetSelectedIndex());
		ctrlUrl.SetWindowText(Text::toT(item->getUrl()).c_str());
		ctrlName.SetWindowText(Text::toT(item->getFeedName()).c_str());
		ctrlInterval.SetWindowText(Util::toStringW(item->getUpdateInterval()).c_str());
		CheckDlgButton(IDC_RSS_ENABLE, item->getEnable() ? TRUE : FALSE);
	} else {
		ctrlUrl.SetWindowText(_T(""));
		ctrlName.SetWindowText(_T(""));
		ctrlInterval.SetWindowText(_T("60"));
		CheckDlgButton(IDC_RSS_ENABLE, TRUE);
	}
	loading = false;

	return 0;
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

	if (!update()) {
		return false;
	}

	for (auto r : removeList)
		RSSManager::getInstance()->removeFeedItem(r);

	for (auto i : rssList) {
		RSSManager::getInstance()->updateFeedItem(i.feedItem, i.getUrl(), i.getFeedName(), i.getUpdateInterval(), i.getEnable());
	}
	return true;
}

LRESULT RssFeedsPage::onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	add();
	return 0;
}

LRESULT RssFeedsPage::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	remove();
	return 0;
}

LRESULT RssFeedsPage::onUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	update();
	return 0;
}

void RssFeedsPage::fillList() {

	sort(rssList.begin(), rssList.end(), [](const RSSConfigItem& a, const RSSConfigItem& b) { return compare(a.getFeedName(), b.getFeedName()) < 0; });
	ctrlRssList.DeleteAllItems();
	int pos = 0;
	for (auto& i : rssList) {
		ctrlRssList.InsertItem(pos++, Text::toT(i.getFeedName()).c_str());
	}
}

void RssFeedsPage::remove() {
	if (ctrlRssList.GetSelectedCount() == 1) {
		//TODO ask to remove data also.
		int i = ctrlRssList.GetSelectedIndex();
		ctrlRssList.DeleteItem(i);
		auto r = rssList.begin();
		advance(r, i);
		removeList.push_back((*r).feedItem);
		rssList.erase(r);
	}
}

void RssFeedsPage::add() {

	if(!validateSettings(nullptr))
		return;

	auto url = Text::fromT(WinUtil::getEditText(ctrlUrl));
	auto feedname = WinUtil::getEditText(ctrlName);
	auto updateInt = Util::toInt(Text::fromT(WinUtil::getEditText(ctrlInterval)));
	bool enabled = IsDlgButtonChecked(IDC_RSS_ENABLE) ? true : false;

	auto feed = std::make_shared<RSS>(url, Text::fromT(feedname), 0, updateInt);
	rssList.emplace_back(RSSConfigItem(feed));
	fillList();
	//Select the new item
	restoreSelection(feedname);

}

bool RssFeedsPage::update() {
	if (ctrlRssList.GetSelectedCount() == 1) {
		
		int i = ctrlRssList.GetSelectedIndex();
		auto& curItem = rssList[i];

		if (!validateSettings(curItem.feedItem))
			return false;

		curItem.setFeedName(Text::fromT(WinUtil::getEditText(ctrlName)));
		curItem.setUrl(Text::fromT(WinUtil::getEditText(ctrlUrl)));
		curItem.setUpdateInterval(Util::toInt(Text::fromT(WinUtil::getEditText(ctrlInterval))));
		curItem.setEnable(IsDlgButtonChecked(IDC_RSS_ENABLE) ? true : false);
		fillList();
		
		//Select the new item
		restoreSelection(Text::toT(curItem.getFeedName()));
	}
	return true;
}

bool RssFeedsPage::validateSettings(const RSSPtr& aFeed) {
	auto url = Text::fromT(WinUtil::getEditText(ctrlUrl));
	auto feedname = Text::fromT(WinUtil::getEditText(ctrlName));

	if (url.empty() || feedname.empty()) {
		MessageBox(_T("URL and Name must not be empty"));
		return false;
	}

	bool exists = find_if(rssList.begin(), rssList.end(), [&](const RSSConfigItem& a)
	{
		if (url == a.getUrl() || feedname == a.getFeedName()) {
			return !aFeed || (aFeed != a.feedItem); //Skip the current item
		}
		return false;

	}) != rssList.end();

	if (exists) {
		MessageBox(_T("An item with the same URL or Name already exists"));
		return false;
	}

	return true;
}

void RssFeedsPage::restoreSelection(const tstring& curSel) {
	if (!curSel.empty()) {
		int i = ctrlRssList.find(curSel);
		if (i != -1)
			ctrlRssList.SelectItem(i);
	}
}