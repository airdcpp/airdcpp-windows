/*
 * Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
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

#include <airdcpp/core/localization/ResourceManager.h>
#include <airdcpp/settings/SettingsManager.h>
#include <airdcpp/connection/ConnectionManager.h>
#include <airdcpp/transfer/download/DownloadManager.h>
#include <airdcpp/util/PathUtil.h>
#include <airdcpp/transfer/TransferInfoManager.h>
#include <airdcpp/transfer/upload/upload_bundles/UploadBundleManager.h>
#include <airdcpp/queue/QueueManager.h>

#include <airdcpp/transfer/download/Download.h>
#include <airdcpp/queue/QueueItem.h>
#include <airdcpp/transfer/upload/upload_bundles/UploadBundle.h>

#include <web-server/ContextMenuManager.h>
#include <web-server/WebServerManager.h>

#include <windows/util/WinUtil.h>
#include <windows/TransferView.h>
#include <windows/MainFrm.h>
#include <windows/ResourceLoader.h>
#include <windows/util/ActionUtil.h>

#include <windows/components/BarShader.h>

namespace wingui {
int TransferView::columnIndexes[] = { COLUMN_USER, COLUMN_FILE, COLUMN_HUB_CONNECTIONS, COLUMN_STATUS, COLUMN_TIMELEFT, COLUMN_SPEED, COLUMN_SIZE, COLUMN_PATH, COLUMN_IP/*, COLUMN_ENCRYPTION*/ };
int TransferView::columnSizes[] = { 150, 250, 150, 275, 75, 75, 75, 200, 175/*, 50*/ };

static ResourceManager::Strings columnNames[] = { ResourceManager::USER, ResourceManager::BUNDLE_FILENAME, ResourceManager::HUB_SEGMENTS, ResourceManager::STATUS,
	ResourceManager::TIME_LEFT, ResourceManager::SPEED, ResourceManager::SIZE, ResourceManager::PATH,
	ResourceManager::IP_BARE/*, ResourceManager::ENCRYPTION*/};

TransferView::~TransferView() {
	arrows.Destroy();
	OperaColors::ClearCache();
}

LRESULT TransferView::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	
	arrows.Create(16, 16, ILC_COLOR32 | ILC_MASK,  0, 3);
	arrows.AddIcon(CIcon(ResourceLoader::loadIcon(IDI_DOWNLOAD, 16)));
	arrows.AddIcon(CIcon(ResourceLoader::loadIcon(IDI_UPLOAD, 16)));
	arrows.AddIcon(CIcon(ResourceLoader::loadIcon(IDI_D_USER, 16)));
	arrows.AddIcon(CIcon(ResourceLoader::loadIcon(IDI_U_USER, 16)));

	ctrlTransfers.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_TRANSFERS);
	ctrlTransfers.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);

	WinUtil::splitTokens(columnIndexes, SETTING(TRANSFERVIEW_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(TRANSFERVIEW_WIDTHS), COLUMN_LAST);

	for(uint8_t j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_TIMELEFT || j == COLUMN_SPEED) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlTransfers.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}

	ctrlTransfers.addCopyHandler(COLUMN_IP, &ColumnInfo::filterCountry);

	ctrlTransfers.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlTransfers.setVisible(SETTING(MAINFRAME_VISIBLE));

	ctrlTransfers.SetBkColor(WinUtil::bgColor);
	ctrlTransfers.SetTextBkColor(WinUtil::bgColor);
	ctrlTransfers.SetTextColor(WinUtil::textColor);
	ctrlTransfers.setFlickerFree(WinUtil::bgBrush);
	ctrlTransfers.SetFont(WinUtil::listViewFont);

	ctrlTransfers.SetImageList(arrows, LVSIL_SMALL);
	ctrlTransfers.setSortColumn(COLUMN_USER);

	ConnectionManager::getInstance()->addListener(this);
	DownloadManager::getInstance()->addListener(this);
	UploadBundleManager::getInstance()->receiver.addListener(this);
	QueueManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);
	TransferInfoManager::getInstance()->addListener(this);
	return 0;
}

void TransferView::prepareClose() {
	ctrlTransfers.saveHeaderOrder(SettingsManager::TRANSFERVIEW_ORDER, SettingsManager::TRANSFERVIEW_WIDTHS,
		SettingsManager::MAINFRAME_VISIBLE);

	ConnectionManager::getInstance()->removeListener(this);
	DownloadManager::getInstance()->removeListener(this);
	UploadBundleManager::getInstance()->receiver.removeListener(this);
	QueueManager::getInstance()->removeListener(this);
	SettingsManager::getInstance()->removeListener(this);
	TransferInfoManager::getInstance()->removeListener(this);
	
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
					ActionUtil::appendPreviewMenu(transferMenu, Text::fromT(ii->target));
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

			{
				vector<uint32_t> tokens;
				ctrlTransfers.forEachSelectedT([&tokens](const ItemInfo* ii) {
					if (!ii->isBundle) {
						auto ti = TransferInfoManager::getInstance()->findTransfer(ii->token);
						if (ti) {
							tokens.push_back(ti->getToken());
						}
					}
					});

				EXT_CONTEXT_MENU(transferMenu, Transfer, tokens);
			}

			transferMenu.appendItem(TSTRING(CLOSE_CONNECTION), [=] { handleDisconnect(); });
		} else {
			transferMenu.InsertSeparatorFirst(TSTRING(BUNDLE));
			if (ii->onlineUsers == 1) {
				appendUserItems(transferMenu);
				setDefaultItem(transferMenu);
				if(ii->download)
					transferMenu.appendItem(TSTRING(REMOVE_BUNDLE_SOURCE), [=] { handleRemoveBundleSource(); });
				transferMenu.appendSeparator();
			}
			
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
				auto b = QueueManager::getInstance()->findBundle(ii->getLocalBundleToken());
				if (b)
					bundles.push_back(b);
			}
		});

		if (!bundles.empty()) {
			transferMenu.appendSeparator();

			auto usingDisconnect = ranges::all_of(bundles, Flags::IsSet(Bundle::FLAG_AUTODROP));
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
	if (!ActionUtil::getUCParams(m_hWnd, uc, ucLineParams))
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
		
		UserCommandManager::getInstance()->userCommand(itemI->user, uc, tmp, true);
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

			COLORREF textclr = ii->download ? SETTING(PROGRESS_TEXT_COLOR_DOWN) : SETTING(PROGRESS_TEXT_COLOR_UP);

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

			if (ii->size == 0) ii->size = 1;

			WinUtil::drawProgressBar(cd->nmcd.hdc, rc, clr, textclr, SETTING(PROGRESS_BACK_COLOR), ii->statusString,
				ii->size, ii->actual, SETTING(PROGRESSBAR_ODC_STYLE), SETTING(PROGRESS_OVERRIDE_COLORS2), SETTING(PROGRESS_3DDEPTH), SETTING(PROGRESS_LIGHTEN));

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
		} else if((colIndex != COLUMN_USER) && (colIndex != COLUMN_HUB_CONNECTIONS) && (colIndex != COLUMN_STATUS) && (colIndex != COLUMN_IP) &&
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


LRESULT TransferView::onInfoTip(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	if (!SETTING(SHOW_INFOTIPS)) return 0;

	NMLVGETINFOTIP* pInfoTip = (NMLVGETINFOTIP*)pnmh;
	tstring InfoTip = ctrlTransfers.GetColumnTexts(pInfoTip->iItem);
	auto aItem = ctrlTransfers.getItemData(pInfoTip->iItem);
	if (aItem && !aItem->encryption.empty())
		InfoTip += _T("\r\n") + TSTRING(ENCRYPTION) + _T(": ") + aItem->encryption;

	pInfoTip->cchTextMax = InfoTip.size();
	_tcsncpy(pInfoTip->pszText, InfoTip.c_str(), INFOTIPSIZE);
	pInfoTip->pszText[INFOTIPSIZE - 1] = NULL;

	return 0;
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
					break;
				case 3:
					i->grant();
					break;
				case 4:
					i->handleFav();
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
		case COLUMN_HUB_CONNECTIONS: {
			if(a->connections == b->connections)
				return Util::DefaultSort(a->getText(COLUMN_HUB_CONNECTIONS).c_str(), b->getText(COLUMN_HUB_CONNECTIONS).c_str());
			return compare(a->connections, b->connections);
		}
		case COLUMN_STATUS: return 0;
		case COLUMN_TIMELEFT: return compare(a->timeLeft, b->timeLeft);
		case COLUMN_SPEED: return compare(a->speed, b->speed);
		case COLUMN_SIZE: return compare(a->size, b->size);
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
			for(const auto& iiFile: children) {
				if(compare(ui.token, iiFile->token) == 0) {
					return iiFile;
				}
			}
		}
	}
	return nullptr;
}


void TransferView::regroupItem(ItemInfo* ii, const UpdateInfo& ui, int pos) noexcept {
	auto parent = ii->parent ? ii->parent : ii;

	/* if bundle has changed, regroup the item */
	bool changeParent = (ui.bundle != ii->bundle);
	if (parent->isBundle) {
		if (!ui.IP.empty() && parent->onlineUsers == 1) {
			parent->encryption = ui.Encryption;
			parent->ip = ui.IP;
			parent->flagIndex = ui.flagIndex;
			updateItem(parent, UpdateInfo::MASK_ENCRYPTION | UpdateInfo::MASK_IP);
		} else if (parent->onlineUsers > 1 && !parent->ip.empty()) {
			parent->encryption = Util::emptyStringT;
			parent->ip = Util::emptyStringT;
			parent->flagIndex = 0;
			updateItem(parent, UpdateInfo::MASK_ENCRYPTION | UpdateInfo::MASK_IP);
		}
	}

	if (changeParent) {
		if (!ii->hasBundle()) {
			ctrlTransfers.DeleteItem(pos);
		} else if (ctrlTransfers.removeGroupedItem(ii, false)) {
			parent->updateUser(ctrlTransfers.findChildren(parent->getGroupCond()));
		}
	}
	ii->update(ui);

	if (changeParent) {
		if (!ii->hasBundle()) {
			ii->parent = nullptr;
			ctrlTransfers.insertItem(ii, ii->download ? IMAGE_DOWNLOAD : IMAGE_UPLOAD);
		} else {
			ctrlTransfers.insertGroupedItem(ii, ii->parent ? !ii->parent->collapsed : SETTING(EXPAND_BUNDLES));
		}
	} else if (ii == parent || !parent->collapsed) {
		updateItem(ii, ui.updateMask);
	}
}

LRESULT TransferView::onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	TaskQueue::List t;
	tasks.get(t);
	if(t.size() > 2) {
		ctrlTransfers.SetRedraw(FALSE);
	}

	for(auto& i: t) {
		if(i.first == ADD_ITEM) {
			auto const &ui = static_cast<UpdateInfo&>(*i.second);
			//LogManager::getInstance()->message("Transferview, ADD_ITEM: " + ui.token);
			auto ii = new ItemInfo(ui.user, ui.token, ui.download);
			ii->update(ui);
			if (ii->hasBundle()) {
				ctrlTransfers.insertGroupedItem(ii, SETTING(EXPAND_BUNDLES));
			} else {
				ctrlTransfers.insertItem(ii, ii->download ? IMAGE_DOWNLOAD : IMAGE_UPLOAD);
			}
		} else if(i.first == REMOVE_ITEM) {
			auto &ui = static_cast<UpdateInfo&>(*i.second);
			//LogManager::getInstance()->message("Transferview, REMOVE_ITEM: " + ui.token);

			int pos = -1;
			auto ii = findItem(ui, pos);
			if(ii) {
				if (ii->hasBundle()) {
					auto parent = ii->parent;
					if (ctrlTransfers.removeGroupedItem(ii, true))
						parent->updateUser(ctrlTransfers.findChildren(parent->getGroupCond()));
				} else {
					dcassert(pos != -1);
					ctrlTransfers.DeleteItem(pos);
					delete ii;
				}
			}
		} else if(i.first == UPDATE_ITEM) {
			auto &ui = static_cast<UpdateInfo&>(*i.second);
			/*if (!ui.download) {
				auto uploadBundle = UploadBundleInfoReceiver::getInstance()->findByUploadToken(ui.token);
				if (uploadBundle) {
					ui.bundle = uploadBundle->getToken();
				}
			}*/

			//dcassert(!ui.target.empty());
			int pos = -1;
			auto ii = findItem(ui, pos);
			if(ii) {
				if(ii->hasBundle() || !ui.bundle.empty())  {
					regroupItem(ii, ui, pos);
					continue;
				}
				ii->update(ui);
				updateItem(ii, ui.updateMask);
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
			updateItem(pp->parent, ui.updateMask);
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
	if(ui.updateMask & UpdateInfo::MASK_ENCRYPTION) {
		encryption = ui.Encryption;
	}	
	if(ui.updateMask & UpdateInfo::MASK_CONNECTIONS) {
		connections = ui.connections;
	}
	if(ui.updateMask & UpdateInfo::MASK_BUNDLE) {
		bundle = ui.bundle;
	}
	if(ui.updateMask & UpdateInfo::MASK_USERS) {
		onlineUsers = ui.onlineUsers;
	}
	if(ui.updateMask & UpdateInfo::MASK_USER) {
		user = ui.user;
	}
}

void TransferView::updateItem(const ItemInfo* aII, uint32_t updateMask) {
	int ii = ctrlTransfers.findItem(aII);
	if (ii == -1)
		return;

	if(	updateMask & UpdateInfo::MASK_STATUS || updateMask & UpdateInfo::MASK_STATUS_STRING ||
		updateMask & UpdateInfo::MASK_POS || updateMask & UpdateInfo::MASK_ACTUAL) {
		ctrlTransfers.updateItem(ii, COLUMN_STATUS);
	}

	//if(updateMask & UpdateInfo::MASK_ENCRYPTION) {
		//ctrlTransfers.updateItem(ii, COLUMN_ENCRYPTION);
	//}

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
	if(updateMask & UpdateInfo::MASK_CONNECTIONS || updateMask & UpdateInfo::MASK_USER) {
		ctrlTransfers.updateItem(ii, COLUMN_HUB_CONNECTIONS);
	}
	if(updateMask & UpdateInfo::MASK_USERS || updateMask & UpdateInfo::MASK_CONNECTIONS) {
		ctrlTransfers.updateItem(ii, COLUMN_USER);
	}
	if(updateMask & UpdateInfo::MASK_USER) {
		ctrlTransfers.updateItem(ii, COLUMN_USER);
	}
}

void TransferView::UpdateInfo::setUpdateFlags(const TransferInfoPtr& aInfo, int aUpdatedProperties) noexcept {
	if (aUpdatedProperties & TransferInfo::UpdateFlags::TARGET)
		setTarget(Text::toT(aInfo->getTarget()));
	if (aUpdatedProperties & TransferInfo::UpdateFlags::TYPE)
		setType(aInfo->getType());
	if (aUpdatedProperties & TransferInfo::UpdateFlags::SIZE)
		setSize(aInfo->getSize());
	if (aUpdatedProperties & TransferInfo::UpdateFlags::STATUS || aUpdatedProperties & TransferInfo::UpdateFlags::FLAGS) {
		if (aInfo->getState() == TransferInfo::STATE_RUNNING) {
			auto posStr = Util::formatBytesW(aInfo->getBytesTransferred());
			auto percent = (double)aInfo->getBytesTransferred() * 100.0 / (double)aInfo->getSize();
			auto elapsed = Util::formatSecondsW((GET_TICK() - aInfo->getStarted()) / 1000);
			auto statusStr = getRunningStatus(aInfo->getFlags());

			statusStr += aInfo->isDownload() ?
				CTSTRING_F(DOWNLOADED_BYTES, posStr.c_str() % percent % elapsed) :
				CTSTRING_F(UPLOADED_BYTES, posStr.c_str() % percent % elapsed);

			setStatusString(statusStr);
		} else {
			setStatusString(Text::toT(aInfo->getStatusString()));
		}
	}
	if (aUpdatedProperties & TransferInfo::UpdateFlags::STATE) {
		if (aInfo->getState() == TransferInfo::STATE_RUNNING) {
			setStatus(ItemInfo::STATUS_RUNNING);
		} else {
			setStatus(ItemInfo::STATUS_WAITING);
		}
	}
	if (aUpdatedProperties & TransferInfo::UpdateFlags::BYTES_TRANSFERRED)
		setActual(aInfo->getBytesTransferred());
	if (aUpdatedProperties & TransferInfo::UpdateFlags::USER)
		setUser(aInfo->getHintedUser());
	if (aUpdatedProperties & TransferInfo::UpdateFlags::SPEED)
		setSpeed(aInfo->getSpeed());
	if (aUpdatedProperties & TransferInfo::UpdateFlags::SECONDS_LEFT)
		setTimeLeft(aInfo->getTimeLeft());
	if (aUpdatedProperties & TransferInfo::UpdateFlags::IP) {
		setIP(FormatUtil::toCountryInfo(aInfo->getIp()));
	}
	if (aUpdatedProperties & TransferInfo::UpdateFlags::ENCRYPTION)
		setEncryptionInfo(Text::toT(aInfo->getEncryption()));
}

static tstring getFile(const Transfer::Type& type, const tstring& fileName) {
	tstring file;
	if (type == Transfer::TYPE_TREE) {
		file = _T("TTH: ") + fileName;
	} else if (type == Transfer::TYPE_FULL_LIST) {
		file = TSTRING(TYPE_FILE_LIST);
	} else if (type == Transfer::TYPE_PARTIAL_LIST) {
		file = TSTRING(TYPE_FILE_LIST_PARTIAL);
	} else {
		file = fileName;
	}

	return file;
}

TransferView::ItemInfo* TransferView::ItemInfo::createParent() {
	auto ii = new ItemInfo(user, bundle, download);
	ii->connections = 0;
	ii->hits = 0;
	dcassert(!bundle.empty());
	ii->isBundle = true;
	ii->bundle = bundle;

	if (download) {
		auto b = QueueManager::getInstance()->findBundle(getLocalBundleToken());
		if (b) {
			ii->target = Text::toT(b->getTarget());
			ii->size = b->getSize();

			auto sources = QueueManager::getInstance()->getSourceCount(b);
			ii->onlineUsers = sources.online;
		}
	} else {
		auto b = UploadBundleManager::getInstance()->receiver.findByBundleToken(bundle);
		if (b) {
			ii->target = Text::toT(b->getTarget());
			ii->size = b->getSize();
			ii->encryption = encryption;
			ii->ip = ip;
			ii->flagIndex = flagIndex;
		}
	}

	ii->statusString = TSTRING(CONNECTING);
	return ii;
}

const string& TransferView::ItemInfo::getGroupCond() const {
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
				if ((onlineUsers == 1 || hits == 2) && user.user) {
					return FormatUtil::getNicks(user);
				} else {
					return TSTRING_F(X_USERS_WAITING, onlineUsers % ((hits - connections - 1) > 0 ? hits - connections - 1 : 0));
				}
			} else if (hits == -1 || isBundle) {
				return FormatUtil::getNicks(user);
			} else {
				return TSTRING_F(X_USERS, hits);
			}
		case COLUMN_HUB_CONNECTIONS:
			if (isBundle) {
				return TSTRING_F(X_CONNECTIONS, connections);
			} else {
				return FormatUtil::getHubNames(user);
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
				if (PathUtil::isDirectoryPath(target)) {
					return PathUtil::getLastDir(target); //directory bundle
				}

				return PathUtil::getFileName(target); //file bundle
			}
			return getFile(type, PathUtil::getFileName(target));
		case COLUMN_SIZE: return Util::formatBytesW(size); 
		case COLUMN_PATH: return PathUtil::getFilePath(target);
		case COLUMN_IP: return ip;
		//case COLUMN_ENCRYPTION: return Encryption;
		default: return Util::emptyStringT;
	}
}

void TransferView::on(DownloadManagerListener::BundleTick, const BundleList& bundles, uint64_t /*aTick*/) noexcept {
	for(const auto& b: bundles) {
		double ratio = 0;
		int64_t totalSpeed = 0;

		auto ui = new UpdateInfo(b->getStringToken(), true);
		OrderedStringSet flags;
		for(const auto& d: b->getDownloads()) {
			if(d->getStart() > 0) {
				d->appendFlags(flags);

				totalSpeed += static_cast<int64_t>(d->getAverageSpeed());
				ratio += d->getPos() > 0 ? (double)d->getActual() / (double)d->getPos() : 1.00;
			}
		}

		if (auto connections = b->getDownloads().size(); connections > 0) {
			ratio = ratio / connections;

			ui->setStatus(ItemInfo::STATUS_RUNNING);
			ui->setSize(b->getSize());
			ui->setPos(b->getDownloadedBytes());
			ui->setActual((int64_t)((double)ui->pos * (ratio == 0 ? 1.00 : ratio)));
			ui->setTimeLeft(b->getSecondsLeft());
			ui->setSpeed(totalSpeed);
			ui->setConnections(connections);

			uint64_t timeSinceStarted = GET_TICK() - b->getStart();
			if (timeSinceStarted < 1000) {
				// bundle is starting
				ui->setStatusString(TSTRING(DOWNLOAD_STARTING));
			} else {
				tstring pos = Text::toT(Util::formatBytes(ui->pos));
				double percent = (double)ui->pos*100.0/(double)ui->size;
				dcassert(percent <= 100.00);
				tstring elapsed = Util::formatSecondsW(timeSinceStarted / 1000);

				ui->setStatusString(getRunningStatus(flags) + TSTRING_F(DOWNLOADED_BYTES, pos.c_str() % percent % elapsed.c_str()));
			}
		}

		speak(UPDATE_BUNDLE, ui);
	}
}

void TransferView::on(UploadBundleInfoReceiverListener::BundleComplete, const string& aBundleToken, const string& aBundleName) noexcept {
	onBundleComplete(aBundleToken, aBundleName, true);
}

void TransferView::on(UploadBundleInfoReceiverListener::BundleTick, const TickUploadBundleList& aBundles) noexcept {
	for (const auto& [b, flags] : aBundles) {
		auto ui = new UpdateInfo(b->getToken(), false);

		if (b->getConnectionCount() > 0) {
			ui->setStatus(ItemInfo::STATUS_RUNNING);
			ui->setSize(b->getSize());
			ui->setPos(b->getUploaded());
			ui->setActual(b->getActual());
			ui->setTimeLeft(b->getSecondsLeft());
			ui->setTotalSpeed(b->getTotalSpeed());
			ui->setSpeed(b->getSpeed());
			ui->setOnlineUsers(1);
			if (b->getSingleUser()) {
				ui->setTotalSpeed(0);
			} else {
				ui->setTotalSpeed(b->getTotalSpeed());
			}
			ui->setConnections(b->getConnectionCount());

			uint64_t timeSinceStarted = GET_TICK() - b->getStart();
			if (timeSinceStarted < 1000) {
				ui->setStatusString(TSTRING(UPLOAD_STARTING));
			} else {
				tstring pos = Text::toT(Util::formatBytes(ui->pos));
				double percent = (double)ui->pos * 100.0 / (double)ui->size;
				dcassert(percent <= 100.00);
				tstring elapsed = Util::formatSecondsW(timeSinceStarted / 1000);

				ui->setStatusString(getRunningStatus(flags) + TSTRING_F(UPLOADED_BYTES, pos.c_str() % percent % elapsed.c_str()));
			}
		}

		speak(UPDATE_BUNDLE, ui);
	}
}

tstring TransferView::getRunningStatus(const OrderedStringSet& aFlags) noexcept {
	tstring ret;
	for (const auto& f : aFlags) {
		ret += _T("[") + Text::toT(f) + _T("]");
	}

	if (!aFlags.empty()) {
		ret += _T(" ");
	}

	return ret;
}


void TransferView::onBundleComplete(const string& aBundleToken, const string& aBundleName, bool aIsUpload) {
	auto ui = new UpdateInfo(aBundleToken, !aIsUpload);

	ui->setStatus(ItemInfo::STATUS_WAITING);	

	ui->setPos(0);
	ui->setStatusString(aIsUpload ? TSTRING(UPLOAD_FINISHED_IDLE) : TSTRING(DOWNLOAD_FINISHED_IDLE));
	ui->setConnections(0);
	ui->setOnlineUsers(0);

	if (SETTING(POPUP_BUNDLE_DLS) && !aIsUpload) {
		WinUtil::showPopup(_T("The following bundle has finished downloading: ") + Text::toT(aBundleName), TSTRING(DOWNLOAD_FINISHED_IDLE));
	} else if (SETTING(POPUP_BUNDLE_ULS) && aIsUpload) {
		WinUtil::showPopup(_T("The following bundle has finished uploading: ") + Text::toT(aBundleName), TSTRING(UPLOAD_FINISHED_IDLE));
	}
	
	speak(UPDATE_BUNDLE, ui);
}

void TransferView::onBundleWaiting(const BundlePtr& aBundle, bool aRemoved) {
	auto ui = new UpdateInfo(aBundle->getStringToken(), true);
	ui->setStatus(ItemInfo::STATUS_WAITING);
	if (aRemoved) {
		ui->setStatusString(TSTRING(BUNDLE_REMOVED));
	} else {
		ui->setStatusString(TSTRING(WAITING));
	}

	ui->setConnections(0);
	speak(UPDATE_BUNDLE, ui);
}

void TransferView::on(QueueManagerListener::BundleRemoved, const BundlePtr& aBundle) noexcept {
	if (aBundle->getStatus() < Bundle::STATUS_DOWNLOADED)
		onBundleWaiting(aBundle, true);
}

void TransferView::on(QueueManagerListener::BundleSize, const BundlePtr& aBundle) noexcept {
	auto ui = new UpdateInfo(aBundle->getStringToken(), true);
	ui->setSize(aBundle->getSize());
	speak(UPDATE_BUNDLE, ui);
}

void TransferView::on(UploadBundleInfoReceiverListener::BundleSizeName, const string& aBundleToken, const string& aNewTarget, int64_t aSize) noexcept {
	auto ui = new UpdateInfo(aBundleToken, false);
	ui->setSize(aSize);
	ui->setTarget(Text::toT(aNewTarget));
	speak(UPDATE_BUNDLE, ui);
}

void TransferView::on(QueueManagerListener::BundleSources, const BundlePtr& aBundle) noexcept {
	auto ui = new UpdateInfo(aBundle->getStringToken(), true);

	auto sources = QueueManager::getInstance()->getSourceCount(aBundle);
	ui->setOnlineUsers(sources.online);
	speak(UPDATE_BUNDLE, ui);
}

void TransferView::on(QueueManagerListener::BundleDownloadStatus, const BundlePtr& aBundle) noexcept {
	onBundleWaiting(aBundle, false);
}

void TransferView::on(QueueManagerListener::BundleStatusChanged, const BundlePtr& aBundle) noexcept {
	if (aBundle->getStatus() == Bundle::STATUS_DOWNLOADED) {
		onBundleComplete(aBundle->getStringToken(), aBundle->getName(), false);
	}
}

void TransferView::onBundleName(const BundlePtr& aBundle) {
	auto ui = new UpdateInfo(aBundle->getStringToken(), true);
	ui->setTarget(Text::toT(aBundle->getTarget()));
	speak(UPDATE_BUNDLE, ui);
}

void TransferView::on(TransferInfoManagerListener::Added, const TransferInfoPtr& aInfo) noexcept {
	auto ui = new UpdateInfo(aInfo->getStringToken(), aInfo->isDownload());
	ui->setUser(aInfo->getHintedUser());
	ui->setStatus(ItemInfo::STATUS_WAITING);
	ui->setStatusString(TSTRING(CONNECTING));
	speak(ADD_ITEM, ui);
}

void TransferView::on(TransferInfoManagerListener::Updated, const TransferInfoPtr& aInfo, int aUpdatedProperties, bool aTick) noexcept {
	if (aTick) {
		return;
	}

	auto ui = new UpdateInfo(aInfo->getStringToken(), aInfo->isDownload());
	ui->setUpdateFlags(aInfo, aUpdatedProperties);
	// ui->setBundle(aInfo->getBundle());
	ui->setBundle(getBundle(aInfo));

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(TransferInfoManagerListener::Removed, const TransferInfoPtr& aInfo) noexcept {
	speak(REMOVE_ITEM, new UpdateInfo(aInfo->getStringToken(), aInfo->isDownload()));
}

void TransferView::on(TransferInfoManagerListener::Failed, const TransferInfoPtr& aInfo) noexcept {
	if (SETTING(POPUP_DOWNLOAD_FAILED)) {
		WinUtil::showPopup(
			TSTRING(FILE) + _T(": ") + Text::toT(PathUtil::getFileName(aInfo->getTarget())) + _T("\n") +
			TSTRING(USER) + _T(": ") + FormatUtil::getNicks(aInfo->getHintedUser()) + _T("\n") +
			TSTRING(REASON) + _T(": ") + Text::toT(aInfo->getStatusString()), TSTRING(DOWNLOAD_FAILED), NIIF_WARNING);
	}
}

void TransferView::on(TransferInfoManagerListener::Completed, const TransferInfoPtr& aInfo) noexcept {
	if (!aInfo->isDownload() && SETTING(POPUP_UPLOAD_FINISHED) && aInfo->getType() != Transfer::TYPE_TREE) {
		if (!SETTING(UPLOADFILE).empty() && !SETTING(SOUNDS_DISABLED))
			WinUtil::playSound(Text::toT(SETTING(UPLOADFILE)));

		if (SETTING(POPUP_UPLOAD_FINISHED)) {
			WinUtil::showPopup(TSTRING(FILE) + _T(": ") + Text::toT(aInfo->getName()) + _T("\n") +
				TSTRING(USER) + _T(": ") + FormatUtil::getNicks(aInfo->getHintedUser()), TSTRING(UPLOAD_FINISHED_IDLE));
		}
	}
}

const string& TransferView::getBundle(const TransferInfoPtr& aInfo) noexcept {
	if (aInfo->isDownload()) {
		return aInfo->getBundle();
	} else {
		auto bundle = UploadBundleManager::getInstance()->receiver.findByConnectionToken(aInfo->getStringToken());
		if (bundle) {
			// dcdebug("Found bundle %s for upload %s\n", bundle->getToken().c_str(), aInfo->getStringToken().c_str());
			return bundle->getToken();
		} else {
			// dcdebug("No bundle for upload %s\n", aInfo->getStringToken().c_str());
		}
	}

	return Util::emptyString;
}

void TransferView::on(TransferInfoManagerListener::Tick, const TransferInfo::List& aTransfers, int aUpdatedProperties) noexcept {
	for (const auto& t : aTransfers) {
		auto ui = new UpdateInfo(t->getStringToken(), t->isDownload());
		ui->setBundle(getBundle(t));
		/*if (t->isDownload()) {
			ui->setBundle(t->getBundle());
		} else {
			auto bundle = UploadBundleInfoReceiver::getInstance()->findByUploadToken(t->getStringToken());
			if (bundle) {
				ui->setBundle(bundle->getToken());
			}
		}*/
		

		ui->setUpdateFlags(t, aUpdatedProperties);
		tasks.add(UPDATE_ITEM, unique_ptr<Task>(ui));
	}

	PostMessage(WM_SPEAKER);
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
				ranges::for_each(children, f);
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
		auto m = ctrlTransfers.getItemData(q);
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
	for (const auto& [_, parentPair] : ctrlTransfers.getParents()) {
		auto l = parentPair.parent;
		if(l->collapsed) {
			ctrlTransfers.Expand(l, ctrlTransfers.findItem(l));
		}
	}
}

void TransferView::handleSlowDisconnect() {
	auto slowDisconnect = [=](const ItemInfo* ii) {
		QueueManager::getInstance()->toggleSlowDisconnectBundle(ii->getLocalBundleToken());
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
		ActionUtil::search(PathUtil::getLastDir(ii->target), true);
	};

	performActionBundles(search);
}


void TransferView::handleSearchAlternates() {
	auto search = [=](const ItemInfo* ii) {
		TTHValue tth;
		int64_t size = 0;
		if(QueueManager::getInstance()->getSearchInfo(Text::fromT(ii->target), tth, size)) {
			ActionUtil::searchHash(tth, PathUtil::getFileName(Text::fromT(ii->target)), size);
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
		ActionUtil::removeBundle(ii->getLocalBundleToken());
	};

	performActionBundles(removeBundle);
}

void TransferView::handleRemoveBundleSource() {
	auto removeSource = [=](const ItemInfo* ii) {
		QueueManager::getInstance()->removeBundleSource(ii->getLocalBundleToken(), ii->user, QueueItem::Source::FLAG_REMOVED);
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
		ActionUtil::openFolder(ii->target);
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

	if (ctrlTransfers.GetFont() != WinUtil::listViewFont){
		ctrlTransfers.SetFont(WinUtil::listViewFont);
		refresh = true;
	}

	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}
}