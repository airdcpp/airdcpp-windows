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


#if !defined(QUEUE_FRAME2_H)
#define QUEUE_FRAME2_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"

#include "../client/QueueManager.h"


class QueueFrame2 : public MDITabChildWindowImpl<QueueFrame2>, public StaticFrame<QueueFrame2, ResourceManager::DOWNLOAD_QUEUE, IDC_QUEUE2>,
	private QueueManagerListener, private DownloadManagerListener, private Async<QueueFrame2>
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("QueueFrame2"), IDR_QUEUE2, 0, COLOR_3DFACE);

	QueueFrame2() : closed(false) {}

	~QueueFrame2() {}

	typedef MDITabChildWindowImpl<QueueFrame2> baseClass;

	BEGIN_MSG_MAP(QueueFrame2)
		NOTIFY_HANDLER(IDC_QUEUE_LIST, LVN_GETDISPINFO, ctrlQueue.onGetDispInfo)
		NOTIFY_HANDLER(IDC_QUEUE_LIST, LVN_COLUMNCLICK, ctrlQueue.onColumnClick)
		NOTIFY_HANDLER(IDC_QUEUE_LIST, LVN_GETINFOTIP, ctrlQueue.onInfoTip)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, onLButton)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_REMOVE_OFFLINE, onRemoveOffline)
		COMMAND_ID_HANDLER(IDC_READD_ALL, onReaddAll)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);

	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onReaddAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemoveOffline(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) {
		ctrlQueue.SetFocus();
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

	LRESULT onLButton(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

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
				if (i->parent == NULL) {
					if (i->collapsed) {
						//insert the children at first expand, collect some garbage.
						if (ctrlQueue.findChildren(i->bundle->getToken()).empty()) {
							AddBundleQueueItems(i->bundle);
							ctrlQueue.resort();
						} else {
							ctrlQueue.Expand(i, pos);
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


	void UpdateLayout(BOOL bResizeBars = TRUE );

private:
	class QueueItemInfo;
	enum {
		COLUMN_FIRST,
		COLUMN_NAME = COLUMN_FIRST,
		COLUMN_SIZE,
		COLUMN_PRIORITY,
		COLUMN_DOWNLOADED,
		COLUMN_SOURCES,
		COLUMN_PATH,
		COLUMN_LAST
	};

	/*
	Currently its treating every QueueItemInfo as a bundle or Qi and information is called directly, we might want to cache the needed infos.
	*/
	class QueueItemInfo : public FastAlloc<QueueItemInfo>, boost::noncopyable {
	public:
		/*
		Constructors with bundle and QueueItem, even if we might want to change to caching information of the items, 
		these 2 basic constructors could feed the correct information.
		*/
		QueueItemInfo(const BundlePtr& aBundle) : bundle(aBundle), qi(nullptr), collapsed(true), parent(NULL), hits(-1) {}
		QueueItemInfo(const QueueItemPtr& aQi) : bundle(nullptr), qi(aQi), collapsed(true), parent(NULL), hits(-1) {}
		
		~QueueItemInfo() {
			dcdebug("itemInfo destructed %s \r\n", bundle ? bundle->getName().c_str() : qi->getTargetFileName().c_str());
		}

		BundlePtr bundle;
		QueueItemPtr qi;

		QueueItemInfo* parent;
		bool collapsed;
		int16_t hits;

		inline const string& getGroupCond() const { 
			if (bundle)
				return bundle->getToken();
			else
				return qi->getBundle()->getToken();
		}

		QueueItemInfo* createParent() { return this; }

		const tstring getText(int col) const;
		static int compareItems(const QueueItemInfo* a, const QueueItemInfo* b, int col);
		int getImageIndex() const;
	};

	static int columnIndexes[COLUMN_LAST];
	static int columnSizes[COLUMN_LAST];

	void onRenameBundle(BundlePtr b);
	void onBundleAdded(const BundlePtr& aBundle);
	void onBundleRemoved(const BundlePtr& aBundle);
	void onBundleUpdated(const BundlePtr& aBundle);

	void AddBundleQueueItems(const BundlePtr& aBundle);

	void onQueueItemRemoved(const QueueItemPtr& aQI);
	void onQueueItemUpdated(const QueueItemPtr& aQI);
	void onQueueItemAdded(const QueueItemPtr& aQI);

	void AppendBundleMenu(BundleList& bl, OMenu& bundleMenu);
	void AppendQiMenu(QueueItemList& ql, OMenu& fileMenu);
	static tstring handleCopyMagnet(const QueueItemInfo* ii);

	bool closed;

	/*contains all ItemInfos, bundles mapped with token, queueItems with target, any point for adding Qi tokens??*/
	std::unordered_map<std::string, QueueItemInfo*> itemInfos;

	typedef TypedTreeListViewCtrl<QueueItemInfo, IDC_QUEUE_LIST, string, noCaseStringHash, noCaseStringEq, NO_GROUP_UNIQUE_CHILDREN | VIRTUAL_CHILDREN> ListType;
	ListType ctrlQueue;

	CStatusBarCtrl ctrlStatus;
	int statusSizes[6];

	//bundle update listeners
	void on(QueueManagerListener::BundleAdded, const BundlePtr& aBundle) noexcept;
	void on(QueueManagerListener::BundleRemoved, const BundlePtr& aBundle) noexcept;
	void on(QueueManagerListener::BundleMoved, const BundlePtr& aBundle) noexcept;
	void on(QueueManagerListener::BundleMerged, const BundlePtr& aBundle, const string&) noexcept;
	void on(QueueManagerListener::BundleSize, const BundlePtr& aBundle) noexcept;
	void on(QueueManagerListener::BundlePriority, const BundlePtr& aBundle) noexcept;
	void on(QueueManagerListener::BundleStatusChanged, const BundlePtr& aBundle) noexcept;
	void on(QueueManagerListener::BundleSources, const BundlePtr& aBundle) noexcept;

	void on(DownloadManagerListener::BundleTick, const BundleList& tickBundles, uint64_t aTick) noexcept;

	//QueueItem update listeners
	void on(QueueManagerListener::Removed, const QueueItemPtr& aQI, bool /*finished*/) noexcept;
	void on(QueueManagerListener::Added, QueueItemPtr& aQI) noexcept;
	void on(QueueManagerListener::SourcesUpdated, const QueueItemPtr& aQI) noexcept;
	void on(QueueManagerListener::StatusUpdated, const QueueItemPtr& aQI) noexcept;



};
#endif // !defined(QUEUE_FRAME_H)