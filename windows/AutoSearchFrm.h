/*
 * Copyright (C) 2012-2018 AirDC++ Project
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


#if !defined(AUTOSEARCH_FRM_H)
#define AUTOSEARCH_FRM_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"

#include "TabbedDialog.h"
#include "AutoSearchItemSettings.h"
#include "Async.h"

#include <airdcpp/modules/AutoSearchManager.h>

#define AS_STATUS_MSG_MAP 17

class AutoSearchFrame : public MDITabChildWindowImpl<AutoSearchFrame>, public StaticFrame<AutoSearchFrame, ResourceManager::AUTO_SEARCH, IDC_AUTOSEARCH>,
	private AutoSearchManagerListener, private SettingsManagerListener, public Async<AutoSearchFrame>
{
public:
	
	typedef MDITabChildWindowImpl<AutoSearchFrame> baseClass;

	AutoSearchFrame() : loading(true), closed(false), ctrlStatusContainer(STATUSCLASSNAME, this, AS_STATUS_MSG_MAP) { }
	~AutoSearchFrame() { }

	DECLARE_FRAME_WND_CLASS_EX(_T("AutoSearchFrame"), IDR_AUTOSEARCH, 0, COLOR_3DFACE);
		
	BEGIN_MSG_MAP(AutoSearchFrame)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_TIMER, onTimer)
		COMMAND_ID_HANDLER(IDC_ADD, onAdd)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_CHANGE, onChange)
		COMMAND_ID_HANDLER(IDC_DUPLICATE, onDuplicate)
		COMMAND_ID_HANDLER(IDC_MANAGE_GROUPS, onManageGroups)
		NOTIFY_HANDLER(IDC_AUTOSEARCH, LVN_ITEMCHANGED, onItemChanged)
		NOTIFY_HANDLER(IDC_AUTOSEARCH, NM_DBLCLK, onDoubleClick)
		NOTIFY_HANDLER(IDC_AUTOSEARCH, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_AUTOSEARCH, NM_CUSTOMDRAW, onCustomDraw)
		NOTIFY_HANDLER(IDC_AUTOSEARCH, LVN_GETDISPINFO, ctrlAutoSearch.onGetDispInfo)
		NOTIFY_HANDLER(IDC_AUTOSEARCH, LVN_COLUMNCLICK, ctrlAutoSearch.onColumnClick)
		NOTIFY_HANDLER(IDC_AUTOSEARCH, LVN_GETINFOTIP, ctrlAutoSearch.onInfoTip)
		CHAIN_MSG_MAP(baseClass)
		ALT_MSG_MAP(AS_STATUS_MSG_MAP)
		NOTIFY_CODE_HANDLER(TTN_GETDISPINFO, onGetToolTip)
	END_MSG_MAP()

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	void updateStatus();
	LRESULT onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDuplicate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onManageGroups(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL & /*bHandled*/);
	LRESULT onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
	LRESULT onGetToolTip(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/);

	void handleSearch(bool onBackground);
	void handleState(bool disabled);
	void handleMoveToGroup(const string& aGroupName);

	void addItem(const AutoSearchPtr& as);
	void UpdateLayout(BOOL bResizeBars = TRUE);

	static string id;
private:
	class ItemInfo;
public:
	TypedListViewCtrl<ItemInfo, IDC_AUTOSEARCH, LVITEM_GROUPING>& getList() { return ctrlAutoSearch; }

private:
	enum {
		COLUMN_FIRST,
		COLUMN_VALUE = COLUMN_FIRST,
		COLUMN_TYPE,
		COLUMN_SEARCH_STATUS,
		COLUMN_LASTSEARCH,
		COLUMN_BUNDLES,
		COLUMN_ACTION,
		COLUMN_EXPIRATION,
		COLUMN_PATH,
		COLUMN_REMOVE,
		COLUMN_USERMATCH,
		COLUMN_ERROR,
		COLUMN_LAST
	};

	class ItemInfo {
	public:
		ItemInfo(const AutoSearchPtr& aAutosearch, bool aupdate = true) : asItem(aAutosearch) {
			if(aupdate)
				update(aAutosearch);
		}
		~ItemInfo() { }

		inline const tstring& getText(int col) const { return columns[col]; }

		static int compareItems(const ItemInfo* a, const ItemInfo* b, int col) {
			return Util::DefaultSort(a->columns[col].c_str(), b->columns[col].c_str());
		}

		int getImageIndex() const { return asItem->getStatus(); }

		//int getImage(int col) const;
		void update(const AutoSearchPtr& u);

		tstring formatSearchDate(const time_t aTime);

		GETSET(int, groupId, GroupId);
		AutoSearchPtr asItem;

		tstring columns[COLUMN_LAST];

	};

	static int columnSizes[COLUMN_LAST];
	static int columnIndexes[COLUMN_LAST];
	TypedListViewCtrl<ItemInfo, IDC_AUTOSEARCH, LVITEM_GROUPING> ctrlAutoSearch;

	void updateList();
	void addFromDialog(AutoSearchItemSettings& dlg);

	void save() {
		AutoSearchManager::getInstance()->save();
	}

	void addStatusText(const string& aText, uint8_t sev);
	deque<tstring> lastLinesList;
	tstring lastLines;

	void addListEntry(ItemInfo* ii);
	void updateItem(const AutoSearchPtr as);

	void removeItem(const AutoSearchPtr as);

	CButton ctrlAdd, ctrlRemove, ctrlChange, ctrlDuplicate, ctrlManageGroups;
	CStatusBarCtrl ctrlStatus;
	CToolTipCtrl ctrlTooltips;
	CContainedWindow ctrlStatusContainer;
	int statusSizes[4];
	bool closed;
	bool loading;
	std::unordered_map<ProfileToken, ItemInfo> itemInfos;

	void createPages(TabbedDialog& dlg, AutoSearchItemSettings& options);


	virtual void on(AutoSearchManagerListener::ItemRemoved, const AutoSearchPtr& aToken) noexcept;
	virtual void on(AutoSearchManagerListener::ItemAdded, const AutoSearchPtr& as) noexcept;
	virtual void on(AutoSearchManagerListener::ItemUpdated, const AutoSearchPtr& as, bool setDirty) noexcept;
	virtual void on(AutoSearchManagerListener::SearchForeground, const AutoSearchPtr& as, const string& searchString) noexcept;
	virtual void on(AutoSearchManagerListener::ItemSearched, const AutoSearchPtr&/* as*/, const string& aMsg) noexcept;
};
#endif // !defined(AUTOSEARCH_FRM_H)