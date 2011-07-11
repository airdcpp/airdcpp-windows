/*
 * Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
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
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "../client/ResourceManager.h"
#include "../client/SettingsManager.h"
#include "../client/ConnectionManager.h"
#include "../client/DownloadManager.h"
#include "../client/UploadManager.h"
#include "../client/QueueManager.h"
#include "../client/QueueItem.h"

#include "WinUtil.h"
#include "TransferView.h"
#include "MainFrm.h"

#include "BarShader.h"

int TransferView::columnIndexes[] = { COLUMN_USER, COLUMN_HUB, COLUMN_STATUS, COLUMN_TIMELEFT, COLUMN_TOTALTIMELEFT, COLUMN_SPEED, COLUMN_FILE, COLUMN_SIZE, COLUMN_PATH, COLUMN_CIPHER, COLUMN_IP, COLUMN_RATIO };
int TransferView::columnSizes[] = { 150, 150, 250, 75, 75, 75, 175, 100, 200, 100, 150, 50 };

static ResourceManager::Strings columnNames[] = { ResourceManager::USER, ResourceManager::HUB_SEGMENTS, ResourceManager::STATUS,
	ResourceManager::TIME_LEFT, ResourceManager::TOTAL_TIME_LEFT, ResourceManager::SPEED, ResourceManager::FILENAME, ResourceManager::SIZE, ResourceManager::PATH,
	ResourceManager::CIPHER, ResourceManager::IP_BARE, ResourceManager::RATIO};

TransferView::~TransferView() {
	arrows.Destroy();
	OperaColors::ClearCache();
}

LRESULT TransferView::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	arrows.CreateFromImage(IDB_ARROWS, 16, 3, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	ctrlTransfers.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_TRANSFERS);
	ctrlTransfers.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);

	WinUtil::splitTokens(columnIndexes, SETTING(MAINFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(MAINFRAME_WIDTHS), COLUMN_LAST);

	for(uint8_t j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_TIMELEFT || j == COLUMN_TOTALTIMELEFT /*ttlf*/ || j == COLUMN_SPEED) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlTransfers.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}

	ctrlTransfers.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlTransfers.setVisible(SETTING(MAINFRAME_VISIBLE));

	ctrlTransfers.SetBkColor(WinUtil::bgColor);
	ctrlTransfers.SetTextBkColor(WinUtil::bgColor);
	ctrlTransfers.SetTextColor(WinUtil::textColor);
	ctrlTransfers.setFlickerFree(WinUtil::bgBrush);

	ctrlTransfers.SetImageList(arrows, LVSIL_SMALL);
	ctrlTransfers.setSortColumn(COLUMN_USER);

	noGroup = BOOLSETTING(DOWNLOADS_EXPAND);

	ConnectionManager::getInstance()->addListener(this);
	DownloadManager::getInstance()->addListener(this);
	UploadManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);
	return 0;
}

void TransferView::prepareClose() {
	ctrlTransfers.saveHeaderOrder(SettingsManager::MAINFRAME_ORDER, SettingsManager::MAINFRAME_WIDTHS,
		SettingsManager::MAINFRAME_VISIBLE);

	ConnectionManager::getInstance()->removeListener(this);
	DownloadManager::getInstance()->removeListener(this);
	UploadManager::getInstance()->removeListener(this);
	QueueManager::getInstance()->removeListener(this);
	SettingsManager::getInstance()->removeListener(this);
	
}

LRESULT TransferView::onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	RECT rc;
	GetClientRect(&rc);
	ctrlTransfers.MoveWindow(&rc);

	return 0;
}

LRESULT TransferView::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlTransfers && ctrlTransfers.GetSelectedCount() > 0) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlTransfers, pt);
		}

		OMenu transferMenu, previewMenu, copyMenu;
		const ItemInfo* ii = ctrlTransfers.getItemData(ctrlTransfers.GetNextItem(-1, LVNI_SELECTED));
		bool parent = !ii->parent && ctrlTransfers.findChildren(ii->getGroupCond()).size() > 1;

		transferMenu.CreatePopupMenu();
		previewMenu.CreatePopupMenu();
		copyMenu.CreatePopupMenu();
		previewMenu.InsertSeparatorFirst(TSTRING(PREVIEW_MENU));
		
		if(!parent) {
			transferMenu.InsertSeparatorFirst(TSTRING(MENU_TRANSFERS));
			appendUserItems(transferMenu, Util::emptyString); // TODO: hubhint
			
			transferMenu.AppendMenu(MF_STRING, IDC_REMOVE_FILE, CTSTRING(REMOVE_FILE));
			transferMenu.AppendMenu(MF_SEPARATOR);
			transferMenu.AppendMenu(MF_STRING, IDC_FORCE, CTSTRING(FORCE_ATTEMPT));

			if(ii->download) {
				transferMenu.AppendMenu(MF_SEPARATOR);
				transferMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_FOR_ALTERNATES));
				transferMenu.AppendMenu(MF_STRING, IDC_MENU_SLOWDISCONNECT, CTSTRING(SETCZDC_DISCONNECTING_ENABLE));
				transferMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)previewMenu, CTSTRING(PREVIEW_MENU));
				
			}

			int i = -1;
			if((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
				const ItemInfo* itemI = ctrlTransfers.getItemData(i);
				
				if(itemI->user.user)
					prepareMenu(transferMenu, UserCommand::CONTEXT_CHAT, ClientManager::getInstance()->getHubs(itemI->user.user->getCID(), itemI->user.hint));
			}
			transferMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)copyMenu, CTSTRING(COPY));
			transferMenu.AppendMenu(MF_SEPARATOR);
			transferMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(CLOSE_CONNECTION));
			

			switch(SETTING(TRANSFERLIST_DBLCLICK)) {
				case 0:
		          transferMenu.SetMenuDefaultItem(IDC_PRIVATEMESSAGE);
		          break;
				case 1:
		          transferMenu.SetMenuDefaultItem(IDC_GETLIST);
		          break;
				case 2:
		          transferMenu.SetMenuDefaultItem(IDC_MATCH_QUEUE);
		          break;
				case 3:
		          transferMenu.SetMenuDefaultItem(IDC_GRANTSLOT);
		          break;
				case 4:
		          transferMenu.SetMenuDefaultItem(IDC_ADD_TO_FAVORITES);
		          break;
				case 5:
		          transferMenu.SetMenuDefaultItem(IDC_BROWSELIST);
		          break;
			  
			}
		} else {
			transferMenu.InsertSeparatorFirst(TSTRING(SETTINGS_SEGMENT));
			transferMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_FOR_ALTERNATES));
			transferMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)previewMenu, CTSTRING(PREVIEW_MENU));
			transferMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)copyMenu, CTSTRING(COPY));
			transferMenu.AppendMenu(MF_STRING, IDC_MENU_SLOWDISCONNECT, CTSTRING(SETCZDC_DISCONNECTING_ENABLE));
			transferMenu.AppendMenu(MF_SEPARATOR);
			transferMenu.AppendMenu(MF_STRING, IDC_FORCE, CTSTRING(CONNECT_ALL));
			transferMenu.AppendMenu(MF_STRING, IDC_DISCONNECT_ALL, CTSTRING(DISCONNECT_ALL));
			transferMenu.AppendMenu(MF_SEPARATOR);
			transferMenu.AppendMenu(MF_STRING, IDC_EXPAND_ALL, CTSTRING(EXPAND_ALL));
			transferMenu.AppendMenu(MF_STRING, IDC_COLLAPSE_ALL, CTSTRING(COLLAPSE_ALL));
			transferMenu.AppendMenu(MF_SEPARATOR);
			transferMenu.AppendMenu(MF_STRING, IDC_REMOVEALL, CTSTRING(REMOVE_ALL));
			transferMenu.AppendMenu(MF_STRING, IDC_REMOVE_FILE, CTSTRING(REMOVE_FILE));
			
	


		}

		transferMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_UNCHECKED);
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_NICK, CTSTRING(COPY_NICK));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_FILENAME, CTSTRING(FILENAME));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_SIZE, CTSTRING(SIZE));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_PATH, CTSTRING(PATH));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_IP, CTSTRING(IP_BARE));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_HUB, CTSTRING(HUB));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_SPEED, CTSTRING(SPEED));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_STATUS, CTSTRING(STATUS));
	copyMenu.AppendMenu(MF_STRING, IDC_COPY_ALL, CTSTRING(ALL));

		if(ii->download) {
			if(!ii->target.empty()) {
				string target = Text::fromT(ii->target);
				string ext = Util::getFileExt(target);
				if(ext.size()>1) ext = ext.substr(1);
				PreviewAppsSize = WinUtil::SetupPreviewMenu(previewMenu, ext);
				if(previewMenu.GetMenuItemCount() > 1) {
					transferMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MFS_ENABLED);
				} else {
					transferMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MFS_DISABLED);
				}

				const QueueItem::StringMap& queue = QueueManager::getInstance()->lockQueue();

				QueueItem::StringIter qi = queue.find(&target);

				bool slowDisconnect = false;
				if(qi != queue.end())
					slowDisconnect = qi->second->isSet(QueueItem::FLAG_AUTODROP);

				QueueManager::getInstance()->unlockQueue();

				if(slowDisconnect) {
					transferMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_CHECKED);
				}
			}
		}

		transferMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
		return TRUE; 
	} else {
		bHandled = FALSE;
		return FALSE;
	}
}

void TransferView::runUserCommand(UserCommand& uc) {
	if(!WinUtil::getUCParams(m_hWnd, uc, ucLineParams))
		return;

	StringMap ucParams = ucLineParams;

	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const ItemInfo* itemI = ctrlTransfers.getItemData(i);
		if(!itemI->user.user || !itemI->user.user->isOnline())
			continue;

		StringMap tmp = ucParams;
		ucParams["fileFN"] = Text::fromT(itemI->target);

		// compatibility with 0.674 and earlier
		ucParams["file"] = ucParams["fileFN"];
		
		ClientManager::getInstance()->userCommand(itemI->user, uc, tmp, true);
	}
}

LRESULT TransferView::onForce(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ItemInfo* ii = ctrlTransfers.getItemData(i);
		ctrlTransfers.SetItemText(i, COLUMN_STATUS, CTSTRING(CONNECTING_FORCED));

		if(ii->parent == NULL && ii->hits != -1) {
			const vector<ItemInfo*>& children = ctrlTransfers.findChildren(ii->getGroupCond());
			for(vector<ItemInfo*>::const_iterator j = children.begin(); j != children.end(); ++j) {
				ItemInfo* ii = *j;

				int h = ctrlTransfers.findItem(ii);
				if(h != -1)
					ctrlTransfers.SetItemText(h, COLUMN_STATUS, CTSTRING(CONNECTING_FORCED));

				ii->transferFailed = false;
				ConnectionManager::getInstance()->force(ii->token);
			}
		} else {
			ii->transferFailed = false;
			ConnectionManager::getInstance()->force(ii->token);
		}
	}
	return 0;
}

void TransferView::ItemInfo::removeAll() {
	if(hits <= 1) {
		QueueManager::getInstance()->removeSource(user, QueueItem::Source::FLAG_REMOVED);
	} else {
		if(!BOOLSETTING(CONFIRM_DELETE) || ::MessageBox(0, _T("Do you really want to remove this item?"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
			QueueManager::getInstance()->remove(Text::fromT(target));
	}
}

LRESULT TransferView::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {
		ItemInfo* ii = reinterpret_cast<ItemInfo*>(cd->nmcd.lItemlParam);

		int colIndex = ctrlTransfers.findColumn(cd->iSubItem);
		cd->clrTextBk = WinUtil::bgColor;

		if((colIndex == COLUMN_STATUS) && (ii->status != ItemInfo::STATUS_WAITING)) {
			if(!BOOLSETTING(SHOW_PROGRESS_BARS)) {
				bHandled = FALSE;
				return 0;
			}

			// Get the text to draw
			// Get the color of this bar
			COLORREF clr = SETTING(PROGRESS_OVERRIDE_COLORS) ? 
				(ii->download ? (ii->parent ? SETTING(PROGRESS_SEGMENT_COLOR) : SETTING(DOWNLOAD_BAR_COLOR)) :
				SETTING(UPLOAD_BAR_COLOR)) : GetSysColor(COLOR_HIGHLIGHT);

			//this is just severely broken, msdn says GetSubItemRect requires a one based index
			//but it wont work and index 0 gives the rect of the whole item
			CRect rc;
			if(cd->iSubItem == 0) {
				//use LVIR_LABEL to exclude the icon area since we will be painting over that
				//later
				ctrlTransfers.GetItemRect((int)cd->nmcd.dwItemSpec, &rc, LVIR_LABEL);
			} else {
				ctrlTransfers.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, &rc);
			}
				
			/* Thanks & credits for Stealthy style go to phaedrus */

			// fixes issues with double border
			rc.top -= 1;

			// Real rc, the original one.
			CRect real_rc = rc;
			// We need to offset the current rc to (0, 0) to paint on the New dc
			rc.MoveToXY(0, 0);

			CDC cdc;
			cdc.CreateCompatibleDC(cd->nmcd.hdc);

			HBITMAP pOldBmp = cdc.SelectBitmap(CreateCompatibleBitmap(cd->nmcd.hdc,  real_rc.Width(),  real_rc.Height()));
			HDC& dc = cdc.m_hDC;

			HFONT oldFont = (HFONT)SelectObject(dc, WinUtil::font);
			SetBkMode(dc, TRANSPARENT);
			
			// Draw the background and border of the bar	
			if(ii->size == 0) ii->size = 1;		
			
			COLORREF oldcol;
			if(BOOLSETTING(PROGRESSBAR_ODC_STYLE)) {
				// New style progressbar tweaks the current colors
				HLSTRIPLE hls_bk = OperaColors::RGB2HLS(cd->clrTextBk);

				// Create pen (ie outline border of the cell)
				HPEN penBorder = ::CreatePen(PS_SOLID, 1, OperaColors::blendColors(cd->clrTextBk, clr, (hls_bk.hlstLightness > 0.75) ? 0.6 : 0.4));
				HGDIOBJ pOldPen = ::SelectObject(dc, penBorder);

				// Draw the outline (but NOT the background) using pen
				HBRUSH hBrOldBg = CreateSolidBrush(cd->clrTextBk);
				hBrOldBg = (HBRUSH)::SelectObject(dc, hBrOldBg);
				::Rectangle(dc, rc.left, rc.top, rc.right, rc.bottom);
				DeleteObject(::SelectObject(dc, hBrOldBg));

				// Set the background color, by slightly changing it
				HBRUSH hBrDefBg = CreateSolidBrush(OperaColors::blendColors(cd->clrTextBk, clr, (hls_bk.hlstLightness > 0.75) ? 0.85 : 0.70));
				HGDIOBJ oldBg = ::SelectObject(dc, hBrDefBg);

				// Draw the outline AND the background using pen+brush
				::Rectangle(dc, rc.left, rc.top, rc.left + (LONG)(rc.Width() * ii->getRatio() + 0.5), rc.bottom);

				// Reset pen
				DeleteObject(::SelectObject(dc, pOldPen));
				// Reset bg (brush)
				DeleteObject(::SelectObject(dc, oldBg));

				// Draw the text over the entire item
                oldcol = ::SetTextColor(dc, SETTING(PROGRESS_OVERRIDE_COLORS2) ? 
					(ii->download ? SETTING(PROGRESS_TEXT_COLOR_DOWN) : SETTING(PROGRESS_TEXT_COLOR_UP)) : 
					clr);
				rc.left += 6;
                ::DrawText(dc, ii->statusString.c_str(), ii->statusString.length(), rc, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
				rc.left -= 6;

				COLORREF a, b;
				OperaColors::EnlightenFlood(clr, a, b);
				OperaColors::FloodFill(cdc, rc.left+1, rc.top+1,  rc.left + (int) ((int64_t)rc.Width() * ii->actual / ii->size), rc.bottom-1, a, b);
				
				// Draw the text only over the bar and with correct color
				::SetTextColor(dc, SETTING(PROGRESS_OVERRIDE_COLORS2) ? 
					(ii->download ? SETTING(PROGRESS_TEXT_COLOR_DOWN) : SETTING(PROGRESS_TEXT_COLOR_UP)) : 
					OperaColors::TextFromBackground(clr));

				rc.right = rc.left + (int) ((int64_t)rc.Width() * ii->actual / ii->size) + 1;
				rc.left += 6;
                ::DrawText(dc, ii->statusString.c_str(), ii->statusString.length(), rc, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
			} else {
				CBarShader statusBar(rc.bottom - rc.top, rc.right - rc.left, SETTING(PROGRESS_BACK_COLOR), ii->size);

				//rc.right = rc.left + (int) (rc.Width() * ii->pos / ii->size); 
				if(!ii->download) {
					statusBar.FillRange(0, ii->actual,  clr);
				} else {
					statusBar.FillRange(0, ii->actual, clr);
				}
				if(ii->pos > ii->actual)
					statusBar.FillRange(ii->actual, ii->pos, SETTING(PROGRESS_COMPRESS_COLOR));

				statusBar.Draw(cdc, rc.top, rc.left, SETTING(PROGRESS_3DDEPTH));

				// Get the color of this text bar
				oldcol = ::SetTextColor(dc, SETTING(PROGRESS_OVERRIDE_COLORS2) ? 
					(ii->download ? SETTING(PROGRESS_TEXT_COLOR_DOWN) : SETTING(PROGRESS_TEXT_COLOR_UP)) : 
					OperaColors::TextFromBackground(clr));

				rc.left += 6;
				rc.right -= 2;
				LONG top = rc.top + (rc.Height() - WinUtil::getTextHeight(cd->nmcd.hdc) - 1)/2 + 1;
				::ExtTextOut(dc, rc.left, top, ETO_CLIPPED, rc, ii->statusString.c_str(), ii->statusString.length(), NULL);
			}
	
			SelectObject(dc, oldFont);
			::SetTextColor(dc, oldcol);

			// New way:
			BitBlt(cd->nmcd.hdc, real_rc.left, real_rc.top, real_rc.Width(), real_rc.Height(), dc, 0, 0, SRCCOPY);
			DeleteObject(cdc.SelectBitmap(pOldBmp));

			//bah crap, if we return CDRF_SKIPDEFAULT windows won't paint the icons
			//so we have to do it
			if(cd->iSubItem == 0){
				LVITEM lvItem;
				lvItem.iItem = cd->nmcd.dwItemSpec;
				lvItem.iSubItem = 0;
				lvItem.mask = LVIF_IMAGE | LVIF_STATE;
				lvItem.stateMask = LVIS_SELECTED;
				ctrlTransfers.GetItem(&lvItem);

				HIMAGELIST imageList = (HIMAGELIST)::SendMessage(ctrlTransfers.m_hWnd, LVM_GETIMAGELIST, LVSIL_SMALL, 0);
				if(imageList) {
					//let's find out where to paint it
					//and draw the background to avoid having 
					//the selection color as background
					CRect iconRect;
					ctrlTransfers.GetSubItemRect((int)cd->nmcd.dwItemSpec, 0, LVIR_ICON, iconRect);
					ImageList_Draw(imageList, lvItem.iImage, cd->nmcd.hdc, iconRect.left, iconRect.top, ILD_TRANSPARENT);
				}
			}
			return CDRF_SKIPDEFAULT;
		} else if(BOOLSETTING(GET_USER_COUNTRY) && (colIndex == COLUMN_IP)) {
			ItemInfo* ii = (ItemInfo*)cd->nmcd.lItemlParam;
			CRect rc;
			ctrlTransfers.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, rc);

			if((WinUtil::getOsMajor() >= 5 && WinUtil::getOsMinor() >= 1) //WinXP & WinSvr2003
				|| (WinUtil::getOsMajor() >= 6)) //Vista & Win7
			{
				SetTextColor(cd->nmcd.hdc, WinUtil::textColor);
				DrawThemeBackground(GetWindowTheme(ctrlTransfers.m_hWnd), cd->nmcd.hdc, LVP_LISTITEM, 3, &rc, &rc );
			} else {
				COLORREF color;
				if(ctrlTransfers.GetItemState((int)cd->nmcd.dwItemSpec, LVIS_SELECTED) & LVIS_SELECTED) {
					if(ctrlTransfers.m_hWnd == ::GetFocus()) {
						color = GetSysColor(COLOR_HIGHLIGHT);
						SetBkColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHT));
						SetTextColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
					} else {
						color = GetBkColor(cd->nmcd.hdc);
						SetBkColor(cd->nmcd.hdc, color);
					}		
				} else {
					color = WinUtil::bgColor;
					SetBkColor(cd->nmcd.hdc, WinUtil::bgColor);
					SetTextColor(cd->nmcd.hdc, WinUtil::textColor);
				}
				HGDIOBJ oldpen = ::SelectObject(cd->nmcd.hdc, CreatePen(PS_SOLID,0, color));
				HGDIOBJ oldbr = ::SelectObject(cd->nmcd.hdc, CreateSolidBrush(color));
				Rectangle(cd->nmcd.hdc,rc.left, rc.top, rc.right, rc.bottom);

				DeleteObject(::SelectObject(cd->nmcd.hdc, oldpen));
				DeleteObject(::SelectObject(cd->nmcd.hdc, oldbr));
			}

			TCHAR buf[256];
			ctrlTransfers.GetItemText((int)cd->nmcd.dwItemSpec, cd->iSubItem, buf, 255);
			buf[255] = 0;
			if(_tcslen(buf) > 0) {
				rc.left += 2;
				LONG top = rc.top + (rc.Height() - 15)/2;
				if((top - rc.top) < 2)
					top = rc.top + 1;

				POINT p = { rc.left, top };
				WinUtil::flagImages.Draw(cd->nmcd.hdc, ii->flagIndex, p, LVSIL_SMALL);
				top = rc.top + (rc.Height() - WinUtil::getTextHeight(cd->nmcd.hdc) - 1)/2;
				::ExtTextOut(cd->nmcd.hdc, rc.left + 30, top + 1, ETO_CLIPPED, rc, buf, _tcslen(buf), NULL);
				return CDRF_SKIPDEFAULT;
			}
		} else if((colIndex != COLUMN_USER) && (colIndex != COLUMN_HUB) && (colIndex != COLUMN_STATUS) && (colIndex != COLUMN_IP) &&
			(ii->status != ItemInfo::STATUS_RUNNING)) {
			cd->clrText = OperaColors::blendColors(WinUtil::bgColor, WinUtil::textColor, 0.4);
			return CDRF_NEWFONT;
		}
		// Fall through
	}
	default:
		return CDRF_DODEFAULT;
	}
}

LRESULT TransferView::onDoubleClickTransfers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;
	if (item->iItem != -1 ) {
		CRect rect;
		ctrlTransfers.GetItemRect(item->iItem, rect, LVIR_ICON);

		// if double click on state icon, ignore...
		if (item->ptAction.x < rect.left)
			return 0;

		ItemInfo* i = ctrlTransfers.getItemData(item->iItem);
		const vector<ItemInfo*>& children = ctrlTransfers.findChildren(i->getGroupCond());
		if(i->parent != NULL || children.size() <= 1) {
			switch(SETTING(TRANSFERLIST_DBLCLICK)) {
				case 0:
					i->pm(Util::emptyString);
					break;
				case 1:
					i->getList(Util::emptyString);
					break;
				case 2:
					i->matchQueue(Util::emptyString);
				case 3:
					i->grant(Util::emptyString);
					break;
				case 4:
					i->addFav();
					break;
				case 5:
					i->browseList(Util::emptyString);
					break;
			}
		}
	}
	return 0;
}

int TransferView::ItemInfo::compareItems(const ItemInfo* a, const ItemInfo* b, uint8_t col) {
	if(a->status == b->status) {
		if(a->download != b->download) {
			return a->download ? -1 : 1;
		}
	} else {
		return (a->status == ItemInfo::STATUS_RUNNING) ? -1 : 1;
	}

	switch(col) {
		case COLUMN_USER: {
			if(a->hits == b->hits)
				return Util::DefaultSort(a->getText(COLUMN_USER).c_str(), b->getText(COLUMN_USER).c_str());
			return compare(a->hits, b->hits);						
		}
		case COLUMN_HUB: {
			if(a->running == b->running)
				return Util::DefaultSort(a->getText(COLUMN_HUB).c_str(), b->getText(COLUMN_HUB).c_str());
			return compare(a->running, b->running);						
		}
		case COLUMN_STATUS: return 0;
		case COLUMN_TIMELEFT: return compare(a->timeLeft, b->timeLeft);
		case COLUMN_TOTALTIMELEFT: return compare(a->timeLeft, b->timeLeft); /*ttlf*/ 
		case COLUMN_SPEED: return compare(a->speed, b->speed);
		case COLUMN_SIZE: return compare(a->size, b->size);
		case COLUMN_RATIO: return compare(a->getRatio(), b->getRatio());
		default: return Util::DefaultSort(a->getText(col).c_str(), b->getText(col).c_str());
	}
}

TransferView::ItemInfo* TransferView::findItem(const UpdateInfo& ui, int& pos) const {
	for(int j = 0; j < ctrlTransfers.GetItemCount(); ++j) {
		ItemInfo* ii = ctrlTransfers.getItemData(j);
		if(ui.token == ii->token) {
			pos = j;
			return ii;
		} else if(ui.download && ii->download && ii->parent == NULL) {
			const vector<ItemInfo*>& children = ctrlTransfers.findChildren(ii->getGroupCond());
			for(vector<ItemInfo*>::const_iterator k = children.begin(); k != children.end(); k++) {
				ItemInfo* ii = *k;
				if(ui.token == ii->token) {
					return ii;
				}
			}
		}
	}
	return NULL;
}

LRESULT TransferView::onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	TaskQueue::List t;
	tasks.get(t);
	if(t.size() > 2) {
		ctrlTransfers.SetRedraw(FALSE);
	}

	for(TaskQueue::Iter i = t.begin(); i != t.end(); ++i) {
		if(i->first == ADD_ITEM) {
			auto_ptr<UpdateInfo> ui(reinterpret_cast<UpdateInfo*>(i->second));
			ItemInfo* ii = new ItemInfo(ui->user, ui->token, ui->download);
			ii->update(*ui);
			if(ii->download) {
				if(noGroup) {
					ctrlTransfers.insertItem(ii, IMAGE_DOWNLOAD);
				}else{
				ctrlTransfers.insertGroupedItem(ii, false);
				}
				
			} else {
				ctrlTransfers.insertItem(ii, IMAGE_UPLOAD);
			}
		} else if(i->first == REMOVE_ITEM) {
			auto_ptr<UpdateInfo> ui(reinterpret_cast<UpdateInfo*>(i->second));

			int pos = -1;
			ItemInfo* ii = findItem(*ui, pos);
			if(ii) {
				if(ui->download) {
					ctrlTransfers.removeGroupedItem(ii);
				} else {
					dcassert(pos != -1);
					ctrlTransfers.DeleteItem(pos);
					delete ii;
				}
			}
		} else if(i->first == UPDATE_ITEM) {
			auto_ptr<UpdateInfo> ui(reinterpret_cast<UpdateInfo*>(i->second));
			int pos = -1;
			ItemInfo* ii = findItem(*ui, pos);
			if(ii) {
				if(ui->download && !noGroup)  {
					ItemInfo* parent = ii->parent ? ii->parent : ii;

					if(ui->type == Transfer::TYPE_FILE || ui->type == Transfer::TYPE_TREE) {
						/* parent item must be updated with correct info about whole file */
						if(ui->status == ItemInfo::STATUS_RUNNING && parent->status == ItemInfo::STATUS_RUNNING && parent->hits == -1) {
							ui->updateMask &= ~UpdateInfo::MASK_POS;
							ui->updateMask &= ~UpdateInfo::MASK_ACTUAL;
							ui->updateMask &= ~UpdateInfo::MASK_SIZE;
							ui->updateMask &= ~UpdateInfo::MASK_STATUS_STRING;
							ui->updateMask &= ~UpdateInfo::MASK_TIMELEFT;
							ui->updateMask &= ~UpdateInfo::MASK_TOTALTIMELEFT; /*ttlf*/
						}
					}

					/* if target has changed, regroup the item */
					bool changeParent = (ui->updateMask & UpdateInfo::MASK_FILE) && (ui->target != ii->target);
					if(changeParent)
						ctrlTransfers.removeGroupedItem(ii, false);
					ii->update(*ui);

					if(changeParent) {
						ctrlTransfers.insertGroupedItem(ii, false);
						parent = ii->parent ? ii->parent : ii;
					} else if(ii == parent || !parent->collapsed) {
						updateItem(ctrlTransfers.findItem(ii), ui->updateMask);
					}
					continue;
				}
				ii->update(*ui);
				dcassert(pos != -1);
				updateItem(pos, ui->updateMask);
			}
		} else if(i->first == UPDATE_PARENT) {
			auto_ptr<UpdateInfo> ui(reinterpret_cast<UpdateInfo*>(i->second));
			ItemInfoList::ParentPair* pp = ctrlTransfers.findParentPair(ui->target);
			
			if(!pp) 
				continue;

			if(ui->user.user) {
				int pos = -1;
				ItemInfo* ii = findItem(*ui, pos);
				if(ii) {
					ii->status = ui->status;
					ii->statusString = ui->statusString;

					if(!pp->parent->collapsed) {
						updateItem(ctrlTransfers.findItem(ii), ui->updateMask);
					}
				}
			}

			pp->parent->update(*ui);
			updateItem(ctrlTransfers.findItem(pp->parent), ui->updateMask);
			}
		}
	

	if(!t.empty()) {
		ctrlTransfers.resort();
		if(t.size() > 2) {
			ctrlTransfers.SetRedraw(TRUE);
		}
	}
	
	return 0;
}

LRESULT TransferView::onRemoveFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const ItemInfo *ii = ctrlTransfers.getItemData(i);
		if(Transfer::TYPE_FULL_LIST)
		QueueManager::getInstance()->remove(Text::fromT(ii->getText(COLUMN_PATH) + Util::getFileName(ii->target)));
		else
		QueueManager::getInstance()->remove(Text::fromT(ii->getText(COLUMN_PATH) + ii->getText(COLUMN_FILE)));
	}

	return 0;
}

LRESULT TransferView::onSearchAlternates(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const ItemInfo *ii = ctrlTransfers.getItemData(i);

		TTHValue tth;
		if(QueueManager::getInstance()->getTTH(Text::fromT(ii->target), tth)) {
			WinUtil::searchHash(tth);
		}
	}

	return 0;
}
	
TransferView::ItemInfo::ItemInfo(const HintedUser& u, string aToken, bool aDownload) : user(u), download(aDownload), transferFailed(false),
	status(STATUS_WAITING), pos(0), size(0), actual(0), speed(0), timeLeft(0), totalTimeLeft(0)/*ttlf*/, ip(Util::emptyStringT), target(Util::emptyStringT),
	flagIndex(0), collapsed(true), parent(NULL), hits(-1), statusString(Util::emptyStringT), running(0), token(aToken) { }

void TransferView::ItemInfo::update(const UpdateInfo& ui) {
	if(ui.type != Transfer::TYPE_LAST)
		type = ui.type;
		
	if(ui.updateMask & UpdateInfo::MASK_STATUS) {
		status = ui.status;
	}
	if(ui.updateMask & UpdateInfo::MASK_STATUS_STRING) {
		// No slots etc from transfermanager better than disconnected from connectionmanager
		if(!transferFailed)
			statusString = ui.statusString;
		transferFailed = ui.transferFailed;
	}
	if(ui.updateMask & UpdateInfo::MASK_SIZE) {
		size = ui.size;
	}
	if(ui.updateMask & UpdateInfo::MASK_POS) {
		pos = ui.pos;
	}
	if(ui.updateMask & UpdateInfo::MASK_ACTUAL) {
		actual = ui.actual;
	}
	if(ui.updateMask & UpdateInfo::MASK_SPEED) {
		speed = ui.speed;
	}
	if(ui.updateMask & UpdateInfo::MASK_FILE) {
		target = ui.target;
	}
	if(ui.updateMask & UpdateInfo::MASK_TIMELEFT) {
		timeLeft = ui.timeLeft;
	}
	if(ui.updateMask & UpdateInfo::MASK_TOTALTIMELEFT) {
		totalTimeLeft = ui.totalTimeLeft;
	}/*ttlf*/
	if(ui.updateMask & UpdateInfo::MASK_IP) {
		flagIndex = ui.flagIndex;
		ip = ui.IP;
	}
	if(ui.updateMask & UpdateInfo::MASK_CIPHER) {
		cipher = ui.cipher;
	}	
	if(ui.updateMask & UpdateInfo::MASK_SEGMENT) {
		running = ui.running;
	}
}

void TransferView::updateItem(int ii, uint32_t updateMask) {
	if(	updateMask & UpdateInfo::MASK_STATUS || updateMask & UpdateInfo::MASK_STATUS_STRING ||
		updateMask & UpdateInfo::MASK_POS || updateMask & UpdateInfo::MASK_ACTUAL) {
		ctrlTransfers.updateItem(ii, COLUMN_STATUS);
	}
	if(updateMask & UpdateInfo::MASK_POS || updateMask & UpdateInfo::MASK_ACTUAL) {
		ctrlTransfers.updateItem(ii, COLUMN_RATIO);
	}
	if(updateMask & UpdateInfo::MASK_SIZE) {
		ctrlTransfers.updateItem(ii, COLUMN_SIZE);
	}
	if(updateMask & UpdateInfo::MASK_SPEED) {
		ctrlTransfers.updateItem(ii, COLUMN_SPEED);
	}
	if(updateMask & UpdateInfo::MASK_FILE) {
		ctrlTransfers.updateItem(ii, COLUMN_PATH);
		ctrlTransfers.updateItem(ii, COLUMN_FILE);
	}
	if(updateMask & UpdateInfo::MASK_TIMELEFT) {
		ctrlTransfers.updateItem(ii, COLUMN_TIMELEFT);
	}
	if(updateMask & UpdateInfo::MASK_TOTALTIMELEFT) {
		ctrlTransfers.updateItem(ii, COLUMN_TOTALTIMELEFT);
	}/*ttlf*/
	if(updateMask & UpdateInfo::MASK_IP) {
		ctrlTransfers.updateItem(ii, COLUMN_IP);
	}
	if(updateMask & UpdateInfo::MASK_SEGMENT) {
		ctrlTransfers.updateItem(ii, COLUMN_HUB);
	}
	if(updateMask & UpdateInfo::MASK_CIPHER) {
		ctrlTransfers.updateItem(ii, COLUMN_CIPHER);
	}	
}

void TransferView::on(ConnectionManagerListener::Added, const ConnectionQueueItem* aCqi) {
	UpdateInfo* ui = new UpdateInfo(aCqi->getUser(), aCqi->getToken(), aCqi->getDownload());

	if(ui->download) {
		string aTarget; int64_t aSize; int aFlags;
		if(QueueManager::getInstance()->getQueueInfo(aCqi->getUser(), aTarget, aSize, aFlags)) {
			Transfer::Type type = Transfer::TYPE_FILE;
			if(aFlags & QueueItem::FLAG_USER_LIST)
				type = Transfer::TYPE_FULL_LIST;
			else if(aFlags & QueueItem::FLAG_PARTIAL_LIST)
				type = Transfer::TYPE_PARTIAL_LIST;
			
			ui->setType(type);
			ui->setTarget(Text::toT(aTarget));
			ui->setSize(aSize);
		}
	}

	ui->setStatus(ItemInfo::STATUS_WAITING);
	ui->setStatusString(TSTRING(CONNECTING));

	speak(ADD_ITEM, ui);
}

void TransferView::on(ConnectionManagerListener::StatusChanged, const ConnectionQueueItem* aCqi) {
	UpdateInfo* ui = new UpdateInfo(aCqi->getUser(), aCqi->getToken(), aCqi->getDownload());
	string aTarget;	int64_t aSize; int aFlags = 0;

	if(QueueManager::getInstance()->getQueueInfo(aCqi->getUser(), aTarget, aSize, aFlags)) {
		Transfer::Type type = Transfer::TYPE_FILE;
		if(aFlags & QueueItem::FLAG_USER_LIST)
			type = Transfer::TYPE_FULL_LIST;
		else if(aFlags & QueueItem::FLAG_PARTIAL_LIST)
			type = Transfer::TYPE_PARTIAL_LIST;
	
		ui->setType(type);
		ui->setTarget(Text::toT(aTarget));
		ui->setSize(aSize);
	}

	ui->setStatusString(TSTRING(CONNECTING));
	ui->setStatus(ItemInfo::STATUS_WAITING);

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(ConnectionManagerListener::Removed, const ConnectionQueueItem* aCqi) {
	speak(REMOVE_ITEM, new UpdateInfo(aCqi->getUser(), aCqi->getToken(), aCqi->getDownload()));
}

void TransferView::on(ConnectionManagerListener::Failed, const ConnectionQueueItem* aCqi, const string& aReason) {
	UpdateInfo* ui = new UpdateInfo(aCqi->getUser(), aCqi->getToken(), aCqi->getDownload());
	if(aCqi->getUser().user->isSet(User::OLD_CLIENT)) {
		ui->setStatusString(TSTRING(SOURCE_TOO_OLD));
	} 
	else {
		ui->setStatusString(Text::toT(aReason));
	}
	ui->setStatus(ItemInfo::STATUS_WAITING);
	speak(UPDATE_ITEM, ui);
}

static tstring getFile(const Transfer::Type& type, const tstring& fileName) {
	tstring file;

	if(type == Transfer::TYPE_TREE) {
		file = _T("TTH: ") + fileName;
	} else if(type == Transfer::TYPE_FULL_LIST) {
		file = TSTRING(FILE_LIST);
	} else if(type == Transfer::TYPE_PARTIAL_LIST) {
		file = TSTRING(FILE_LIST_PARTIAL);
	} else {
		file = fileName;
	}
	return file;
}

const tstring TransferView::ItemInfo::getText(uint8_t col) const {
	switch(col) {
		case COLUMN_USER: return (hits == -1) ? WinUtil::getNicks(user) : (Util::toStringW(hits) + _T(' ') + TSTRING(USERS));
		case COLUMN_HUB: return (hits == -1) ? WinUtil::getHubNames(user).first : (Util::toStringW(running) + _T(' ') + TSTRING(NUMBER_OF_SEGMENTS));
		case COLUMN_STATUS: return statusString;
		case COLUMN_TIMELEFT: return (status == STATUS_RUNNING) ? Util::formatSeconds(timeLeft) : Util::emptyStringT;
		case COLUMN_TOTALTIMELEFT: return (status == STATUS_RUNNING) ? Util::formatSeconds(totalTimeLeft) : Util::emptyStringT;
		case COLUMN_SPEED: return (status == STATUS_RUNNING) ? (Util::formatBytesW(speed) + _T("/s")) : Util::emptyStringT;
		case COLUMN_FILE: return getFile(type, Util::getFileName(target));
		case COLUMN_SIZE: return Util::formatBytesW(size); 
		case COLUMN_PATH: return Util::getFilePath(target);
		case COLUMN_IP: return ip;
		case COLUMN_RATIO: return (status == STATUS_RUNNING) ? Util::toStringW(getRatio()) : Util::emptyStringT;
		case COLUMN_CIPHER: return cipher;
		default: return Util::emptyStringT;
	}
}
		
void TransferView::starting(UpdateInfo* ui, const Transfer* t) {
	ui->setPos(t->getPos());
	ui->setTarget(Text::toT(t->getPath()));
	ui->setType(t->getType());
	const UserConnection& uc = t->getUserConnection();
	ui->setCipher(Text::toT(uc.getCipherName()));
	const string& ip = uc.getRemoteIp();
	const string& country = Util::getIpCountry(ip);
	if(country.empty()) {
		ui->setIP(Text::toT(ip), 0);
	} else {
		ui->setIP(Text::toT(country + " (" + ip + ")"), WinUtil::getFlagIndexByCode(country.c_str()));
	}
}

void TransferView::on(DownloadManagerListener::Requesting, const Download* d) throw() {
	UpdateInfo* ui = new UpdateInfo(d->getHintedUser(), d->getUserConnection().getToken(), true);
	
	starting(ui, d);
	
	ui->setActual(d->getActual());
	ui->setSize(d->getSize());
	ui->setStatus(ItemInfo::STATUS_RUNNING);	ui->updateMask &= ~UpdateInfo::MASK_STATUS; // hack to avoid changing item status
	ui->setStatusString(TSTRING(REQUESTING) + _T(" ") + getFile(d->getType(), Text::toT(Util::getFileName(d->getPath()))) + _T("..."));

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(DownloadManagerListener::Starting, const Download* aDownload) {
	UpdateInfo* ui = new UpdateInfo(aDownload->getHintedUser(), aDownload->getUserConnection().getToken(), true);
	
	ui->setStatus(ItemInfo::STATUS_RUNNING);
	ui->setStatusString(TSTRING(DOWNLOAD_STARTING));
	ui->setTarget(Text::toT(aDownload->getPath()));
	ui->setType(aDownload->getType());
	
	speak(UPDATE_ITEM, ui);
}

void TransferView::on(DownloadManagerListener::Tick, const DownloadList& dl) {
	for(DownloadList::const_iterator j = dl.begin(); j != dl.end(); ++j) {
		Download* d = *j;
		UpdateInfo* ui = new UpdateInfo(d->getHintedUser(), d->getUserConnection().getToken(), true);
		ui->setStatus(ItemInfo::STATUS_RUNNING);
		ui->setActual(d->getActual());
		ui->setPos(d->getPos());
		ui->setSize(d->getSize());
		ui->setTimeLeft(d->getSecondsLeft());
		uint64_t timeleft = 0;
		/* ttlf */
		int64_t avg = DownloadManager::getInstance()->getAverageSpeed(d->getPath());	 
		uint64_t avgpos = DownloadManager::getInstance()->getAveragePos(d->getPath());	 
		uint64_t totalsize = QueueManager::getInstance()->fileQueue.getTotalSize(d->getPath());	 
		if(totalsize == 0)	 
			totalsize = avgpos;	 

		timeleft =  (avg > 0) ? ((totalsize - avgpos) / avg) : 0;	 
 	 
		if(timeleft >= 0){	 
			ui->setTotalTimeLeft(timeleft);	 
		}else{	 
			ui->setTotalTimeLeft(0);	 
		}
		
		ui->setSpeed(static_cast<int64_t>(d->getAverageSpeed()));
		ui->setType(d->getType());

		tstring pos = Util::formatBytesW(d->getPos());
		double percent = (double)d->getPos()*100.0/(double)d->getSize();
		tstring elapsed = Util::formatSeconds((GET_TICK() - d->getStart())/1000);

		tstring statusString;

		if(d->isSet(Download::FLAG_PARTIAL)) {
			statusString += _T("[P]");
		}
		if(d->getUserConnection().isSecure()) {
			if(d->getUserConnection().isTrusted()) {
				statusString += _T("[S]");
			} else {
				statusString += _T("[U]");
			}
		}
		if(d->isSet(Download::FLAG_TTH_CHECK)) {
			statusString += _T("[T]");
		}
		if(d->isSet(Download::FLAG_ZDOWNLOAD)) {
			statusString += _T("[Z]");
		}
		if(d->isSet(Download::FLAG_CHUNKED)) {
			statusString += _T("[C]");
		}
		if(d->getUserConnection().isSet(UserConnection::FLAG_MCN1)) {
			statusString += _T("[M]");
		}
		if(!statusString.empty()) {
			statusString += _T(" ");
		}
		statusString += Text::tformat(TSTRING(DOWNLOADED_BYTES), pos.c_str(), percent, elapsed.c_str());
		ui->setStatusString(statusString);
			
		tasks.add(UPDATE_ITEM, ui);
	}

	PostMessage(WM_SPEAKER);
}

void TransferView::on(DownloadManagerListener::Failed, const Download* aDownload, const string& aReason) {
	UpdateInfo* ui = new UpdateInfo(aDownload->getHintedUser(), aDownload->getUserConnection().getToken(), true, true);
	ui->setStatus(ItemInfo::STATUS_WAITING);
	ui->setPos(0);
	ui->setSize(aDownload->getSize());
	ui->setTarget(Text::toT(aDownload->getPath()));
	ui->setType(aDownload->getType());

	tstring tmpReason = Text::toT(aReason);
	if(aDownload->isSet(Download::FLAG_SLOWUSER)) {
		tmpReason += _T(": ") + TSTRING(SLOW_USER);
	} else if(aDownload->getOverlapped() && !aDownload->isSet(Download::FLAG_OVERLAP)) {
		tmpReason += _T(": ") + TSTRING(OVERLAPPED_SLOW_SEGMENT);
	}

	ui->setStatusString(tmpReason);

	if(BOOLSETTING(POPUP_DOWNLOAD_FAILED)) {
		MainFrame::getMainFrame()->ShowBalloonTip(
			TSTRING(FILE) + _T(": ") + Util::getFileName(ui->target) + _T("\n")+
			TSTRING(USER) + _T(": ") + WinUtil::getNicks(ui->user) + _T("\n")+
			TSTRING(REASON) + _T(": ") + Text::toT(aReason), TSTRING(DOWNLOAD_FAILED), NIIF_WARNING);
	}

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(DownloadManagerListener::Status, const UserConnection* uc, const string& aReason) {
	UpdateInfo* ui = new UpdateInfo(uc->getHintedUser(), uc->getToken(), true);
	ui->setStatus(ItemInfo::STATUS_WAITING);
	ui->setPos(0);
	ui->setStatusString(Text::toT(aReason));

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(UploadManagerListener::Starting, const Upload* aUpload) {
	UpdateInfo* ui = new UpdateInfo(aUpload->getHintedUser(), aUpload->getUserConnection().getToken(), false);

	starting(ui, aUpload);
	
	ui->setStatus(ItemInfo::STATUS_RUNNING);
	ui->setActual(aUpload->getStartPos() + aUpload->getActual());
	ui->setSize(aUpload->getType() == Transfer::TYPE_TREE ? aUpload->getSize() : aUpload->getFileSize());
	ui->setRunning(1);
	
	if(!aUpload->isSet(Upload::FLAG_RESUMED)) {
		ui->setStatusString(TSTRING(UPLOAD_STARTING));
	}

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(UploadManagerListener::Tick, const UploadList& ul) {
	for(UploadList::const_iterator j = ul.begin(); j != ul.end(); ++j) {
		Upload* u = *j;

		if (u->getPos() == 0) continue;

		UpdateInfo* ui = new UpdateInfo(u->getHintedUser(), u->getUserConnection().getToken(), false);
		ui->setActual(u->getStartPos() + u->getActual());
		ui->setPos(u->getStartPos() + u->getPos());
		ui->setTimeLeft(u->getSecondsLeft(true)); // we are interested when whole file is finished and not only one chunk
		ui->setTotalTimeLeft(u->getSecondsLeft(true));
		/*ttlf - update*/
		ui->setSpeed(static_cast<int64_t>(u->getAverageSpeed()));

		tstring pos = Util::formatBytesW(ui->pos);
		double percent = (double)ui->pos*100.0/(double)(u->getType() == Transfer::TYPE_TREE ? u->getSize() : u->getFileSize());
		tstring elapsed = Util::formatSeconds((GET_TICK() - u->getStart())/1000); 
		
		tstring statusString;

		if(u->isSet(Upload::FLAG_PARTIAL)) {
			statusString += _T("[P]");
		}
		if(u->getUserConnection().isSecure()) {
			if(u->getUserConnection().isTrusted()) {
				statusString += _T("[S]");
			} else {
				statusString += _T("[U]");
			}
		}
		if(u->isSet(Upload::FLAG_ZUPLOAD)) {
			statusString += _T("[Z]");
		}
		if(u->isSet(Upload::FLAG_CHUNKED)) {
			statusString += _T("[C]");
		}
		if(u->getUserConnection().isSet(UserConnection::FLAG_MCN1)) {
			statusString += _T("[M]");
		}
		if(!statusString.empty()) {
			statusString += _T(" ");
		}			
		statusString += Text::tformat(TSTRING(UPLOADED_BYTES), pos.c_str(), percent, elapsed.c_str());

		ui->setStatusString(statusString);
					
		tasks.add(UPDATE_ITEM, ui);
	}

	PostMessage(WM_SPEAKER);
}

void TransferView::onTransferComplete(const Transfer* aTransfer, bool isUpload, const string& aFileName, bool isTree) {
	UpdateInfo* ui = new UpdateInfo(aTransfer->getHintedUser(), aTransfer->getUserConnection().getToken(), !isUpload);

	ui->setStatus(ItemInfo::STATUS_WAITING);	
	ui->setPos(0);
	ui->setStatusString(isUpload ? TSTRING(UPLOAD_FINISHED_IDLE) : TSTRING(DOWNLOAD_FINISHED_IDLE));
	ui->setRunning(0);

	if(isUpload && BOOLSETTING(POPUP_UPLOAD_FINISHED) && !isTree) {
		MainFrame::getMainFrame()->ShowBalloonTip(
			TSTRING(FILE) + _T(": ") + Text::toT(aFileName) + _T("\n")+
			TSTRING(USER) + _T(": ") + WinUtil::getNicks(aTransfer->getHintedUser()), TSTRING(UPLOAD_FINISHED_IDLE));
	}
	
	speak(UPDATE_ITEM, ui);
}

void TransferView::ItemInfo::disconnect() {
	ConnectionManager::getInstance()->disconnect(token);
}
		
LRESULT TransferView::onPreviewCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const ItemInfo *ii = ctrlTransfers.getItemData(i);

		const QueueItem::StringMap& queue = QueueManager::getInstance()->lockQueue();

		string tmp = Text::fromT(ii->target);
		QueueItem::StringIter qi = queue.find(&tmp);

		string aTempTarget;
		if(qi != queue.end())
			aTempTarget = qi->second->getTempTarget();

		QueueManager::getInstance()->unlockQueue();

		WinUtil::RunPreviewCommand(wID - IDC_PREVIEW_APP, aTempTarget);
	}

	return 0;
}

void TransferView::CollapseAll() {
	for(int q = ctrlTransfers.GetItemCount()-1; q != -1; --q) {
		ItemInfo* m = (ItemInfo*)ctrlTransfers.getItemData(q);
		if(m->download && m->parent) {
			ctrlTransfers.deleteItem(m); 
		}
		if(m->download && !m->parent && !m->collapsed) {
			m->collapsed = true;
			ctrlTransfers.SetItemState(ctrlTransfers.findItem(m), INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
		 }
	}
}

void TransferView::ExpandAll() {
	for(ItemInfoList::ParentMap::const_iterator i = ctrlTransfers.getParents().begin(); i != ctrlTransfers.getParents().end(); ++i) {
		ItemInfo* l = (*i).second.parent;
		if(l->collapsed) {
			ctrlTransfers.Expand(l, ctrlTransfers.findItem(l));
		}
	}
}

LRESULT TransferView::onDisconnectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const ItemInfo* ii = ctrlTransfers.getItemData(i);
		
		const vector<ItemInfo*>& children = ctrlTransfers.findChildren(ii->getGroupCond());
		for(vector<ItemInfo*>::const_iterator j = children.begin(); j != children.end(); ++j) {
			ItemInfo* ii = *j;
			ii->disconnect();

			int h = ctrlTransfers.findItem(ii);
			if(h != -1)
				ctrlTransfers.SetItemText(h, COLUMN_STATUS, CTSTRING(DISCONNECTED));
		}

		ctrlTransfers.SetItemText(i, COLUMN_STATUS, CTSTRING(DISCONNECTED));
	}
	return 0;
}

LRESULT TransferView::onSlowDisconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const ItemInfo *ii = ctrlTransfers.getItemData(i);

		const QueueItem::StringMap& queue = QueueManager::getInstance()->lockQueue();

		string tmp = Text::fromT(ii->target);
		QueueItem::StringIter qi = queue.find(&tmp);

		if(qi != queue.end()) {
			if(qi->second->isSet(QueueItem::FLAG_AUTODROP)) {
				qi->second->unsetFlag(QueueItem::FLAG_AUTODROP);
			} else {
				qi->second->setFlag(QueueItem::FLAG_AUTODROP);
			}
		}

		QueueManager::getInstance()->unlockQueue();
	}

	return 0;
}

void TransferView::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw() {
	bool refresh = false;
	if(ctrlTransfers.GetBkColor() != WinUtil::bgColor) {
		ctrlTransfers.SetBkColor(WinUtil::bgColor);
		ctrlTransfers.SetTextBkColor(WinUtil::bgColor);
		ctrlTransfers.setFlickerFree(WinUtil::bgBrush);
		refresh = true;
	}
	if(ctrlTransfers.GetTextColor() != WinUtil::textColor) {
		ctrlTransfers.SetTextColor(WinUtil::textColor);
		refresh = true;
	}

	if(noGroup != BOOLSETTING(DOWNLOADS_EXPAND))
		refresh = true;

	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

void TransferView::on(QueueManagerListener::StatusUpdated, const QueueItem* qi) throw() {
	UpdateInfo* ui = new UpdateInfo(const_cast<QueueItem*>(qi), true);
	ui->setTarget(Text::toT(qi->getTarget()));

	Transfer::Type type = Transfer::TYPE_FILE;
	if(qi->getFlags() & QueueItem::FLAG_USER_LIST)
		type = Transfer::TYPE_FULL_LIST;
	else if(qi->getFlags() & QueueItem::FLAG_PARTIAL_LIST)
		type = Transfer::TYPE_PARTIAL_LIST;

	ui->setType(type);

	if(qi->isRunning() ) {
		double ratio = 0;
		int64_t totalSpeed = 0;
		int16_t segs = 0;
		
		bool partial = false, trusted = false, untrusted = false, tthcheck = false, zdownload = false, chunked = false, mcn = false;

		for(DownloadList::const_iterator i = qi->getDownloads().begin(); i != qi->getDownloads().end(); i++) {
			Download *d = *i;

			if(d->getStart() > 0) {
				segs++;

				if(d->isSet(Download::FLAG_PARTIAL)) {
					partial = true;
				}
				if(d->getUserConnection().isSecure()) {
					if(d->getUserConnection().isTrusted()) {
						trusted = true;
					} else {
						untrusted = true;
					}
				}
				if(d->isSet(Download::FLAG_TTH_CHECK)) {
					tthcheck = true;
				}
				if(d->isSet(Download::FLAG_ZDOWNLOAD)) {
					zdownload = true;
				}
				if(d->isSet(Download::FLAG_CHUNKED)) {
					chunked = true;
				}
				if(d->getUserConnection().isSet(UserConnection::FLAG_MCN1)) {
					mcn=true;
				}
		
				totalSpeed += static_cast<int64_t>(d->getAverageSpeed());
				ratio += d->getPos() > 0 ? (double)d->getActual() / (double)d->getPos() : 1.00;
			}
		}

		ui->setRunning(segs);
		if(segs > 0) {
			ratio = ratio / segs;

			ui->setStatus(ItemInfo::STATUS_RUNNING);
			ui->setSize(qi->getSize());
			ui->setPos(qi->getDownloadedBytes());
			ui->setActual((int64_t)((double)ui->pos * (ratio == 0 ? 1.00 : ratio)));
			ui->setTimeLeft((totalSpeed > 0) ? ((ui->size - ui->pos) / totalSpeed) : 0);
			/*ttlf*/
			//Original version freezed here, split up, removed lockqueue
			/*Folder averages added, so its correct now -Night */
			uint64_t timeleft = 0;
			if(totalSpeed > 0){
				int64_t avg = DownloadManager::getInstance()->getAverageSpeed(qi->getTarget());
				uint64_t pos = DownloadManager::getInstance()->getAveragePos(qi->getTarget());
				uint64_t totalsize = QueueManager::getInstance()->fileQueue.getTotalSize(qi->getTarget());
				if(totalsize == 0)
				totalsize = pos;
				
				timeleft =  (avg > 0) ? ((totalsize - pos) / avg) : 0;
			}
			if(timeleft >= 0){
				ui->setTotalTimeLeft(timeleft);
			}else{
				ui->setTotalTimeLeft(0);
			}

			ui->setSpeed(totalSpeed);

			if(qi->getFileBegin() == 0) {
				// file is starting
				const_cast<QueueItem*>(qi)->setFileBegin(GET_TICK());

				ui->setStatusString(TSTRING(DOWNLOAD_STARTING));
				if ((!SETTING(BEGINFILE).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
					PlaySound(Text::toT(SETTING(BEGINFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);

				if(BOOLSETTING(POPUP_DOWNLOAD_START)) {
					MainFrame::getMainFrame()->ShowBalloonTip(TSTRING(FILE) + _T(": ") + Util::getFileName(ui->target), TSTRING(DOWNLOAD_STARTING));
				}
			} else {
				uint64_t time = GET_TICK() - qi->getFileBegin();
				if(time > 1000) {
					tstring pos = Util::formatBytesW(ui->pos);
					double percent = (double)ui->pos*100.0/(double)ui->size;
					tstring elapsed = Util::formatSeconds(time/1000);
					tstring flag;
					
					if(partial) {
						flag += _T("[P]");
					}
					if(trusted) {
						flag += _T("[S]");
					}
					if(untrusted) {
						flag += _T("[U]");
					}
					if(tthcheck) {
						flag += _T("[T]");
					}
					if(zdownload) {
						flag += _T("[Z]");
					}
					if(chunked) {
						flag += _T("[C]");
					}
					if(mcn) {
						flag += _T("[M]");
					}

					if(!flag.empty()) {
						flag += _T(" ");
					}
					
					ui->setStatusString(flag + Text::tformat(TSTRING(DOWNLOADED_BYTES), pos.c_str(), percent, elapsed.c_str()));
				}
			}
		}
	} else {
		const_cast<QueueItem*>(qi)->setFileBegin(0);

		ui->setSize(qi->getSize());
		ui->setStatus(ItemInfo::STATUS_WAITING);
		ui->setRunning(0);
	}

	speak(UPDATE_PARENT, ui);
}

void TransferView::on(QueueManagerListener::Finished, const QueueItem* qi, const string&, const Download* download) throw() {
	// update download item
	UpdateInfo* ui = new UpdateInfo(download->getHintedUser(), download->getUserConnection().getToken(), true);

	ui->setStatus(ItemInfo::STATUS_WAITING);	
	ui->setPos(0);
	ui->setStatusString( TSTRING(DOWNLOAD_FINISHED_IDLE));

	speak(UPDATE_ITEM, ui);

	// update file item
	ui = new UpdateInfo(const_cast<QueueItem*>(qi), true, true);
	ui->user = download->getHintedUser();
	ui->token = download->getUserConnection().getToken();

	ui->setTarget(Text::toT(qi->getTarget()));
	ui->setPos(0);
	ui->setActual(0);
	ui->setTimeLeft(0);
	ui->setTotalTimeLeft(0);/*ttlf*/
	ui->setStatusString(TSTRING(DOWNLOAD_FINISHED_IDLE));
	ui->setStatus(ItemInfo::STATUS_WAITING);
	ui->setRunning(0);
	
	if(BOOLSETTING(POPUP_DOWNLOAD_FINISHED)) {
		MainFrame::getMainFrame()->ShowBalloonTip(TSTRING(FILE) + _T(": ") + Util::getFileName(ui->target), TSTRING(DOWNLOAD_FINISHED_IDLE));
	}

	speak(UPDATE_PARENT, ui);
}

void TransferView::on(QueueManagerListener::Removed, const QueueItem* qi) throw() {
	UpdateInfo* ui = new UpdateInfo(const_cast<QueueItem*>(qi), true);
	ui->setTarget(Text::toT(qi->getTarget()));
	ui->setPos(0);
	ui->setActual(0);
	ui->setTimeLeft(0);
	ui->setTotalTimeLeft(0);/*ttlf*/
	ui->setStatusString(TSTRING(DISCONNECTED));
	ui->setStatus(ItemInfo::STATUS_WAITING);
	ui->setRunning(0);

	speak(UPDATE_PARENT, ui);
}

LRESULT TransferView::onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	string sCopy;
	if(ctrlTransfers.GetSelectedCount() == 1) {
		const ItemInfo* ii = ctrlTransfers.getSelectedItem();
		tstring buf;

		switch (wID) {
			case IDC_COPY_NICK:
				buf = ii->getText(COLUMN_USER);
				sCopy = Text::fromT(buf);
				break;
			case IDC_COPY_FILENAME:
				buf = ii->getText(COLUMN_FILE);
				sCopy = Text::fromT(buf);
				break;
			case IDC_COPY_SIZE:
				buf = ii->getText(COLUMN_SIZE);
				sCopy = Text::fromT(buf);
				break;
			case IDC_COPY_PATH:
				buf = ii->getText(COLUMN_PATH);
				sCopy = Text::fromT(buf);
				break;
			case IDC_COPY_IP:
				buf = ii->getText(COLUMN_IP);
				sCopy = Text::fromT(buf);
				break;
			case IDC_COPY_HUB:
				buf = ii->getText(COLUMN_HUB);
				sCopy = Text::fromT(buf);
				break;
			case IDC_COPY_SPEED:
				buf = ii->getText(COLUMN_SPEED);
				sCopy = Text::fromT(buf);
				break;
			case IDC_COPY_STATUS:
				buf = ii->getText(COLUMN_STATUS);
				sCopy = Text::fromT(buf);
				break;
			case IDC_COPY_ALL:
				buf = (ii->getText(COLUMN_USER) + _T("   ") + ii->getText(COLUMN_HUB) + _T("   ") + ii->getText(COLUMN_FILE) + _T("   ") + ii->getText(COLUMN_SIZE) + _T("   ") + ii->getText(COLUMN_PATH) + _T("   ") + ii->getText(COLUMN_IP)  + _T("   ") + ii->getText(COLUMN_SPEED)+  _T("  ") + ii->getText(COLUMN_STATUS));
				sCopy = Text::fromT(buf);
				break;

			default:
				dcdebug("TRANSFERVIEW DON'T GO HERE\n");
				return 0;
		}
		
		if (!sCopy.empty())
			WinUtil::setClipboard(Text::toT(sCopy));
	}
	return S_OK;
}


/**
 * @file
 * $Id: TransferView.cpp 500 2010-06-25 22:08:18Z bigmuscle $
 */
