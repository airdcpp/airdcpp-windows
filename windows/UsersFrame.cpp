/* 
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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

#include "MainFrm.h"
#include "ResourceLoader.h"
#include "UsersFrame.h"
#include "LineDlg.h"

#include "../client/ClientManager.h"
#include "../client/QueueManager.h"

int UsersFrame::columnIndexes[] = { COLUMN_FAVORITE, COLUMN_SLOT, COLUMN_NICK, COLUMN_HUB, COLUMN_SEEN, COLUMN_QUEUED, COLUMN_DESCRIPTION };
int UsersFrame::columnSizes[] = { 25, 25, 200, 300, 150, 100, 200 };
static ResourceManager::Strings columnNames[] = { ResourceManager::FAVORITE, ResourceManager::AUTO_GRANT_SLOT, ResourceManager::NICK, ResourceManager::LAST_HUB, ResourceManager::LAST_SEEN, ResourceManager::QUEUED, ResourceManager::DESCRIPTION };

struct FieldName {
	string field;
	tstring name;
	tstring (*convert)(const string &val);
};
static tstring formatBytes(const string& val) {
	return Text::toT(Util::formatBytes(val));
}

static tstring formatSpeed(const string& val) {
	return Text::toT(boost::str(boost::format("%1%/s") % Util::formatBytes(val)));
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
	//{ "U4", _T("Search port (v4): "), &Text::toT },
	//{ "U6", _T("Search port (v6): "), &Text::toT },
	//{ "SU", _T("Features: "), &Text::toT },
	{ "VE", _T("Application version: "), &Text::toT },
	{ "AP", _T("Application: "), &Text::toT },
	{ "ID", _T("CID: "), &Text::toT },
	//{ "KP", _T("TLS Keyprint: "), &Text::toT },
	{ "CO", _T("Connection: "), &Text::toT },
	//{ "CT", _T("Client type: "), &Text::toT },
	{ "TA", _T("Tag: "), &Text::toT },
	{ "", _T(""), 0 }
};

UsersFrame::UsersFrame() : closed(false), startup(true), 
	ctrlShowInfoContainer(WC_BUTTON, this, STATUS_MAP), 
	ctrlFilterContainer(WC_EDIT, this, STATUS_MAP), 
	showInfo(SETTING(FAV_USERS_SHOW_INFO)),
	listFav(SETTING(USERS_FILTER_FAVORITE)),
	filterQueued(SETTING(USERS_FILTER_QUEUE)),
	filterOnline(SETTING(USERS_FILTER_ONLINE)),
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

	SetSplitterPanes(ctrlUsers.m_hWnd, ctrlInfo.m_hWnd);
	m_nProportionalPos = SETTING(FAV_USERS_SPLITTER_POS);
	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);

	images.Create(16, 16, ILC_COLOR32 | ILC_MASK,  0, 2);
	images.AddIcon(ResourceLoader::loadIcon(IDI_FAV_USER, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_FAV_USER_OFF, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_USER_ON, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_USER_OFF, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_GRANT_ON, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_GRANT_OFF, 16));

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
		ctrlUsers.InsertColumn(j, CTSTRING_I(columnNames[j]), LVCFMT_LEFT, columnSizes[j], j);
	}
	
	ctrlUsers.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlUsers.setVisible(SETTING(USERS_FRAME_VISIBLE));
	ctrlUsers.setSortColumn(COLUMN_NICK);

	filter.addFilterBox(ctrlStatus.m_hWnd);
	filter.addColumnBox(ctrlStatus.m_hWnd, ctrlUsers.getColumnList());
	filter.addMethodBox(ctrlStatus.m_hWnd);
	ctrlFilterContainer.SubclassWindow(filter.getFilterBox().m_hWnd); //subclass the CEdit in order to handle its messages.

	ClientManager::getInstance()->lockRead();
	auto ul = ClientManager::getInstance()->getUsers();
	for(auto& u: ul | map_values) {
		if(u->getCID() == CID()) //these are not users. hub?
			continue;
		userInfos.emplace(u, UserInfo(u, Util::emptyString));
	}

	ClientManager::getInstance()->unlockRead();
	updateList();

	FavoriteManager::getInstance()->addListener(this);
	ClientManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);
	UploadManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);

	CRect rc(SETTING(USERS_LEFT), SETTING(USERS_TOP), SETTING(USERS_RIGHT), SETTING(USERS_BOTTOM));
	if(! (rc.top == 0 && rc.bottom == 0 && rc.left == 0 && rc.right == 0) )
		MoveWindow(rc, TRUE);

	WinUtil::SetIcon(m_hWnd, IDI_USERS);

	startup = false;
	updateStatus();

	bHandled = FALSE;
	return TRUE;

}

LRESULT UsersFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlUsers && ctrlUsers.GetSelectedCount() > 0 ) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlUsers, pt);
		}

		UserPtr u = nullptr;
		tstring x;

		if (ctrlUsers.GetSelectedCount() == 1) {
			auto ui = ctrlUsers.getItemData(WinUtil::getFirstSelectedIndex(ctrlUsers));
			x = ui->columns[COLUMN_NICK];
			u = ui->getUser();
		} else {
			x = _T("");
		}
	
		OMenu usersMenu;
		usersMenu.CreatePopupMenu();
		usersMenu.AppendMenu(MF_STRING, IDC_OPEN_USER_LOG, CTSTRING(OPEN_USER_LOG));
		usersMenu.AppendMenu(MF_SEPARATOR);
		appendUserItems(usersMenu, true, u);

		Bundle::SourceBundleList sourceBundles, badSourceBundles;
		if (u) {
			QueueManager::getInstance()->getSourceInfo(u, sourceBundles, badSourceBundles);

			if (!sourceBundles.empty() || !badSourceBundles.empty()) {
				usersMenu.appendSeparator();

				auto formatBundle = [this] (pair<BundlePtr, Bundle::BundleSource>& bs) -> tstring {
					return Text::toT(bs.first->getName()) + _T(" (") + Util::toStringW(bs.second.files) + _T(" ") + TSTRING(FILES) + _T(", ") + Util::formatBytesW(bs.second.size) + _T(")");
				};

				//current sources
				if (!sourceBundles.empty()) {
					auto removeMenu = usersMenu.createSubMenu(TSTRING(REMOVE_SOURCE), true);

					for(auto& bs: sourceBundles) {
						removeMenu->appendItem(formatBundle(bs), [=] {
								QueueManager::getInstance()->removeBundleSource(bs.first, bs.second.user, QueueItem::Source::FLAG_REMOVED); 
						}, OMenu::FLAG_THREADED);
					}

					removeMenu->appendSeparator();
					removeMenu->appendItem(TSTRING(ALL), [=] {
						for(auto& bs: sourceBundles) {
							QueueManager::getInstance()->removeBundleSource(bs.first, bs.second.user.user, QueueItem::Source::FLAG_REMOVED);
						}
					}, OMenu::FLAG_THREADED);
				}

				//bad sources
				if (!badSourceBundles.empty()) {
					auto readdMenu = usersMenu.createSubMenu(TSTRING(READD_SOURCE), true);
					for(auto& bs: badSourceBundles) {
						readdMenu->appendItem(formatBundle(bs), [=] { 
							QueueManager::getInstance()->readdBundleSource(bs.first, bs.second.user); 
						}, OMenu::FLAG_THREADED);
					}

					readdMenu->appendSeparator();
					readdMenu->appendItem(TSTRING(ALL), [=] {
						for(auto& bs: badSourceBundles) {
							QueueManager::getInstance()->readdBundleSource(bs.first, bs.second.user);
						}
					}, OMenu::FLAG_THREADED);
				}
			}
		}

		usersMenu.AppendMenu(MF_SEPARATOR);
		usersMenu.AppendMenu(MF_STRING, IDC_EDIT, CTSTRING(PROPERTIES));
		usersMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
		
		if (!x.empty())
			usersMenu.InsertSeparatorFirst(x);
		
		usersMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);

		return TRUE; 
	}
	bHandled = FALSE;
	return FALSE; 
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
		/*
		case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {
			//fix the image background colors for selected item when explorer theme is disabled
			const int nItem = static_cast<int>(cd->nmcd.dwItemSpec);
			if(!SETTING(USE_EXPLORER_THEME) && ctrlUsers.GetItemState(nItem, LVNI_SELECTED)) {
				const uint8_t col = ctrlUsers.findColumn(cd->iSubItem);
				if((col != COLUMN_FAVORITE) && (col != COLUMN_SLOT)){
					bHandled = FALSE;
					return 0;
				} 

				UserInfo *ui = reinterpret_cast<UserInfo*>(cd->nmcd.lItemlParam);
				CRect rc;
				ctrlUsers.GetSubItemRect(nItem, cd->iSubItem, LVIR_BOUNDS, rc); //why doesnt lvir_bounds get the correct subitem rect? 
				::FillRect(cd->nmcd.hdc, rc, CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT)));
	
				ctrlUsers.GetSubItemRect(nItem, cd->iSubItem, LVIR_ICON, rc);
				images.DrawEx(ui->getImage(ctrlUsers.findColumn(cd->iSubItem)), cd->nmcd.hdc, rc, GetSysColor(COLOR_HIGHLIGHT), CLR_DEFAULT, ILD_NORMAL);
				return CDRF_NEWFONT | CDRF_DOERASE;
			}
		}	*/

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
		int w[3];
		ctrlStatus.GetClientRect(sr);
		int tmp = (sr.Width() /2) - 16;
			
		w[0] = sr.right - tmp;
		w[1] = w[0] + (tmp-16)/2;
		w[2] = w[0] + (tmp-16);
			
		ctrlStatus.SetParts(3, w);

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
		
		ctrlStatus.GetRect(2, sr);
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
	}
	return 0;
}

LRESULT UsersFrame::onEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlUsers.GetSelectedCount() == 1) {
		int i = ctrlUsers.GetNextItem(-1, LVNI_SELECTED);
		UserInfo* ui = ctrlUsers.getItemData(i);
		if(!ui->isFavorite)
			return 0;
		dcassert(i != -1);
		LineDlg dlg;
		dlg.description = TSTRING(DESCRIPTION);
		dlg.title = ui->columns[COLUMN_NICK];
		dlg.line = ui->columns[COLUMN_DESCRIPTION];
		if(dlg.DoModal(m_hWnd)) {
			FavoriteManager::getInstance()->setUserDescription(ui->user, Text::fromT(dlg.line));
			ui->columns[COLUMN_DESCRIPTION] = dlg.line;
			ctrlUsers.updateItem(i);
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

LRESULT UsersFrame::onClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	if(startup)
		return 0;

	NMITEMACTIVATE* l = (NMITEMACTIVATE*)pnmh;

	if(l->iItem != -1) {
		auto ui = ctrlUsers.getItemData(l->iItem);
		if((ctrlUsers.findColumn(l->iSubItem) == COLUMN_SLOT) && hitIcon(l->iItem, l->iSubItem)) {
			ui->grantSlot = !ui->grantSlot;
			if(ui->isFavorite){
				FavoriteManager::getInstance()->setAutoGrant(ui->getUser(), ui->grantSlot);
				setImages(ui, l->iItem);
			} else {
				ui->grantSlot ? ui->grantTimeless() : ui->ungrant(); // timeless grant
			}
		} else if((ctrlUsers.findColumn(l->iSubItem) == COLUMN_FAVORITE) && hitIcon(l->iItem, l->iSubItem)) {  //TODO: confirm removal.
			if(ui->isFavorite)
				FavoriteManager::getInstance()->removeFavoriteUser(ui->getUser());
			else
				FavoriteManager::getInstance()->addFavoriteUser(HintedUser(ui->getUser(), ui->getHubUrl()));
		}
	}
	
	bHandled = FALSE;
  	return 0;
}

BOOL UsersFrame::hitIcon(int aItem, int aSubItem) {
	CRect rc;
	CPoint pt = GetMessagePos();
	ctrlUsers.ScreenToClient(&pt);
	ctrlUsers.GetSubItemRect(aItem, aSubItem, LVIR_ICON, rc);

	return PtInRect(&rc, pt);
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
	case COLUMN_FAVORITE: return compare(a->isFavorite, b->isFavorite);
	case COLUMN_SLOT: return compare(a->grantSlot, b->grantSlot);
	case COLUMN_QUEUED: return compare(a->user->getQueued(), b->user->getQueued());
	default: return Util::DefaultSort(a->columns[col].c_str(), b->columns[col].c_str());
	}
}

void UsersFrame::updateInfoText(const UserInfo* ui){
	if(!showInfo)
		return;
	map<string, Identity> idents = ClientManager::getInstance()->getIdentities(ui->getUser());
	if(!idents.empty()) {
		tstring tmp = _T("");
		for(auto& id: idents) {
			auto info = id.second.getInfo();
		
			tmp += _T("\r\nUser Information: \r\n");
			tmp += _T("Hub: ") + Text::toT(id.first) + _T("\r\n");
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
			x->second.update(aUser);
			ctrlUsers.insertItem(&x->second, 0);
		}
	} else {
		updateUser(aUser);
	}
	updateStatus();
}

void UsersFrame::updateUser(const UserPtr& aUser) {
	auto i = userInfos.find(aUser);
	if(i != userInfos.end()) {
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
	}
	updateStatus();
}

void UsersFrame::updateList() {
	ctrlUsers.SetRedraw(FALSE);
	ctrlUsers.DeleteAllItems();
	ctrlInfo.SetWindowText(_T(""));

	auto i = userInfos.begin();
	auto filterPrep = filter.prepare();
	auto filterInfoF = [this, &i](int column) { return Text::fromT(i->second.getText(column)); };

	for(; i != userInfos.end(); ++i) {
		if((filter.empty() || filter.match(filterPrep, filterInfoF)) && show(i->second.getUser(), false)) {
			int p = ctrlUsers.insertItem(&i->second,0);
			i->second.update(i->second.getUser());
			setImages(&i->second, p);
			ctrlUsers.updateItem(p);
		}
	}
	ctrlUsers.SetRedraw(TRUE);
	ctrlUsers.resort();
	updateStatus();
}

bool UsersFrame::matches(const UserInfo &ui) {
	if(!filter.empty() && !filter.match(filter.prepare(), [this, &ui](int column) { return Text::fromT(ui.getText(column)); })) {
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
	} else if(listFav && !isFav(u)){
		return false;
	}
	return true;
}

void UsersFrame::setImages(UserInfo *ui, int pos/* = -1*/) {
	if(pos == -1)
		pos = ctrlUsers.findItem(ui);

	ctrlUsers.SetItem(pos, ctrlUsers.findColumnIndex(COLUMN_FAVORITE), LVIF_IMAGE, NULL, ui->getImage(COLUMN_FAVORITE), 0, 0, NULL);
	ctrlUsers.SetItem(pos, ctrlUsers.findColumnIndex(COLUMN_SLOT), LVIF_IMAGE, NULL, ui->getImage(COLUMN_SLOT), 0, 0, NULL);
	ctrlUsers.SetItem(pos, ctrlUsers.findColumnIndex(COLUMN_SEEN), LVIF_IMAGE, NULL, ui->getImage(COLUMN_SEEN), 0, 0, NULL);

}
void UsersFrame::updateStatus() {
	ctrlStatus.SetText(1, (Text::toT(Util::toString(ctrlUsers.GetItemCount()) + " ") + TSTRING(USERS)).c_str());
}

void UsersFrame::UserInfo::update(const UserPtr& u) {
	auto fu = FavoriteManager::getInstance()->getFavoriteUser(u);
	if(fu) {
		isFavorite = true;
		grantSlot = fu->isSet(FavoriteUser::FLAG_GRANTSLOT);
		
		setHubUrl(fu->getUrl().empty() ? hubUrl : fu->getUrl());

		//gets nicks and hubnames and updates the hint url
		string url = getHubUrl();
		auto ui = move(ClientManager::getInstance()->getNickHubPair(u, url));
		setHubUrl(url);

		columns[COLUMN_NICK] = u->isOnline() ? Text::toT(ui.first) : fu->getNick().empty() ? Text::toT(ui.first) : Text::toT(fu->getNick());
		columns[COLUMN_HUB] = u->isOnline() ? Text::toT(ui.second) : Text::toT(fu->getUrl()); 
		columns[COLUMN_SEEN] = u->isOnline() ? TSTRING(ONLINE) : fu->getLastSeen() ? Text::toT(Util::formatTime("%Y-%m-%d %H:%M", fu->getLastSeen())) : TSTRING(UNKNOWN);
		columns[COLUMN_DESCRIPTION] = Text::toT(fu->getDescription());
	} else {
		isFavorite = false;
		grantSlot = hasReservedSlot();
		
		//gets nicks and hubnames and updates the hint url
		string url = getHubUrl();
		auto ui = move(ClientManager::getInstance()->getNickHubPair(u, url));
		setHubUrl(url);

		columns[COLUMN_NICK] = Text::toT(ui.first);
		columns[COLUMN_HUB] = u->isOnline() ? Text::toT(ui.second) : Text::toT(getHubUrl());
		columns[COLUMN_SEEN] = u->isOnline() ? TSTRING(ONLINE) : TSTRING(OFFLINE);
		columns[COLUMN_DESCRIPTION] = Util::emptyStringT;
	}

	columns[COLUMN_QUEUED] = Util::formatBytesW(u->getQueued());
}

			
LRESULT UsersFrame::onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlUsers.GetSelectedCount() == 1) {
		int i = ctrlUsers.GetNextItem(-1, LVNI_SELECTED);
		const UserInfo* ui = ctrlUsers.getItemData(i);
		dcassert(i != -1);

		ParamMap params;
		params["hubNI"] = Util::listToString(ClientManager::getInstance()->getHubNames(ui->getUser()->getCID()));
		params["hubURL"] = Util::listToString(ClientManager::getInstance()->getHubUrls(ui->getUser()->getCID()));
		params["userCID"] = ui->getUser()->getCID().toBase32(); 
		params["userNI"] = ClientManager::getInstance()->getNicks(ui->getUser()->getCID())[0];
		params["myCID"] = ClientManager::getInstance()->getMe()->getCID().toBase32();

		string file = LogManager::getInstance()->getPath(ui->getUser(), params);
		if(Util::fileExists(file)) {
			WinUtil::viewLog(file);
		} else {
			MessageBox(CTSTRING(NO_LOG_FOR_USER), CTSTRING(NO_LOG_FOR_USER), MB_OK );	  
		}	
	}
	return 0;
}

LRESULT UsersFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		FavoriteManager::getInstance()->removeListener(this);
		ClientManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		UploadManager::getInstance()->removeListener(this);
		QueueManager::getInstance()->removeListener(this);

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
			SettingsManager::getInstance()->set(SettingsManager::FAV_USERS_SPLITTER_POS, m_nProportionalPos);
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
void UsersFrame::on(UserAdded, const FavoriteUser& aUser) noexcept { 
	callAsync([=] { addUser(aUser.getUser(), aUser.getUrl()); } ); 
}
void UsersFrame::on(UserRemoved, const FavoriteUser& aUser) noexcept { 
	callAsync([=] { updateUser(aUser.getUser()); } ); 
}
void UsersFrame::on(StatusChanged, const UserPtr& aUser) noexcept { 
	callAsync([=] { updateUser(aUser); } ); 
}

void UsersFrame::on(UserConnected, const OnlineUser& aUser, bool) noexcept {
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

void UsersFrame::on(UploadManagerListener::SlotsUpdated, const UserPtr& aUser) noexcept {
	callAsync([=] { updateUser(aUser); });
}

void UsersFrame::on(QueueManagerListener::SourceFilesUpdated, const UserPtr& aUser) noexcept {
	callAsync([=] { updateUser(aUser); });
}