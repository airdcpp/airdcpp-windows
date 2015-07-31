/*
 * Copyright (C) 2012-2015 AirDC++ Project
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

#include "AutoSearchOptionsDlg.h"
#include "AutoSearchItemSettings.h"
#include "Async.h"

#include "../client/AutoSearchManager.h"


class AutoSearchFrame : public MDITabChildWindowImpl<AutoSearchFrame>, public StaticFrame<AutoSearchFrame, ResourceManager::AUTO_SEARCH, IDC_AUTOSEARCH>,
	private AutoSearchManagerListener, private SettingsManagerListener, public Async<AutoSearchFrame>
{
public:
	
	typedef MDITabChildWindowImpl<AutoSearchFrame> baseClass;

	AutoSearchFrame() : loading(true), closed(false) { }
	~AutoSearchFrame() { }

	DECLARE_FRAME_WND_CLASS_EX(_T("AutoSearchFrame"), IDR_AUTOSEARCH, 0, COLOR_3DFACE);
		
	BEGIN_MSG_MAP(AutoSearchFrame)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		COMMAND_ID_HANDLER(IDC_ADD, onAdd)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_CHANGE, onChange)
		COMMAND_ID_HANDLER(IDC_DUPLICATE, onDuplicate)
		NOTIFY_HANDLER(IDC_AUTOSEARCH, LVN_ITEMCHANGED, onItemChanged)
		NOTIFY_HANDLER(IDC_AUTOSEARCH, NM_DBLCLK, onDoubleClick)
		NOTIFY_HANDLER(IDC_AUTOSEARCH, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_AUTOSEARCH, NM_CUSTOMDRAW, onCustomDraw)
		NOTIFY_HANDLER(IDC_AUTOSEARCH, LVN_GETDISPINFO, ctrlAutoSearch.onGetDispInfo)
		NOTIFY_HANDLER(IDC_AUTOSEARCH, LVN_COLUMNCLICK, ctrlAutoSearch.onColumnClick)
		NOTIFY_HANDLER(IDC_AUTOSEARCH, LVN_GETINFOTIP, ctrlAutoSearch.onInfoTip)
		COMMAND_HANDLER(IDC_AUTOSEARCH_ENABLE_TIME, EN_CHANGE, onAsTime)		
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDuplicate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL & /*bHandled*/);
	LRESULT onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
	LRESULT onAsTime(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/);
	void handleSearch(bool onBackground);
	void handleState(bool disabled);

	void UpdateLayout(BOOL bResizeBars = TRUE);

private:
	class ItemInfo;
public:
	TypedListViewCtrl<ItemInfo, IDC_AUTOSEARCH>& getList() { return ctrlAutoSearch; }

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
		COLUMN_REMOVE,
		COLUMN_PATH,
		COLUMN_USERMATCH,
		COLUMN_ERROR,
		COLUMN_LAST
	};

	class ItemInfo {
	public:
		ItemInfo(const AutoSearchPtr& aAutosearch) : asItem(aAutosearch) {
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

		AutoSearchPtr asItem;

		tstring columns[COLUMN_LAST];

	};

	static int columnSizes[COLUMN_LAST];
	static int columnIndexes[COLUMN_LAST];
	TypedListViewCtrl<ItemInfo, IDC_AUTOSEARCH> ctrlAutoSearch;

	void updateList();
	void addFromDialog(AutoSearchItemSettings& dlg);

	void save() {
		AutoSearchManager::getInstance()->AutoSearchSave();
	}

	void addEntry(const AutoSearchPtr as);
	void updateItem(const AutoSearchPtr as);

	void removeItem(const AutoSearchPtr as);

	CButton ctrlAdd, ctrlRemove, ctrlChange, ctrlDuplicate;
	CEdit ctrlAsTime;
	CStatic ctrlAsTimeLabel;
	CUpDownCtrl Timespin;
	bool closed;
	bool loading;
	std::unordered_map<ProfileToken, ItemInfo> itemInfos;


	virtual void on(AutoSearchManagerListener::RemoveItem, const AutoSearchPtr& aToken) noexcept;
	virtual void on(AutoSearchManagerListener::AddItem, const AutoSearchPtr& as) noexcept;
	virtual void on(AutoSearchManagerListener::UpdateItem, const AutoSearchPtr& as, bool setDirty) noexcept;
	virtual void on(AutoSearchManagerListener::SearchForeground, const AutoSearchPtr& as, const string& searchString) noexcept;
};
#endif // !defined(AUTOSEARCH_FRM_H)