/*
* Copyright (C) 2011-2016 AirDC++ Project
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

#if !defined(RSS_INFO_FRAME_H)
#define RSS_INFO_FRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatTabCtrl.h"
#include "WinUtil.h"
#include "FilteredListViewCtrl.h"
#include "RssConfigDlg.h"
#include <airdcpp/RSSManager.h>
#include <airdcpp/AirUtil.h>
#include <airdcpp/TaskQueue.h>


#define RSS_STATUS_MSG_MAP 11

class RssInfoFrame : public MDITabChildWindowImpl<RssInfoFrame>, public StaticFrame<RssInfoFrame, ResourceManager::RSSINFO, IDC_RSSFRAME>, 
	private RSSManagerListener, public CSplitterImpl<RssInfoFrame>,  private Async<RssInfoFrame>
{
public:

	DECLARE_FRAME_WND_CLASS_EX(_T("RssInfoFrame"), IDR_RSSINFO, 0, COLOR_3DFACE);

	RssInfoFrame();
	~RssInfoFrame() { }

	typedef MDITabChildWindowImpl<RssInfoFrame> baseClass;

	BEGIN_MSG_MAP(RssInfoFrame)
		NOTIFY_HANDLER(IDC_RSS_LIST, LVN_GETDISPINFO, ctrlRss.list.onGetDispInfo)
		NOTIFY_HANDLER(IDC_RSS_LIST, LVN_COLUMNCLICK, ctrlRss.list.onColumnClick)
		NOTIFY_HANDLER(IDC_RSS_LIST, LVN_GETINFOTIP, ctrlRss.list.onInfoTip)
		NOTIFY_HANDLER(IDC_RSS_LIST, NM_CUSTOMDRAW, onCustomDraw)
		NOTIFY_HANDLER(IDC_RSS_LIST, NM_DBLCLK, onDoubleClickList)
		NOTIFY_HANDLER(IDC_RSS_TREE, TVN_SELCHANGED, onSelChanged)
		NOTIFY_HANDLER(IDC_RSS_TREE, NM_CLICK, onTreeItemClick)
		NOTIFY_HANDLER(IDC_RSS_TREE, NM_RCLICK, onTreeItemClick)
		MESSAGE_HANDLER(WM_SETFOCUS, OnFocus)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_TIMER, onTimer)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		CHAIN_MSG_MAP(baseClass)
		CHAIN_MSG_MAP(CSplitterImpl<RssInfoFrame>)
		ALT_MSG_MAP(RSS_STATUS_MSG_MAP)
			COMMAND_ID_HANDLER(IDC_RSS_UPDATE, onConfig);
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onDoubleClickList(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onConfig(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		RssDlg dlg;
		dlg.DoModal();
		return 0;
	}


	LRESULT onSelChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /* bHandled */) {
		NMTREEVIEW* nmtv = (NMTREEVIEW*)pnmh;
		if (nmtv->itemNew.lParam != -1) {
			curSel = nmtv->itemNew.lParam;
			curItem = nmtv->itemNew.hItem;
			reloadList();
		}

		return 0;
	}

	LRESULT onTreeItemClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /* bHandled */) {
		DWORD dwPos = ::GetMessagePos();
		POINT pt;
		pt.x = GET_X_LPARAM(dwPos);
		pt.y = GET_Y_LPARAM(dwPos);

		ctrlTree.ScreenToClient(&pt);

		UINT uFlags;
		HTREEITEM ht = ctrlTree.HitTest(pt, &uFlags);
		if (ht && ht == curItem)
			reloadList();

		return 0;
	}

	LRESULT OnFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlRss.list.SetFocus();
		return 0;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE);
	
	void reloadList();
	size_t getTotalListItemCount() {
		return ItemInfos.size();
	}
	void createColumns();

private:

	class ItemInfo;
	typedef TypedListViewCtrl<ItemInfo, IDC_RSS_LIST> ListBaseType;
	typedef FilteredListViewCtrl <ListBaseType, RssInfoFrame, IDC_RSS_LIST, FLV_HAS_OPTIONS> ListType;
	friend class ListType;
	ListType ctrlRss;

	CTreeViewCtrl ctrlTree;

	enum {
		COLUMN_FIRST,
		COLUMN_FILE = COLUMN_FIRST,
		COLUMN_LINK,
		COLUMN_DATE,
		COLUMN_CATEGORIE,
		COLUMN_LAST
	};

	class ItemInfo {
	public:
		ItemInfo(const RSSDataPtr& aFeedData) : item(aFeedData) {
			setDupe(AirUtil::checkDirDupe(item->getTitle(), 0));
		}
		~ItemInfo() { }

		const tstring getText(int col) const;

		static int compareItems(const ItemInfo* a, const ItemInfo* b, int col) {
			return Util::DefaultSort(a->getText(col).c_str(), b->getText(col).c_str());
		}

		int getImageIndex() const { return -1; }

		RSSDataPtr item;
		GETSET(DupeType, dupe, Dupe);
	};


	HTREEITEM addTreeItem(const HTREEITEM& parent, int item, const tstring& name, HTREEITEM insertAfter = TVI_SORT) {
		TVINSERTSTRUCT tvis = { 0 };
		tvis.hParent = parent;
		tvis.hInsertAfter = insertAfter;
		tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
		tvis.item.pszText = (LPWSTR)name.c_str();
		tvis.item.iImage = item;
		tvis.item.iSelectedImage = item;
		tvis.item.lParam = item;
		return ctrlTree.InsertItem(&tvis);
	}

	TaskQueue tasks;
	void addGuiTask(std::function<void()> f) {
		tasks.add(0, unique_ptr<AsyncTask>(new AsyncTask(f)));
	}

	bool show(const ItemInfo* aItem);

	void clearData(const RSSPtr& aFeed);

	void onItemAdded(const RSSDataPtr& aData);

	void addCategory(const RSSPtr& aFeed);
	
	void handleOpenFolder();

	RSSPtr getSelectedCategory();
	ItemInfo* getSelectedListitem();

	//categories by name
	unordered_map<RSSPtr, HTREEITEM> categories;

	HTREEITEM treeParent;
	HTREEITEM curItem;
	int curSel;

	//RSSdata, itemInfos by title
	unordered_map<string, ItemInfo*> ItemInfos;

	static int columnSizes[COLUMN_LAST];
	static int columnIndexes[COLUMN_LAST];

	CStatusBarCtrl ctrlStatus;
	int statusSizes[2];
	CContainedWindow ctrlStatusContainer;

	CButton ctrlConfig;

	virtual void on(RSSManagerListener::RSSDataAdded, const RSSDataPtr& aData) noexcept;
	virtual void on(RSSManagerListener::RSSFeedRemoved, const RSSPtr& aFeed) noexcept;
	virtual void on(RSSManagerListener::RSSFeedChanged, const RSSPtr& aFeed) noexcept;
	virtual void on(RSSManagerListener::RSSFeedAdded, const RSSPtr& aFeed) noexcept;
	virtual void on(RSSManagerListener::RSSDataCleared, const RSSPtr& aFeed) noexcept;
};

#endif //