/*
* Copyright (C) 2012-2021 AirDC++ Project
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

#include <airdcpp/modules/AutoSearchManager.h>
#include <airdcpp/StringTokenizer.h>
#include <airdcpp/ResourceManager.h>
#include <airdcpp/SimpleXML.h>

#define setMinMax(x, y, z) \
	updown.Attach(GetDlgItem(x)); \
	updown.SetRange32(y, z); \
	updown.Detach();

#define ATTACH(id, var) var.Attach(GetDlgItem(id))

RssFilterPage::RssFilterPage(const string& aName, RSSPtr aFeed) : name(aName), feedItem(aFeed) {
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

	ATTACH(IDC_RSS_BROWSE, cBrowse);

	CRect rc;
	ctrlRssFilterList.GetClientRect(rc);
	ctrlRssFilterList.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT);
	ctrlRssFilterList.InsertColumn(0, CTSTRING(PATTERN), LVCFMT_LEFT, (rc.Width() / 3 * 2), 0);
	ctrlRssFilterList.InsertColumn(1, CTSTRING(PATH), LVCFMT_LEFT, (rc.Width() / 3), 0);

	ATTACH(IDC_MATCHER_TYPE, cMatcherType);
	cMatcherType.AddString(CTSTRING(PLAIN_TEXT));
	cMatcherType.AddString(CTSTRING(REGEXP));
	cMatcherType.AddString(CTSTRING(WILDCARDS));
	cMatcherType.SetCurSel(0);

	::SetWindowText(GetDlgItem(IDC_FILTER_REMOVE), CTSTRING(REMOVE));
	::SetWindowText(GetDlgItem(IDC_FILTER_ADD), CTSTRING(ADD));
	::SetWindowText(GetDlgItem(IDC_FILTER_UPDATE), CTSTRING(UPDATE));
	::SetWindowText(GetDlgItem(IDC_GROUP_LABEL), CTSTRING(AUTOSEARCH_GROUP));
	::SetWindowText(GetDlgItem(IDC_SKIP_DUPES), CTSTRING(SKIP_DUPES));
	::SetWindowText(GetDlgItem(IDC_RSS_FILTER_ACTION_TEXT), CTSTRING(ACTION));
	::SetWindowText(GetDlgItem(IDC_RSS_BROWSE), CTSTRING(BROWSE));
	::SetWindowText(GetDlgItem(IDC_TYPE), CTSTRING(SETTINGS_ST_MATCH_TYPE));
	::SetWindowText(GetDlgItem(IDC_FORMAT_TIME), CTSTRING(RSS_FORMAT_TIME_PARAMS));


	::EnableWindow(GetDlgItem(IDC_FILTER_REMOVE), false);
	::EnableWindow(GetDlgItem(IDC_FILTER_UPDATE), false);

	{
		Lock l(RSSManager::getInstance()->getCS());
		filterList = feedItem->getRssFilterList();

	}

	ATTACH(IDC_RSS_FILTER_ACTION, cAction);
	cAction.InsertString(0, CTSTRING(DOWNLOAD));
	cAction.InsertString(1, CTSTRING(REMOVE));
	cAction.InsertString(2, CTSTRING(ADD_AUTO_SEARCH_DISABLED));
	cAction.SetCurSel(0);

	::SetWindowText(GetDlgItem(IDC_RSS_EXPIRY_DAYS_LABEL), CTSTRING(AUTOSEARCH_EXPIRY_DAYS));
	setMinMax(IDC_EXPIRE_INT_SPIN, 0, 999);
	ATTACH(IDC_EXPIRE_INT, ctrlExpireDays);
	ctrlExpireDays.SetWindowText(_T("3"));

	cGroups.Attach(GetDlgItem(IDC_ASGROUP_BOX));
	cGroups.AddString(_T("---"));
	auto groups = AutoSearchManager::getInstance()->getGroups();
	for (const auto& i : groups) {
		cGroups.AddString(Text::toT(i).c_str());
	}
	cGroups.SetCurSel(0);

	loading = false;
	fillList();

	ctrlRssFilterList.SelectItem(0);
	CheckDlgButton(IDC_SKIP_DUPES, TRUE);
	CenterWindow(GetParent());
	SetWindowText(CTSTRING(RSS_CONFIG));

	return TRUE;
}

LRESULT RssFilterPage::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlRssFilterList) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		if (pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlRssFilterList, pt);
		}

		vector<RSSFilter> filters;
		int sel = ctrlRssFilterList.GetSelectedCount();
		if (sel > 0) {
			int i = -1;
			while ((i = ctrlRssFilterList.GetNextItem(i, LVNI_SELECTED)) != -1) {
				auto filter = filterList[i];
				filters.push_back(filter);
			}
		}
		auto txt = getClipBoardText();
		if (!filters.empty() || !txt.empty()) {
			OMenu menu;
			menu.CreatePopupMenu();
			if (!filters.empty())
				menu.appendItem(TSTRING(COPY), [=] { handleCopyFilters(filters); });
			if (!txt.empty())
				menu.appendItem(TSTRING(PASTE), [=] { handlePasteFilters(txt); });

			menu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
			return TRUE;
		}
		
	}
	bHandled = FALSE;
	return FALSE;
}

void RssFilterPage::handleCopyFilters(const vector<RSSFilter>& aList) {
	SimpleXML xml;
	RSSManager::getInstance()->saveFilters(xml, aList);
	string tmp = xml.toXML();
	WinUtil::setClipboard(Text::toT(tmp));
}

void RssFilterPage::handlePasteFilters(const string& aClipText) {

	if (aClipText.empty())
		return;

	vector<RSSFilter> list;

	try {
		SimpleXML xml;
		xml.fromXML(aClipText);
		RSSManager::getInstance()->loadFilters(xml, list);
	} catch (...) { }

	if (list.empty())
		return;

	for (auto f : list) {
		bool exists = find_if(filterList.begin(), filterList.end(), [&](const RSSFilter& a)
		{ return f.getFilterPattern() == a.getFilterPattern(); }) != filterList.end();

		if (!exists)
			filterList.emplace_back(f);
	}

	fillList();
}

string RssFilterPage::getClipBoardText() {
	
	if (!IsClipboardFormatAvailable(CF_TEXT))
		return Util::emptyString;
	if (!::OpenClipboard(WinUtil::mainWnd))
		return Util::emptyString;

	string tmp;
	HGLOBAL hglb = GetClipboardData(CF_TEXT);
	if (hglb != NULL) {
		char* lptstr = (char*)GlobalLock(hglb);
		if (lptstr != NULL) {
			tmp = lptstr;
			GlobalUnlock(hglb);
		}
	}
	CloseClipboard();
	return tmp;
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
		int i = ctrlRssFilterList.GetNextItem(-1, LVNI_SELECTED);
		advance(item, i);
		ctrlAutoSearchPattern.SetWindowText(Text::toT(item->getFilterPattern()).c_str());
		ctrlTarget.SetWindowText(Text::toT(item->getDownloadTarget()).c_str());
		cMatcherType.SetCurSel(item->getMethod());
		cAction.SetCurSel(item->getFilterAction());
		ctrlExpireDays.SetWindowText(Util::toStringW(item->getExpireDays()).c_str());

		int g = cGroups.FindString(0, Text::toT(item->getAutosearchGroup()).c_str());
		cGroups.SetCurSel(g > 0 ? g : 0);
		CheckDlgButton(IDC_SKIP_DUPES, item->skipDupes ? TRUE : FALSE);
		CheckDlgButton(IDC_FORMAT_TIME, item->getFormatTimeParams() ? TRUE : FALSE);

	} else {
		ctrlAutoSearchPattern.SetWindowText(_T(""));
		ctrlTarget.SetWindowText(_T(""));
		cMatcherType.SetCurSel(0);
		cGroups.SetCurSel(0);
		cAction.SetCurSel(0);
		ctrlExpireDays.SetWindowText(_T("3"));
		CheckDlgButton(IDC_SKIP_DUPES, TRUE);
		CheckDlgButton(IDC_FORMAT_TIME, FALSE);
	}
	fixControls();
	loading = false;

	return 0;
}

bool RssFilterPage::write() {

	if (!update()) {
		return false;
	}

	RSSManager::getInstance()->updateFilterList(feedItem, filterList);
	return true;
}

LRESULT RssFilterPage::onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto asPattern = Text::fromT(WinUtil::getEditText(ctrlAutoSearchPattern));
	auto dlTarget = Text::fromT(WinUtil::getEditText(ctrlTarget));
	if (!validateSettings(asPattern)) {
		return 0;
	}


	add(asPattern, dlTarget, cMatcherType.GetCurSel());
	return 0;
}

LRESULT RssFilterPage::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (ctrlRssFilterList.GetSelectedCount() == 1) {
		int i = ctrlRssFilterList.GetNextItem(-1, LVNI_SELECTED);
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

	CRect rect;
	cBrowse.GetWindowRect(rect);
	auto pt = rect.BottomRight();
	pt.x = pt.x - rect.Width();
	cBrowse.SetState(true);

	OMenu targetMenu;
	targetMenu.CreatePopupMenu();
	targetMenu.InsertSeparatorFirst(CTSTRING(DOWNLOAD_TO));

	appendDownloadTo(targetMenu, false, true, nullopt, nullopt, File::getVolumes());

	targetMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_VERPOSANIMATION, pt);
	return 0;
}

void RssFilterPage::handleDownload(const string& aTarget, Priority/* p*/, bool /*isWhole*/) {
	SetDlgItemText(IDC_RSS_DOWNLOAD_PATH, Text::toT(aTarget).c_str());
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

void RssFilterPage::add(const string& aPattern, const string& aTarget, int aMethod) {
	tstring grp = Util::emptyStringT;
	if (cGroups.GetCurSel() > 0) {
		grp.resize(cGroups.GetWindowTextLength());
		grp.resize(cGroups.GetWindowText(&grp[0], grp.size() + 1));
	}
	bool skipDupes = IsDlgButtonChecked(IDC_SKIP_DUPES) ? true : false;
	bool formatTime = IsDlgButtonChecked(IDC_FORMAT_TIME) ? true : false;
	int action = cAction.GetCurSel();
	int expireDays = Util::toInt(Text::fromT(WinUtil::getEditText(ctrlExpireDays)));

	filterList.emplace_back(RSSFilter(aPattern, aTarget, aMethod, Text::fromT(grp), skipDupes, action, expireDays, formatTime));
	fillList();
	restoreSelection(Text::toT(aPattern));
}

bool RssFilterPage::update() {
	if (ctrlRssFilterList.GetSelectedCount() == 1) {

		auto asPattern = Text::fromT(WinUtil::getEditText(ctrlAutoSearchPattern));
		auto dlTarget = Text::fromT(WinUtil::getEditText(ctrlTarget));
		
		int i = ctrlRssFilterList.GetNextItem(-1, LVNI_SELECTED);
		auto fItem = filterList[i];
		remove(i);

		if (!validateSettings(asPattern)) {
			filterList.push_back(fItem);
			fillList();
			restoreSelection(Text::toT(fItem.getFilterPattern()));
			return false;
		}

		add(asPattern, dlTarget, cMatcherType.GetCurSel());
	}
	return true;
}

bool RssFilterPage::validateSettings(const string& aPattern) {

	if (aPattern.empty()) {
		MessageBox(CTSTRING(PATTERN_EMPTY));
		return false;
	}

	bool exists = find_if(filterList.begin(), filterList.end(), [&](const RSSFilter& a)
	{
		return aPattern == a.getFilterPattern();

	}) != filterList.end();

	if (exists) {
		MessageBox(CTSTRING(PATTERN_EXISTS));
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

void RssFilterPage::fixControls() {
	BOOL enable = cAction.GetCurSel() == 0 || cAction.GetCurSel() == 2;
	::EnableWindow(GetDlgItem(IDC_RSS_DOWNLOAD_PATH), enable);
	::EnableWindow(GetDlgItem(IDC_RSS_BROWSE), enable);
	::EnableWindow(GetDlgItem(IDC_ASGROUP_BOX), enable);
	::EnableWindow(GetDlgItem(IDC_GROUP_LABEL), enable);
	::EnableWindow(GetDlgItem(IDC_RSS_DOWNLOAD_PATH_TEXT), enable);
	::EnableWindow(GetDlgItem(IDC_RSS_EXPIRY_DAYS_LABEL), enable);
	::EnableWindow(GetDlgItem(IDC_EXPIRE_INT), enable);
	::EnableWindow(GetDlgItem(IDC_EXPIRE_INT_SPIN), enable);
	::EnableWindow(GetDlgItem(IDC_FORMAT_TIME), enable);

}
