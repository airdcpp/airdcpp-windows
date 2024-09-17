/*
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(LIST_VIEW_ARROWS_H)
#define LIST_VIEW_ARROWS_H

#include <uxtheme.h>

#include <airdcpp/SettingsManager.h>

namespace wingui {
template<class T>
class ListViewArrows {
public:
	ListViewArrows() { }
	virtual ~ListViewArrows() { }

	typedef ListViewArrows<T> thisClass;

	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_DESTROY, onDestroy)
		MESSAGE_HANDLER(WM_SETTINGCHANGE, onSettingChange)
	END_MSG_MAP()

	void updateArrow() {
		T* pThis = (T*)this;
		
		CHeaderCtrl headerCtrl = pThis->GetHeader();
		const int itemCount = headerCtrl.GetItemCount();
		for(int i=0; i < itemCount; ++i){
			HDITEM item;
			item.mask = HDI_FORMAT;
			headerCtrl.GetItem(i, &item);
			item.mask = HDI_FORMAT;

			//clear the previous state
			item.fmt &=  ~(HDF_SORTUP | HDF_SORTDOWN);

			if( i == pThis->getSortColumn()){
				item.fmt |= (pThis->isAscending() ? HDF_SORTUP : HDF_SORTDOWN);
			}

			headerCtrl.SetItem(i, &item);
		}
	}

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		T* pThis = (T*)this;
		_Module.AddSettingChangeNotify(pThis->m_hWnd);

		if(SETTING(USE_EXPLORER_THEME)) {
			SetWindowTheme(pThis->m_hWnd, L"explorer", NULL);
		}

		bHandled = FALSE;
		return 0;
	}

	LRESULT onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		T* pThis = (T*)this;
		_Module.RemoveSettingChangeNotify(pThis->m_hWnd);

		DeleteObject(downArrow);
		DeleteObject(upArrow);

		bHandled = FALSE;
		return 0;
	}

	LRESULT onSettingChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) { 
		bHandled = FALSE;
		return 1;
	}
private:
	CBitmap upArrow;
	CBitmap downArrow;
};
}

#endif // !defined(LIST_VIEW_ARROWS_H)
