/*
 * Copyright (C) 2012-2013 AirDC++ Project
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
#include "ExListViewCtrl.h"

#include "AutoSearchDlg.h"
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
		COMMAND_ID_HANDLER(IDC_MOVE_UP, onMoveUp)
		COMMAND_ID_HANDLER(IDC_MOVE_DOWN, onMoveDown)
		NOTIFY_HANDLER(IDC_AUTOSEARCH, LVN_ITEMCHANGED, onItemChanged)
		NOTIFY_HANDLER(IDC_AUTOSEARCH, NM_DBLCLK, onDoubleClick)
		NOTIFY_HANDLER(IDC_AUTOSEARCH, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_AUTOSEARCH, NM_CUSTOMDRAW, onCustomDraw)
		COMMAND_HANDLER(IDC_AUTOSEARCH_ENABLE_TIME, EN_CHANGE, onAsTime)
		COMMAND_HANDLER(IDC_AUTOSEARCH_RECHECK_TIME, EN_KILLFOCUS, onAsRTime)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDuplicate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMoveUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMoveDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);

	void handleSearch(bool onBackground);
	void handleState(bool disabled);

	LRESULT onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/){
		NMITEMACTIVATE* asItem = (NMITEMACTIVATE*)pnmh;
		if(asItem->iItem >= 0) {
			PostMessage(WM_COMMAND, IDC_CHANGE, 0);
		} else if(asItem->iItem == -1) {
			PostMessage(WM_COMMAND, IDC_ADD, 0);
		}
		return 0;
	}

	LRESULT onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlAutoSearch.SetFocus();
		return 0;
	}

	LRESULT onAsTime(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
		if(loading)
			return 0;

		tstring val(ctrlAsTime.GetWindowTextLength() +2, _T('\0'));
		ctrlAsTime.GetWindowText(&val[0],val.size());
		int value = Util::toInt(Text::fromT(val));
		if(value < 1) {
			value = 1;
			ctrlAsTime.SetWindowText(Text::toT(Util::toString(value)).c_str());
		}
		SettingsManager::getInstance()->set(SettingsManager::AUTOSEARCH_EVERY, value);
		return 0;
	}
	LRESULT onAsRTime(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
		if(loading)
			return 0;

		tstring val(ctrlAsRTime.GetWindowTextLength() +2, _T('\0'));
		ctrlAsRTime.GetWindowText(&val[0],val.size());
		int value = Util::toInt(Text::fromT(val));
		if(value < 30) {
			value = 30;
			ctrlAsRTime.SetWindowText(Text::toT(Util::toString(value)).c_str());
		}
		SettingsManager::getInstance()->set(SettingsManager::AUTOSEARCH_RECHECK_TIME, value);
		return 0;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE);

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

	static int columnSizes[COLUMN_LAST];
	static int columnIndexes[COLUMN_LAST];
	ExListViewCtrl ctrlAutoSearch;

	void appendDialogParams(const AutoSearchPtr& as, AutoSearchDlg& dlg);
	void setItemProperties(AutoSearchPtr& as, const AutoSearchDlg& dlg, const string& aSearchString);

	void updateList() {

		ctrlAutoSearch.SetRedraw(FALSE);
		
		AutoSearchList lst = AutoSearchManager::getInstance()->getSearchItems();
		
		for(auto i = lst.begin(); i != lst.end(); ++i) {
			const AutoSearchPtr as = *i;	
			addEntry(as, ctrlAutoSearch.GetItemCount());
		}

		ctrlAutoSearch.SetRedraw(TRUE);
		ctrlAutoSearch.Invalidate();
	}

	void addFromDialog(const AutoSearchDlg& dlg);

	void save() {
		AutoSearchManager::getInstance()->AutoSearchSave();
	}

	void addEntry(const AutoSearchPtr as, int pos);
	void updateItem(const AutoSearchPtr as);
	tstring formatSearchDate(const time_t aTime);

	int findItem(ProfileToken aToken);
	void removeItem(const AutoSearchPtr as);

	CButton ctrlAdd, ctrlRemove, ctrlChange, ctrlDown, ctrlUp, ctrlDuplicate;
	CEdit ctrlAsTime;
	CStatic ctrlAsTimeLabel;
	CUpDownCtrl Timespin;

	CEdit ctrlAsRTime;
	CStatic ctrlAsRTimeLabel;
	CUpDownCtrl RTimespin;

	bool closed;
	bool loading;

	virtual void on(AutoSearchManagerListener::RemoveItem, const AutoSearchPtr& aToken) noexcept;
	virtual void on(AutoSearchManagerListener::AddItem, const AutoSearchPtr& as) noexcept;
	virtual void on(AutoSearchManagerListener::UpdateItem, const AutoSearchPtr& as, bool setDirty) noexcept;
	virtual void on(AutoSearchManagerListener::SearchForeground, const AutoSearchPtr& as, const string& searchString) noexcept;
};
#endif // !defined(AUTOSEARCH_FRM_H)