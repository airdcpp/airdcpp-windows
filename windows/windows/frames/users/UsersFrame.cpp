/* 
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
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
#include <windows/ResourceLoader.h>
#include <windows/frames/users/UsersFrame.h>
#include <windows/dialog/LineDlg.h>
#include <windows/util/FormatUtil.h>
#include <windows/util/ActionUtil.h>

#include <airdcpp/hub/ClientManager.h>
#include <airdcpp/favorites/FavoriteUserManager.h>
#include <airdcpp/user/ignore/IgnoreManager.h>
#include <airdcpp/core/localization/Localization.h>
#include <airdcpp/events/LogManager.h>
#include <airdcpp/util/PathUtil.h>
#include <airdcpp/queue/QueueManager.h>

#include <web-server/ContextMenuManager.h>
#include <web-server/WebServerManager.h>

namespace wingui {
string UsersFrame::id = "Users";

int UsersFrame::columnIndexes[] = { COLUMN_FAVORITE, COLUMN_SLOT, COLUMN_NICK, COLUMN_HUB, COLUMN_SEEN, COLUMN_QUEUED, COLUMN_DESCRIPTION, COLUMN_LIMITER, COLUMN_IGNORE, COLUMN_SHARED, COLUMN_TAG, COLUMN_IP4, COLUMN_IP6 };
int UsersFrame::columnSizes[] = { 60, 90, 200, 300, 150, 100, 200, 70, 60,80, 250, 120, 120 };

static ResourceManager::Strings columnNames[] = { ResourceManager::FAVORITE, ResourceManager::AUTO_GRANT_SLOT, ResourceManager::NICK, ResourceManager::LAST_HUB, ResourceManager::LAST_SEEN, ResourceManager::QUEUED, 
	ResourceManager::DESCRIPTION, ResourceManager::OVERRIDE_LIMITER, ResourceManager::SETTINGS_IGNORE, ResourceManager::SHARED, ResourceManager::TAG, ResourceManager::IP_V4, ResourceManager::IP_V6 };
static ColumnType columnTypes [] = { COLUMN_IMAGE, COLUMN_IMAGE, COLUMN_TEXT, COLUMN_TEXT, COLUMN_TEXT, COLUMN_NUMERIC_OTHER, COLUMN_TEXT, COLUMN_TEXT, COLUMN_TEXT, COLUMN_TEXT, COLUMN_TEXT, COLUMN_TEXT };

struct FieldName {
	string field;
	tstring name;
	tstring (*convert)(const string &val);
};
static tstring formatBytes(const string& val) {
	return Util::formatBytesW(Util::toInt64(val));
}

static tstring formatSpeed(const string& val) {
	return FormatUtil::formatConnectionSpeedW(Util::toInt64(val));
}

static const FieldName fields[] =
{
	{ "NI", _T("Nick: "), &Text::toT },
	{ "AW", _T("Away: "), &Text::toT },
	{ "DE", _T("Description: "), &Text::toT },
	{ "EM", _T("E-Mail: "), &Text::toT },
	{ "SS", _T("Shared: "), &formatBytes },
	{ "SF", _T("Shared files: "), &Text::toT },
	{ "US", _T("Upload speed: "), &formatSpeed },
	{ "DS", _T("Download speed: "), &formatSpeed },
	{ "SL", _T("Total slots: "), &Text::toT },
	{ "FS", _T("Free slots: "), &Text::toT },
	{ "HN", _T("Hubs (normal): "), &Text::toT },
	{ "HR", _T("Hubs (registered): "), &Text::toT },
	{ "HO", _T("Hubs (op): "), &Text::toT },
	{ "I4", _T("IP (v4): "), &Text::toT },
	{ "I6", _T("IP (v6): "), &Text::toT },
	{ "U4", _T("Search port (v4): "), &Text::toT },
	{ "U6", _T("Search port (v6): "), &Text::toT },
	{ "SU", _T("Features: "), &Text::toT },
	{ "VE", _T("Application version: "), &Text::toT },
	{ "AP", _T("Application: "), &Text::toT },
	{ "ID", _T("CID: "), &Text::toT },
	{ "KP", _T("TLS Keyprint: "), &Text::toT },
	{ "CO", _T("Connection: "), &Text::toT },
	//{ "CT", _T("Client type: "), &Text::toT },
	{ "TA", _T("Tag: "), &Text::toT },
	{ "", _T(""), 0 }
};

UsersFrame::UsersFrame() : closed(false), startup(true), 
	ctrlShowInfoContainer(WC_BUTTON, this, STATUS_MAP), 
	showInfo(SETTING(FAV_USERS_SHOW_INFO)),
	listFav(SETTING(USERS_FILTER_FAVORITE)),
	filterQueued(SETTING(USERS_FILTER_QUEUE)),
	filterOnline(SETTING(USERS_FILTER_ONLINE)),
	filterIgnored(SETTING(USERS_FILTER_IGNORE)),
	filter(COLUMN_LAST, [this] { updateList(); })
{ }

LRESULT UsersFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	ctrlUsers.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE, IDC_USERS);
	ctrlUsers.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_SUBITEMIMAGES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
	
	ctrlInfo.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE, WS_EX_CLIENTEDGE);
	ctrlInfo.Subclass();
	ctrlInfo.SetReadOnly(TRUE);
	ctrlInfo.SetFont(WinUtil::font);
	ctrlInfo.SetBackgroundColor(WinUtil::bgColor); 
	ctrlInfo.SetDefaultCharFormat(WinUtil::m_ChatTextGeneral);

	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
	SetSplitterPanes(ctrlUsers.m_hWnd, ctrlInfo.m_hWnd);
	SetSplitterPosPct(SETTING(FAV_USERS_SPLITTER_POS) / 100);

	images.Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 2);
	images.AddIcon(CIcon(ResourceLoader::loadIcon(IDI_FAV_USER, 16)));
	images.AddIcon(CIcon(ResourceLoader::loadIcon(IDI_FAV_USER_OFF, 16)));
	images.AddIcon(CIcon(ResourceLoader::loadIcon(IDI_USER_ON, 16)));
	images.AddIcon(CIcon(ResourceLoader::loadIcon(IDI_USER_OFF, 16)));
	images.AddIcon(CIcon(ResourceLoader::loadIcon(IDI_GRANT_ON, 16)));
	images.AddIcon(CIcon(ResourceLoader::loadIcon(IDI_GRANT_OFF, 16)));

	ctrlUsers.SetImageList(images, LVSIL_SMALL);
	
	ctrlUsers.SetFont(WinUtil::listViewFont); //this will also change the columns font
	ctrlUsers.SetBkColor(WinUtil::bgColor);
	ctrlUsers.SetTextBkColor(WinUtil::bgColor);
	ctrlUsers.SetTextColor(SETTING(NORMAL_COLOUR)); //user list color for normal user
	ctrlUsers.noDefaultItemImages = true;

	ctrlShowInfo.Create(ctrlStatus.m_hWnd, rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_SHOW_INFO);
	ctrlShowInfo.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlShowInfo.SetFont(WinUtil::systemFont);
	ctrlShowInfo.SetCheck(showInfo ? BST_CHECKED : BST_UNCHECKED);
	
	ctrlShowFav.Create(ctrlStatus.m_hWnd, rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_SHOW_FAV);
	ctrlShowFav.SetWindowText(CTSTRING(FAVORITE_USERS));
	ctrlShowFav.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlShowFav.SetFont(WinUtil::systemFont);
	ctrlShowFav.SetCheck(listFav ? BST_CHECKED : BST_UNCHECKED);

	ctrlShowQueued.Create(ctrlStatus.m_hWnd, rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_FILTER_QUEUED);
	ctrlShowQueued.SetWindowText(CTSTRING(QUEUED_BUNDLES));
	ctrlShowQueued.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlShowQueued.SetFont(WinUtil::systemFont);
	ctrlShowQueued.SetCheck(filterQueued ? BST_CHECKED : BST_UNCHECKED);

	ctrlShowOnline.Create(ctrlStatus.m_hWnd, rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_SHOW_ONLINE);
	ctrlShowOnline.SetWindowText(CTSTRING(ONLINE));
	ctrlShowOnline.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlShowOnline.SetFont(WinUtil::systemFont);
	ctrlShowOnline.SetCheck(filterOnline ? BST_CHECKED : BST_UNCHECKED);

	ctrlShowIgnored.Create(ctrlStatus.m_hWnd, rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_FILTER_IGNORED);
	ctrlShowIgnored.SetWindowText(CTSTRING(IGNORED_USERS));
	ctrlShowIgnored.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlShowIgnored.SetFont(WinUtil::systemFont);
	ctrlShowIgnored.SetCheck(filterIgnored ? BST_CHECKED : BST_UNCHECKED);

	ctrlShowInfoContainer.SubclassWindow(ctrlStatus.m_hWnd);

	ctrlTooltips.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON, WS_EX_TOPMOST);	 
	ctrlTooltips.SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);	  
	CToolInfo ti(TTF_SUBCLASS, ctrlShowInfo.m_hWnd);
	ti.cbSize = sizeof(CToolInfo);
	ti.lpszText = (LPWSTR)CTSTRING(SHOW_HIDE_INFORMATION);
	ctrlTooltips.AddTool(&ti);
	ctrlTooltips.SetDelayTime(TTDT_AUTOPOP, 15000);
	ctrlTooltips.Activate(TRUE);
	
	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(USERS_FRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(USERS_FRAME_WIDTHS), COLUMN_LAST);
	
	for(uint8_t j=0; j<COLUMN_LAST; j++) {
		ctrlUsers.InsertColumn(j, CTSTRING_I(columnNames[j]), LVCFMT_LEFT, columnSizes[j], j, columnTypes[j]);
	}
	
	ctrlUsers.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlUsers.setVisible(SETTING(USERS_FRAME_VISIBLE));
	ctrlUsers.setSortColumn(COLUMN_NICK);

	ctrlUsers.addCopyHandler(COLUMN_IP4, &ColumnInfo::filterCountry);
	ctrlUsers.addCopyHandler(COLUMN_IP6, &ColumnInfo::filterCountry);

	filter.addFilterBox(ctrlStatus.m_hWnd);
	filter.addColumnBox(ctrlStatus.m_hWnd, ctrlUsers.getColumnList());
	filter.addMethodBox(ctrlStatus.m_hWnd);

	auto cm = ClientManager::getInstance();
	{
		RLock l(cm->getCS());
		for (auto& u : cm->getUsersUnsafe() | views::values) {
			if (u->getCID() == CID()) // hub
				continue;
			userInfos.emplace(u, UserInfo(u, Util::emptyString, false));
		}
	}

	// outside the lock
	for (auto& ui: userInfos | views::values)
		ui.update(ui.getUser());

	updateList();

	FavoriteUserManager::getInstance()->addListener(this);
	ClientManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);
	IgnoreManager::getInstance()->addListener(this);

	CRect rc(SETTING(USERS_LEFT), SETTING(USERS_TOP), SETTING(USERS_RIGHT), SETTING(USERS_BOTTOM));
	if(! (rc.top == 0 && rc.bottom == 0 && rc.left == 0 && rc.right == 0) )
		MoveWindow(rc, TRUE);

	WinUtil::SetIcon(m_hWnd, IDI_USERS);

	startup = false;
	updateStatus();

	ctrlUsers.addClickHandler(COLUMN_FAVORITE, bind(&UsersFrame::handleClickFavorite, this, placeholders::_1), false);
	ctrlUsers.addClickHandler(COLUMN_SLOT, bind(&UsersFrame::handleClickSlot, this, placeholders::_1), false);
	ctrlUsers.addClickHandler(COLUMN_LIMITER, bind(&UsersFrame::handleClickLimiter, this, placeholders::_1), true);
	ctrlUsers.addClickHandler(COLUMN_DESCRIPTION, bind(&UsersFrame::handleClickDesc, this, placeholders::_1), true);
	ctrlUsers.addClickHandler(COLUMN_IGNORE, bind(&UsersFrame::handleClickIgnore, this, placeholders::_1), true);

	::SetTimer(m_hWnd, 0, 500, 0);

	bHandled = FALSE;
	return TRUE;
}

LRESULT UsersFrame::onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if (statusDirty) {
		updateStatus();
		statusDirty = false;
	}
	return 0;
}

LRESULT UsersFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) != ctrlUsers || ctrlUsers.GetSelectedCount() <= 0) {
		bHandled = FALSE;
		return FALSE;
	}

	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	if(pt.x == -1 && pt.y == -1) {
		WinUtil::getContextMenuPos(ctrlUsers, pt);
	}

	UserInfo* ui = nullptr;
	bool isMe = false;
	if (ctrlUsers.GetSelectedCount() == 1) {
		ui = ctrlUsers.getItemData(WinUtil::getFirstSelectedIndex(ctrlUsers));
		isMe = ui->getUser() == ClientManager::getInstance()->getMe();
	}

	int row = ctrlUsers.GetNextItem(-1, LVNI_SELECTED);
	
	OMenu usersMenu;
	usersMenu.CreatePopupMenu();
	ctrlUsers.appendCopyMenu(usersMenu);

	if (!isMe) {
		usersMenu.AppendMenu(MF_STRING, IDC_OPEN_USER_LOG, CTSTRING(OPEN_USER_LOG));
		usersMenu.appendSeparator();
		appendUserItems(usersMenu, true, ui ? ui->getUser() : nullptr, ui && !ui->getHubUrl().empty());

		if (ui) {
			usersMenu.appendSeparator();
			if (!ui->getUser()->isIgnored()) {
				usersMenu.appendItem(TSTRING(IGNORE_USER), [=] { handleClickIgnore(row); });
			} else {
				usersMenu.appendItem(TSTRING(UNIGNORE_USER), [=] { handleClickIgnore(row); });
			}

			Bundle::SourceBundleList sourceBundles, badSourceBundles;
			QueueManager::getInstance()->getSourceInfo(ui->getUser(), sourceBundles, badSourceBundles);

			if (!sourceBundles.empty() || !badSourceBundles.empty()) {
				usersMenu.appendSeparator();

				auto formatBundle = [this](pair<BundlePtr, Bundle::BundleSource>& bs) -> tstring {
					return Text::toT(bs.first->getName()) + _T(" (") + WinUtil::toStringW(bs.second.files) + _T(" ") + TSTRING(FILES) + _T(", ") + Util::formatBytesW(bs.second.size) + _T(")");
				};

				//current sources
				auto removeMenu = usersMenu.createSubMenu(TSTRING(REMOVE_SOURCE), true);
				if (!sourceBundles.empty()) {
					for (auto& bs : sourceBundles) {
						removeMenu->appendItem(formatBundle(bs), [=] {
							QueueManager::getInstance()->removeBundleSource(bs.first, bs.second.getUser(), QueueItem::Source::FLAG_REMOVED);
						}, OMenu::FLAG_THREADED);
					}

					removeMenu->appendSeparator();
					removeMenu->appendItem(TSTRING(ALL), [=] {
						for (auto& bs : sourceBundles) {
							QueueManager::getInstance()->removeBundleSource(bs.first, bs.second.getUser().user, QueueItem::Source::FLAG_REMOVED);
						}
					}, OMenu::FLAG_THREADED);
				}

				//bad sources
				auto readdMenu = usersMenu.createSubMenu(TSTRING(READD_SOURCE), true);
				if (!badSourceBundles.empty()) {
					for (auto& bs : badSourceBundles) {
						readdMenu->appendItem(formatBundle(bs), [=] {
							QueueManager::getInstance()->readdBundleSourceHooked(bs.first, bs.second.getUser());
						}, OMenu::FLAG_THREADED);
					}

					readdMenu->appendSeparator();
					readdMenu->appendItem(TSTRING(ALL), [=] {
						for (auto& bs : badSourceBundles) {
							QueueManager::getInstance()->readdBundleSourceHooked(bs.first, bs.second.getUser());
						}
					}, OMenu::FLAG_THREADED);
				}
			}

			if (ui->isFavorite) {
				usersMenu.appendSeparator();
				usersMenu.appendItem(TSTRING(OVERRIDE_LIMITER), [=] { handleClickLimiter(row); }, ui->noLimiter ? OMenu::FLAG_CHECKED : 0);
				usersMenu.appendItem(TSTRING(PROPERTIES), [=] { handleClickDesc(row); });

				usersMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
			}
		}
	}
		
	if (ui)
		usersMenu.InsertSeparatorFirst(ui->columns[COLUMN_NICK]);
		
	usersMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);

	return TRUE; 
}

LRESULT UsersFrame::onCustomDrawList(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {

	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;
	switch(cd->nmcd.dwDrawStage) {

		case CDDS_PREPAINT:
			return CDRF_NOTIFYITEMDRAW;

		case CDDS_ITEMPREPAINT: {
			UserInfo *ui = reinterpret_cast<UserInfo*>(cd->nmcd.lItemlParam);
			if (!listFav && (ui != NULL) && ui->isFavorite) {
				cd->clrText = SETTING(FAVORITE_COLOR);
			}

			return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
		}
		
		case CDDS_SUBITEM | CDDS_ITEMPREPAINT: 
		{
			// dim fields that can't be changed for this user
			auto ui = reinterpret_cast<UserInfo*>(cd->nmcd.lItemlParam);
			if (!ui)
				return CDRF_DODEFAULT;

			//either this or it needs to be owner drawn
			if (!listFav && ui->isFavorite)
				cd->clrText = SETTING(FAVORITE_COLOR);
			else 
				cd->clrText = SETTING(NORMAL_COLOUR);

			int colIndex = ctrlUsers.findColumn(cd->iSubItem);
			if (!ui->isFavorite && colIndex == COLUMN_LIMITER) {
				cd->clrText = WinUtil::blendColors(SETTING(NORMAL_COLOUR), SETTING(BACKGROUND_COLOR));
			}
			else if (SETTING(GET_USER_COUNTRY) && (colIndex == COLUMN_IP4 || colIndex == COLUMN_IP6)) {
				CRect rc;
				ctrlUsers.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, rc);

				SetTextColor(cd->nmcd.hdc, cd->clrText);
				DrawThemeBackground(GetWindowTheme(ctrlUsers.m_hWnd), cd->nmcd.hdc, LVP_LISTITEM, 3, &rc, &rc);

				TCHAR buf[256];
				ctrlUsers.GetItemText((int)cd->nmcd.dwItemSpec, cd->iSubItem, buf, 255);
				buf[255] = 0;
				if (_tcslen(buf) > 0) {
					rc.left += 2;
					LONG top = rc.top + (rc.Height() - 15) / 2;
					if ((top - rc.top) < 2)
						top = rc.top + 1;

					POINT p = { rc.left, top };

					auto countryInfo = FormatUtil::toCountryInfo(ui->getIp());
					ResourceLoader::flagImages.Draw(cd->nmcd.hdc, countryInfo.flagIndex, p, LVSIL_SMALL);

					top = rc.top + (rc.Height() - WinUtil::getTextHeight(cd->nmcd.hdc) - 1) / 2;
					::ExtTextOut(cd->nmcd.hdc, rc.left + 30, top + 1, ETO_CLIPPED, rc, buf, _tcslen(buf), NULL);
					return CDRF_SKIPDEFAULT;
				}
			}
			return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
		}
		default:
			return CDRF_DODEFAULT;
	}
}

void UsersFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
		
	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[2];
		ctrlStatus.GetClientRect(sr);
			
		w[0] = sr.right - (sr.Width() / 4) -16;
		w[1] = w[0] + (sr.Width() / 4);
			
		ctrlStatus.SetParts(2, w);

		ctrlStatus.GetRect(0, sr);
		//filters
		sr.left += 5;
		sr.right = sr.left + 130;
		filter.getFilterBox().MoveWindow(sr);
		sr.left  = sr.right + 4;
		sr.right = sr.left + 100;
		filter.getFilterColumnBox().MoveWindow(sr);
		sr.left  = sr.right + 4;
		sr.right = sr.left + 100;
		filter.getFilterMethodBox().MoveWindow(sr);

		sr.left = sr.right +4;
		sr.right = sr.left + ctrlShowFav.GetWindowTextLength() * WinUtil::getTextWidth(ctrlShowFav.m_hWnd, WinUtil::systemFont) + 20;
		ctrlShowFav.MoveWindow(sr);

		sr.left = sr.right + 4;
		sr.right = sr.left + ctrlShowQueued.GetWindowTextLength() * WinUtil::getTextWidth(ctrlShowQueued.m_hWnd, WinUtil::systemFont) + 24;
		ctrlShowQueued.MoveWindow(sr);

		sr.left = sr.right + 8;
		sr.right = sr.left + ctrlShowOnline.GetWindowTextLength() * WinUtil::getTextWidth(ctrlShowOnline.m_hWnd, WinUtil::systemFont) + 24;
		ctrlShowOnline.MoveWindow(sr);

		sr.left = sr.right + 8;
		sr.right = sr.left + ctrlShowIgnored.GetWindowTextLength() * WinUtil::getTextWidth(ctrlShowIgnored.m_hWnd, WinUtil::systemFont) + 24;
		ctrlShowIgnored.MoveWindow(sr);
		
		ctrlStatus.GetRect(1, sr);
		sr.left = sr.right;
		sr.right = sr.left + 16;
		ctrlShowInfo.MoveWindow(sr);
	}
	
	if(!showInfo) {
		if(GetSinglePaneMode() == SPLIT_PANE_NONE)
			SetSinglePaneMode(SPLIT_PANE_LEFT);
	} else {
		if(GetSinglePaneMode() != SPLIT_PANE_NONE)
			SetSinglePaneMode(SPLIT_PANE_NONE);
	}
	CRect rc = rect;
	ctrlUsers.MoveWindow(rc);
	SetSplitterRect(rc);
}

LRESULT UsersFrame::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	vector<UserInfo*> removeList;
	int i = -1;
	while( (i =  ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		UserInfo *ui = ctrlUsers.getItemData(i);
		if(ui->isFavorite) {
			removeList.push_back(ui);
		}
	}
	for(auto& u: removeList){
		u->remove();
	}
	return 0;
}

LRESULT UsersFrame::onShow(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	switch(wID) {
		case IDC_SHOW_INFO: {
			showInfo = !showInfo;
			SettingsManager::getInstance()->set(SettingsManager::FAV_USERS_SHOW_INFO, showInfo);
			UpdateLayout(FALSE);
			if(ctrlUsers.GetSelectedCount() == 1)
				updateInfoText(ctrlUsers.getSelectedItem());
			break;
		}
		case IDC_SHOW_FAV: {
			listFav = !listFav;
			updateList();
			SettingsManager::getInstance()->set(SettingsManager::USERS_FILTER_FAVORITE, listFav);
			break;
		}
		case IDC_FILTER_QUEUED: {
			filterQueued = !filterQueued;
			updateList();
			SettingsManager::getInstance()->set(SettingsManager::USERS_FILTER_QUEUE, filterQueued);
			break;
		}
		case IDC_SHOW_ONLINE: {
			filterOnline = !filterOnline;
			updateList();
			SettingsManager::getInstance()->set(SettingsManager::USERS_FILTER_ONLINE, filterOnline);
			break;
		}

		case IDC_FILTER_IGNORED: {
			filterIgnored = !filterIgnored;
			updateList();
			SettingsManager::getInstance()->set(SettingsManager::USERS_FILTER_IGNORE, filterIgnored);
			break;
		}
	}
	return 0;
}

LRESULT UsersFrame::onItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	if(startup)
		return 0;

	NMITEMACTIVATE* l = (NMITEMACTIVATE*)pnmh;
	 if ( (l->uChanged & LVIF_STATE) && (l->uNewState & LVIS_SELECTED) != (l->uOldState & LVIS_SELECTED) ) {
		if (l->uNewState & LVIS_SELECTED){
			if(l->iItem != -1)
				updateInfoText(ctrlUsers.getItemData(l->iItem));
		} else { //deselected
			ctrlInfo.SetWindowText(_T(""));
		}
	 }
	bHandled = FALSE;
  	return 0;
}

bool UsersFrame::handleClickIgnore(int row) {
	auto ui = ctrlUsers.getItemData(row);
	if (ui){
		if (ui->getUser()->isIgnored())
			IgnoreManager::getInstance()->removeIgnore(ui->getUser());
		else
			IgnoreManager::getInstance()->storeIgnore(ui->getUser());
	}
	return true;
}

bool UsersFrame::handleClickDesc(int row) {
	auto ui = ctrlUsers.getItemData(row);
	if (!ui->isFavorite)
		return true;

	LineDlg dlg;
	dlg.description = TSTRING(DESCRIPTION);
	dlg.title = ui->columns[COLUMN_NICK];
	dlg.line = ui->columns[COLUMN_DESCRIPTION];
	if (dlg.DoModal(m_hWnd)) {
		FavoriteUserManager::getInstance()->setUserDescription(ui->user, Text::fromT(dlg.line));
		ui->columns[COLUMN_DESCRIPTION] = dlg.line;
		ctrlUsers.updateItem(row);
	}
	return true;
}

bool UsersFrame::handleClickSlot(int row) {
	auto ui = ctrlUsers.getItemData(row);
	ui->grantSlot = !ui->grantSlot;
	if (ui->isFavorite) {
		FavoriteUserManager::getInstance()->setAutoGrant(ui->getUser(), ui->grantSlot);
		setImages(ui, row);
	} else {
		ui->grantSlot ? ui->grantTimeless() : ui->ungrant(); // timeless grant
	}
	return true;
}

bool UsersFrame::handleClickFavorite(int row) {
	auto ui = ctrlUsers.getItemData(row);
	if (ui->isFavorite)
		FavoriteUserManager::getInstance()->removeFavoriteUser(ui->getUser());
	else
		FavoriteUserManager::getInstance()->addFavoriteUser(HintedUser(ui->getUser(), ui->getHubUrl()));
	return true;
}

bool UsersFrame::handleClickLimiter(int row) {
	auto ui = ctrlUsers.getItemData(row);
	if (ui->isFavorite) {
		FavoriteUserManager::getInstance()->changeLimiterOverride(ui->getUser());
		updateUser(ui->getUser());
		//return true;
	}
	return true;
}

LRESULT UsersFrame::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*) pnmh;

	if(item->iItem != -1) {
	    switch(SETTING(USERLIST_DBLCLICK)) {
		    case 0:
				ctrlUsers.getItemData(item->iItem)->getList();
		        break;
		    case 1: break; //public message
		    case 2:
				ctrlUsers.getItemData(item->iItem)->pm();
		        break;
		    case 3:
		        ctrlUsers.getItemData(item->iItem)->matchQueue();
		        break;
		    case 4:
		        ctrlUsers.getItemData(item->iItem)->grant();
		        break;
		    case 5: break; //already a favorite user
			case 6:
				ctrlUsers.getItemData(item->iItem)->browseList();
				break;
		}	
	} else {
		bHandled = FALSE;
	}

	return 0;
}

LRESULT UsersFrame::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	switch(kd->wVKey) {
	case VK_DELETE:
		PostMessage(WM_COMMAND, IDC_REMOVE, 0);
		break;
	case VK_RETURN:
		PostMessage(WM_COMMAND, IDC_EDIT, 0);
		break;
	default:
		bHandled = FALSE;
	}
	return 0;
}

int UsersFrame::UserInfo::compareItems(const UserInfo* a, const UserInfo* b, int col) {
	switch(col) {
	case COLUMN_LIMITER: if (a->isFavorite == b->isFavorite) return compare(a->noLimiter, b->noLimiter);
	case COLUMN_FAVORITE: return compare(a->isFavorite, b->isFavorite);
	case COLUMN_SLOT: return compare(a->grantSlot, b->grantSlot);
	case COLUMN_QUEUED: return compare(a->user->getQueued(), b->user->getQueued());
	case COLUMN_IGNORE: return compare(a->user->isIgnored(), b->user->isIgnored());
	case COLUMN_SHARED: return compare(a->getShareSize(), b->getShareSize()); //Highest share size is saved for comparing
	default: return Util::DefaultSort(a->columns[col].c_str(), b->columns[col].c_str());
	}
}

void UsersFrame::updateInfoText(const UserInfo* ui){
	if(!showInfo)
		return;

	auto ouList = ClientManager::getInstance()->getOnlineUsers(ui->getUser());
	if (!ouList.empty()) {
		tstring tmp = _T("");
		for (const auto& ou: ouList) {
			// const auto& ident = ou->getIdentity();
			auto info = ou->getIdentity().getInfo();
		
			tmp += _T("\r\nUser Information: \r\n");
			tmp += _T("Hub: ") + Text::toT(ou->getHubUrl()) + _T("\r\n");
			for(auto f = fields; !f->field.empty(); ++f) {
				auto i = info.find(f->field);
				if(i != info.end()) {
					tmp += f->name + f->convert(i->second) + _T("\r\n");
					info.erase(i);
				}
			}
		}
		ctrlInfo.SetWindowText(tmp.c_str());
	} else
		ctrlInfo.SetWindowText(_T(""));
}

void UsersFrame::addUser(const UserPtr& aUser, const string& aUrl) {
	if(!show(aUser, true)) {
		return;
	}

	auto ui = userInfos.find(aUser);
	if(ui == userInfos.end()) {
		auto x = userInfos.emplace(aUser, UserInfo(aUser, aUrl)).first;
		if(matches(x->second)) {
			ctrlUsers.insertItem(&x->second, 0);
		}
	} else {
		updateUser(aUser);
	}

	statusDirty = true;
}

void UsersFrame::updateUser(const UserPtr& aUser) {
	auto i = userInfos.find(aUser);
	if (i == userInfos.end()) {
		return;
	}

	auto ui = &i->second;
	ui->update(aUser);
	if(!show(aUser, false)) {
		ctrlUsers.deleteItem(ui);
		if(!show(aUser, true)) {
			userInfos.erase(aUser);
		}
		return;
	}

	if(matches(*ui)) {
		int pos = ctrlUsers.findItem(ui);
		if(pos != -1) {
			ctrlUsers.updateItem(pos);
		} else {
			pos = ctrlUsers.insertItem(ui, 0);
		}

		setImages(ui, pos);
	}

	statusDirty = true;
}

void UsersFrame::updateList() {
	ctrlUsers.SetRedraw(FALSE);
	ctrlUsers.DeleteAllItems();
	ctrlInfo.SetWindowText(_T(""));

	auto i = userInfos.begin();
	auto filterInfoF = [this, &i](int column) { return Text::fromT(i->second.getText(column)); };
	auto filterNumericF = [&](int column) -> double {
		switch (column) {
		case COLUMN_QUEUED: return i->first->getQueued();
		default: dcassert(0); return 0;
		}
	};

	auto filterPrep = filter.prepare(filterInfoF, filterNumericF);

	for(; i != userInfos.end(); ++i) {
		if ((filter.empty() || filter.match(filterPrep)) && show(i->second.getUser(), false)) {
			int p = ctrlUsers.insertItem(&i->second,0);
			setImages(&i->second, p);
			ctrlUsers.updateItem(p);
		}
	}
	ctrlUsers.SetRedraw(TRUE);
	ctrlUsers.resort();

	statusDirty = true;
}

bool UsersFrame::matches(const UserInfo &ui) {
	auto filterNumericF = [&](int column) -> double {
		switch (column) {
		case COLUMN_QUEUED: return ui.getUser()->getQueued();
		default: dcassert(0); return 0;
		}
	};

	if (!filter.empty() && !filter.match(filter.prepare([this, &ui](int column) { return Text::fromT(ui.getText(column)); }, filterNumericF))) {
		return false;
	}

	return show(ui.getUser(), false);
}

static bool isFav(const UserPtr &u) { return u->isFavorite(); }

bool UsersFrame::show(const UserPtr &u, bool any) const {
	if(any && (u->isOnline() || isFav(u) || u->getQueued())) {
		return true;
	} else if(filterOnline && !u->isOnline()){
		return false;
	} else if(filterQueued && u->getQueued() == 0){
		return false;
	} else if (filterIgnored && !u->isIgnored()){
		return false;
	} else if(listFav && !isFav(u)){
		return false;
	}
	return true;
}

void UsersFrame::setImages(const UserInfo *ui, int pos/* = -1*/) {
	if(pos == -1)
		pos = ctrlUsers.findItem(ui);

	ctrlUsers.SetItem(pos, ctrlUsers.findColumnIndex(COLUMN_FAVORITE), LVIF_IMAGE, NULL, ui->getImage(COLUMN_FAVORITE), 0, 0, NULL);
	ctrlUsers.SetItem(pos, ctrlUsers.findColumnIndex(COLUMN_SLOT), LVIF_IMAGE, NULL, ui->getImage(COLUMN_SLOT), 0, 0, NULL);
	ctrlUsers.SetItem(pos, ctrlUsers.findColumnIndex(COLUMN_SEEN), LVIF_IMAGE, NULL, ui->getImage(COLUMN_SEEN), 0, 0, NULL);

}
void UsersFrame::updateStatus() {
	ctrlStatus.SetText(1, TSTRING_F(X_USERS, ctrlUsers.GetItemCount()).c_str());
}


UsersFrame::UserInfo::UserInfo(const UserPtr& u, const string& aUrl, bool updateInfo) : user(u), hubUrl(aUrl), isFavorite(false), grantSlot(false) {
	if (updateInfo)
		update(user);
}

void UsersFrame::UserInfo::update(const UserPtr& u) {
	auto fu = !u->isSet(User::FAVORITE) ? nullopt : FavoriteUserManager::getInstance()->getFavoriteUser(u);
	if (fu) {
		isFavorite = true;
		noLimiter = fu->isSet(FavoriteUser::FLAG_SUPERUSER);
		grantSlot = fu->isSet(FavoriteUser::FLAG_GRANTSLOT);

		if (fu->getUrl().empty()) fu->setUrl(hubUrl);
		setHubUrl(fu->getUrl());
	} else {
		noLimiter = false;
		isFavorite = false;
		grantSlot = hasReservedSlot();
	}

	//gets nicks and hubnames and updates the hint url
	string url = getHubUrl();
	auto ui = std::move(getUserInfo(u, url));
	setHubUrl(url);

	if (fu) {
		columns[COLUMN_NICK] = u->isOnline() ? Text::toT(ui.Nicks) : fu->getNick().empty() ? Text::toT(ui.Nicks) : Text::toT(fu->getNick());
		columns[COLUMN_HUB] = u->isOnline() ? Text::toT(ui.Hubs) : Text::toT(fu->getUrl()); 
		columns[COLUMN_SEEN] = u->isOnline() ? TSTRING(ONLINE) : fu->getLastSeen() ? FormatUtil::formatDateTimeW(fu->getLastSeen()) : Text::toT(ui.lastSeen);
		columns[COLUMN_DESCRIPTION] = Text::toT(fu->getDescription());
		columns[COLUMN_LIMITER] = noLimiter ? TSTRING(YES) : TSTRING(NO);
	} else {
		columns[COLUMN_NICK] = Text::toT(ui.Nicks);
		columns[COLUMN_HUB] = Text::toT(ui.Hubs);
		columns[COLUMN_SEEN] = Text::toT(ui.lastSeen);
		columns[COLUMN_DESCRIPTION] = Util::emptyStringT;
		columns[COLUMN_LIMITER] = _T("-");
	}

	columns[COLUMN_SHARED] = Text::toT(ui.Shared);
	columns[COLUMN_TAG] = Text::toT(ui.Tag);
	columns[COLUMN_IP4] = Text::toT(ui.ip4);
	columns[COLUMN_IP6] = Text::toT(ui.ip6);
	columns[COLUMN_QUEUED] = Util::formatBytesW(u->getQueued());
	columns[COLUMN_IGNORE] = u->isIgnored() ? TSTRING(YES) : TSTRING(NO);
}


UsersFrame::UserInfo::userData UsersFrame::UserInfo::getUserInfo(const UserPtr& aUser, string& hint_) {
	OnlineUserList ouList;
	string nick = Util::emptyString;
	string hubs = Util::emptyString;
	string shared = Util::emptyString;
	string tag = Util::emptyString;
	string ip4 = Util::emptyString;
	string ip6 = Util::emptyString;
	string seen = aUser->isOnline() ? STRING(ONLINE) : Util::emptyString;

	if(user->isOnline()) {
		auto hinted = ClientManager::getInstance()->getOnlineUsers(HintedUser(aUser, hint_), ouList);
		if (!ouList.empty() && !hinted) { //set the hint to match the first nick
			auto i = ouList.begin();
			hinted = *i;
			ouList.erase(i);
			hint_ = hinted->getHubUrl();
		}

		hubs = hinted ? OnlineUser::HubName()(hinted)+" " : Util::emptyString;
		if (!ouList.empty())
			hubs += Util::listToStringT<OnlineUserList, OnlineUser::HubName>(ouList, hinted ? true : false, hinted ? false : true);

		//add share sizes
		auto ouList2 = ouList;
		//well this is kind of stupid but, save the highest share size for comparing...
		setShareSize(hinted ? hinted->getIdentity().getBytesShared() : 0);

		shared = hinted ? Util::formatBytes(hinted->getIdentity().getBytesShared()) + " " : Util::emptyString;
		ouList2.erase(unique(ouList2.begin(), ouList2.end(), [](const OnlineUserPtr& a, const OnlineUserPtr& b) { return compare(a->getIdentity().getBytesShared(), b->getIdentity().getBytesShared()) == 0; }), ouList2.end());
		ouList2.erase(remove_if(ouList2.begin(), ouList2.end(), [&](const OnlineUserPtr& a) { return compare(a->getIdentity().getBytesShared(), hinted->getIdentity().getBytesShared()) == 0; }), ouList2.end());
		if (!ouList2.empty()) {
			shared += "(";
			for (auto x : ouList2) {
				if (x->getIdentity().getBytesShared() > getShareSize())
					setShareSize(x->getIdentity().getBytesShared());
				shared += Util::formatBytes(x->getIdentity().getBytesShared()) + ", ";
			}

			shared.pop_back();
			shared[shared.length() - 1] = ')';
		}

		ouList.erase(unique(ouList.begin(), ouList.end(), [](const OnlineUserPtr& a, const OnlineUserPtr& b) { return compare(OnlineUser::Nick()(a), OnlineUser::Nick()(b)) == 0; }), ouList.end());
		if (hinted) {
			//erase users with the hinted nick
			auto p = equal_range(ouList.begin(), ouList.end(), hinted, OnlineUser::NickSort());
			ouList.erase(p.first, p.second);
		}

		nick = hinted ? OnlineUser::Nick()(hinted)+" " : Util::emptyString;
		if (!ouList.empty())
			nick += Util::listToStringT<OnlineUserList, OnlineUser::Nick>(ouList, hinted ? true : false, hinted ? false : true);

		if (hinted) {
			setIp(hinted->getIdentity().getTcpConnectIp());
			ip4 = hinted->getIdentity().getIp4();
			ip6 = hinted->getIdentity().getIp6();
			tag = hinted->getIdentity().getTag();

			const string& country = hinted->getIdentity().getCountry();
			if (!country.empty()) {
				if(!ip4.empty())
					ip4 = country + " (" + ip4 + ")";
				if (!ip6.empty())
					ip6 = country + " (" + ip6 + ")";
			}
		}
	}
	
	if (nick.empty()) {
		//offline
		auto ofu = ClientManager::getInstance()->getOfflineUser(aUser->getCID());
		if (ofu) {
			nick = ofu->getNick();
			hubs = ofu->getUrl();
			seen = ofu->getLastSeen() ? FormatUtil::formatDateTime(ofu->getLastSeen()) : STRING(UNKNOWN);
			hint_ = ofu->getUrl();
		} else {
			nick = aUser->getCID().toBase32();
		}
	}

	return{ nick, hubs, shared, tag, ip4, ip6, seen };
}

int UsersFrame::UserInfo::getImage(int col) const {
	switch (col) {
		case COLUMN_FAVORITE: return isFavorite ? FAVORITE_ON_ICON : FAVORITE_OFF_ICON;
		case COLUMN_SLOT: return grantSlot ? GRANT_ON_ICON : GRANT_OFF_ICON; //todo show given extra slot
		case COLUMN_SEEN: return user->isOnline() ? USER_ON_ICON : USER_OFF_ICON;
		default: return -1;
	}
}

void UsersFrame::UserInfo::remove() { 
	FavoriteUserManager::getInstance()->removeFavoriteUser(getUser()); 
}
			
LRESULT UsersFrame::onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlUsers.GetSelectedCount() == 1) {
		int i = ctrlUsers.GetNextItem(-1, LVNI_SELECTED);
		const UserInfo* ui = ctrlUsers.getItemData(i);
		dcassert(i != -1);

		ParamMap params;
		params["hubNI"] = [=] { return Util::listToString(ClientManager::getInstance()->getHubNames(ui->getUser()->getCID())); };
		params["hubURL"] = [=] { return Util::listToString(ClientManager::getInstance()->getHubUrls(ui->getUser()->getCID())); };
		params["userCID"] = [=] { return ui->getUser()->getCID().toBase32(); };
		params["userNI"] = [=] { return ClientManager::getInstance()->getNicks(ui->getUser()->getCID())[0]; };
		params["myCID"] = [=] { return ClientManager::getInstance()->getMyCID().toBase32(); };

		string file = LogManager::getInstance()->getPath(ui->getUser(), params);
		if(PathUtil::fileExists(file)) {
			ActionUtil::viewLog(file);
		} else {
			WinUtil::showMessageBox(TSTRING(NO_LOG_FOR_USER));	  
		}	
	}
	return 0;
}

LRESULT UsersFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		FavoriteUserManager::getInstance()->removeListener(this);
		ClientManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		QueueManager::getInstance()->removeListener(this);
		IgnoreManager::getInstance()->removeListener(this);

		closed = true;
		WinUtil::setButtonPressed(IDC_FAVUSERS, false);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		CRect rc;
		if(!IsIconic()){
			//Get position of window
			GetWindowRect(&rc);
				
			//convert the position so it's relative to main window
			::ScreenToClient(GetParent(), &rc.TopLeft());
			::ScreenToClient(GetParent(), &rc.BottomRight());
				
			//save the position
			SettingsManager::getInstance()->set(SettingsManager::USERS_BOTTOM, (rc.bottom > 0 ? rc.bottom : 0));
			SettingsManager::getInstance()->set(SettingsManager::USERS_TOP, (rc.top > 0 ? rc.top : 0));
			SettingsManager::getInstance()->set(SettingsManager::USERS_LEFT, (rc.left > 0 ? rc.left : 0));
			SettingsManager::getInstance()->set(SettingsManager::USERS_RIGHT, (rc.right > 0 ? rc.right : 0));
			SettingsManager::getInstance()->set(SettingsManager::FAV_USERS_SPLITTER_POS, GetSplitterPosPct() * 100);
		}

		ctrlUsers.saveHeaderOrder(SettingsManager::USERS_FRAME_ORDER,SettingsManager::USERS_FRAME_WIDTHS,SettingsManager::USERS_FRAME_VISIBLE);
	
		ctrlUsers.DeleteAllItems();
		userInfos.clear();

		bHandled = FALSE;
		return 0;
	}
}

void UsersFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	bool refresh = false;
	if(ctrlUsers.GetBkColor() != WinUtil::bgColor) {
		ctrlUsers.SetBkColor(WinUtil::bgColor);
		ctrlUsers.SetTextBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlUsers.GetTextColor() != WinUtil::textColor) {
		ctrlUsers.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(ctrlUsers.GetFont() != WinUtil::listViewFont){
		ctrlUsers.SetFont(WinUtil::listViewFont);
		refresh = true;
	}

	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}
void UsersFrame::on(FavoriteUserManagerListener::FavoriteUserAdded, const FavoriteUser& aUser) noexcept {
	callAsync([=] { addUser(aUser.getUser(), aUser.getUrl()); } ); 
}
void UsersFrame::on(FavoriteUserManagerListener::FavoriteUserRemoved, const FavoriteUser& aUser) noexcept {
	callAsync([=] { updateUser(aUser.getUser()); } ); 
}
void UsersFrame::on(FavoriteUserManagerListener::FavoriteUserUpdated, const UserPtr& aUser) noexcept {
	callAsync([=] { updateUser(aUser); } ); 
}
void UsersFrame::on(FavoriteUserManagerListener::SlotsUpdated, const UserPtr& aUser) noexcept {
	callAsync([=] { updateUser(aUser); });
}

void UsersFrame::on(ClientManagerListener::UserConnected, const OnlineUser& aUser, bool) noexcept {
	if(aUser.getUser()->getCID() == CID())
		return;

	UserPtr user = aUser.getUser();
	string hint = aUser.getHubUrl();
	callAsync([=] { addUser(user, hint); });
}

void UsersFrame::on(ClientManagerListener::UserUpdated, const OnlineUser& aUser) noexcept {
	if(aUser.getUser()->getCID() == CID())
		return;
	UserPtr user = aUser.getUser();
	callAsync([=] { updateUser(user); });
}

void UsersFrame::on(ClientManagerListener::UserDisconnected, const UserPtr& aUser, bool) noexcept {
	if(aUser->getCID() == CID())
		return;
	callAsync([=] { updateUser(aUser); });
}

void UsersFrame::on(QueueManagerListener::SourceFilesUpdated, const UserPtr& aUser) noexcept {
	callAsync([=] { updateUser(aUser); });
}
}