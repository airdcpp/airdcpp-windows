/*
 * Copyright (C) 2012-2024 AirDC++ Project
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
#include <windows/Resource.h>
#include <windows/MainFrm.h>
#include <windows/util/ActionUtil.h>

#include <windows/frames/autosearch/AutoSearchFrm.h>
#include <windows/ResourceLoader.h>
#include <windows/frames/search/SearchFrm.h>
#include <windows/frames/autosearch/AutoSearchGroupDlg.h>
#include <windows/frames/autosearch/AutoSearchGeneralPage.h>
#include <windows/frames/autosearch/AutosearchAdvancedPage.h>
#include <windows/frames/autosearch/AutosearchSearchTimesPage.h>
#include <windows/frames/autosearch/AutoSearchItemSettings.h>

#include <airdcpp/settings/SettingsManager.h>

namespace wingui {
#define MAX_STATUS_LINES 10

//static ColumnType columnTypes [] = { COLUMN_TEXT, COLUMN_TEXT, COLUMN_TEXT, COLUMN_NUMERIC_OTHER, COLUMN_TEXT, COLUMN_TEXT, COLUMN_TIME, COLUMN_TEXT, COLUMN_TEXT, COLUMN_TEXT, COLUMN_TEXT };

int AutoSearchFrame::columnIndexes[] = { COLUMN_VALUE, COLUMN_TYPE, COLUMN_SEARCH_STATUS, COLUMN_LASTSEARCH, COLUMN_BUNDLES, COLUMN_ACTION, COLUMN_EXPIRATION,
COLUMN_PATH, COLUMN_REMOVE, COLUMN_USERMATCH, COLUMN_ERROR, COLUMN_PRIORITY };

int AutoSearchFrame::columnSizes[] = { 300, 125, 150, 125, 500, 100, 100, 300, 100, 200, 300, 125 };
static ResourceManager::Strings columnNames[] = { ResourceManager::SETTINGS_VALUE, ResourceManager::TYPE, ResourceManager::SEARCHING_STATUS, ResourceManager::LAST_SEARCH, 
ResourceManager::BUNDLES, ResourceManager::ACTION, ResourceManager::EXPIRATION, ResourceManager::PATH, ResourceManager::AUTO_REMOVE, ResourceManager::USER_MATCH, ResourceManager::LAST_ERROR, ResourceManager::PRIORITY };

string AutoSearchFrame::id = "AutoSearch";

AutoSearchFrame::AutoSearchFrame() : ctrlStatusContainer((LPTSTR)STATUSCLASSNAME, this, AS_STATUS_MSG_MAP), statusTextHandler(ctrlStatus, 0, MAX_STATUS_LINES) { }

LRESULT AutoSearchFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	ctrlStatusContainer.SubclassWindow(ctrlStatus.m_hWnd);

	ctrlAutoSearch.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_AUTOSEARCH);
	ctrlAutoSearch.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);	
	ctrlAutoSearch.SetBkColor(WinUtil::bgColor);
	ctrlAutoSearch.SetTextBkColor(WinUtil::bgColor);
	ctrlAutoSearch.SetTextColor(WinUtil::textColor);

	// Insert columns
	WinUtil::splitTokens(columnIndexes, SETTING(AUTOSEARCHFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(AUTOSEARCHFRAME_WIDTHS), COLUMN_LAST);
	
	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = LVCFMT_LEFT;
		ctrlAutoSearch.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	
	ctrlAutoSearch.SetColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlAutoSearch.SetImageList(ResourceLoader::getAutoSearchStatuses(), LVSIL_SMALL);
	ctrlAutoSearch.setVisible(SETTING(AUTOSEARCHFRAME_VISIBLE));
	ctrlAutoSearch.setSortColumn(COLUMN_VALUE);

		//create buttons
	ctrlAdd.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_ADD);
	ctrlAdd.SetWindowText(CTSTRING(ADD));
	ctrlAdd.SetFont(WinUtil::systemFont);

	ctrlRemove.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_REMOVE);
	ctrlRemove.SetWindowText(CTSTRING(REMOVE));
	ctrlRemove.SetFont(WinUtil::systemFont);

	ctrlChange.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_CHANGE);
	ctrlChange.SetWindowText(CTSTRING(SETTINGS_CHANGE));
	ctrlChange.SetFont(WinUtil::systemFont);

	ctrlDuplicate.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_DUPLICATE);
	ctrlDuplicate.SetWindowText(CTSTRING(DUPLICATE));
	ctrlDuplicate.SetFont(WinUtil::systemFont);

	ctrlManageGroups.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON, 0, IDC_MANAGE_GROUPS);
	ctrlManageGroups.SetWindowText(CTSTRING(MANAGE_GROUPS));
	ctrlManageGroups.SetFont(WinUtil::systemFont);

	statusTextHandler.init(*this);

	AutoSearchManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	{
		//Create itemInfos
		RLock l(AutoSearchManager::getInstance()->getCS());
		auto& lst = AutoSearchManager::getInstance()->getSearchItems();
		for (auto as : lst | views::values) {
			itemInfos.emplace(as->getToken(), ItemInfo(as, false));
		}
	}
	//fill the list
	callAsync([=] { updateList(); });

	WinUtil::SetIcon(m_hWnd, IDI_AUTOSEARCH);
	::SetTimer(m_hWnd, 0, 1000, 0);

	memzero(statusSizes, sizeof(statusSizes));
	statusSizes[0] = 16;
	ctrlStatus.SetParts(5, statusSizes);

	loading = false;
	bHandled = FALSE;
	return TRUE;

}

void AutoSearchFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	
	RECT rect;
	GetClientRect(&rect);
	UpdateBarsPosition(rect, bResizeBars);

	CRect sr;
	int w[4];
	ctrlStatus.GetClientRect(sr);

	w[0] = sr.right - statusSizes[0] - statusSizes[1] - 16;
	w[1] = w[0] + statusSizes[0];
	w[2] = w[1] + statusSizes[1];
	w[3] = w[2] + 16;

	ctrlStatus.SetParts(4, w);

	statusTextHandler.onUpdateLayout(w[0]);

	CRect rc = rect;
	int tmp = sr.top + 32;
	rc.bottom -= tmp;
	ctrlAutoSearch.MoveWindow(rc);

	rc = rect;

	const int button_width = 80;

	const long bottom = rc.bottom - 2;

	//buttons
	rc.bottom = bottom;
	rc.top = bottom - 22;
	rc.left = 20;
	rc.right = rc.left + button_width;
	ctrlAdd.MoveWindow(rc);

	rc.OffsetRect(button_width+2, 0);
	ctrlRemove.MoveWindow(rc);

	rc.OffsetRect(button_width+2, 0);
	ctrlChange.MoveWindow(rc);

	rc.OffsetRect(button_width+2, 0);
	ctrlDuplicate.MoveWindow(rc);

	rc.OffsetRect(button_width + 4, 0);
	rc.right = rc.left + button_width + 40;
	ctrlManageGroups.MoveWindow(rc);

}

LRESULT AutoSearchFrame::onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	updateStatus();
	bHandled = TRUE;
	return 0;
}

void AutoSearchFrame::updateStatus() {
	auto aTime = AutoSearchManager::getInstance()->getNextSearch();
	tstring tmp;
	if (aTime == 0) {
		tmp = Util::emptyStringT;
	}
	else if (!ClientManager::getInstance()->getMe()->isOnline()) {
		tmp = TSTRING(NO_HUBS_TO_SEARCH_FROM);
	} else {
		auto time_left = aTime < GET_TIME() ? 0 : (aTime - GET_TIME());
		if (time_left == 0 && ClientManager::getInstance()->hasSearchQueueOverflow())
			tmp = TSTRING(SEARCH_QUEUE_OVERFLOW);
		else
			tmp = TSTRING(AS_NEXT_SEARCH_IN) + _T(" ") + Text::toT(Util::formatDuration(time_left, false, false));
	}

	bool u = false;
	int w = WinUtil::getStatusTextWidth(tmp, ctrlStatus.m_hWnd);
	if (statusSizes[0] < w) {
		statusSizes[0] = w;
		u = true;
	}
	ctrlStatus.SetText(1, tmp.c_str());

	if (u)
		UpdateLayout(TRUE);

}

/*void AutoSearchFrame::addStatusText(const string& aText, uint8_t sev) {
	
}*/

LRESULT AutoSearchFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)pnmh;

	switch(cd->nmcd.dwDrawStage) {
		case CDDS_PREPAINT:
			return CDRF_NOTIFYITEMDRAW;

		case CDDS_ITEMPREPAINT: {
			ItemInfo *ii = reinterpret_cast<ItemInfo*>(cd->nmcd.lItemlParam);
			if (ii) {
				auto status = ii->asItem->getStatus();
				if (status == AutoSearch::STATUS_FAILED_EXTRAS || status == AutoSearch::STATUS_FAILED_MISSING || !ii->asItem->getLastError().empty()) {
					cd->clrText = SETTING(ERROR_COLOR);
					return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
				}
			}
			return CDRF_NOTIFYSUBITEMDRAW;
		}
		default:
			return CDRF_DODEFAULT;
	}
}

LRESULT AutoSearchFrame::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL & /*bHandled*/) {
	NMITEMACTIVATE* l = (NMITEMACTIVATE*) pnmh;
	if (l->iItem >= 0) {
		PostMessage(WM_COMMAND, IDC_CHANGE, 0);
	} else if (l->iItem == -1) {
		PostMessage(WM_COMMAND, IDC_ADD, 0);
	}
	return 0;
}

LRESULT AutoSearchFrame::onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/) {
	ctrlAutoSearch.SetFocus();
	return 0;
}

LRESULT AutoSearchFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	if(reinterpret_cast<HWND>(wParam) == ctrlAutoSearch) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		
		OMenu asMenu;
		asMenu.CreatePopupMenu();

		CRect rc;

		ctrlAutoSearch.GetHeader().GetWindowRect(&rc);
		if (PtInRect(&rc, pt)) {
			return 0;
		}

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlAutoSearch, pt);
		}

		int enable = ctrlAutoSearch.GetSelectedCount() == 1 ? MFS_ENABLED : MFS_DISABLED;

		auto index = WinUtil::getFirstSelectedIndex(ctrlAutoSearch);
		if(ctrlAutoSearch.GetSelectedCount() > 1) {
			asMenu.appendItem(TSTRING(ENABLE_AUTOSEARCH), [=] { handleState(false); });
			asMenu.appendItem(TSTRING(DISABLE_AUTOSEARCH), [=] { handleState(true); });
			asMenu.appendSeparator();
		} else if(ctrlAutoSearch.GetSelectedCount() == 1) {
			asMenu.appendItem(TSTRING(SEARCH), [=] { handleSearch(true); });
			asMenu.appendItem(TSTRING(SEARCH_FOREGROUND), [=] { handleSearch(false); });
			asMenu.appendSeparator();
			
			if(ctrlAutoSearch.GetCheckState(index) == 1) {
				asMenu.appendItem(TSTRING(DISABLE_AUTOSEARCH), [=] { handleState(true); });
			} else {
				asMenu.appendItem(TSTRING(ENABLE_AUTOSEARCH), [=] { handleState(false); });
			}
			asMenu.appendSeparator();
		}

		if (ctrlAutoSearch.GetSelectedCount() > 0) {
			auto groups = AutoSearchManager::getInstance()->getGroups();
			if (!groups.empty()) {
				OMenu* groupsMenu = asMenu.createSubMenu(_T("Move to group"));
				groupsMenu->appendItem(_T("---"), [=] { handleMoveToGroup(Util::emptyString); });
				for (auto grp : groups) {
					groupsMenu->appendItem(Text::toT(grp), [=] { handleMoveToGroup(grp); });
				}
				asMenu.appendSeparator();
			}
		}
		
		asMenu.AppendMenu(MF_STRING, IDC_ADD, CTSTRING(ADD));
		asMenu.AppendMenu(MF_STRING, IDC_CHANGE, CTSTRING(SETTINGS_CHANGE));
		asMenu.AppendMenu(MF_STRING, IDC_DUPLICATE, CTSTRING(DUPLICATE));

		tstring title;
		if (ctrlAutoSearch.GetSelectedCount() == 1) {
			BundleList bundles;
			AutoSearch::FinishedPathMap fpl;
			auto as = ctrlAutoSearch.getItemData(index)->asItem;
			title = Text::toT(as->getDisplayName());


			if (as->usingIncrementation()) {
				asMenu.appendSeparator();
				asMenu.appendItem(TSTRING(INCREASE_NUM), [=] { 
					AutoSearchManager::getInstance()->changeNumber(as, true);
				}, (as->getMaxNumber() > 0 && as->getCurNumber() >= as->getMaxNumber()) ? OMenu::FLAG_DISABLED : 0);
				asMenu.appendItem(TSTRING(DECREASE_NUM), [=] { 
					AutoSearchManager::getInstance()->changeNumber(as, false);
				}, as->getCurNumber() == 0 ? OMenu::FLAG_DISABLED : 0);
			}

			AutoSearchManager::getInstance()->getMenuInfo(as, bundles, fpl);
			if (!bundles.empty() || !fpl.empty()) {
				asMenu.appendSeparator();
				if (!bundles.empty()) {
					auto bundleMenu = asMenu.createSubMenu(CTSTRING(REMOVE_BUNDLE), true);
					for(auto& b: bundles) {
						auto token = b->getToken();
						bundleMenu->appendItem(Text::toT(b->getName()), [=] { ActionUtil::removeBundle(token); });
					}
				}

				auto pathMenu = asMenu.createSubMenu(CTSTRING(OPEN_FOLDER), false);
				if (!bundles.empty()) {
					pathMenu->InsertSeparatorFirst(CTSTRING(QUEUED_BUNDLES));
					for(auto& b: bundles) {
						string path = b->getTarget();
						pathMenu->appendItem(Text::toT(path), [=] { ActionUtil::openFolder(Text::toT(path)); });
					}
				}

				if (!fpl.empty()) {
					pathMenu->InsertSeparatorLast(CTSTRING(FINISHED_BUNDLES));
					for (const auto& [path, timeFinished] : fpl) {
						pathMenu->appendItem(Text::toT(path) + (timeFinished > 0 ? _T(" (") + FormatUtil::formatDateTimeW(timeFinished) + _T(")") : Util::emptyStringT), [=] { ActionUtil::openFolder(Text::toT(path)); });
					}

					pathMenu->appendSeparator();
					pathMenu->appendItem(CTSTRING(CLEAR_FINISHED_PATHS), [=] { AutoSearchManager::getInstance()->clearPaths(as); });
				}
			}

			if (!as->getLastError().empty()) {
				asMenu.appendSeparator();
				asMenu.appendItem(TSTRING(CLEAR_ERROR), [=] { 
					as->setLastError(Util::emptyString); 
					updateItem(as);
				});
			}
		}

		asMenu.AppendMenu(MF_SEPARATOR);
		asMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));

		asMenu.EnableMenuItem(IDC_CHANGE, enable);
		asMenu.EnableMenuItem(IDC_DUPLICATE, enable);
		asMenu.SetMenuDefaultItem(IDC_CHANGE);

		//make a menu title from the search string, its probobly too long to fit but atleast it shows something.
		if (!title.empty())
			asMenu.InsertSeparatorFirst(title);
		
		asMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);

		if (!title.empty())
			asMenu.RemoveFirstItem();

		return TRUE; 
	}
	
	bHandled = FALSE;
	return FALSE; 
}

LRESULT AutoSearchFrame::onChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (ctrlAutoSearch.GetSelectedCount() == 1) {
		int sel = ctrlAutoSearch.GetNextItem(-1, LVNI_SELECTED);
		auto ii = ctrlAutoSearch.getItemData(sel);

		TabbedDialog dlg(STRING(AUTOSEARCH_DLG));
		AutoSearchItemSettings options(ii->asItem, false);
		createPages(dlg, options);

		if (dlg.DoModal() == IDOK) {
			options.setItemProperties(ii->asItem, options.searchString);
			AutoSearchManager::getInstance()->updateAutoSearch(ii->asItem);
		}
	}
	return 0;
}

LRESULT AutoSearchFrame::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	//use a removelist, the selection position will change when removing multiple.
	AutoSearchList removelist;

	int i = -1;
	while ((i = ctrlAutoSearch.GetNextItem(i, LVNI_SELECTED)) != -1) {
		removelist.push_back(ctrlAutoSearch.getItemData(i)->asItem);
	}

	if (WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_AS_REMOVAL, TSTRING(REALLY_REMOVE))) {
		for (auto a = removelist.begin(); a != removelist.end(); ++a)
			AutoSearchManager::getInstance()->removeAutoSearch(*a);
	}
	return 0;
}

LRESULT AutoSearchFrame::onManageGroups(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	AsGroupsDlg dlg;
	dlg.DoModal();

	updateList();
	return 0;
}

LRESULT AutoSearchFrame::onItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMITEMACTIVATE* l = (NMITEMACTIVATE*)pnmh;

	::EnableWindow(GetDlgItem(IDC_REMOVE), (ctrlAutoSearch.GetSelectedCount() >= 1));
	::EnableWindow(GetDlgItem(IDC_CHANGE), (ctrlAutoSearch.GetSelectedCount() == 1));
	::EnableWindow(GetDlgItem(IDC_DUPLICATE), (ctrlAutoSearch.GetSelectedCount() == 1));

	if (!loading && l->iItem != -1 && ((l->uNewState & LVIS_STATEIMAGEMASK) != (l->uOldState & LVIS_STATEIMAGEMASK))) {
		AutoSearchPtr as = ctrlAutoSearch.getItemData(l->iItem)->asItem;
		if (as) {
			AutoSearchManager::getInstance()->setItemActive(as, Util::toBool(ctrlAutoSearch.GetCheckState(l->iItem)));
		}
		bHandled = TRUE;
		return 1;
	}
	return 0;
}

LRESULT AutoSearchFrame::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*)pnmh;
	switch (kd->wVKey) {
	case VK_DELETE:
		if (ctrlAutoSearch.GetSelectedCount() >= 1) {
			PostMessage(WM_COMMAND, IDC_REMOVE, 0);
		}
		break;
	case VK_RETURN:
		if (WinUtil::isShift() || WinUtil::isCtrl() || WinUtil::isAlt()) {
		}
		else {
			if (ctrlAutoSearch.GetSelectedCount() == 1) {
				PostMessage(WM_COMMAND, IDC_CHANGE, 0);
			}
		}
		break;
	default:
		break;
	}
	return 0;
}

LRESULT AutoSearchFrame::onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TabbedDialog dlg(STRING(AUTOSEARCH_DLG));
	AutoSearchItemSettings options;
	createPages(dlg, options);

	if (dlg.DoModal() == IDOK) {
		SettingsManager::getInstance()->set(SettingsManager::LAST_AS_FILETYPE, options.fileTypeId);
		addFromDialog(options);
	}
	return 0;
}

void AutoSearchFrame::createPages(TabbedDialog& dlg, AutoSearchItemSettings& options) {
	dlg.addPage<AutoSearchGeneralPage>(shared_ptr<AutoSearchGeneralPage>(new AutoSearchGeneralPage(options, STRING(SETTINGS_GENERAL))));
	dlg.addPage<AutosearchSearchTimesPage>(shared_ptr<AutosearchSearchTimesPage>(new AutosearchSearchTimesPage(options, STRING(SEARCH_TIMES))));
	dlg.addPage<AutoSearchAdvancedPage>(shared_ptr<AutoSearchAdvancedPage>(new AutoSearchAdvancedPage(options, STRING(SETTINGS_ADVANCED))));
	
}

LRESULT AutoSearchFrame::onDuplicate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (ctrlAutoSearch.GetSelectedCount() == 1) {
		int sel = ctrlAutoSearch.GetNextItem(-1, LVNI_SELECTED);
		AutoSearchPtr as = ctrlAutoSearch.getItemData(sel)->asItem;

		TabbedDialog dlg(STRING(AUTOSEARCH_DLG));

		AutoSearchItemSettings options(as, true);
		createPages(dlg, options);

		if (dlg.DoModal() == IDOK) {
			addFromDialog(options);
		}
	}
	return 0;
}

void AutoSearchFrame::updateList() {
	ctrlAutoSearch.SetRedraw(FALSE);
	ctrlAutoSearch.DeleteAllItems();

	auto groups = AutoSearchManager::getInstance()->getGroups();
	int groupid = 0;
	ctrlAutoSearch.insertGroup(groupid, Util::emptyStringT, LVGA_HEADER_LEFT);
	for (auto i : groups) {
		groupid++;
		ctrlAutoSearch.insertGroup(groupid, Text::toT(i), LVGA_HEADER_LEFT);
	}

	for (auto &ii : itemInfos) {
		(&ii.second)->update((&ii.second)->asItem);
		addListEntry(&ii.second);
	}
	ctrlAutoSearch.resort();
	ctrlAutoSearch.SetRedraw(TRUE);
	ctrlAutoSearch.Invalidate();
}


void AutoSearchFrame::addFromDialog(AutoSearchItemSettings& dlg) {
	string search = dlg.searchString + "\r\n";
	string::size_type j = 0;
	string::size_type i = 0;
	
	while((i = search.find("\r\n", j)) != string::npos) {
		string str = search.substr(j, i-j);
		j = i +2;
		if(str.size() >= 5) { //dont accept shorter search strings than 5 chars
			auto as = std::make_shared<AutoSearch>();

			dlg.setItemProperties(as, str);
			AutoSearchManager::getInstance()->addAutoSearch(as, false);
		} else if(search.size() < 5) { // dont report if empty line between/end when adding multiple
			MessageBox(CTSTRING(LINE_EMPTY_OR_TOO_SHORT));
		}
	}
}


void AutoSearchFrame::handleSearch(bool onBackground) {
	if(ctrlAutoSearch.GetSelectedCount() == 1) {
		int sel = ctrlAutoSearch.GetNextItem(-1, LVNI_SELECTED);
		MainFrame::getMainFrame()->addThreadedTask([=] {
			auto ii = ctrlAutoSearch.getItemData(sel);
			if (ii) {
				AutoSearchManager::getInstance()->searchItem(ii->asItem, onBackground ? AutoSearchManager::TYPE_MANUAL_BG : AutoSearchManager::TYPE_MANUAL_FG);
			}
		});
	}
}
void AutoSearchFrame::handleMoveToGroup(const string& aGroupName) {
	int i = -1;
	while ((i = ctrlAutoSearch.GetNextItem(i, LVNI_SELECTED)) != -1) {
		AutoSearchManager::getInstance()->moveItemToGroup(ctrlAutoSearch.getItemData(i)->asItem, aGroupName);
	}
}


void AutoSearchFrame::on(AutoSearchManagerListener::SearchForeground, const AutoSearchPtr& as, const string& searchString) noexcept {
	callAsync([=] { SearchFrame::openWindow(Text::toT(searchString), 0, Search::SIZE_DONTCARE, as->getFileType()); });
}

void AutoSearchFrame::handleState(bool disabled) {
	//just set the checkstate, onitemchanged will handle it from there
	int i = -1;
	while ((i = ctrlAutoSearch.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ctrlAutoSearch.SetCheckState(i, disabled ? FALSE : TRUE);
	}
}

void AutoSearchFrame::addItem(const AutoSearchPtr& as) {
	if (!as)
		return;
	auto ui = itemInfos.find(as->getToken());
	if (ui == itemInfos.end()) {
		auto x = itemInfos.emplace(as->getToken(), ItemInfo(as)).first;
		ctrlAutoSearch.SetRedraw(FALSE);
		addListEntry(&x->second);
		ctrlAutoSearch.resort();
		ctrlAutoSearch.SetRedraw(TRUE);
	}
}


void AutoSearchFrame::addListEntry(ItemInfo* ii) {
	loading = true;
	int i = ctrlAutoSearch.insertItem(ctrlAutoSearch.GetItemCount(), ii, 0, ii->getGroupId()/*I_GROUPIDCALLBACK*/);
	ctrlAutoSearch.SetItem(i, 0, LVIF_IMAGE, NULL, ii->asItem->getStatus(), 0, 0, NULL);
	ctrlAutoSearch.updateItem(i);
	ctrlAutoSearch.SetCheckState(i, ii->asItem->getEnabled());
	loading = false;
}

void AutoSearchFrame::updateItem(const AutoSearchPtr as) {
	auto i = itemInfos.find(as->getToken());
	if (i != itemInfos.end()) {
		auto ii = &i->second;
		ii->update(as);
		int pos = ctrlAutoSearch.findItem(ii);
		if (pos >= 0) {
			ctrlAutoSearch.SetCheckState(pos, as->getEnabled());
			LVITEM lvi = { 0 };
			lvi.mask = LVIF_GROUPID | LVIF_IMAGE;
			lvi.iItem = pos;
			lvi.iGroupId = ii->getGroupId();
			lvi.iImage = as->getStatus();
			lvi.iSubItem = 0;
			ctrlAutoSearch.SetItem(&lvi);
			ctrlAutoSearch.updateItem(pos);
		}
	}
}

void AutoSearchFrame::removeItem(const AutoSearchPtr as) {
	auto i = itemInfos.find(as->getToken());
	if (i != itemInfos.end()) {
		ctrlAutoSearch.deleteItem(&i->second);
		itemInfos.erase(i);
	}
}

void AutoSearchFrame::ItemInfo::update(const AutoSearchPtr& as) {
	string target = as->getTarget();
	if (target.empty()) {
		target = CSTRING(SETTINGS_DOWNLOAD_DIRECTORY);
	}

	columns[COLUMN_VALUE] = Text::toT(as->getDisplayName());
	columns[COLUMN_LASTSEARCH] = (as->getLastSearch() > 0 ? formatSearchDate(as->getLastSearch()) : _T("Unknown"));
	columns[COLUMN_TYPE] = Text::toT(as->getDisplayType());
	columns[COLUMN_BUNDLES] = Text::toT(AutoSearchManager::getInstance()->getBundleStatuses(as));
	columns[COLUMN_SEARCH_STATUS] = Text::toT(as->getSearchingStatus());
	columns[COLUMN_EXPIRATION] = Text::toT(as->getExpiration());
	columns[COLUMN_ERROR] = Text::toT(as->getLastError());
	columns[COLUMN_ACTION] = as->getAction() == 0 ? TSTRING(DOWNLOAD) : as->getAction() == 1 ? TSTRING(ADD_TO_QUEUE) : TSTRING(AS_REPORT);
	columns[COLUMN_EXPIRATION] = Text::toT(as->getExpiration());
	columns[COLUMN_REMOVE] = as->getRemove() ? TSTRING(YES) : TSTRING(NO);
	columns[COLUMN_PATH] = Text::toT(target);
	columns[COLUMN_USERMATCH] = Text::toT(as->getNickPattern());
	columns[COLUMN_PRIORITY] = Text::toT(Util::formatPriority(as->getPriority()) + (as->isRecent() ? " ( " + STRING(RECENT) + " )" : ""));

	setGroupId(AutoSearchManager::getInstance()->getGroupIndex(as));

}


tstring AutoSearchFrame::ItemInfo::formatSearchDate(const time_t aTime) {
	char buf[20];
	if (strftime(buf, 20, "%x %X", localtime(&aTime))) {
		return Text::toT(string(buf));
	} else {
		return _T("-");
	}
}


void AutoSearchFrame::on(AutoSearchManagerListener::ItemRemoved, const AutoSearchPtr& as) noexcept { 
	callAsync([=] { removeItem(as); if(SETTING(AUTOSEARCH_BOLD)) setDirty(); }); 
}

void AutoSearchFrame::on(AutoSearchManagerListener::ItemAdded, const AutoSearchPtr& as) noexcept { 
	callAsync([=] { addItem(as); if(SETTING(AUTOSEARCH_BOLD)) setDirty();  });
}

void AutoSearchFrame::on(AutoSearchManagerListener::ItemUpdated, const AutoSearchPtr& as, bool aSetDirty) noexcept {
	callAsync([=] { updateItem(as); if (aSetDirty && SETTING(AUTOSEARCH_BOLD)) setDirty();  }); 
}

void AutoSearchFrame::on(AutoSearchManagerListener::ItemSearched, const AutoSearchPtr& /*as*/, const string& aMsg) noexcept {
	callAsync([=] { 
		statusTextHandler.addStatusText(Text::toT(aMsg), LogMessage::SEV_INFO); 
	});
}

LRESULT AutoSearchFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {

	if(!closed) {
		SettingsManager::getInstance()->removeListener(this);
		AutoSearchManager::getInstance()->removeListener(this);
		WinUtil::setButtonPressed(IDC_AUTOSEARCH, false);
		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	}else {
		ctrlAutoSearch.saveHeaderOrder(SettingsManager::AUTOSEARCHFRAME_ORDER, SettingsManager::AUTOSEARCHFRAME_WIDTHS, SettingsManager::AUTOSEARCHFRAME_VISIBLE);
		ctrlAutoSearch.DeleteAllItems();
		itemInfos.clear();

		bHandled = FALSE;
		return 0;
	}
}
}