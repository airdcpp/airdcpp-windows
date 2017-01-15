/* 
 * 
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

#if !defined(__RECENTS_FRAME_H__)
#define __RECENTS_FRAME_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatTabCtrl.h"
#include "FilteredListViewCtrl.h"

#include <airdcpp/RecentManager.h>

class RecentsFrame : public MDITabChildWindowImpl<RecentsFrame>, public StaticFrame<RecentsFrame, ResourceManager::RECENT_HUBS, IDC_RECENTS>, 
	private RecentManagerListener, private SettingsManagerListener, private Async<RecentsFrame>
{
public:
	typedef MDITabChildWindowImpl<RecentsFrame> baseClass;
		
	RecentsFrame();
	~RecentsFrame() { };

	DECLARE_FRAME_WND_CLASS_EX(_T("RecentsFrame"), IDR_RECENTS, 0, COLOR_3DFACE);
		

	BEGIN_MSG_MAP(RecentsFrame)
		NOTIFY_HANDLER(IDC_RECENTS_LIST, LVN_GETDISPINFO, ctrlList.list.onGetDispInfo)
		NOTIFY_HANDLER(IDC_RECENTS_LIST, LVN_COLUMNCLICK, ctrlList.list.onColumnClick)
		NOTIFY_HANDLER(IDC_RECENTS_LIST, LVN_GETINFOTIP, ctrlList.list.onInfoTip)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		NOTIFY_HANDLER(IDC_RECENTS, LVN_KEYDOWN, onKeyDown)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()
		
	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	void UpdateLayout(BOOL bResizeBars = TRUE);
	
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL & /*bHandled*/);
	LRESULT onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL & /*bHandled*/);

	void createColumns();
	size_t getTotalListItemCount() {
		return itemInfos.size();
	}

	static string id;
private:
	class ItemInfo;
	typedef TypedListViewCtrl<ItemInfo, IDC_RECENTS_LIST> ListBaseType;
	typedef FilteredListViewCtrl <ListBaseType, RecentsFrame, IDC_RECENTS_LIST, FLV_HAS_OPTIONS> ListType;
	friend class ListType;
	ListType ctrlList;


	class ItemInfo {
	public:
		ItemInfo(const RecentEntryPtr& aRecent) : item(aRecent) { }
		~ItemInfo() { }

		const tstring getText(int col) const;

		static int compareItems(const ItemInfo* a, const ItemInfo* b, int col) {
			return Util::DefaultSort(a->getText(col).c_str(), b->getText(col).c_str());
		}

		int getImageIndex() const { return 0; }

		RecentEntryPtr item;
	};

	unordered_map<string, unique_ptr<ItemInfo>> itemInfos; //map to url for now, we only have hubs here...

	enum {
		COLUMN_FIRST,
		COLUMN_NAME = COLUMN_FIRST,
		COLUMN_DESCRIPTION,
		COLUMN_SERVER,
		COLUMN_LAST
	};

	bool closed;
	
	static int columnSizes[COLUMN_LAST];
	static int columnIndexes[COLUMN_LAST];
	
	void updateList();
	void addEntry(ItemInfo* ii);

	CImageList listImages;

	CStatusBarCtrl ctrlStatus;
	int statusSizes[2];
	
	void on(RecentManagerListener::RecentAdded, const RecentEntryPtr& entry) noexcept { 
		callAsync([=] {
			auto i = itemInfos.find(entry->getUrl());
			if (i == itemInfos.end()) {
				auto ii = itemInfos.emplace(entry->getUrl(), unique_ptr<ItemInfo>(new ItemInfo(entry))).first;
				addEntry(ii->second.get());
			}
		});
	}
	void on(RecentManagerListener::RecentRemoved, const RecentEntryPtr& entry) noexcept { 
		callAsync([=] {
			auto i = itemInfos.find(entry->getUrl());
			if (i != itemInfos.end()) {
				ctrlList.list.deleteItem(i->second.get());
				itemInfos.erase(i);
			}
		});
	}
	void on(RecentManagerListener::RecentUpdated, const RecentEntryPtr& entry) noexcept;

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;
};

#endif