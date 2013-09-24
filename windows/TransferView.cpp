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
#include "Resource.h"

#include "../client/ResourceManager.h"
#include "../client/SettingsManager.h"
#include "../client/DownloadManager.h"
#include "../client/UploadManager.h"
#include "../client/QueueManager.h"
#include "../client/QueueItem.h"
#include "../client/GeoManager.h"
#include "../client/AirUtil.h"
#include "../client/Localization.h"
#include "../client/version.h"

#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm_ext/for_each.hpp>

#include "WinUtil.h"
#include "TransferView.h"
#include "MainFrm.h"
#include "ResourceLoader.h"

#include "BarShader.h"

int TransferView::columnIndexes[] = { COLUMN_USER, COLUMN_FILE, COLUMN_HUB, COLUMN_STATUS, COLUMN_TIMELEFT, COLUMN_SPEED, COLUMN_SIZE, COLUMN_PATH, COLUMN_CIPHER, COLUMN_IP, COLUMN_RATIO };
int TransferView::columnSizes[] = { 150, 250, 150, 275, 75, 75, 75, 200, 100, 175, 50 };

static ResourceManager::Strings columnNames[] = { ResourceManager::USER, ResourceManager::BUNDLE_FILENAME, ResourceManager::HUB_SEGMENTS, ResourceManager::STATUS,
	ResourceManager::TIME_LEFT, ResourceManager::SPEED, ResourceManager::SIZE, ResourceManager::PATH,
	ResourceManager::CIPHER, ResourceManager::IP_BARE, ResourceManager::RATIO};

TransferView::~TransferView() {
	arrows.Destroy();
	OperaColors::ClearCache();
}

LRESULT TransferView::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	
	arrows.Create(16, 16, ILC_COLOR32 | ILC_MASK,  0, 3);
	arrows.AddIcon(ResourceLoader::loadIcon(IDI_DOWNLOAD, 16));
	arrows.AddIcon(ResourceLoader::loadIcon(IDI_UPLOAD, 16));
	arrows.AddIcon(ResourceLoader::loadIcon(IDI_D_USER, 16));
	arrows.AddIcon(ResourceLoader::loadIcon(IDI_U_USER, 16));

	ctrlTransfers.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_TRANSFERS);
	ctrlTransfers.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);

	WinUtil::splitTokens(columnIndexes, SETTING(TRANSFERVIEW_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(TRANSFERVIEW_WIDTHS), COLUMN_LAST);

	for(uint8_t j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_TIMELEFT || j == COLUMN_SPEED) ? LVCFMT_RIGHT : LVCFMT_LEFT;
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

	ConnectionManager::getInstance()->addListener(this);
	DownloadManager::getInstance()->addListener(this);
	UploadManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);
	return 0;
}

void TransferView::prepareClose() {
	ctrlTransfers.saveHeaderOrder(SettingsManager::TRANSFERVIEW_ORDER, SettingsManager::TRANSFERVIEW_WIDTHS,
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

void TransferView::setDefaultItem(OMenu& aMenu) {
	switch(SETTING(TRANSFERLIST_DBLCLICK)) {
		case 0:
		    aMenu.SetMenuDefaultItem(IDC_PRIVATEMESSAGE);
		    break;
		case 1:
		    aMenu.SetMenuDefaultItem(IDC_GETLIST);
		    break;
		case 2:
		    aMenu.SetMenuDefaultItem(IDC_MATCH_QUEUE);
		    break;
		case 3:
		    aMenu.SetMenuDefaultItem(IDC_GRANTSLOT);
		    break;
		case 4:
		    aMenu.SetMenuDefaultItem(IDC_ADD_TO_FAVORITES);
		    break;
		case 5:
		    aMenu.SetMenuDefaultItem(IDC_BROWSELIST);
		    break;
			  
	}
}

LRESULT TransferView::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	auto selCount = ctrlTransfers.GetSelectedCount();
	if (reinterpret_cast<HWND>(wParam) == ctrlTransfers && ctrlTransfers.GetSelectedCount() > 0) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlTransfers, pt);
		}

		OMenu transferMenu;
		const auto ii = ctrlTransfers.getItemData(ctrlTransfers.GetNextItem(-1, LVNI_SELECTED));
		bool parent = ii->isBundle;

		transferMenu.CreatePopupMenu();
		
		if(!parent) {
			transferMenu.InsertSeparatorFirst(TSTRING(MENU_TRANSFERS));
			appendUserItems(transferMenu);
			setDefaultItem(transferMenu);
			
			transferMenu.appendItem(TSTRING(REMOVE_FILE), [=] { handleRemoveFile(); });
			transferMenu.appendSeparator();
			transferMenu.appendItem(TSTRING(FORCE_ATTEMPT), [=] { handleForced(); });

			if(ii->download) {
				transferMenu.appendSeparator();
				transferMenu.appendItem(TSTRING(SEARCH_FOR_ALTERNATES), [=] { handleSearchAlternates(); });

				if(!ii->target.empty() && selCount == 1) {
					WinUtil::appendPreviewMenu(transferMenu, Text::fromT(ii->target));
				}
			}

			int i = -1;
			if((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
				const auto itemI = ctrlTransfers.getItemData(i);
				if(itemI->user.user)
					prepareMenu(transferMenu, UserCommand::CONTEXT_USER, ClientManager::getInstance()->getHubUrls(itemI->user.user->getCID()));
			}

			ctrlTransfers.appendCopyMenu(transferMenu);
			transferMenu.appendSeparator();
			transferMenu.appendItem(TSTRING(CLOSE_CONNECTION), [=] { handleDisconnect(); });
		} else {
			transferMenu.InsertSeparatorFirst(TSTRING(BUNDLE));
			if (ii->users == 1) {
				appendUserItems(transferMenu);
				setDefaultItem(transferMenu);
				if(ii->download)
					transferMenu.appendItem(TSTRING(REMOVE_BUNDLE_SOURCE), [=] { handleRemoveBundleSource(); });
			}
			transferMenu.appendSeparator();
			transferMenu.appendItem(TSTRING(OPEN_FOLDER), [=] { handleOpenFolder(); });
			ctrlTransfers.appendCopyMenu(transferMenu);
			transferMenu.appendItem(TSTRING(SEARCH_DIRECTORY), [=] { handleSearchDir(); });
			transferMenu.appendSeparator();

			transferMenu.appendItem(TSTRING(CONNECT_ALL), [=] { handleForced(); });
			transferMenu.appendItem(TSTRING(DISCONNECT_ALL), [=] { handleDisconnect(); });
			transferMenu.appendSeparator();

			transferMenu.appendItem(TSTRING(EXPAND_ALL), [=] { handleExpandAll(); });
			transferMenu.appendItem(TSTRING(COLLAPSE_ALL), [=] { handleCollapseAll(); });
		}


		BundleList bundles;
		performActionBundles([&](const ItemInfo* ii) {
			if (ii->download) {
				auto b = QueueManager::getInstance()->findBundle(ii->bundle);
				if (b)
					bundles.push_back(b);
			}
		});

		if (!bundles.empty()) {
			transferMenu.appendSeparator();
			WinUtil::appendBundlePrioMenu(transferMenu, bundles);

			auto usingDisconnect = all_of(bundles.begin(), bundles.end(), [](const BundlePtr& aBundle) { return aBundle->isSet(Bundle::FLAG_AUTODROP); });
			transferMenu.appendItem(TSTRING(SETCZDC_DISCONNECTING_ENABLE), [=] { handleSlowDisconnect(); }, usingDisconnect ? OMenu::FLAG_CHECKED : 0);

			transferMenu.appendItem(TSTRING(REMOVE_BUNDLE), [=] { handleRemoveBundle(); });
		}

		transferMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
		return TRUE; 
	} else {
		bHandled = FALSE;
		return FALSE;
	}
}

void TransferView::runUserCommand(UserCommand& uc) {
	if(!WinUtil::getUCParams(m_hWnd, uc, ucLineParams))
		return;

	auto ucParams = ucLineParams;

	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const auto itemI = ctrlTransfers.getItemData(i);
		if(!itemI->user.user || !itemI->user.user->isOnline())
			continue;

		auto tmp = ucParams;
		ucParams["fileFN"] = Text::fromT(itemI->target);

		// compatibility with 0.674 and earlier
		ucParams["file"] = ucParams["fileFN"];
		
		ClientManager::getInstance()->userCommand(itemI->user, uc, tmp, true);
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
		auto ii = reinterpret_cast<ItemInfo*>(cd->nmcd.lItemlParam);

		int colIndex = ctrlTransfers.findColumn(cd->iSubItem);
		cd->clrTextBk = WinUtil::bgColor;

		if((colIndex == COLUMN_STATUS) && (ii->status != ItemInfo::STATUS_WAITING)) {
			if(!SETTING(SHOW_PROGRESS_BARS)) {
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
			if(SETTING(PROGRESSBAR_ODC_STYLE)) {
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

				rc.right = rc.left + (int) ((int64_t)rc.Width() * ii->actual / ii->size);

				COLORREF a, b;
				OperaColors::EnlightenFlood(clr, a, b);
				OperaColors::FloodFill(cdc, rc.left+1, rc.top+1,  rc.right, rc.bottom-1, a, b);
				
				// Draw the text only over the bar and with correct color
				::SetTextColor(dc, SETTING(PROGRESS_OVERRIDE_COLORS2) ? 
					(ii->download ? SETTING(PROGRESS_TEXT_COLOR_DOWN) : SETTING(PROGRESS_TEXT_COLOR_UP)) : 
					OperaColors::TextFromBackground(clr));

				rc.right += 1;
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
		} else if(SETTING(GET_USER_COUNTRY) && (colIndex == COLUMN_IP)) {
			auto ii = (ItemInfo*)cd->nmcd.lItemlParam;
			CRect rc;
			ctrlTransfers.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, rc);

			SetTextColor(cd->nmcd.hdc, WinUtil::textColor);
			DrawThemeBackground(GetWindowTheme(ctrlTransfers.m_hWnd), cd->nmcd.hdc, LVP_LISTITEM, 3, &rc, &rc );

			TCHAR buf[256];
			ctrlTransfers.GetItemText((int)cd->nmcd.dwItemSpec, cd->iSubItem, buf, 255);
			buf[255] = 0;
			if(_tcslen(buf) > 0) {
				rc.left += 2;
				LONG top = rc.top + (rc.Height() - 15)/2;
				if((top - rc.top) < 2)
					top = rc.top + 1;

				POINT p = { rc.left, top };
				ResourceLoader::flagImages.Draw(cd->nmcd.hdc, ii->flagIndex, p, LVSIL_SMALL);
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

		auto i = ctrlTransfers.getItemData(item->iItem);
		if (!i->isBundle) {
			switch(SETTING(TRANSFERLIST_DBLCLICK)) {
				case 0:
					i->pm();
					break;
				case 1:
					i->getList();
					break;
				case 2:
					i->matchQueue();
				case 3:
					i->grant();
					break;
				case 4:
					i->addFav();
					break;
				case 5:
					i->browseList();
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
		case COLUMN_SPEED: return compare(a->speed, b->speed);
		case COLUMN_SIZE: return compare(a->size, b->size);
		case COLUMN_RATIO: return compare(a->getRatio(), b->getRatio());
		default: return Util::DefaultSort(a->getText(col).c_str(), b->getText(col).c_str());
	}
}

TransferView::ItemInfo* TransferView::findItem(const UpdateInfo& ui, int& pos) const {
	for(int j = 0; j < ctrlTransfers.GetItemCount(); ++j) {
		auto ii = ctrlTransfers.getItemData(j);
		if(compare(ui.token, ii->token) == 0) {
			pos = j;
			return ii;
		} else if(ii->isBundle) {
			const auto& children = ctrlTransfers.findChildren(ii->getGroupCond());
			for(const auto& ii: children) {
				if(compare(ui.token, ii->token) == 0) {
					return ii;
				}
			}
		}
	}
	return nullptr;
}

LRESULT TransferView::onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	TaskQueue::List t;
	tasks.get(t);
	if(t.size() > 2) {
		ctrlTransfers.SetRedraw(FALSE);
	}

	for(auto& i: t) {
		if(i.first == ADD_ITEM) {
			auto &ui = static_cast<UpdateInfo&>(*i.second);
			//LogManager::getInstance()->message("Transferview, ADD_ITEM: " + ui.token);
			auto ii = new ItemInfo(ui.user, ui.token, ui.download);
			ii->update(ui);
			if (!ii->bundle.empty()) {
				ctrlTransfers.insertBundle(ii, SETTING(EXPAND_BUNDLES));
			} else {
				ctrlTransfers.insertItem(ii, ii->download ? IMAGE_DOWNLOAD : IMAGE_UPLOAD);
			}
		} else if(i.first == REMOVE_ITEM) {
			auto &ui = static_cast<UpdateInfo&>(*i.second);
			//LogManager::getInstance()->message("Transferview, REMOVE_ITEM: " + ui.token);

			int pos = -1;
			auto ii = findItem(ui, pos);
			if(ii) {
				if (!ii->bundle.empty()) {
					auto parent = ii->parent;
					if (ctrlTransfers.removeBundle(ii, true))
						parent->updateUser(ctrlTransfers.findChildren(parent->getGroupCond()));
				} else {
					dcassert(pos != -1);
					ctrlTransfers.DeleteItem(pos);
					delete ii;
				}
			}
		} else if(i.first == UPDATE_ITEM) {
			auto &ui = static_cast<UpdateInfo&>(*i.second);
			//dcassert(!ui.target.empty());
			int pos = -1;
			auto ii = findItem(ui, pos);
			if(ii) {
				if(!ii->bundle.empty() || !ui.bundle.empty())  {
					auto parent = ii->parent ? ii->parent : ii;

					/* if bundle has changed, regroup the item */
					bool changeParent = (ui.bundle != ii->bundle);
					if (parent->isBundle) {
						if (!ui.IP.empty() && parent->users == 1) {
							parent->cipher = ui.cipher;
							parent->ip = ui.IP;
							parent->flagIndex = ui.flagIndex;
							updateItem(ctrlTransfers.findItem(parent), UpdateInfo::MASK_CIPHER | UpdateInfo::MASK_IP);
						} else if (parent->users > 1 && !parent->ip.empty()) {
							parent->cipher = Util::emptyStringT;
							parent->ip = Util::emptyStringT;
							parent->flagIndex = 0;
							updateItem(ctrlTransfers.findItem(parent), UpdateInfo::MASK_CIPHER | UpdateInfo::MASK_IP);
						}
					}

					if(changeParent) {
						//LogManager::getInstance()->message("CHANGEPARENT, REMOVE");
						if (ii->bundle.empty()) {
							ctrlTransfers.DeleteItem(pos);
						} else if (ctrlTransfers.removeBundle(ii, false)) {
							parent->updateUser(ctrlTransfers.findChildren(parent->getGroupCond()));
						}
					}
					ii->update(ui);

					if(changeParent) {
						if (ii->bundle.empty()) {
							ii->parent = nullptr;
							ctrlTransfers.insertItem(ii, ii->download ? IMAGE_DOWNLOAD : IMAGE_UPLOAD);
						} else {
							ctrlTransfers.insertBundle(ii, ii->parent ? !ii->parent->collapsed : SETTING(EXPAND_BUNDLES));
						}
					} else if(ii == parent || !parent->collapsed) {
						updateItem(ctrlTransfers.findItem(ii), ui.updateMask);
					}

					continue;
				}
				ii->update(ui);
				dcassert(pos != -1);
				updateItem(pos, ui.updateMask);
			}
		} else if(i.first == UPDATE_BUNDLE) {
			auto &ui = static_cast<UpdateInfo&>(*i.second);

			auto pp = ctrlTransfers.findParentPair(ui.token);
			if(!pp) {
				continue;
			}

			/*if(ui.user.user || !ui.bundle.empty()) {
				int pos = -1;
				ItemInfo* ii = findItem(ui, pos);
				if(ii) {
					ii->status = ui.status;
					ii->statusString = ui.statusString;

					if(!pp->parent->collapsed) {
						updateItem(ctrlTransfers.findItem(ii), ui.updateMask);
					}
				}
			} */

			pp->parent->update(ui);
			updateItem(ctrlTransfers.findItem(pp->parent), ui.updateMask);
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

void TransferView::ItemInfo::updateUser(const vector<ItemInfo*>& aChildren) {
	HintedUser u;
	for(const auto& ii: aChildren) {
		if (!u.user) {
			u = ii->user;
		} else if (u.user != ii->user.user) {
			return;
		}
	}

	if (u.user)
		user = u;
}

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
	if(ui.updateMask & UpdateInfo::MASK_TOTALSPEED) {
		totalSpeed = ui.totalSpeed;
	}
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
	if(ui.updateMask & UpdateInfo::MASK_BUNDLE) {
		bundle = ui.bundle;
	}
	if(ui.updateMask & UpdateInfo::MASK_USERS) {
		users = ui.users;
	}
	if(ui.updateMask & UpdateInfo::MASK_USER) {
		user = ui.user;
	}
	if(ui.updateMask & UpdateInfo::MASK_PRIORITY) {
		prio = ui.prio;
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
	if(updateMask & UpdateInfo::MASK_IP) {
		ctrlTransfers.updateItem(ii, COLUMN_IP);
	}
	if(updateMask & UpdateInfo::MASK_SEGMENT) {
		ctrlTransfers.updateItem(ii, COLUMN_HUB);
	}
	if(updateMask & UpdateInfo::MASK_CIPHER) {
		ctrlTransfers.updateItem(ii, COLUMN_CIPHER);
	}
	if(updateMask & UpdateInfo::MASK_USERS) {
		ctrlTransfers.updateItem(ii, COLUMN_USER);
	}
	if(updateMask & UpdateInfo::MASK_USER) {
		ctrlTransfers.updateItem(ii, COLUMN_USER);
	}
	if(updateMask & UpdateInfo::MASK_PRIORITY) {
		ctrlTransfers.updateItem(ii, COLUMN_FILE);
	}
}

void TransferView::on(ConnectionManagerListener::UserUpdated, const ConnectionQueueItem* aCqi) noexcept {
	auto ui = new UpdateInfo(aCqi->getToken(), aCqi->getDownload());
	ui->setUser(aCqi->getHintedUser());
	speak(UPDATE_ITEM, ui);
}

void TransferView::on(ConnectionManagerListener::Added, const ConnectionQueueItem* aCqi) {
	auto ui = new UpdateInfo(aCqi->getToken(), aCqi->getDownload());
	if(ui->download) {
		string aTarget, bundleToken; int64_t aSize; int aFlags;
		if(QueueManager::getInstance()->getQueueInfo(aCqi->getHintedUser(), aTarget, aSize, aFlags, bundleToken)) {
			auto type = Transfer::TYPE_FILE;
			if(aFlags & QueueItem::FLAG_USER_LIST)
				type = Transfer::TYPE_FULL_LIST;
			else if(aFlags & QueueItem::FLAG_PARTIAL_LIST)
				type = Transfer::TYPE_PARTIAL_LIST;
			
			ui->setType(type);
			ui->setTarget(Text::toT(aTarget));
			ui->setSize(aSize);
			ui->setBundle(bundleToken);
		}
	}

	ui->setUser(aCqi->getHintedUser());
	ui->setStatus(ItemInfo::STATUS_WAITING);
	ui->setStatusString(TSTRING(CONNECTING));

	speak(ADD_ITEM, ui);
}

void TransferView::on(ConnectionManagerListener::Forced, const ConnectionQueueItem* aCqi) noexcept {
	auto ui = new UpdateInfo(aCqi->getToken(), aCqi->getDownload(), false);
	ui->setStatusString(TSTRING(CONNECTING_FORCED));
	speak(UPDATE_ITEM, ui);
}

void TransferView::onUpdateFileInfo(const HintedUser& aUser, const string& aToken, bool updateStatus) {
	string aTarget, bundleToken;	int64_t aSize; int aFlags = 0;
	if(QueueManager::getInstance()->getQueueInfo(aUser, aTarget, aSize, aFlags, bundleToken)) {
		auto ui = new UpdateInfo(aToken, true);
		auto type = Transfer::TYPE_FILE;
		if(aFlags & QueueItem::FLAG_USER_LIST)
			type = Transfer::TYPE_FULL_LIST;
		else if(aFlags & QueueItem::FLAG_PARTIAL_LIST)
			type = Transfer::TYPE_PARTIAL_LIST;
	
		ui->setType(type);
		ui->setTarget(Text::toT(aTarget));
		ui->setSize(aSize);
		ui->setBundle(bundleToken);
		if (updateStatus) {
			ui->setStatusString(TSTRING(CONNECTING));
			ui->setStatus(ItemInfo::STATUS_WAITING);
		}
		speak(UPDATE_ITEM, ui);
	}
}

void TransferView::on(ConnectionManagerListener::Removed, const ConnectionQueueItem* aCqi) {
	speak(REMOVE_ITEM, new UpdateInfo(aCqi->getToken(), aCqi->getDownload()));
}

void TransferView::on(ConnectionManagerListener::Failed, const ConnectionQueueItem* aCqi, const string& aReason) {
	auto ui = new UpdateInfo(aCqi->getToken(), aCqi->getDownload());
	if(aCqi->getUser()->isSet(User::OLD_CLIENT)) {
		ui->setStatusString(TSTRING(SOURCE_TOO_OLD));
	} else {
		ui->setStatusString(Text::toT(aReason));
	}
	ui->setBundle(aCqi->getLastBundle());
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


TransferView::ItemInfo* TransferView::ItemInfo::createParent() {
	auto ii = new ItemInfo(user, bundle, download);
	ii->running = 0;
	ii->hits = 0;
	dcassert(!bundle.empty());
	ii->isBundle = true;
	ii->bundle = bundle;

	if (download) {
		auto b = QueueManager::getInstance()->findBundle(bundle);
		if (b) {
			ii->target = Text::toT(b->getTarget());
			ii->size = b->getSize();
			ii->prio = b->getPriority();
		}
	} else {
		auto b = UploadManager::getInstance()->findBundle(bundle);
		if (b) {
			ii->target = Text::toT(b->getTarget());
			ii->size = b->getSize();
			ii->cipher = cipher;
			ii->ip = ip;
			ii->flagIndex = flagIndex;
		}
	}

	ii->statusString = TSTRING(CONNECTING);
	return ii;
}

inline const string& TransferView::ItemInfo::getGroupCond() const {
	//dcassert(!bundle.empty());
	if (!bundle.empty()) {
		return bundle;
	} else {
		return token;
	}
}

const tstring TransferView::ItemInfo::getText(uint8_t col) const {
	switch(col) {
		case COLUMN_USER:
			if (isBundle && download) {
				if ((users == 1 || hits == 2) && user.user) {
					return WinUtil::getNicks(user);
				} else {
					return TSTRING_F(X_USERS_WAITING, users % ((hits-running-1) > 0 ? hits-running-1 : 0));
				}
			} else if (hits == -1 || isBundle) {
				return WinUtil::getNicks(user);
			} else {
				return TSTRING_F(X_USERS, hits);
			}
		case COLUMN_HUB: 
			if (isBundle) {
				return TSTRING_F(X_CONNECTIONS, running);
			} else {
				return WinUtil::getHubNames(user);
			}
		case COLUMN_STATUS: return statusString;
		case COLUMN_TIMELEFT: return (status == STATUS_RUNNING) ? Util::formatSecondsW(timeLeft) : Util::emptyStringT;
		case COLUMN_SPEED:
			if (isBundle && !download && totalSpeed > 0) {
				return (status == STATUS_RUNNING) ? (Util::formatBytesW(speed) + _T("/s (") + (Util::formatBytesW(totalSpeed)) + _T("/s)")) : Util::emptyStringT;
			}
			return (status == STATUS_RUNNING) ? (Util::formatBytesW(speed) + _T("/s")) : Util::emptyStringT;
		case COLUMN_FILE:
			if (isBundle) {
				tstring ret;
				if (!target.empty() && target[target.size() -1] == '\\') {
					ret = Util::getLastDir(target); //directory bundle
				} else {
					ret = Util::getFileName(target); //file bundle
				}

				if (download)
					ret += _T(" (") + Text::toT(AirUtil::getPrioText(prio)) + _T(")");
				return ret;
			}
			return getFile(type, Util::getFileName(target));
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
	const auto& uc = t->getUserConnection();
	ui->setCipher(Text::toT(uc.getCipherName()));

	const auto& ip = uc.getRemoteIp();
	const auto& country = GeoManager::getInstance()->getCountry(uc.getRemoteIp());
	if(country.empty()) {
		ui->setIP(Text::toT(ip), 0);
	} else {
		ui->setIP(Text::toT(country + " (" + ip + ")"), Localization::getFlagIndexByCode(country.c_str()));
	}
}

void TransferView::on(DownloadManagerListener::Requesting, const Download* d, bool hubChanged) noexcept {
	//LogManager::getInstance()->message("Requesting " + d->getToken());
	auto ui = new UpdateInfo(d->getToken(), true);
	if (hubChanged)
		ui->setUser(d->getHintedUser());

	starting(ui, d);
	
	ui->setActual(d->getActual());
	ui->setSize(d->getSize());
	ui->setStatus(ItemInfo::STATUS_RUNNING);	ui->updateMask &= ~UpdateInfo::MASK_STATUS; // hack to avoid changing item status
	ui->setStatusString(TSTRING(REQUESTING) + _T(" ") + getFile(d->getType(), Text::toT(Util::getFileName(d->getPath()))) + _T("..."));
	ui->setBundle(d->getBundle() ? d->getBundle()->getToken() : Util::emptyString);

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(DownloadManagerListener::Starting, const Download* aDownload) {
	//LogManager::getInstance()->message("Starting " + aDownload->getToken());
	auto ui = new UpdateInfo(aDownload->getToken(), true);

	ui->setStatus(ItemInfo::STATUS_RUNNING);
	ui->setStatusString(TSTRING(DOWNLOAD_STARTING));
	ui->setTarget(Text::toT(aDownload->getPath()));
	ui->setType(aDownload->getType());

	ui->setBundle(aDownload->getBundle() ? aDownload->getBundle()->getToken() : Util::emptyString);
	speak(UPDATE_ITEM, ui);
}

void TransferView::on(DownloadManagerListener::BundleTick, const BundleList& bundles, uint64_t /*aTick*/) {
	for(const auto& b: bundles) {
		double ratio = 0;
		int64_t totalSpeed = 0;
		bool partial=false, trusted=false, untrusted=false, tthcheck=false, zdownload=false, chunked=false, mcn=false;

		auto ui = new UpdateInfo(b->getToken(), true);
		for(const auto& d: b->getDownloads()) {
			if(d->getStart() > 0) {
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
					mcn = true;
				}

				totalSpeed += static_cast<int64_t>(d->getAverageSpeed());
				ratio += d->getPos() > 0 ? (double)d->getActual() / (double)d->getPos() : 1.00;
			}
		}

		if(b->getRunning() > 0) {
			ratio = ratio / b->getRunning();

			ui->setStatus(ItemInfo::STATUS_RUNNING);
			ui->setSize(b->getSize());
			ui->setPos(b->getDownloadedBytes());
			ui->setActual((int64_t)((double)ui->pos * (ratio == 0 ? 1.00 : ratio)));
			ui->setTimeLeft(b->getSecondsLeft());
			ui->setSpeed(totalSpeed);
			ui->setUsers(b->getRunningUsers().size());
			ui->setRunning(b->getRunning());

			if(b->getBundleBegin() == 0) {
				// file is starting
				b->setBundleBegin(GET_TICK());

				ui->setStatusString(TSTRING(DOWNLOAD_STARTING));
			} else {
				uint64_t time = GET_TICK() - b->getBundleBegin();
				if(time > 1000) {
					tstring pos = Text::toT(Util::formatBytes(ui->pos));
					double percent = (double)ui->pos*100.0/(double)ui->size;
					dcassert(percent <= 100.00);
					tstring elapsed = Util::formatSecondsW(time/1000);
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

					ui->setStatusString(flag + TSTRING_F(DOWNLOADED_BYTES, pos.c_str() % percent % elapsed.c_str()));
				}
			}
		}

		speak(UPDATE_BUNDLE, ui);
	}
}

void TransferView::on(DownloadManagerListener::Tick, const DownloadList& dl) {
	for(const auto& d: dl) {
		auto ui = new UpdateInfo(d->getToken(), true);
		ui->setStatus(ItemInfo::STATUS_RUNNING);
		ui->setActual(d->getActual());
		ui->setPos(d->getPos());
		ui->setSize(d->getSize());
		ui->setTimeLeft(d->getSecondsLeft());
		ui->setBundle(d->getBundle() ? d->getBundle()->getToken() : Util::emptyString);
		
		
		ui->setSpeed(static_cast<int64_t>(d->getAverageSpeed()));
		ui->setType(d->getType());

		tstring pos = Util::formatBytesW(d->getPos());
		double percent = (double)d->getPos()*100.0/(double)d->getSize();
		tstring elapsed = Util::formatSecondsW((GET_TICK() - d->getStart())/1000);

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
		statusString += CTSTRING_F(DOWNLOADED_BYTES, pos.c_str() % percent % elapsed);
		ui->setStatusString(statusString);
			
		tasks.add(UPDATE_ITEM, unique_ptr<Task>(ui));
	}

	PostMessage(WM_SPEAKER);
}

void TransferView::on(DownloadManagerListener::Failed, const Download* aDownload, const string& aReason) {
	//LogManager::getInstance()->message("Failed " + aDownload->getToken());
	auto ui = new UpdateInfo(aDownload->getToken(), true, true);
	ui->setStatus(ItemInfo::STATUS_WAITING);
	ui->setPos(0);
	ui->setSize(aDownload->getSize());
	ui->setTarget(Text::toT(aDownload->getPath()));
	ui->setType(aDownload->getType());
	ui->setBundle(aDownload->getBundle() ? aDownload->getBundle()->getToken() : Util::emptyString);

	tstring tmpReason = Text::toT(aReason);
	if(aDownload->isSet(Download::FLAG_SLOWUSER)) {
		tmpReason += _T(": ") + TSTRING(SLOW_USER);
	} else if(aDownload->getOverlapped() && !aDownload->isSet(Download::FLAG_OVERLAP)) {
		tmpReason += _T(": ") + TSTRING(OVERLAPPED_SLOW_SEGMENT);
	}

	ui->setStatusString(tmpReason);

	if(SETTING(POPUP_DOWNLOAD_FAILED)) {
		WinUtil::showPopup(
			TSTRING(FILE) + _T(": ") + Util::getFileName(ui->target) + _T("\n")+
			TSTRING(USER) + _T(": ") + WinUtil::getNicks(aDownload->getHintedUser()) + _T("\n")+
			TSTRING(REASON) + _T(": ") + Text::toT(aReason), TSTRING(DOWNLOAD_FAILED), NIIF_WARNING);
	}

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(DownloadManagerListener::Status, const UserConnection* uc, const string& aReason) {
	//LogManager::getInstance()->message("Status " + uc->getToken());
	dcassert(!uc->getToken().empty());
	auto ui = new UpdateInfo(uc->getToken(), true);
	ui->setStatus(ItemInfo::STATUS_WAITING);
	ui->setPos(0);
	ui->setStatusString(Text::toT(aReason));
	ui->setBundle(!uc->getLastBundle().empty() ? uc->getLastBundle() : Util::emptyString);

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(DownloadManagerListener::TargetChanged, const string& aTarget, const string& aToken, const string& bundleToken) noexcept {
	auto ui = new UpdateInfo(aToken, true);
	ui->setTarget(Text::toT(aTarget));
	ui->setBundle(bundleToken);
	speak(UPDATE_ITEM, ui);
}

void TransferView::on(UploadManagerListener::Starting, const Upload* aUpload) {
	UpdateInfo* ui = new UpdateInfo(aUpload->getToken(), false);
	starting(ui, aUpload);
	
	ui->setStatus(ItemInfo::STATUS_RUNNING);
	ui->setActual(aUpload->getStartPos() + aUpload->getActual());
	ui->setSize(aUpload->getType() == Transfer::TYPE_TREE ? aUpload->getSize() : aUpload->getFileSize());
	ui->setRunning(1);
	ui->setBundle(aUpload->getBundle() ? aUpload->getBundle()->getToken() : Util::emptyString);
	
	if(!aUpload->isSet(Upload::FLAG_RESUMED)) {
		ui->setStatusString(TSTRING(UPLOAD_STARTING));
	}

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(UploadManagerListener::BundleTick, const UploadBundleList& bundles) {
	for(const auto& b: bundles) {
		auto ui = new UpdateInfo(b->getToken(), false);


		double ratio = 0;
		int64_t totalSpeed = 0;
		
		bool partial=false, trusted=false, untrusted=false, chunked=false, mcn=false, zupload=false;

		for(const auto& u: b->getUploads()) {
			if(u->getStart() > 0) {
				if(u->isSet(Upload::FLAG_PARTIAL)) {
					partial = true;
				}
				if(u->getUserConnection().isSecure()) {
					if(u->getUserConnection().isTrusted()) {
						trusted = true;
					} else {
						untrusted = true;
					}
				}

				if(u->isSet(Upload::FLAG_CHUNKED)) {
					chunked = true;
				}
				if(u->getUserConnection().isSet(UserConnection::FLAG_MCN1)) {
					mcn = true;
				}
				if(u->isSet(Upload::FLAG_ZUPLOAD)) {
					zupload = true;
				}

				totalSpeed += static_cast<int64_t>(u->getAverageSpeed());
				ratio += u->getPos() > 0 ? (double)u->getActual() / (double)u->getPos() : 1.00;
			}
		}

		if(b->getRunning() > 0) {
			ratio = ratio / b->getRunning();

			ui->setStatus(ItemInfo::STATUS_RUNNING);
			ui->setSize(b->getSize());
			ui->setPos(b->getUploaded());
			ui->setActual((int64_t)((double)ui->pos * (ratio == 0 ? 1.00 : ratio)));
			ui->setTimeLeft(b->getSecondsLeft());
			ui->setSpeed(totalSpeed);
			ui->setUsers(1);
			if (b->getSingleUser()) {
				ui->setTotalSpeed(0);
			} else {
				ui->setTotalSpeed(b->getTotalSpeed());
			}
			ui->setRunning(b->getRunning());

			if(b->getBundleBegin() == 0) {
				// file is starting
				b->setBundleBegin(GET_TICK());

				ui->setStatusString(TSTRING(DOWNLOAD_STARTING));
			} else {
				uint64_t time = GET_TICK() - b->getBundleBegin();
				if(time > 1000) {
					tstring pos = Text::toT(Util::formatBytes(ui->pos));
					double percent = (double)ui->pos*100.0/(double)ui->size;
					dcassert(percent <= 100.00);
					tstring elapsed = Util::formatSecondsW(time/1000);
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
					if(zupload) {
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

					ui->setStatusString(flag + TSTRING_F(UPLOADED_BYTES, pos.c_str() % percent % elapsed.c_str()));
				}
			}
		}
			
		speak(UPDATE_BUNDLE, ui);
	}
}

void TransferView::on(UploadManagerListener::Tick, const UploadList& ul) {
	for(const auto& u: ul) {
		if (u->getPos() == 0) continue;
		if (u->getToken().empty()) {
			continue;
		}

		auto ui = new UpdateInfo(u->getToken(), false);
		ui->setActual(u->getStartPos() + u->getActual());
		ui->setPos(u->getStartPos() + u->getPos());
		ui->setTimeLeft(u->getSecondsLeft(true)); // we are interested when whole file is finished and not only one chunk
		ui->setSpeed(static_cast<int64_t>(u->getAverageSpeed()));
		ui->setBundle(u->getBundle() ? u->getBundle()->getToken() : Util::emptyString);

		tstring pos = Util::formatBytesW(ui->pos);
		double percent = (double)ui->pos*100.0/(double)(u->getType() == Transfer::TYPE_TREE ? u->getSize() : u->getFileSize());
		tstring elapsed = Util::formatSecondsW((GET_TICK() - u->getStart())/1000); 
		
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
		statusString += CTSTRING_F(UPLOADED_BYTES, pos.c_str() % percent % elapsed);

		ui->setStatusString(statusString);
					
		tasks.add(UPDATE_ITEM, unique_ptr<Task>(ui));
	}

	PostMessage(WM_SPEAKER);
}

void TransferView::onTransferComplete(const Transfer* aTransfer, bool isUpload, const string& aFileName, bool isTree, const string& bundleToken) {
	//LogManager::getInstance()->message("Transfer complete " + aTransfer->getToken());
	auto ui = new UpdateInfo(aTransfer->getToken(), !isUpload);

	ui->setStatus(ItemInfo::STATUS_WAITING);	
	ui->setPos(0);
	ui->setStatusString(isUpload ? TSTRING(UPLOAD_FINISHED_IDLE) : TSTRING(DOWNLOAD_FINISHED_IDLE));
	ui->setRunning(0);
	ui->setBundle(bundleToken);

	if(isUpload && SETTING(POPUP_UPLOAD_FINISHED) && !isTree) {
		if(!SETTING(UPLOADFILE).empty() && !SETTING(SOUNDS_DISABLED))
			WinUtil::playSound(Text::toT(SETTING(UPLOADFILE)));

		if (SETTING(POPUP_UPLOAD_FINISHED)) {
			WinUtil::showPopup(TSTRING(FILE) + _T(": ") + Text::toT(aFileName) + _T("\n") +
				TSTRING(USER) + _T(": ") + WinUtil::getNicks(aTransfer->getHintedUser()), TSTRING(UPLOAD_FINISHED_IDLE));
		}
	}
	
	speak(UPDATE_ITEM, ui);
}

void TransferView::onBundleComplete(const string& bundleToken, const string& bundleName, bool isUpload) {
	auto ui = new UpdateInfo(bundleToken, !isUpload);

	ui->setStatus(ItemInfo::STATUS_WAITING);	

	ui->setPos(0);
	ui->setStatusString(isUpload ? TSTRING(UPLOAD_FINISHED_IDLE) : TSTRING(DOWNLOAD_FINISHED_IDLE));
	ui->setRunning(0);
	ui->setUsers(0);

	if(SETTING(POPUP_BUNDLE_DLS) && !isUpload) {
		WinUtil::showPopup(_T("The following bundle has finished downloading: ") + Text::toT(bundleName), TSTRING(DOWNLOAD_FINISHED_IDLE));
	} else if(SETTING(POPUP_BUNDLE_ULS) && isUpload) {
		WinUtil::showPopup(_T("The following bundle has finished uploading: ") + Text::toT(bundleName), TSTRING(UPLOAD_FINISHED_IDLE));
	}
	
	speak(UPDATE_BUNDLE, ui);
}

void TransferView::onBundleStatus(const BundlePtr& aBundle, bool removed) {
	auto ui = new UpdateInfo(aBundle->getToken(), true);
	ui->setStatus(ItemInfo::STATUS_WAITING);
	if (removed) {
		ui->setStatusString(TSTRING(BUNDLE_REMOVED));
	} else {
		ui->setStatusString(TSTRING(WAITING));
	}
	aBundle->setBundleBegin(0);
	ui->setUsers(0);
	ui->setRunning(0);
	speak(UPDATE_BUNDLE, ui);
}

void TransferView::on(QueueManagerListener::BundleSize, const BundlePtr& aBundle) noexcept {
	auto ui = new UpdateInfo(aBundle->getToken(), true);
	ui->setSize(aBundle->getSize());
	speak(UPDATE_BUNDLE, ui);
}

void TransferView::on(UploadManagerListener::BundleSizeName, const string& bundleToken, const string& newTarget, int64_t aSize) noexcept {
	auto ui = new UpdateInfo(bundleToken, false);
	ui->setSize(aSize);
	ui->setTarget(Text::toT(newTarget));
	speak(UPDATE_BUNDLE, ui);
}

void TransferView::on(QueueManagerListener::BundleStatusChanged, const BundlePtr& aBundle) noexcept {
	if (aBundle->getStatus() == Bundle::STATUS_DOWNLOADED)
		onBundleComplete(aBundle->getToken(), aBundle->getName(), false); 
}

void TransferView::onBundleName(const BundlePtr& aBundle) {
	auto ui = new UpdateInfo(aBundle->getToken(), true);
	ui->setTarget(Text::toT(aBundle->getTarget()));
	ui->setPriority(aBundle->getPriority());
	speak(UPDATE_BUNDLE, ui);
}

LRESULT TransferView::onKeyDownTransfers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	if(kd->wVKey == VK_DELETE) {
		handleDisconnect();
	}
	return 0;
}


void TransferView::performActionFiles(std::function<void (const ItemInfo* aInfo)> f, bool oncePerParent /*false*/) {
	ctrlTransfers.filteredForEachSelectedT([&](const ItemInfo* ii) {
		if (ii->hits > 1) {
			//perform only for the children
			const auto& children = ctrlTransfers.findChildren(ii->getGroupCond());
			if (oncePerParent && !children.empty()) {
				f(children.front());
			} else {
				boost::for_each(children, f);
			}
		} else {
			//perform for the parent
			f(ii);
		}
	});
}

void TransferView::performActionBundles(std::function<void (const ItemInfo* aInfo)> f) {
	ctrlTransfers.forEachSelectedT([&](const ItemInfo* ii) {
		if (ii->isBundle) {
			f(ii);
		}
	});
}

void TransferView::handleCollapseAll() {
	for(int q = ctrlTransfers.GetItemCount()-1; q != -1; --q) {
		auto m = (ItemInfo*)ctrlTransfers.getItemData(q);
		if(m->download && m->parent) {
			ctrlTransfers.deleteItem(m); 
		}
		if(m->download && !m->parent && !m->collapsed) {
			m->collapsed = true;
			ctrlTransfers.SetItemState(ctrlTransfers.findItem(m), INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
		 }
	}
}

void TransferView::handleExpandAll() {
	for(auto& p: ctrlTransfers.getParents()) {
		auto l = p.second.parent;
		if(l->collapsed) {
			ctrlTransfers.Expand(l, ctrlTransfers.findItem(l));
		}
	}
}

void TransferView::handleSlowDisconnect() {
	auto slowDisconnect = [=](const ItemInfo* ii) {
		QueueManager::getInstance()->onSlowDisconnect(ii->bundle);
	};

	performActionBundles(slowDisconnect);
}

void TransferView::handleRemoveFile() {
	auto remove = [=](const ItemInfo* ii) {
		QueueManager::getInstance()->removeFile(Text::fromT(ii->target));
	};

	performActionFiles(remove);
}

void TransferView::handleSearchDir() {
	auto search = [=](const ItemInfo* ii) {
		size_t pos = ii->target.rfind(' ');
		WinUtil::searchAny(Util::getLastDir(ii->target.substr(0, pos != string::npos ? pos : ii->target.length())));
	};

	performActionBundles(search);
}


void TransferView::handleSearchAlternates() {
	auto search = [=](const ItemInfo* ii) {
		TTHValue tth;
		int64_t size = 0;
		if(QueueManager::getInstance()->getSearchInfo(Text::fromT(ii->target), tth, size)) {
			WinUtil::searchHash(tth, Util::getFileName(Text::fromT(ii->target)), size);
		}
	};

	performActionFiles(search);
}

void TransferView::handleForced() {
	auto force = [=](const ItemInfo* ii) {
		ConnectionManager::getInstance()->force(ii->token);
	};

	performActionFiles(force);
}

void TransferView::handleRemoveBundle() {
	auto removeBundle = [=](const ItemInfo* ii) {
		WinUtil::removeBundle(ii->bundle);
	};

	performActionBundles(removeBundle);
}

void TransferView::handleRemoveBundleSource() {
	auto removeSource = [=](const ItemInfo* ii) {
		QueueManager::getInstance()->removeBundleSource(ii->bundle, ii->user, QueueItem::Source::FLAG_REMOVED);
	};

	performActionBundles(removeSource);
}

void TransferView::handleDisconnect() {
	auto disconnect = [=](const ItemInfo* ii) {
		ConnectionManager::getInstance()->disconnect(ii->token);
	};

	performActionFiles(disconnect);
}

void TransferView::handleOpenFolder() {
	auto ii = ctrlTransfers.getItemData(ctrlTransfers.GetNextItem(-1, LVNI_SELECTED));
	if (ii) {
		WinUtil::openFolder(ii->target);
	}
}



void TransferView::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
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

	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}