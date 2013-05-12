/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(LIST_VIEW_ARROWS_H)
#define LIST_VIEW_ARROWS_H

#include "WinUtil.h"

#include <uxtheme.h>

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

	void rebuildArrows()
	{
		//these arrows aren't used anyway so no need to create them
		if(WinUtil::comCtlVersion >= MAKELONG(0,6)){
			return;
		}

		POINT pathArrowLong[9] = {{0L,7L},{7L,7L},{7L,6L},{6L,6L},{6L,4L},{5L,4L},{5L,2L},{4L,2L},{4L,0L}};
		POINT pathArrowShort[7] = {{0L,6L},{1L,6L},{1L,4L},{2L,4L},{2L,2L},{3L,2L},{3L,0L}};

		CDC dc;
		CBrushHandle brush;
		CPen penLight;
		CPen penShadow;

		HPEN oldPen;
		HBITMAP oldBitmap;

		const int bitmapWidth = 8;
		const int bitmapHeight = 8;
		const RECT rect = {0, 0, bitmapWidth, bitmapHeight};

		T* pThis = (T*)this;

		if(!dc.CreateCompatibleDC(pThis->GetDC()))
			return;

		if(!brush.CreateSysColorBrush(COLOR_3DFACE))
			return;

		if(!penLight.CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_3DHIGHLIGHT)))
			return;

		if(!penShadow.CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_3DSHADOW)))
			return;

		if (upArrow.IsNull())
			upArrow.CreateCompatibleBitmap(pThis->GetDC(), bitmapWidth, bitmapHeight);

		if (downArrow.IsNull())
			downArrow.CreateCompatibleBitmap(pThis->GetDC(), bitmapWidth, bitmapHeight);

		// create up arrow
		oldBitmap = dc.SelectBitmap(upArrow);
		dc.FillRect(&rect, brush);
		oldPen = dc.SelectPen(penLight);
		dc.Polyline(pathArrowLong, sizeof(pathArrowLong)/sizeof(pathArrowLong[0]));
		dc.SelectPen(penShadow);
		dc.Polyline(pathArrowShort, sizeof(pathArrowShort)/sizeof(pathArrowShort[0]));

		// create down arrow
		dc.SelectBitmap(downArrow);
		dc.FillRect(&rect, brush);
		for (int i=0; i < sizeof(pathArrowShort)/sizeof(pathArrowShort[0]); ++i)
		{
			POINT& pt = pathArrowShort[i];
			pt.x = bitmapWidth - pt.x;
			pt.y = bitmapHeight - pt.y;
		}
		dc.SelectPen(penLight);
		dc.Polyline(pathArrowShort, sizeof(pathArrowShort)/sizeof(pathArrowShort[0]));
		for (int i=0; i < sizeof(pathArrowLong)/sizeof(pathArrowLong[0]); ++i)
		{
			POINT& pt = pathArrowLong[i];
			pt.x = bitmapWidth - pt.x;
			pt.y = bitmapHeight - pt.y;
		}
		dc.SelectPen(penShadow);
		dc.Polyline(pathArrowLong, sizeof(pathArrowLong)/sizeof(pathArrowLong[0]));

		dc.SelectPen(oldPen);
		dc.SelectBitmap(oldBitmap);
		DeleteObject(penShadow);
		DeleteObject(penLight);
		DeleteObject(brush);
	}

	void updateArrow() {
		T* pThis = (T*)this;
		
		CHeaderCtrl headerCtrl = pThis->GetHeader();
		const int itemCount = headerCtrl.GetItemCount();
		
		if(WinUtil::comCtlVersion >= MAKELONG(0,6)){
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
		} else {
			if (upArrow.IsNull())
				return;

			HBITMAP bitmap = (pThis->isAscending() ? upArrow : downArrow);

			for (int i=0; i < itemCount; ++i)
			{
				HDITEM item;
				item.mask = HDI_FORMAT;
				headerCtrl.GetItem(i, &item);
				item.mask = HDI_FORMAT | HDI_BITMAP;
				if (i == pThis->getSortColumn()) {
					item.fmt |= HDF_BITMAP | HDF_BITMAP_ON_RIGHT;
					item.hbm = bitmap;
				} else {
					item.fmt &= ~(HDF_BITMAP | HDF_BITMAP_ON_RIGHT);
					item.hbm = 0;
				}
				headerCtrl.SetItem(i, &item);
			}
		}
	}

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		rebuildArrows();
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
		rebuildArrows();
		bHandled = FALSE;
		return 1;
	}
private:
	CBitmap upArrow;
	CBitmap downArrow;
};

#endif // !defined(LIST_VIEW_ARROWS_H)
