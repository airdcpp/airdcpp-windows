/*
* Copyright (C) 2011-2014 AirDC++ Project
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

#include "QueueFrame.h"
#include "MainFrm.h"
#include "PrivateFrame.h"

#include "../client/AirUtil.h"
#include "../client/DownloadManager.h"
#include "ResourceLoader.h"
#include "BarShader.h"

int QueueFrame::columnIndexes[] = { COLUMN_NAME, COLUMN_SIZE, COLUMN_PRIORITY, COLUMN_STATUS, COLUMN_DOWNLOADED, COLUMN_SOURCES, COLUMN_PATH};

int QueueFrame::columnSizes[] = { 450, 70, 100, 130, 200, 80, 500 };

static ResourceManager::Strings columnNames[] = { ResourceManager::NAME, ResourceManager::SIZE, ResourceManager::PRIORITY, ResourceManager::STATUS, ResourceManager::DOWNLOADED, ResourceManager::SOURCES, ResourceManager::PATH };

static ResourceManager::Strings groupNames[] = { ResourceManager::FILE_LISTS, ResourceManager::TEMP_ITEMS, ResourceManager::BUNDLES };

LRESULT QueueFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{

	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	ctrlQueue.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_QUEUE_LIST);
	ctrlQueue.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
	ctrlQueue.SetImageList(ResourceLoader::getFileImages(), LVSIL_SMALL);

	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(QUEUEFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(QUEUEFRAME_WIDTHS), COLUMN_LAST);

	for (uint8_t j = 0; j < COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE) ? LVCFMT_RIGHT : (j == COLUMN_DOWNLOADED) ? LVCFMT_CENTER : LVCFMT_LEFT;
		ctrlQueue.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}

	ctrlQueue.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlQueue.setSortColumn(COLUMN_NAME);
	ctrlQueue.setVisible(SETTING(QUEUEFRAME_VISIBLE));
	ctrlQueue.SetBkColor(WinUtil::bgColor);
	ctrlQueue.SetTextBkColor(WinUtil::bgColor);
	ctrlQueue.SetTextColor(WinUtil::textColor);
	ctrlQueue.setFlickerFree(WinUtil::bgBrush);

	CRect rc(SETTING(QUEUE_LEFT), SETTING(QUEUE_TOP), SETTING(QUEUE_RIGHT), SETTING(QUEUE_BOTTOM));
	if (!(rc.top == 0 && rc.bottom == 0 && rc.left == 0 && rc.right == 0))
		MoveWindow(rc, TRUE);

	//add the groups
	for (uint8_t i = 0; i < GROUP_LAST; ++i) {
		ctrlQueue.insertGroup(i, TSTRING_I(groupNames[i]), LVGA_HEADER_CENTER);
	}

	{
		auto qm = QueueManager::getInstance();

		RLock l(qm->getCS());
		for (const auto& b : qm->getBundles() | map_values)
			onBundleAdded(b);

		for (const auto& q : qm->getFileQueue() | map_values) {
			if (!q->getBundle())
				onQueueItemAdded(q);
		}
	}
	ctrlQueue.resort();

	QueueManager::getInstance()->addListener(this);
	DownloadManager::getInstance()->addListener(this);

	memzero(statusSizes, sizeof(statusSizes));
	statusSizes[0] = 16;
	ctrlStatus.SetParts(6, statusSizes);
	updateStatus();

	::SetTimer(m_hWnd, 0, 500, 0);

	WinUtil::SetIcon(m_hWnd, IDI_QUEUE);
	bHandled = FALSE;
	return 1;
}

void QueueFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);

	if (ctrlStatus.IsWindow()) {
		CRect sr;
		int w[6];
		ctrlStatus.GetClientRect(sr);
		w[5] = sr.right - 16;
#define setw(x) w[x] = max(w[x+1] - statusSizes[x], 0)
		setw(4); setw(3); setw(2); setw(1);

		w[0] = 16;

		ctrlStatus.SetParts(6, w);

		ctrlStatus.GetRect(0, sr);
	}
	CRect rc = rect;
	ctrlQueue.MoveWindow(&rc);
}


LRESULT QueueFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if (!closed) {
		QueueManager::getInstance()->removeListener(this);
		DownloadManager::getInstance()->removeListener(this);
		closed = true;
		WinUtil::setButtonPressed(IDC_QUEUE, false);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		CRect rc;
		if (!IsIconic()){
			//Get position of window
			GetWindowRect(&rc);
			//convert the position so it's relative to main window
			::ScreenToClient(GetParent(), &rc.TopLeft());
			::ScreenToClient(GetParent(), &rc.BottomRight());
			//save the position
			SettingsManager::getInstance()->set(SettingsManager::QUEUE_BOTTOM, (rc.bottom > 0 ? rc.bottom : 0));
			SettingsManager::getInstance()->set(SettingsManager::QUEUE_TOP, (rc.top > 0 ? rc.top : 0));
			SettingsManager::getInstance()->set(SettingsManager::QUEUE_LEFT, (rc.left > 0 ? rc.left : 0));
			SettingsManager::getInstance()->set(SettingsManager::QUEUE_RIGHT, (rc.right > 0 ? rc.right : 0));
		}

		ctrlQueue.saveHeaderOrder(SettingsManager::QUEUEFRAME_ORDER,
			SettingsManager::QUEUEFRAME_WIDTHS, SettingsManager::QUEUEFRAME_VISIBLE);
	
		ctrlQueue.DeleteAllItems();
		for (auto& i : itemInfos)
			delete i.second;

		itemInfos.clear();

		bHandled = FALSE;
		return 0;
	}
}

LRESULT QueueFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)pnmh;

	switch (cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: 
	{
			auto ii = reinterpret_cast<QueueItemInfo*>(cd->nmcd.lItemlParam);

			int colIndex = ctrlQueue.findColumn(cd->iSubItem);
			cd->clrTextBk = WinUtil::bgColor;

			if (colIndex == COLUMN_DOWNLOADED) {
				if (!SETTING(SHOW_PROGRESS_BARS) || !SETTING(SHOW_QUEUE_BARS) || (ii->getSize() == -1)) { // file lists don't have size in queue, don't even start to draw...
					bHandled = FALSE;
					return 0;
				}

				// Get the text to draw
				// Get the color of this bar
				COLORREF clr = SETTING(PROGRESS_OVERRIDE_COLORS) ? 
					(ii->parent ? SETTING(PROGRESS_SEGMENT_COLOR) : SETTING(DOWNLOAD_BAR_COLOR)) : GetSysColor(COLOR_HIGHLIGHT);

				//this is just severely broken, msdn says GetSubItemRect requires a one based index
				//but it wont work and index 0 gives the rect of the whole item
				CRect rc;
				if (cd->iSubItem == 0) {
					//use LVIR_LABEL to exclude the icon area since we will be painting over that
					//later
					ctrlQueue.GetItemRect((int)cd->nmcd.dwItemSpec, &rc, LVIR_LABEL);
				}
				else {
					ctrlQueue.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, &rc);
				}

				// fixes issues with double border
				rc.top -= 1;

				// Real rc, the original one.
				CRect real_rc = rc;
				// We need to offset the current rc to (0, 0) to paint on the New dc
				rc.MoveToXY(0, 0);

				CDC cdc;
				cdc.CreateCompatibleDC(cd->nmcd.hdc);

				HBITMAP pOldBmp = cdc.SelectBitmap(CreateCompatibleBitmap(cd->nmcd.hdc, real_rc.Width(), real_rc.Height()));
				HDC& dc = cdc.m_hDC;

				HFONT oldFont = (HFONT)SelectObject(dc, WinUtil::font);
				SetBkMode(dc, TRANSPARENT);

				COLORREF oldcol;
				if (SETTING(PROGRESSBAR_ODC_STYLE)) {
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
					::Rectangle(dc, rc.left, rc.top, rc.left + (LONG)(rc.Width() * 1 + 0.5), rc.bottom);

					// Reset pen
					DeleteObject(::SelectObject(dc, pOldPen));
					// Reset bg (brush)
					DeleteObject(::SelectObject(dc, oldBg));

					// Draw the text over the entire item
					oldcol = ::SetTextColor(dc, SETTING(PROGRESS_OVERRIDE_COLORS2) ? SETTING(PROGRESS_TEXT_COLOR_DOWN) : clr);
	
					//Want to center the text, DT_CENTER wont work with the changing text colours so center with the drawing rect..
					CRect textRect = rc;
					int textWidth = WinUtil::getTextWidth(ii->getText(colIndex), dc);
					textRect.left += (textRect.right / 2) - (textWidth / 2);
					::DrawText(dc, ii->getText(colIndex).c_str(), ii->getText(colIndex).length(), textRect, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);

					rc.right = rc.left + (int)((int64_t)rc.Width() * ii->getDownloadedBytes() / ii->getSize());

					COLORREF a, b;
					OperaColors::EnlightenFlood(clr, a, b);
					OperaColors::FloodFill(cdc, rc.left + 1, rc.top + 1, rc.right, rc.bottom - 1, a, b);

					// Draw the text only over the bar and with correct color
					::SetTextColor(dc, SETTING(PROGRESS_OVERRIDE_COLORS2) ? SETTING(PROGRESS_TEXT_COLOR_DOWN) : 
						OperaColors::TextFromBackground(clr));

					textRect.right = textRect.left > rc.right ? textRect.left : rc.right;
					::DrawText(dc, ii->getText(colIndex).c_str(), ii->getText(colIndex).length(), textRect, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
				} else {
					CBarShader statusBar(rc.bottom - rc.top, rc.right - rc.left, SETTING(PROGRESS_BACK_COLOR), ii->getSize());

					statusBar.FillRange(0, ii->getDownloadedBytes(), clr);

					statusBar.Draw(cdc, rc.top, rc.left, SETTING(PROGRESS_3DDEPTH));

					// Get the color of this text bar
					oldcol = ::SetTextColor(dc, SETTING(PROGRESS_OVERRIDE_COLORS2) ? SETTING(PROGRESS_TEXT_COLOR_DOWN) :
						OperaColors::TextFromBackground(clr));

					rc.left += 6;
					rc.right -= 2;
					LONG top = rc.top + (rc.Height() - WinUtil::getTextHeight(cd->nmcd.hdc) - 1) / 2 + 1;
					::ExtTextOut(dc, rc.left, top, ETO_CLIPPED, rc, ii->getText(colIndex).c_str(), ii->getText(colIndex).length(), NULL);
				}
				SelectObject(dc, oldFont);
				::SetTextColor(dc, oldcol);

				// New way:
				BitBlt(cd->nmcd.hdc, real_rc.left, real_rc.top, real_rc.Width(), real_rc.Height(), dc, 0, 0, SRCCOPY);
				DeleteObject(cdc.SelectBitmap(pOldBmp));
				
				//bah crap, if we return CDRF_SKIPDEFAULT windows won't paint the icons
				//so we have to do it
				if (!SETTING(USE_EXPLORER_THEME) && cd->iSubItem == 0){
					LVITEM lvItem;
					lvItem.iItem = cd->nmcd.dwItemSpec;
					lvItem.iSubItem = 0;
					lvItem.mask = LVIF_IMAGE | LVIF_STATE;
					lvItem.stateMask = LVIS_SELECTED;
					ctrlQueue.GetItem(&lvItem);

					HIMAGELIST imageList = (HIMAGELIST)::SendMessage(ctrlQueue.m_hWnd, LVM_GETIMAGELIST, LVSIL_SMALL, 0);
					if (imageList) {
						//let's find out where to paint it
						//and draw the background to avoid having 
						//the selection color as background
						CRect iconRect;
						ctrlQueue.GetSubItemRect((int)cd->nmcd.dwItemSpec, 0, LVIR_ICON, iconRect);
						ImageList_Draw(imageList, lvItem.iImage, cd->nmcd.hdc, iconRect.left, iconRect.top, ILD_TRANSPARENT);
					}
				}
				return CDRF_SKIPDEFAULT;
		}

	}
	default:
		return CDRF_DODEFAULT;
	}
}

LRESULT QueueFrame::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	if (kd->wVKey == VK_DELETE) {
		BundleList bl;
		QueueItemList ql;
		getSelectedItems(bl, ql);
		if (!bl.empty()) {
			handleRemoveBundles(bl, false);
		} else {
			handleRemoveFiles(ql);
		}
	}
	return 0;
}

void QueueFrame::getSelectedItems(BundleList& bl, QueueItemList& ql) {
	int sel = -1;
	while ((sel = ctrlQueue.GetNextItem(sel, LVNI_SELECTED)) != -1) {
		QueueItemInfo* qii = (QueueItemInfo*) ctrlQueue.GetItemData(sel);
		if (qii->bundle)
			bl.push_back(qii->bundle);
		else
			ql.push_back(qii->qi);
	}
}

LRESULT QueueFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	if (reinterpret_cast<HWND>(wParam) == ctrlQueue && ctrlQueue.GetSelectedCount() > 0) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		if (pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlQueue, pt);
		}

		OMenu menu;
		menu.CreatePopupMenu();
		BundleList bl;
		QueueItemList queueItems;

		getSelectedItems(bl, queueItems);

		if (!bl.empty()) {
			AppendBundleMenu(bl, menu);
		} else if (!queueItems.empty())
			AppendQiMenu(queueItems, menu);

		menu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
		return TRUE;
	}

	bHandled = FALSE;
	return FALSE;
}

/*Bundle Menu*/
void QueueFrame::AppendBundleMenu(BundleList& bl, OMenu& bundleMenu) {
	OMenu* removeMenu = bundleMenu.getMenu();
	OMenu* readdMenu = bundleMenu.getMenu();

	if (bl.size() == 1) {
		bundleMenu.InsertSeparatorFirst(Text::toT(bl.front()->getName()));
	} else {
		bundleMenu.InsertSeparatorFirst(CTSTRING_F(X_BUNDLES, bl.size()));
	}

	WinUtil::appendBundlePrioMenu(bundleMenu, bl);

	/* Insert sub menus */
	auto formatUser = [this](Bundle::BundleSource& bs) -> tstring {
		auto& u = bs.user;
		tstring nick = WinUtil::escapeMenu(WinUtil::getNicks(u));
		// add hub hint to menu
		bool addHint = !u.hint.empty(), addSpeed = u.user->getSpeed() > 0;
		nick += _T(" (") + TSTRING(FILES) + _T(": ") + Util::toStringW(bs.files);
		if (addHint || addSpeed) {
			nick += _T(", ");
			if (addSpeed) {
				nick += TSTRING(SPEED) + _T(": ") + Util::formatBytesW(u.user->getSpeed()) + _T("/s)");
			}
			if (addHint) {
				if (addSpeed) {
					nick += _T(", ");
				}
				nick += TSTRING(HUB) + _T(": ") + Text::toT(u.hint);
			}
		}
		nick += _T(")");
		return nick;
	};

	if (bl.size() == 1) {
		BundlePtr b = bl.front();
		//current sources
		auto bundleSources = move(QueueManager::getInstance()->getBundleSources(b));
		if (!bundleSources.empty()) {
			removeMenu->appendItem(TSTRING(ALL), [b] {
				auto sources = move(QueueManager::getInstance()->getBundleSources(b));
				for (auto& si : sources)
					QueueManager::getInstance()->removeBundleSource(b, si.user.user, QueueItem::Source::FLAG_REMOVED);
			}, OMenu::FLAG_THREADED);
			removeMenu->appendSeparator();
		}

		for (auto& bs : bundleSources) {
			auto u = bs.user;
			removeMenu->appendItem(formatUser(bs), [=] { QueueManager::getInstance()->removeBundleSource(b, u, QueueItem::Source::FLAG_REMOVED); }, OMenu::FLAG_THREADED);
		}

		//bad sources
		auto badBundleSources = move(QueueManager::getInstance()->getBadBundleSources(b));

		if (!badBundleSources.empty()) {
			readdMenu->appendItem(TSTRING(ALL), [=] {
				auto sources = move(QueueManager::getInstance()->getBadBundleSources(b));
				for (auto& si : sources)
					QueueManager::getInstance()->readdBundleSource(b, si.user);
			}, OMenu::FLAG_THREADED);
			readdMenu->appendSeparator();
		}

		for (auto& bs : badBundleSources) {
			auto u = bs.user;
			readdMenu->appendItem(formatUser(bs), [=] { QueueManager::getInstance()->readdBundleSource(b, u); }, OMenu::FLAG_THREADED);
		}
		/* Sub menus end */

		// search
		bundleMenu.appendItem(TSTRING(SEARCH_BUNDLE_ALT), [=] {
			auto bundle = b;
			QueueManager::getInstance()->searchBundle(bundle, true);
		}, OMenu::FLAG_THREADED);

		bundleMenu.appendSeparator();

		WinUtil::appendSearchMenu(bundleMenu, b->getName());
		bundleMenu.appendItem(TSTRING(SEARCH_DIRECTORY), [=] {
			WinUtil::searchAny(b->isFileBundle() ? Util::getLastDir(Text::toT(b->getTarget())) : Text::toT(b->getName()));
		});

		bundleMenu.appendItem(TSTRING(OPEN_FOLDER), [=] { WinUtil::openFolder(Text::toT(b->getTarget())); });

		bundleMenu.appendItem(TSTRING(RENAME), [=] { onRenameBundle(b); });
		bundleMenu.appendSeparator();

		if (b->isFailed()) {
			bundleMenu.appendItem(TSTRING(RETRY_SHARING), [=] { QueueManager::getInstance()->shareBundle(b, false); }, OMenu::FLAG_THREADED);
			if (b->getStatus() == Bundle::STATUS_SHARING_FAILED || b->getStatus() == Bundle::STATUS_FAILED_MISSING) {
				bundleMenu.appendItem(TSTRING(FORCE_SHARING), [=] { QueueManager::getInstance()->shareBundle(b, true); }, OMenu::FLAG_THREADED);
			}
			bundleMenu.appendSeparator();
		}

		readdMenu->appendThis(TSTRING(READD_SOURCE), true);
		removeMenu->appendThis(TSTRING(REMOVE_SOURCE), true);

		bundleMenu.appendItem(TSTRING(USE_SEQ_ORDER), [=] {
			auto bundle = b;
			QueueManager::getInstance()->onUseSeqOrder(bundle);
		}, b->getSeqOrder() ? OMenu::FLAG_CHECKED : 0 | OMenu::FLAG_THREADED);
	}

	bundleMenu.appendItem(TSTRING(MOVE_BUNDLE), [=] { handleMoveBundles(bl); });
	bundleMenu.appendItem(TSTRING(REMOVE), [=] { handleRemoveBundles(bl, false); });
	bundleMenu.appendItem(TSTRING(REMOVE_WITH_FILES), [=] { handleRemoveBundles(bl, true); });
}

/*QueueItem Menu*/
void QueueFrame::AppendQiMenu(QueueItemList& ql, OMenu& fileMenu) {

	/* Do we need to control segment counts??
	OMenu segmentsMenu;
	segmentsMenu.CreatePopupMenu();
	segmentsMenu.InsertSeparatorFirst(TSTRING(MAX_SEGMENTS_NUMBER));
	for (int i = IDC_SEGMENTONE; i <= IDC_SEGMENTTEN; i++)
		segmentsMenu.AppendMenu(MF_STRING, i, (Util::toStringW(i - 109) + _T(" ") + TSTRING(SEGMENTS)).c_str());
	*/

	if (ql.size() == 1) {
		QueueItemPtr qi = ql.front();

		OMenu* pmMenu = fileMenu.getMenu();
		OMenu* browseMenu = fileMenu.getMenu();
		OMenu* removeAllMenu = fileMenu.getMenu();
		OMenu* removeMenu = fileMenu.getMenu();
		OMenu* readdMenu = fileMenu.getMenu();
		OMenu* getListMenu = fileMenu.getMenu();


		/* Create submenus */
		//segmentsMenu.CheckMenuItem(qi->getMaxSegments(), MF_BYPOSITION | MF_CHECKED);

		bool hasPMItems = false;
		auto sources = move(QueueManager::getInstance()->getSources(qi));

		//remove all sources from this file
		if (!sources.empty()) {
			removeMenu->appendItem(TSTRING(ALL), [=] {
				auto sources = QueueManager::getInstance()->getSources(qi);
				for (auto& si : sources)
					QueueManager::getInstance()->removeFileSource(qi->getTarget(), si.getUser(), QueueItem::Source::FLAG_REMOVED);
			}, OMenu::FLAG_THREADED);
			removeMenu->appendSeparator();
		}

		for (auto& s : sources) {
			tstring nick = WinUtil::escapeMenu(WinUtil::getNicks(s.getUser()));
			// add hub hint to menu
			if (!s.getUser().hint.empty())
				nick += _T(" (") + Text::toT(s.getUser().hint) + _T(")");

			auto u = s.getUser();
			auto target = qi->getTarget();

			// get list
			getListMenu->appendItem(nick, [=] {
				try {
					QueueManager::getInstance()->addList(u, QueueItem::FLAG_CLIENT_VIEW);
				} catch (const QueueException& e) {
					ctrlStatus.SetText(1, Text::toT(e.getError()).c_str());
				}
			});

			// browse list
			browseMenu->appendItem(nick, [=] {
				try {
					QueueManager::getInstance()->addList(u, QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_PARTIAL_LIST);
				} catch (const QueueException& e) {
					ctrlStatus.SetText(1, Text::toT(e.getError()).c_str());
				}
			});

			// remove source (this file)
			removeMenu->appendItem(nick, [=] { QueueManager::getInstance()->removeFileSource(target, u, QueueItem::Source::FLAG_REMOVED); }, OMenu::FLAG_THREADED);
			//remove source (all files)
			removeAllMenu->appendItem(nick, [=]{ QueueManager::getInstance()->removeSource(u, QueueItem::Source::FLAG_REMOVED); }, OMenu::FLAG_THREADED);

			// PM
			if (s.getUser().user->isOnline()) {
				pmMenu->appendItem(nick, [=] { PrivateFrame::openWindow(u); });
				hasPMItems = true;
			}
		}

		auto badSources = move(QueueManager::getInstance()->getBadSources(qi));
		if (!badSources.empty()) {
			readdMenu->appendItem(TSTRING(ALL), [=] {
				auto sources = QueueManager::getInstance()->getBadSources(qi);
				for (auto& si : sources)
					QueueManager::getInstance()->readdQISource(qi->getTarget(), si.getUser());
			}, OMenu::FLAG_THREADED);
			readdMenu->appendSeparator();
		}

		for (auto& s : badSources) {
			tstring nick = WinUtil::getNicks(s.getUser());
			if (s.isSet(QueueItem::Source::FLAG_FILE_NOT_AVAILABLE)) {
				nick += _T(" (") + TSTRING(FILE_NOT_AVAILABLE) + _T(")");
			}
			else if (s.isSet(QueueItem::Source::FLAG_BAD_TREE)) {
				nick += _T(" (") + TSTRING(INVALID_TREE) + _T(")");
			}
			else if (s.isSet(QueueItem::Source::FLAG_NO_NEED_PARTS)) {
				nick += _T(" (") + TSTRING(NO_NEEDED_PART) + _T(")");
			}
			else if (s.isSet(QueueItem::Source::FLAG_NO_TTHF)) {
				nick += _T(" (") + TSTRING(SOURCE_TOO_OLD) + _T(")");
			}
			else if (s.isSet(QueueItem::Source::FLAG_SLOW_SOURCE)) {
				nick += _T(" (") + TSTRING(SLOW_USER) + _T(")");
			}
			else if (s.isSet(QueueItem::Source::FLAG_UNTRUSTED)) {
				nick += _T(" (") + TSTRING(CERTIFICATE_NOT_TRUSTED) + _T(")");
			}

			// add hub hint to menu
			if (!s.getUser().hint.empty())
				nick += _T(" (") + Text::toT(s.getUser().hint) + _T(")");

			auto u = s.getUser();
			auto target = qi->getTarget();
			readdMenu->appendItem(nick, [=] { QueueManager::getInstance()->readdQISource(target, u); });
		}
		/* Submenus end */

		fileMenu.InsertSeparatorFirst(TSTRING(FILE));
		fileMenu.appendItem(CTSTRING(SEARCH_FOR_ALTERNATES), [=] { handleSearchQI(qi); });

		if (!qi->isSet(QueueItem::FLAG_USER_LIST)) {
			WinUtil::appendPreviewMenu(fileMenu, qi->getTarget());
		}

		//fileMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)segmentsMenu, CTSTRING(MAX_SEGMENTS_NUMBER));

		WinUtil::appendFilePrioMenu(fileMenu, ql);

		browseMenu->appendThis(TSTRING(BROWSE_FILE_LIST), true);
		getListMenu->appendThis(TSTRING(GET_FILE_LIST), true);
		pmMenu->appendThis(TSTRING(SEND_PRIVATE_MESSAGE), true);

		fileMenu.AppendMenu(MF_SEPARATOR);

		ListType::MenuItemList customItems{
			{ TSTRING(MAGNET_LINK), &handleCopyMagnet }
		};

		ctrlQueue.appendCopyMenu(fileMenu, customItems);
		WinUtil::appendSearchMenu(fileMenu, Util::getFilePath(qi->getTarget()));

		fileMenu.AppendMenu(MF_SEPARATOR);
		
		//Todo: move items
		//fileMenu.AppendMenu(MF_STRING, IDC_MOVE, CTSTRING(MOVE_RENAME_FILE));

		fileMenu.appendItem(TSTRING(OPEN_FOLDER), [=] { WinUtil::openFolder(Text::toT(qi->getTarget())); });
		fileMenu.AppendMenu(MF_SEPARATOR);

		readdMenu->appendThis(TSTRING(READD_SOURCE), true);
		removeMenu->appendThis(TSTRING(REMOVE_SOURCE), true);
		removeAllMenu->appendThis(TSTRING(REMOVE_FROM_ALL), true);

		fileMenu.AppendMenu(MF_STRING, IDC_REMOVE_OFFLINE, CTSTRING(REMOVE_OFFLINE));
		//TODO: rechecker
		//fileMenu.AppendMenu(MF_SEPARATOR);
		//fileMenu.AppendMenu(MF_STRING, IDC_RECHECK, CTSTRING(RECHECK_FILE));
	} else {
		fileMenu.InsertSeparatorFirst(TSTRING(FILES));
		//fileMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)segmentsMenu, CTSTRING(MAX_SEGMENTS_NUMBER));

		WinUtil::appendFilePrioMenu(fileMenu, ql);

		fileMenu.AppendMenu(MF_SEPARATOR);
		fileMenu.AppendMenu(MF_STRING, IDC_REMOVE_OFFLINE, CTSTRING(REMOVE_OFFLINE));
		fileMenu.AppendMenu(MF_STRING, IDC_READD_ALL, CTSTRING(READD_ALL));
	}

	fileMenu.appendItem(TSTRING(REMOVE), [=] {
		handleRemoveFiles(ql);
	}, OMenu::FLAG_THREADED);
}

LRESULT QueueFrame::onRemoveOffline(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while ((i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const QueueItemInfo* ii = ctrlQueue.getItemData(i);

		const auto sources = QueueManager::getInstance()->getSources(ii->qi);
		for (const auto& s : sources) {
			if (!s.getUser().user->isOnline()) {
				QueueManager::getInstance()->removeFileSource(ii->qi->getTarget(), s.getUser().user, QueueItem::Source::FLAG_REMOVED);
			}
		}
	}
	return 0;
}
LRESULT QueueFrame::onReaddAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while ((i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const QueueItemInfo* ii = ctrlQueue.getItemData(i);
		if (ii->bundle)
			continue;

		// re-add all sources
		const auto badSources = QueueManager::getInstance()->getBadSources(ii->qi);
		for (const auto& bs : badSources) {
			QueueManager::getInstance()->readdQISource(ii->qi->getTarget(), bs.getUser());
		}
	}
	return 0;
}

/*
OK, here's the deal, we insert bundles as parents and assume every bundle (except file bundles) to have sub items, thus the + expand icon.
The bundle QueueItems(its sub items) are really created and inserted only at expanding the bundle,
once its expanded we start to collect some garbage when collapsing it to avoid continuous allocations and reallocations.
Notes, Mostly there should be no reason to expand every bundle at least with a big queue,
so this way we avoid creating and updating itemInfos we wont be showing,
with a small queue its more likely for the user to expand and collapse the same items more than once.
*/
LRESULT QueueFrame::onLButton(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {

	CPoint pt;
	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);

	LVHITTESTINFO lvhti;
	lvhti.pt = pt;

	int pos = ctrlQueue.SubItemHitTest(&lvhti);
	if (pos != -1) {
		CRect rect;
		ctrlQueue.GetItemRect(pos, rect, LVIR_ICON);

		if (pt.x < rect.left) {
			auto i = ctrlQueue.getItemData(pos);
			if ((i->parent == NULL) && i->bundle && !i->bundle->isFileBundle())  {
				if (i->collapsed) {
					//insert the children at first expand, collect some garbage.
					if (ctrlQueue.findChildren(i->bundle->getToken()).empty()) {
						ctrlQueue.SetRedraw(FALSE);
						AddBundleQueueItems(i->bundle);
						ctrlQueue.resort();
						ctrlQueue.SetRedraw(TRUE);
					} else {
						ctrlQueue.Expand(i, pos, i->getGroupID());
					}
				} else {
					ctrlQueue.Collapse(i, pos);
				}
			}
		}
	}

	bHandled = FALSE;
	return 0;
}

LRESULT QueueFrame::onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	executeGuiTasks();
	updateStatus();
	bHandled = TRUE;
	return 0;
}

tstring QueueFrame::handleCopyMagnet(const QueueItemInfo* aII) {
	return Text::toT(WinUtil::makeMagnet(aII->qi->getTTH(), Util::getFileName(aII->qi->getTarget()), aII->qi->getSize()));
}

void QueueFrame::handleMoveBundles(BundleList bundles) {
	tstring targetPath = Text::toT(Util::getParentDir(bundles.front()->getTarget()));

	if (!WinUtil::browseDirectory(targetPath, m_hWnd)) {
		return;
	}

	string newDir = Util::validatePath(Text::fromT(targetPath), true);
	if (bundles.size() == 1) {
		if (!WinUtil::showQuestionBox(TSTRING_F(CONFIRM_MOVE_DIR_BUNDLE, Text::toT(bundles.front()->getName()) % Text::toT(newDir)), MB_ICONQUESTION)) {
			return;
		}
	} else if (!WinUtil::showQuestionBox(TSTRING_F(CONFIRM_MOVE_DIR_MULTIPLE, bundles.size() % Text::toT(newDir)), MB_ICONQUESTION)) {
		return;
	}

	for (auto& sourceBundle : bundles) {
		MainFrame::getMainFrame()->addThreadedTask([=] {
			QueueManager::getInstance()->moveBundle(sourceBundle, newDir, true);
		});
	}
}

void QueueFrame::handleRemoveBundles(BundleList bundles, bool removeFinished) {
	if (bundles.empty())
		return;

	string tmp;

	bool allFinished = all_of(bundles.begin(), bundles.end(), [](const BundlePtr& b) { return b->isFinished(); });
	if (bundles.size() == 1) {
		if (!allFinished && !WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_QUEUE_REMOVAL, TSTRING_F(CONFIRM_REMOVE_DIR_BUNDLE, Text::toT(bundles.front()->getName())))) {
			return;
		} else if (removeFinished) {
			if (!WinUtil::showQuestionBox(TSTRING_F(CONFIRM_REMOVE_DIR_FINISHED, Text::toT(bundles.front()->getName())), MB_ICONQUESTION)) {
				return;
			}
		}
	} else {
		if (!allFinished && !WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_QUEUE_REMOVAL, TSTRING_F(CONFIRM_REMOVE_DIR_MULTIPLE, bundles.size()))) {
			return;
		} else if (removeFinished) {
			if (!WinUtil::showQuestionBox(TSTRING_F(CONFIRM_REMOVE_DIR_FINISHED_MULTIPLE, bundles.size()), MB_ICONQUESTION)) {
				return;
			}
		}
	}

	MainFrame::getMainFrame()->addThreadedTask([=] {
		for (auto b : bundles)
			QueueManager::getInstance()->removeBundle(b, false, removeFinished);
	});
}

void QueueFrame::handleRemoveFiles(QueueItemList queueitems) {
	if (queueitems.size() >= 1) {
		if (WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_QUEUE_REMOVAL, TSTRING(REALLY_REMOVE))) {
			for (auto& qi : queueitems)
				QueueManager::getInstance()->removeFile(qi->getTarget());
		}
	}
}

void QueueFrame::handleSearchQI(const QueueItemPtr& aQI) {
	if (aQI)
		WinUtil::searchHash(aQI->getTTH(), Util::getFileName(aQI->getTarget()), aQI->getSize());
}

void QueueFrame::onRenameBundle(BundlePtr b) {
	LineDlg dlg;
	dlg.title = TSTRING(RENAME);
	dlg.description = TSTRING(NEW_NAME);
	dlg.line = Text::toT(b->getName());
	if (dlg.DoModal(m_hWnd) == IDOK) {
		auto newName = Util::validatePath(Text::fromT(dlg.line), !b->isFileBundle());
		if (newName == b->getName()) {
			return;
		}

		MainFrame::getMainFrame()->addThreadedTask([=] {
			QueueManager::getInstance()->renameBundle(b, newName);
		});
	}
}

void QueueFrame::onBundleAdded(const BundlePtr& aBundle) {
	auto i = itemInfos.find(const_cast<string*>(&aBundle->getToken()));
	if (i == itemInfos.end()) {
		auto item = new QueueItemInfo(aBundle);
		itemInfos.emplace(const_cast<string*>(&aBundle->getToken()), item);
		ctrlQueue.insertGroupedItem(item, false, item->getGroupID(), !aBundle->isFileBundle()); // file bundles wont be having any children.
	}
}

void QueueFrame::AddBundleQueueItems(const BundlePtr& aBundle) {
	for (auto& qi : aBundle->getQueueItems()){
		if (qi->isFinished()/* && !SETTING(KEEP_FINISHED_FILES)*/)
			continue;
		auto item = new QueueItemInfo(qi);
		itemInfos.emplace(const_cast<string*>(&qi->getTarget()), item);
		ctrlQueue.insertGroupedItem(item, true, item->getGroupID());
	}
}

void QueueFrame::onBundleRemoved(const BundlePtr& aBundle) {
	auto i = itemInfos.find(const_cast<string*>(&aBundle->getToken()));
	if (i != itemInfos.end()) {
		ctrlQueue.removeGroupedItem(i->second, true, i->second->getGroupID()); //also deletes item info
		itemInfos.erase(i);
	}
}

void QueueFrame::onBundleUpdated(const BundlePtr& aBundle) {
	auto i = itemInfos.find(const_cast<string*>(&aBundle->getToken()));
	if (i != itemInfos.end()) {
		int pos = ctrlQueue.findItem(i->second);
		if (pos != -1) {
			ctrlQueue.updateItem(pos);
			if (aBundle->getQueueItems().empty())  //remove the + icon we have nothing to expand.
				ctrlQueue.SetItemState(pos, INDEXTOSTATEIMAGEMASK(0), LVIS_STATEIMAGEMASK);
		}
	}
}

void QueueFrame::onQueueItemRemoved(const QueueItemPtr& aQI) {
	auto i = itemInfos.find(const_cast<string*>(&aQI->getTarget()));
	if (i != itemInfos.end()) {
		ctrlQueue.removeGroupedItem(i->second, true, i->second->getGroupID()); //also deletes item info
		itemInfos.erase(i);
	}
}

void QueueFrame::onQueueItemUpdated(const QueueItemPtr& aQI) {
	auto i = itemInfos.find(const_cast<string*>(&aQI->getTarget()));
	if (i != itemInfos.end()) {
		if (!i->second->parent || !i->second->parent->collapsed) // no need to update if its collapsed right?
			ctrlQueue.updateItem(i->second);
	}
}

void QueueFrame::onQueueItemAdded(const QueueItemPtr& aQI) {
	auto i = itemInfos.find(const_cast<string*>(&aQI->getTarget()));
	if (i == itemInfos.end()) {
		//queueItem not found, look if we have a parent for it and if its expanded
		if (aQI->getBundle()){
			auto parent = itemInfos.find(const_cast<string*>(&aQI->getBundle()->getToken()));
			if ((parent == itemInfos.end()) || parent->second->collapsed)
				return;
		}
		auto item = new QueueItemInfo(aQI);
		itemInfos.emplace(const_cast<string*>(&aQI->getTarget()), item);
		ctrlQueue.insertGroupedItem(item, false, item->getGroupID());
	}
}

void QueueFrame::executeGuiTasks() {
	if (tasks.getTasks().empty())
		return;

	ctrlQueue.SetRedraw(FALSE);
	for (;;) {
		TaskQueue::TaskPair t;
		if (!tasks.getFront(t))
			break;
		static_cast<AsyncTask*>(t.second)->f();
		tasks.pop_front();
	}
	statusDirty = true;
	ctrlQueue.SetRedraw(TRUE);
}

void QueueFrame::updateStatus() {
	if (ctrlStatus.IsWindow() && statusDirty) {
		bool u = false;
		int bundles = 0;
		auto qm = QueueManager::getInstance();
		{
			RLock l(qm->getCS());
			bundles = qm->getBundles().size();
		}

		tstring tmp = TSTRING(QUEUED_BUNDLES) + _T(": ") + Util::toStringW(bundles);
		int w = WinUtil::getTextWidth(tmp, ctrlStatus.m_hWnd);
		if (statusSizes[3] < w) {
			statusSizes[3] = w;
			u = true;
		}
		ctrlStatus.SetText(4, (tmp).c_str());

		tmp = TSTRING(QUEUE_SIZE) + _T(": ") + Util::formatBytesW(qm->getTotalQueueSize());
		w = WinUtil::getTextWidth(tmp, ctrlStatus.m_hWnd);
		if (statusSizes[4] < w) {
			statusSizes[4] = w;
			u = true;
		}
		ctrlStatus.SetText(5, (tmp).c_str());
		
		if (u)
			UpdateLayout(TRUE);

		statusDirty = false;
	}
}

void QueueFrame::on(QueueManagerListener::BundleAdded, const BundlePtr& aBundle) noexcept {
	addGuiTask([=] { onBundleAdded(aBundle); });
}
void QueueFrame::on(QueueManagerListener::BundleRemoved, const BundlePtr& aBundle) noexcept{
	/*TODO: Think of something for this, when bundle is removed its possible that the queueitems get removed from the bundle before we loop them in gui thread,
	so we do it here one by one...*/
	for (auto& q : aBundle->getQueueItems())
		addGuiTask([=] { onQueueItemRemoved(q); });

	addGuiTask([=] { onBundleRemoved(aBundle); });
}
void QueueFrame::on(QueueManagerListener::BundleMoved, const BundlePtr& aBundle) noexcept {
	addGuiTask([=] { onBundleRemoved(aBundle); });
}
void QueueFrame::on(QueueManagerListener::BundleMerged, const BundlePtr& aBundle, const string&) noexcept { 
	addGuiTask([=] { onBundleUpdated(aBundle); });
}
void QueueFrame::on(QueueManagerListener::BundleSize, const BundlePtr& aBundle) noexcept { 
	addGuiTask([=] { onBundleUpdated(aBundle); });
}
void QueueFrame::on(QueueManagerListener::BundlePriority, const BundlePtr& aBundle) noexcept { 
	addGuiTask([=] { onBundleUpdated(aBundle); });
}
void QueueFrame::on(QueueManagerListener::BundleStatusChanged, const BundlePtr& aBundle) noexcept { 
	addGuiTask([=] { onBundleUpdated(aBundle); });
}
void QueueFrame::on(QueueManagerListener::BundleSources, const BundlePtr& aBundle) noexcept { 
	addGuiTask([=] { onBundleUpdated(aBundle); });
}

void QueueFrame::on(QueueManagerListener::Removed, const QueueItemPtr& aQI, bool /*finished*/) noexcept{
	addGuiTask([=] { onQueueItemRemoved(aQI); });
}
void QueueFrame::on(QueueManagerListener::Added, QueueItemPtr& aQI) noexcept{
	addGuiTask([=] { onQueueItemAdded(aQI); });
}
void QueueFrame::on(QueueManagerListener::SourcesUpdated, const QueueItemPtr& aQI) noexcept {
	addGuiTask([=] { onQueueItemUpdated(aQI); });
}
void QueueFrame::on(QueueManagerListener::StatusUpdated, const QueueItemPtr& aQI) noexcept{
	addGuiTask([=] { onQueueItemUpdated(aQI); });
}

void QueueFrame::on(DownloadManagerListener::BundleTick, const BundleList& tickBundles, uint64_t /*aTick*/) noexcept{
	for (auto& b : tickBundles) {
		addGuiTask([=] { onBundleUpdated(b); });
	}
}

/*QueueItemInfo functions*/

int QueueFrame::QueueItemInfo::getImageIndex() const {
	if (bundle)
		return bundle->isFileBundle() ? ResourceLoader::getIconIndex(Text::toT(bundle->getTarget())) : ResourceLoader::DIR_NORMAL;
	else 
		return ResourceLoader::getIconIndex(Text::toT(qi->getTarget()));
}

const tstring QueueFrame::QueueItemInfo::getText(int col) const {

	switch (col) {
		case COLUMN_NAME: return getName();
		case COLUMN_SIZE: return (getSize() != -1) ? Util::formatBytesW(getSize()) : TSTRING(UNKNOWN);
		case COLUMN_STATUS: return getStatusString();
		case COLUMN_DOWNLOADED: return (getSize() > 0) ? Util::formatBytesW(getDownloadedBytes()) + _T(" (") + Util::toStringW((double)getDownloadedBytes()*100.0 / (double)getSize()) + _T("%)") : Util::emptyStringT;
		case COLUMN_PRIORITY: 	
		{
			if (getPriority() == -1)
				return Util::emptyStringT;
			bool autoPrio = (bundle && bundle->getAutoPriority()) || (qi && qi->getAutoPriority());
			return Text::toT(AirUtil::getPrioText(getPriority())) + (autoPrio ? _T(" (") + TSTRING(AUTO) + _T(")") : Util::emptyStringT);
		}
		case COLUMN_SOURCES: //yeah, useless now if we show sources in status column...
		{
			RLock l(QueueManager::getInstance()->getCS());
			int sources = bundle ? bundle->getSources().size() : qi ? qi->getSources().size() : 0;
			return Util::toStringW(sources) + _T(" source(s)");
		}
		case COLUMN_PATH: return bundle ? Text::toT(bundle->getTarget()) : qi ? Text::toT(qi->getTarget()) : Util::emptyStringT;
		
		default: return Util::emptyStringT;
	}
}

tstring QueueFrame::QueueItemInfo::getName() const {
	if (bundle)
		return Text::toT(bundle->getName());
	else if (qi) {
		//show files in subdirectories as subdir/file.ext
		string path = qi->getTarget();
		if (qi->getBundle())
			return Text::toT(path.substr(qi->getBundle()->getTarget().size(), qi->getTarget().size()));
		else
			return Text::toT(qi->getTargetFileName());
	}
	return Util::emptyStringT;
}

int64_t QueueFrame::QueueItemInfo::getSize() const {
	return bundle ? bundle->getSize() : qi ? qi->getSize() : -1;
}

int QueueFrame::QueueItemInfo::getPriority() const {
	return  bundle ? bundle->getPriority() : qi ? qi->getPriority() : -1;
}

tstring QueueFrame::QueueItemInfo::getStatusString() const {
	//Yeah, think about these a little more...
	if (bundle) {
		if (bundle->isPausedPrio()) 
			return TSTRING(PAUSED);
	
		switch (bundle->getStatus()) {
		case Bundle::STATUS_NEW:
		case Bundle::STATUS_QUEUED: {
			int online = 0;
			Bundle::SourceList sources = QueueManager::getInstance()->getBundleSources(bundle);
			for (const auto& s : sources) {
				if (s.user.user->isOnline())
					online++;
			}

			auto size = sources.size();
			if (bundle->getSpeed() > 0) { // Bundle->isRunning() ?
				return (size == 1) ? TSTRING(USER_ONLINE_RUNNING) : TSTRING_F(USERS_ONLINE, online % size);
			} else {
				if (online > 0) {
					return (size == 1) ? TSTRING(WAITING_USER_ONLINE) : TSTRING_F(WAITING_USERS_ONLINE, online % size);
				} else {
					if (size == 0) {
						return TSTRING(NO_USERS_TO_DOWNLOAD_FROM);
					} else if (size == 1) {
						return TSTRING(USER_OFFLINE);
					}else if (size == 2) {
						return TSTRING(BOTH_USERS_OFFLINE);
					} else {
						return TSTRING_F(ALL_USERS_OFFLINE, size);
					}
				}
			}
		}
		case Bundle::STATUS_DOWNLOADED:
		case Bundle::STATUS_MOVED: return TSTRING(DOWNLOADED);
		case Bundle::STATUS_FAILED_MISSING:
		case Bundle::STATUS_SHARING_FAILED: return _T("Sharing failed");
		case Bundle::STATUS_FINISHED: return _T("Finished");
		case Bundle::STATUS_HASHING: return _T("Hashing...");
		case Bundle::STATUS_HASH_FAILED: return _T("Hash failed");
		case Bundle::STATUS_HASHED: return TSTRING(HASHING_FINISHED_TOTAL_PLAIN);
		case Bundle::STATUS_SHARED: return TSTRING(SHARED);
		default:
			return Util::emptyStringT;
		}
	} else if(qi) {
		
		if (qi->isPausedPrio()) 
			return TSTRING(PAUSED);

		if (qi->isFinished())
			return TSTRING(DOWNLOAD_FINISHED_IDLE);

		int online = 0;
		QueueItem::SourceList sources = QueueManager::getInstance()->getSources(qi);
		for (const auto& s : sources) {
			if (s.getUser().user->isOnline())
				online++;
		}

		auto size = sources.size();
		 if (qi->isWaiting()) {
			if (online > 0) {
				return (size == 1) ? TSTRING(WAITING_USER_ONLINE) : TSTRING_F(WAITING_USERS_ONLINE, online % size);
			} else {
				if (size == 0) {
					return TSTRING(NO_USERS_TO_DOWNLOAD_FROM);
				} else if (size == 1) {
					return TSTRING(USER_OFFLINE);
				} else if (size == 2) {
					return TSTRING(BOTH_USERS_OFFLINE);
				} else {
					return TSTRING_F(ALL_USERS_OFFLINE, size);
				}
			}
		} else {
			return (size == 1) ? TSTRING(USER_ONLINE_RUNNING) : TSTRING_F(USERS_ONLINE, online % size);
		} 
	}
	
	return Util::emptyStringT;
}

int64_t QueueFrame::QueueItemInfo::getDownloadedBytes() const {
	return bundle ? bundle->getDownloadedBytes() : qi ? qi->getDownloadedBytes() : 0;
}

int QueueFrame::QueueItemInfo::compareItems(const QueueItemInfo* a, const QueueItemInfo* b, int col) {
	switch (col) {
	case COLUMN_SIZE: return compare(a->getSize(), b->getSize());
	case COLUMN_PRIORITY: return compare(static_cast<int>(a->getPriority()), static_cast<int>(b->getPriority()));
	case COLUMN_DOWNLOADED: return compare(a->getDownloadedBytes(), b->getDownloadedBytes());
	default: 
		return Util::DefaultSort(a->getText(col).c_str(), b->getText(col).c_str());
	}
}
