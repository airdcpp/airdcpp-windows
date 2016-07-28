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
#include "RssFilterPage.h"
#include "BrowseDlg.h"

#include <airdcpp/StringTokenizer.h>
#include <airdcpp/ResourceManager.h>

#define ATTACH(id, var) var.Attach(GetDlgItem(id))

RssFilterPage::RssFilterPage(const string& aName) : name(aName) {
	loading = true;
}

RssFilterPage::~RssFilterPage() {
	ctrlRssFilterList.Detach();
}

LRESULT RssFilterPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	ATTACH(IDC_RSS_AUTOSEARCH, ctrlAutoSearchPattern);
	::SetWindowText(GetDlgItem(IDC_RSS_AUTOSEARCH_TEXT), CTSTRING(RSS_MATCH_PATTERN));

	ATTACH(IDC_RSS_DOWNLOAD_PATH, ctrlTarget);
	::SetWindowText(GetDlgItem(IDC_RSS_DOWNLOAD_PATH_TEXT), CTSTRING(DOWNLOAD_TO));

	ATTACH(IDC_RSS_FILTER_LIST, ctrlRssFilterList);

	CRect rc;
	ctrlRssFilterList.GetClientRect(rc);
	ctrlRssFilterList.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT);
	ctrlRssFilterList.InsertColumn(0, CTSTRING(PATTERN), LVCFMT_LEFT, (rc.Width() / 3), 0);
	ctrlRssFilterList.InsertColumn(1, CTSTRING(PATH), LVCFMT_LEFT, (rc.Width() / 3 * 2), 0);

	::SetWindowText(GetDlgItem(IDC_FILTER_REMOVE), CTSTRING(REMOVE));
	::SetWindowText(GetDlgItem(IDC_FILTER_ADD), CTSTRING(ADD));
	::SetWindowText(GetDlgItem(IDC_FILTER_UPDATE), CTSTRING(UPDATE));

	::EnableWindow(GetDlgItem(IDC_FILTER_REMOVE), false);
	::EnableWindow(GetDlgItem(IDC_FILTER_UPDATE), false);

	{
		Lock l(RSSManager::getInstance()->getCS());
		filterList = RSSManager::getInstance()->getRssFilterList();

	}
	loading = false;
	fillList();

	ctrlRssFilterList.SelectItem(0);
	CenterWindow(GetParent());
	SetWindowText(CTSTRING(RSS_CONFIG));

	return TRUE;
}

LRESULT RssFilterPage::onSelectionChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	if (loading)
		return 0;

	NM_LISTVIEW* lv = (NM_LISTVIEW*)pnmh;
	::EnableWindow(GetDlgItem(IDC_FILTER_REMOVE), (lv->uNewState & LVIS_FOCUSED));
	::EnableWindow(GetDlgItem(IDC_FILTER_UPDATE), (lv->uNewState & LVIS_FOCUSED));
	loading = true;
	if (ctrlRssFilterList.GetSelectedCount() == 1) {
		auto item = filterList.begin();
		advance(item, ctrlRssFilterList.GetSelectedIndex());
		ctrlAutoSearchPattern.SetWindowText(Text::toT(item->getFilterPattern()).c_str());
		ctrlTarget.SetWindowText(Text::toT(item->getDownloadTarget()).c_str());
	} else {
		ctrlAutoSearchPattern.SetWindowText(_T(""));
		ctrlTarget.SetWindowText(_T(""));
	}
	loading = false;

	return 0;
}

bool RssFilterPage::write() {

	if (!update()) {
		return false;
	}

	RSSManager::getInstance()->updateFilterList(filterList);
	return true;
}

LRESULT RssFilterPage::onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto asPattern = Text::fromT(WinUtil::getEditText(ctrlAutoSearchPattern));
	auto dlTarget = Text::fromT(WinUtil::getEditText(ctrlTarget));
	if (!validateSettings(asPattern)) {
		return 0;
	}

	add(asPattern, dlTarget);
	return 0;
}

LRESULT RssFilterPage::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (ctrlRssFilterList.GetSelectedCount() == 1) {
		int i = ctrlRssFilterList.GetSelectedIndex();
		ctrlRssFilterList.DeleteItem(i);
		remove(i);
		fillList();
		ctrlRssFilterList.SelectItem(i > 0 ? i - 1 : 0);
	}
	return 0;
}

LRESULT RssFilterPage::onUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	update();
	return 0;
}

LRESULT RssFilterPage::onBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	tstring dir = Text::toT(SETTING(DOWNLOAD_DIRECTORY));

	BrowseDlg dlg(m_hWnd, BrowseDlg::TYPE_GENERIC, BrowseDlg::DIALOG_SELECT_FOLDER);
	dlg.setPath(dir);
	if (dlg.show(dir)) {
		SetDlgItemText(IDC_RSS_DOWNLOAD_PATH, dir.c_str());
	}

	return 0;
}

void RssFilterPage::fillList() {
	sort(filterList.begin(), filterList.end(), [](const RSSFilter& a, const RSSFilter& b) { return compare(a.getFilterPattern(), b.getFilterPattern()) < 0; });
	ctrlRssFilterList.DeleteAllItems();
	int pos = 0;
	for (auto& i : filterList) {
		ctrlRssFilterList.InsertItem(pos, Text::toT(i.getFilterPattern()).c_str());
		ctrlRssFilterList.SetItemText(pos, 1, Text::toT(i.getDownloadTarget()).c_str());
		pos++;
	}
}

void RssFilterPage::remove(int i) {
	auto r = filterList.begin();
	advance(r, i);
	filterList.erase(r);
}

void RssFilterPage::add(const string& aPattern, const string& aTarget) {
	filterList.emplace_back(RSSFilter(aPattern, aTarget));
	fillList();
	restoreSelection(Text::toT(aPattern));
}

bool RssFilterPage::update() {
	if (ctrlRssFilterList.GetSelectedCount() == 1) {

		auto asPattern = Text::fromT(WinUtil::getEditText(ctrlAutoSearchPattern));
		auto dlTarget = Text::fromT(WinUtil::getEditText(ctrlTarget));
		
		int i = ctrlRssFilterList.GetSelectedIndex();
		auto fItem = filterList[i];
		remove(i);

		if (!validateSettings(asPattern)) {
			filterList.push_back(fItem);
			fillList();
			restoreSelection(Text::toT(fItem.getFilterPattern()));
			return false;
		}

		add(asPattern, dlTarget);
	}
	return true;
}

bool RssFilterPage::validateSettings(const string& aPattern) {

	if (aPattern.empty()) {
		MessageBox(_T("Pattern empty"));
		return false;
	}

	bool exists = find_if(filterList.begin(), filterList.end(), [&](const RSSFilter& a)
	{
		return aPattern == a.getFilterPattern();

	}) != filterList.end();

	if (exists) {
		MessageBox(_T("An item with the same pattern already exists"));
		return false;
	}

	return true;
}

void RssFilterPage::restoreSelection(const tstring& curSel) {
	if (!curSel.empty()) {
		int i = ctrlRssFilterList.find(curSel);
		if (i != -1)
			ctrlRssFilterList.SelectItem(i);
	}
}
