/*
* Copyright (C) 2011-2021 AirDC++ Project
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

#include "Async.h"
#include "StaticFrame.h"
#include "FilteredListViewCtrl.h"
#include "RssFeedsPage.h"
#include "TabbedDialog.h"
#include "RssFilterPage.h"
#include "DownloadBaseHandler.h"

#include <airdcpp/modules/RSSManager.h>
#include <airdcpp/modules/AutoSearchManager.h>
#include <airdcpp/QueueManagerListener.h>
#include <airdcpp/AirUtil.h>
#include <airdcpp/TaskQueue.h>


#define RSS_STATUS_MSG_MAP 11

class RssInfoFrame : public MDITabChildWindowImpl<RssInfoFrame>, public StaticFrame<RssInfoFrame, ResourceManager::RSS_FEEDS, IDC_RSSFRAME>, 
	private RSSManagerListener, private QueueManagerListener, private AutoSearchManagerListener,
	public CSplitterImpl<RssInfoFrame>,  private Async<RssInfoFrame>, public DownloadBaseHandler<RssInfoFrame>
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
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		CHAIN_MSG_MAP(baseClass)
		CHAIN_MSG_MAP(CSplitterImpl<RssInfoFrame>)
		ALT_MSG_MAP(RSS_STATUS_MSG_MAP)
			COMMAND_ID_HANDLER(IDC_ADD, onConfig);
			COMMAND_ID_HANDLER(IDC_REMOVE, onConfig);
			COMMAND_ID_HANDLER(IDC_CHANGE, onConfig);
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onDoubleClickList(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onConfig(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSelChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /* bHandled */);
	LRESULT onTreeItemClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /* bHandled */);

	LRESULT OnFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlRss.list.SetFocus();
		return 0;
	}

	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE);
	
	void reloadList();
	size_t getTotalListItemCount() {
		return itemInfos.size();
	}
	void createColumns();

	/* DownloadBaseHandler functions */
	void handleDownload(const string& aTarget, Priority p, bool isRelease);


	static string id;
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
		COLUMN_NAME,
		COLUMN_LAST
	};

	class ItemInfo: boost::noncopyable {
	public:
		ItemInfo(const RSSDataPtr& aFeedData) : item(aFeedData) {
			isRelease = AirUtil::isRelease(aFeedData->getTitle());
			setDupe(isRelease ? AirUtil::checkAdcDirectoryDupe(aFeedData->getTitle(), 0) : DUPE_NONE);
			isAutosearchDupe = isRelease && AutoSearchManager::getInstance()->getSearchesByString(aFeedData->getTitle()) != AutoSearchList();
		}
		~ItemInfo() { }

		const tstring getText(int col) const;

		static int compareItems(const ItemInfo* a, const ItemInfo* b, int col) {
			return Util::DefaultSort(a->getText(col).c_str(), b->getText(col).c_str());
		}

		int getImageIndex() const { return isRelease ? 1 : 0; }

		RSSDataPtr item;
		GETSET(DupeType, dupe, Dupe);
		
		bool isAutosearchDupe = false;
		bool isRelease = false;
	};

	HTREEITEM addTreeItem(const HTREEITEM& parent, int img, const tstring& name, HTREEITEM insertAfter = TVI_SORT);

	TaskQueue tasks;
	void addGuiTask(Callback f) {
		tasks.add(0, unique_ptr<AsyncTask>(new AsyncTask(f)));
	}

	bool show(const ItemInfo* aItem);

	void clearData(const RSSPtr& aFeed);

	void onItemAdded(const RSSDataPtr& aData);
	void onItemRemoved(const RSSDataPtr& aData);

	void addFeed(const RSSPtr& aFeed);
	
	void handleOpenFolder();
	void add();
	void change();
	void remove();
	void openDialog(RSSPtr& aFeed);

	void updateDupeType(const string& aName);

	RSSPtr getSelectedFeed();

	vector<ItemInfo*> getSelectedListitems();

	//map feeds to tree item
	unordered_map<RSSPtr, HTREEITEM> feeds;

	HTREEITEM treeParent;
	HTREEITEM curItem;
	int curSel;

	//RSSdata
	vector<unique_ptr<ItemInfo>> itemInfos;

	static int columnSizes[COLUMN_LAST];
	static int columnIndexes[COLUMN_LAST];

	CStatusBarCtrl ctrlStatus;
	int statusSizes[2];
	CContainedWindow ctrlStatusContainer;

	CButton ctrlConfig;
	CImageList treeImages;
	CImageList listImages;

	CButton ctrlAdd;
	CButton ctrlRemove;
	CButton ctrlChange;

	bool closed = false;

	virtual void on(RSSManagerListener::RSSDataAdded, const RSSDataPtr& aData) noexcept;
	virtual void on(RSSManagerListener::RSSDataRemoved, const RSSDataPtr& aData) noexcept;
	virtual void on(RSSManagerListener::RSSFeedRemoved, const RSSPtr& aFeed) noexcept;
	virtual void on(RSSManagerListener::RSSFeedChanged, const RSSPtr& aFeed) noexcept;
	virtual void on(RSSManagerListener::RSSFeedAdded, const RSSPtr& aFeed) noexcept;
	virtual void on(RSSManagerListener::RSSDataCleared, const RSSPtr& aFeed) noexcept;

	//test this to detect dupe type changes...
	virtual void on(QueueManagerListener::BundleStatusChanged, const BundlePtr& aBundle) noexcept;
	virtual void on(QueueManagerListener::BundleRemoved, const BundlePtr& aBundle) noexcept;
	virtual void on(QueueManagerListener::BundleAdded, const BundlePtr& aBundle) noexcept;
	virtual void on(AutoSearchManagerListener::ItemRemoved, const AutoSearchPtr& as) noexcept;
	virtual void on(AutoSearchManagerListener::ItemAdded, const AutoSearchPtr& as) noexcept;

};

#endif //