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

#if !defined(TYPED_LIST_VIEW_CTRL_H)
#define TYPED_LIST_VIEW_CTRL_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <airdcpp/settings/SettingsManager.h>
#include <airdcpp/util/text/StringTokenizer.h>
#include <airdcpp/util/Util.h>

#include <windows/components/ListViewArrows.h>
#include <windows/components/OMenu.h>
#include <windows/util/WinUtil.h>


#define NO_GROUP_UNIQUE_CHILDREN 0x01
#define VIRTUAL_CHILDREN 0x02 //parent handles item creation on expanding
#define LVITEM_GROUPING 0x08 //Items have groups

namespace wingui {
enum ColumnType{
	COLUMN_TEXT,
	COLUMN_SIZE,
	COLUMN_TIME,
	COLUMN_SPEED,
	COLUMN_NUMERIC_OTHER,
	COLUMN_IMAGE
};

class ColumnInfo {
public:
	static tstring filterCountry(const tstring& aText) {
		auto pos = aText.rfind(_T('('));
		return pos != string::npos ? aText.substr(pos + 1, aText.length() - pos - 2) : aText;
	}

	typedef function < tstring(const tstring&) > CopyF;

	struct ClickHandler {
		typedef function < bool(int) > ClickF;
		ClickHandler(ClickF aF, bool aDoubleClick) : f(aF), doubleClick(aDoubleClick) {}
		ClickHandler() {}

		ClickF f = nullptr;
		bool doubleClick = false;
	};

	ColumnInfo(const tstring &aName, int aPos, int aFormat, int aWidth, ColumnType aColType, ClickHandler aClickHandler = ClickHandler()) : name(aName), pos(aPos), width(aWidth),
		format(aFormat), visible(true), colType(aColType), clickHandler(std::move(aClickHandler)) {
	}
	~ColumnInfo() {}

	bool isNumericType() const { return colType != COLUMN_TEXT && colType != COLUMN_IMAGE; }

	tstring name;
	bool visible;
	int pos;
	int width;
	int format;
	ColumnType colType;
	ClickHandler clickHandler;
	CopyF copyF = nullptr;
};

template<class T, int ctrlId, DWORD style = 0>
class TypedListViewCtrl : public CWindowImpl<TypedListViewCtrl<T, ctrlId, style>, CListViewCtrl, CControlWinTraits>,
	public ListViewArrows<TypedListViewCtrl<T, ctrlId, style> >
{
public:
	TypedListViewCtrl() { }
	~TypedListViewCtrl() { for_each(columnList.begin(), columnList.end(), std::default_delete<ColumnInfo>()); }

	typedef TypedListViewCtrl<T, ctrlId, style> thisClass;
	typedef CListViewCtrl baseClass;
	typedef ListViewArrows<thisClass> arrowBase;

	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_MENUCOMMAND, onHeaderMenu)
		MESSAGE_HANDLER(WM_CHAR, onChar)
		MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkgnd)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_ENABLE, onEnable)
		REFLECTED_NOTIFY_CODE_HANDLER(NM_CLICK, onClick)
		REFLECTED_NOTIFY_CODE_HANDLER(NM_DBLCLK, onDoubleClick)
		CHAIN_MSG_MAP(arrowBase)
	END_MSG_MAP();

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {

		 if(SETTING(USE_EXPLORER_THEME)) {
			SetWindowTheme(this->m_hWnd, L"explorer", NULL);
         }

		 if (style & LVITEM_GROUPING) {
			 this->EnableGroupView(TRUE);
			 LVGROUPMETRICS metrics = { 0 };
			 metrics.cbSize = sizeof(metrics);
			 metrics.mask = LVGMF_TEXTCOLOR;
			 metrics.crHeader = SETTING(TEXT_GENERAL_FORE_COLOR);
			 this->SetGroupMetrics(&metrics);
		 }

		bHandled = FALSE;
		return 0;
	}

	class iterator : public std::iterator<random_access_iterator_tag, T*> {
	public:
		iterator() { }
		iterator(const iterator& rhs) : typedList(rhs.typedList), cur(rhs.cur), cnt(rhs.cnt) { }
		iterator& operator=(const iterator& rhs) { typedList = rhs.typedList; cur = rhs.cur; cnt = rhs.cnt; return *this; }

		bool operator==(const iterator& rhs) const { return cur == rhs.cur; }
		bool operator!=(const iterator& rhs) const { return !(*this == rhs); }
		bool operator<(const iterator& rhs) const { return cur < rhs.cur; }

		int operator-(const iterator& rhs) const { 
			return cur - rhs.cur;
		}

		iterator& operator+=(int n) { cur += n; return *this; }
		iterator& operator-=(int n) { return (cur += -n); }
		
		T& operator*() { return *typedList->getItemData(cur); }
		T* operator->() { return &(*(*this)); }
		T& operator[](int n) { return *typedList->getItemData(cur + n); }
		
		iterator operator++(int) {
			iterator tmp(*this);
			operator++();
			return tmp;
		}
		iterator& operator++() {
			++cur;
			return *this;
		}

	private:
		iterator(thisClass* aTypedList) : typedList(aTypedList), cur(aTypedList->GetNextItem(-1, LVNI_ALL)), cnt(aTypedList->GetItemCount()) { 
			if(cur == -1)
				cur = cnt;
		}
		iterator(thisClass* aTypedList, int first) : typedList(aTypedList), cur(first), cnt(aTypedList->GetItemCount()) { 
			if(cur == -1)
				cur = cnt;
		}
		friend class thisClass;
		thisClass* typedList = nullptr;
		int cur = 0;
		int cnt = 0;
	};

	LRESULT onChar(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		if((GetKeyState(VkKeyScan('A') & 0xFF) & 0xFF00) > 0 && (GetKeyState(VK_CONTROL) & 0xFF00) > 0){
			int count = this->GetItemCount();
			for(int i = 0; i < count; ++i)
				ListView_SetItemState(this->m_hWnd, i, LVIS_SELECTED, LVIS_SELECTED);

			return 0;
		}
		
		bHandled = FALSE;
		return 1;
	}

	void addClickHandler(int column, ColumnInfo::ClickHandler::ClickF af, bool doubleClick) {
		columnList[column]->clickHandler = { af, doubleClick };
	}

	void addCopyHandler(int column, ColumnInfo::CopyF af) {
		columnList[column]->copyF = af;
	}

	BOOL hitIcon(int aItem, int aSubItem) {
		CRect rc;
		CPoint pt = GetMessagePos();
		this->ScreenToClient(&pt);
		this->GetSubItemRect(aItem, aSubItem, LVIR_ICON, rc);

		return PtInRect(&rc, pt);
	}

	LRESULT onClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
		return handleClick(pnmh, bHandled, false);
	}

	LRESULT onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
		return handleClick(pnmh, bHandled, true);
	}

	LRESULT handleClick(LPNMHDR pnmh, BOOL& bHandled, bool doubleClick) {
		NMITEMACTIVATE* l = (NMITEMACTIVATE*) pnmh;

		if (l->iItem != -1) {
			int col = findColumn(l->iSubItem);
			auto& handler = columnList[col]->clickHandler;
			if (handler.f && (doubleClick || !handler.doubleClick) && (columnList[col]->colType != COLUMN_IMAGE || hitIcon(l->iItem, l->iSubItem))) {
				if (handler.f(l->iItem)) {
					bHandled = TRUE;
					return TRUE;
				}
			}
		}

		bHandled = FALSE;
		return 0;
	}

	inline void setText(LVITEM& i, const tstring text) { lstrcpyn(i.pszText, text.c_str(), i.cchTextMax); }
	inline void setText(LVITEM& i, const TCHAR* text) { i.pszText = const_cast<TCHAR*>(text); }

	LRESULT onGetDispInfo(int /* idCtrl */, LPNMHDR pnmh, BOOL& /* bHandled */) {
		NMLVDISPINFO* di = (NMLVDISPINFO*)pnmh;
		if(di->item.mask & LVIF_TEXT) {
			di->item.mask |= LVIF_DI_SETITEM;
			setText(di->item, ((T*)di->item.lParam)->getText(columnIndexes[di->item.iSubItem]));
		}
		return 0;
	}
	
	LRESULT onInfoTip(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		if(!SETTING(SHOW_INFOTIPS)) return 0;

		NMLVGETINFOTIP* pInfoTip = (NMLVGETINFOTIP*) pnmh;

		tstring InfoTip = this->GetColumnTexts(pInfoTip->iItem);
		
		pInfoTip->cchTextMax = InfoTip.size();

 		_tcsncpy(pInfoTip->pszText, InfoTip.c_str(), INFOTIPSIZE);
		pInfoTip->pszText[INFOTIPSIZE - 1] = NULL;

		return 0;
	}

	tstring GetColumnTexts(int row) {
#define BUF_SIZE 300
		BOOL NoColumnHeader = (BOOL) (this->GetWindowLongPtr(GWL_STYLE) & LVS_NOCOLUMNHEADER);
		tstring InfoTip;
		tstring buffer;
		TCHAR buf[512];
		//buffer.resize(BUF_SIZE);

		int indexes[32];
		this->GetColumnOrderArray(this->GetHeader().GetItemCount(), indexes);
		LV_COLUMN lvCol;

		for (int i = 0; i < this->GetHeader().GetItemCount(); ++i) {
			this->GetItemText(row, indexes[i], &buf[0], BUF_SIZE);
			buffer = buf;

			if (!buffer.empty()) {
				if (!NoColumnHeader) {
					lvCol.mask = LVCF_TEXT;
					lvCol.pszText = &buf[0];
					lvCol.cchTextMax = BUF_SIZE;
					this->GetColumn(indexes[i], &lvCol);
					InfoTip += lvCol.pszText;
					InfoTip += _T(": ");
				}

				InfoTip += buffer;
				InfoTip += _T("\r\n");
			}
		}

		if (InfoTip.size() > 2)
			InfoTip.erase(InfoTip.size() - 2);
		return InfoTip;
	}

	// Sorting
	LRESULT onColumnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLISTVIEW* l = (NMLISTVIEW*)pnmh;
		if(l->iSubItem != sortColumn) {
			sortAscending = true;
			sortColumn = l->iSubItem;
		} else if(sortAscending) {
			sortAscending = false;
		} else {
			sortColumn = -1;
		}
		this->updateArrow();
		resort();
		return 0;
	}
	void resort() {
		if(sortColumn != -1) {
			this->SortItems(&compareFunc, (LPARAM)this);
		}
	}

	typedef deque < pair < tstring, function<tstring(const T*) >> > MenuItemList;
	inline void appendCopyMenu(OMenu& parent) {
		MenuItemList items;
		appendCopyMenu(parent, items);
	}

	void appendCopyMenu(OMenu& parent, MenuItemList& items) {
		auto copyMenu = parent.createSubMenu(TSTRING(COPY), true);

		int pos = 0;
		auto insertPos = items.begin();
		for (const auto& ci : columnList) {
			if (ci->colType != COLUMN_IMAGE) {
				insertPos = items.emplace(insertPos, ci->name, [=](const T* aItem) {
					auto text = aItem->getText(pos);
					return ci->copyF ? ci->copyF(text) : text; 
				});
				insertPos++;
			}
			pos++;
		}

		for (const auto& i : items) {
			copyMenu->appendItem(i.first, [=] { handleCopy(i.second); });
		}

		copyMenu->appendSeparator();
		copyMenu->appendItem(TSTRING(ALL), [=] { handleCopyAll(items); });
	}

	void handleCopy(function<tstring(const T*)> textF) {
		tstring sCopy;
		forEachSelectedT([&](const T* aItem) {
			if (!sCopy.empty())
				sCopy += _T("\r\n");

			sCopy += textF(aItem);
		});

		if (!sCopy.empty())
			WinUtil::setClipboard(sCopy);
	}

	void handleCopyAll(const MenuItemList& items) {
		tstring sCopy;

		int sel = -1;
		while ((sel = this->GetNextItem(sel, LVNI_SELECTED)) != -1) {
			if (!sCopy.empty())
				sCopy += _T("\r\n");

			for (const auto& i : items) {
				auto colText = i.second(getItemData(sel));
				if (!colText.empty()) {
					if (!sCopy.empty())
						sCopy += _T("\r\n");
					sCopy += i.first + _T(": ") + colText;
				}
			}
		}

		if (!sCopy.empty())
			WinUtil::setClipboard(sCopy);
	}

	int insertItem(const T* item, int image) {
		return insertItem(getSortPos(item), item, image);
	}
	int insertItem(int i, const T* item, int image) {
		return this->InsertItem(LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE, i,
			LPSTR_TEXTCALLBACK, 0, 0, image, (LPARAM)item);
	}

	int insertItem(int i, const T* item, int image, int groupIndex) {
		LV_ITEM lvi;
		lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
		if (style & LVITEM_GROUPING) {
			lvi.mask = lvi.mask | LVIF_GROUPID;
			lvi.iGroupId = groupIndex;
		}
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.pszText = LPSTR_TEXTCALLBACK;
		lvi.iImage = image;
		lvi.lParam = (LPARAM)item;
		return this->InsertItem(&lvi);
	}

	void insertGroup(int id, const tstring& name, DWORD uAlign) {
		LVGROUP lvg = { 0 };
		lvg.cbSize = sizeof(lvg);
		lvg.iGroupId = id;
		lvg.state = LVGS_NORMAL | LVGS_COLLAPSIBLE;
		lvg.mask = LVGF_GROUPID | LVGF_HEADER | LVGF_STATE | LVGF_ALIGN;
		lvg.uAlign = uAlign;
		lvg.pszHeader = (LPWSTR)name.c_str();
		this->InsertGroup(id, &lvg);
	}

	T* getItemData(int iItem) const { return (T*)this->GetItemData(iItem); }
	const T* getSelectedItem() const { return (this->GetSelectedCount() > 0 ? getItemData(this->GetNextItem(-1, LVNI_SELECTED)) : NULL); }

	int findItem(const T* item) const { 
		LVFINDINFO fi = { LVFI_PARAM, NULL, (LPARAM)item };
		return this->FindItem(&fi, -1);
	}
	struct CompFirst {
		CompFirst() { } 
		bool operator()(T& a, const tstring& b) {
			return stricmp(a.getText(0), b) < 0;
		}
	};
	int findItem(const tstring& b, int start = -1, bool aPartial = false) const {
		LVFINDINFO fi = { aPartial ? LVFI_PARTIAL : LVFI_STRING, b.c_str() };
		return this->FindItem(&fi, start);
	}

	void forEach(void (T::*func)()) {
		int n = this->GetItemCount();
		for(int i = 0; i < n; ++i)
			(getItemData(i)->*func)();
	}
	void forEachSelected(void (T::*func)()) {
		int i = -1;
		while( (i = this->GetNextItem(i, LVNI_SELECTED)) != -1)
			(getItemData(i)->*func)();
	}
	template<class _Function>
	_Function forEachT(_Function pred) {
		int n = this->GetItemCount();
		for(int i = 0; i < n; ++i)
			pred(getItemData(i));
		return pred;
	}
	template<class _Function>
	_Function forEachSelectedT(_Function pred) {
		int i = -1;
		while( (i = this->GetNextItem(i, LVNI_SELECTED)) != -1)
			pred(getItemData(i));
		return pred;
	}
	void forEachAtPos(int iIndex, void (T::*func)()) {
		(getItemData(iIndex)->*func)();
	}

	void updateItem(int i) {
		int k = this->GetHeader().GetItemCount();
		for(int j = 0; j < k; ++j)
			this->SetItemText(i, j, LPSTR_TEXTCALLBACK);
	}
	
	void updateItem(int i, int col) { this->SetItemText(i, col, LPSTR_TEXTCALLBACK); }
	void updateItem(const T* item) { int i = findItem(item); if(i != -1) updateItem(i); }
	void deleteItem(const T* item) { int i = findItem(item); if(i != -1) this->DeleteItem(i); }
	void selectItem(const T* item) { int i = findItem(item); if (i != -1) this->SelectItem(i); }
	
	void updateItemImage(const T* item) {
		int i = findItem(item);
		if (i != -1)
			this->SetItem(i, 0, LVIF_IMAGE, NULL, item->getImageIndex(), 0, 0, NULL);
	}


	int getSortPos(const T* a) const {
		int high = this->GetItemCount();
		if((sortColumn == -1) || (high == 0))
			return high;

		high--;

		int low = 0;
		int mid = 0;
		T* b = NULL;
		int comp = 0;
		while( low <= high ) {
			mid = (low + high) / 2;
			b = getItemData(mid);
			comp = T::compareItems(a, b, static_cast<uint8_t>(sortColumn));

			if(!sortAscending)
				comp = -comp;

			if(comp == 0) {
				return mid;
			} else if(comp < 0) {
				high = mid - 1;
			} else if(comp > 0) {
					low = mid + 1;
			}
		}

		comp = T::compareItems(a, b, static_cast<uint8_t>(sortColumn));
		if(!sortAscending)
			comp = -comp;
		if(comp > 0)
			mid++;

		return mid;
	}

	void setSortColumn(int aSortColumn) {
		sortColumn = aSortColumn;
		this->updateArrow();
	}
	int getSortColumn() const { return sortColumn; }
	uint8_t getRealSortColumn() const { return findColumn(sortColumn); }
	bool isAscending() const { return sortAscending; }
	void setAscending(bool s) {
		sortAscending = s;
		this->updateArrow();
	}

	iterator begin() { return iterator(this); }
	iterator end() { return iterator(this, this->GetItemCount()); }

	int InsertColumn(uint8_t nCol, const tstring &columnHeading, int nFormat = LVCFMT_LEFT, int nWidth = -1, int nSubItem = -1, ColumnType aColType = COLUMN_TEXT ){
		if(nWidth == 0) nWidth = 80;
		columnList.push_back(new ColumnInfo(columnHeading, nCol, nFormat, nWidth, aColType));
		columnIndexes.push_back(nCol);
		return CListViewCtrl::InsertColumn(nCol, columnHeading.c_str(), nFormat, nWidth, nSubItem);
	}

	void showMenu(const POINT &pt){
		headerMenu.DestroyMenu();
		headerMenu.CreatePopupMenu();
		MENUINFO inf;
		inf.cbSize = sizeof(MENUINFO);
		inf.fMask = MIM_STYLE;
		inf.dwStyle = MNS_NOTIFYBYPOS;
		headerMenu.SetMenuInfo(&inf);

		int j = 0;
		for(ColumnIter i = columnList.begin(); i != columnList.end(); ++i, ++j) {
			headerMenu.AppendMenu(MF_STRING, IDC_HEADER_MENU, (*i)->name.c_str());
			if((*i)->visible)
				headerMenu.CheckMenuItem(j, MF_BYPOSITION | MF_CHECKED);
		}
		headerMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this->m_hWnd);
	}
		
	void DestroyAllItems() {
		const int count = this->GetItemCount();
		for(int i = 0; i < count; i++) {
			T* p = getItemData(i);
			delete p;
		}

		this->DeleteAllItems();
	}
	
	/*Override handling of window cancelmode, let the owner do the necessary drawing*/
	LRESULT onEnable(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = !wParam;
		return 0;
	}

	LRESULT onEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		return 1;
	}

	void setFlickerFree(HBRUSH flickerBrush) { hBrBg = flickerBrush; }

	LRESULT onContextMenu(UINT /*msg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		//make sure we're not handling the context menu if it's invoked from the
		//keyboard
		if(pt.x == -1 && pt.y == -1) {
			bHandled = FALSE;
			return 0;
		}

		CRect rc;
		this->GetHeader().GetWindowRect(&rc);

		if (PtInRect(&rc, pt)) {
			showMenu(pt);
			return 0;
		}
		bHandled = FALSE;
		return 0;
	}
	
	LRESULT onHeaderMenu(UINT /*msg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		auto ci = columnList[wParam];
		ci->visible = ! ci->visible;

		this->SetRedraw(FALSE);

		if(!ci->visible){
			removeColumn(ci);
		} else {
			if(ci->width == 0) ci->width = 80;
			CListViewCtrl::InsertColumn(ci->pos, ci->name.c_str(), ci->format, ci->width, static_cast<int>(wParam));
			LVCOLUMN lvcl = { 0 };
			lvcl.mask = LVCF_ORDER;
			lvcl.iOrder = ci->pos;
			this->SetColumn(ci->pos, &lvcl);
			for(int i = 0; i < this->GetItemCount(); ++i) {
				LVITEM lvItem;
				lvItem.iItem = i;
				lvItem.iSubItem = 0;
				lvItem.mask = LVIF_PARAM | (noDefaultItemImages ? 0 : LVIF_IMAGE);
				this->GetItem(&lvItem);
				if(!noDefaultItemImages)
					lvItem.iImage = ((T*)lvItem.lParam)->getImageIndex();
				this->SetItem(&lvItem);
				updateItem(i);
			}
		}

		updateColumnIndexes();

		this->SetRedraw();
		this->Invalidate();
		this->UpdateWindow();

		return 0;
	}

	void saveHeaderOrder(SettingsManager::StrSetting order, SettingsManager::StrSetting widths, 
		SettingsManager::StrSetting visible)
	{	
		string tmp, tmp2, tmp3;
		
		saveHeaderOrder(tmp, tmp2, tmp3);
		
		SettingsManager::getInstance()->set(order, tmp);
		SettingsManager::getInstance()->set(widths, tmp2);
		SettingsManager::getInstance()->set(visible, tmp3);
	}

	void saveHeaderOrder(string& order, string& widths, string& visible) noexcept {
/*		int size = GetHeader().GetItemCount();

		std::vector<int> ret(size);
		if(SendMessage(LVM_GETCOLUMNORDERARRAY, static_cast<WPARAM>(ret.size()), reinterpret_cast<LPARAM>(&ret[0]))) {
			for(size_t i = 0; i < ret.size(); ++i) {
				order += Util::toString(ret[i]) + ",";
			}
		}

		for(size_t i = 0; i < ret.size(); ++i) {
			widths += Util::toString(SendMessage(LVM_GETCOLUMNWIDTH, static_cast<WPARAM>(i), 0));
			widths += ",";
		}*/			

		TCHAR buf[512];
		int size = this->GetHeader().GetItemCount();
		for(int i = 0; i < size; ++i) {
			LVCOLUMN lvc;
			lvc.mask = LVCF_TEXT | LVCF_ORDER | LVCF_WIDTH;
			lvc.cchTextMax = 512;
			lvc.pszText = buf;
			this->GetColumn(i, &lvc);
			for(auto c: columnList){
				if(_tcscmp(buf, c->name.c_str()) == 0) {
					c->pos = lvc.iOrder;
					c->width = lvc.cx;
					break;
				}
			}
		}

		for(auto ci: columnList) {
			if(ci->visible){
				visible += "1,";
			} else {
				ci->pos = size++;
				visible += "0,";
			}
			
			order += Util::toString(ci->pos) + ",";
			widths += Util::toString(ci->width) + ",";
		}

		order.erase(order.size()-1, 1);
		widths.erase(widths.size()-1, 1);
		visible.erase(visible.size()-1, 1);

	}
	bool isColumnVisible(int col) {
		return columnList[col]->visible;
	}
	
	void setVisible(const string& vis) {
		StringTokenizer<string> tok(vis, ',');
		StringList l = tok.getTokens();

		auto i = l.begin();
		for(auto j = columnList.begin(); j != columnList.end() && i != l.end(); ++i, ++j) {

			if(Util::toInt(*i) == 0){
				(*j)->visible = false;
				removeColumn(*j);
			}
		}

		updateColumnIndexes();
	}

	void setColumnOrderArray(int iCount, int* columns) {
		LVCOLUMN lvc = {0};
		lvc.mask = LVCF_ORDER;

		int j = 0;
		for(int i = 0; i < iCount;) {
			if(columns[i] == j) {
				lvc.iOrder = columnList[i]->pos = columns[i];
				this->SetColumn(i, &lvc);

				j++;
				i = 0;
			} else {
				i++;
			}
		}
	}
	//find the original position of the column at the current position.
	inline uint8_t findColumn(int col) const { return columnIndexes[col]; }	
	
	//find the current position of the column with original position
	inline int findColumnIndex(int col) const { 
		auto i = find(columnIndexes.begin(), columnIndexes.end(), col);
		return distance(columnIndexes.begin(), i); 
	}	

	typedef vector<ColumnInfo*> ColumnList;
	ColumnList& getColumnList() { return columnList; }
	bool noDefaultItemImages = false;
private:
	int sortColumn = -1;
	bool sortAscending = true;
	int leftMargin = 0;
	HBRUSH hBrBg = WinUtil::bgBrush;
	CMenu headerMenu;	

	static int CALLBACK compareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
		thisClass* t = (thisClass*)lParamSort;
		int result = T::compareItems((T*)lParam1, (T*)lParam2, t->getRealSortColumn());
		return (t->sortAscending ? result : -result);
	}

	typedef ColumnList::const_iterator ColumnIter;

	ColumnList columnList;
	vector<uint8_t> columnIndexes;

	void removeColumn(ColumnInfo* ci){
		TCHAR buf[512];
		LVCOLUMN lvcl = { 0 };
		lvcl.mask = LVCF_TEXT | LVCF_ORDER | LVCF_WIDTH;
		lvcl.pszText = buf;
		lvcl.cchTextMax = 512;

		for(int k = 0; k < this->GetHeader().GetItemCount(); ++k){
			this->GetColumn(k, &lvcl);
			if(_tcscmp(ci->name.c_str(), lvcl.pszText) == 0){
				ci->width = lvcl.cx;
				ci->pos = lvcl.iOrder;

				int itemCount = this->GetHeader().GetItemCount();
				if(itemCount >= 0 && sortColumn > itemCount - 2)
					setSortColumn(0);

				if(sortColumn == ci->pos)
					setSortColumn(0);
	
				this->DeleteColumn(k);

				for(int i = 0; i < this->GetItemCount(); ++i) {
					LVITEM lvItem;
					lvItem.iItem = i;
					lvItem.iSubItem = 0;
					lvItem.mask = LVIF_PARAM | (noDefaultItemImages ? 0 : LVIF_IMAGE);
					this->GetItem(&lvItem);
					if(!noDefaultItemImages)
						lvItem.iImage = ((T*)lvItem.lParam)->getImageIndex();
					this->SetItem(&lvItem);
				}
				break;
			}
		}
	}

	void updateColumnIndexes() {
		columnIndexes.clear();
		
		int columns = this->GetHeader().GetItemCount();
		
		columnIndexes.reserve(columns);

		TCHAR buf[128];
		LVCOLUMN lvcl;

		for(int i = 0; i < columns; ++i) {
			lvcl.mask = LVCF_TEXT;
			lvcl.pszText = buf;
			lvcl.cchTextMax = 128;
			this->GetColumn(i, &lvcl);
			
			for(size_t j = 0; j < columnList.size(); ++j) {
				if(stricmp(columnList[j]->name.c_str(), lvcl.pszText) == 0) {
					columnIndexes.push_back(static_cast<uint8_t>(j));
					break;
				}
			}
		}
	}	
};

// Copyright (C) 2005-2010 Big Muscle, StrongDC++

template<class T, int ctrlId, class K, class hashFunc, class equalKey, DWORD style>
class TypedTreeListViewCtrl : public TypedListViewCtrl<T, ctrlId, style> 
{
public:

	TypedTreeListViewCtrl() { }
	~TypedTreeListViewCtrl() { }

	typedef TypedTreeListViewCtrl<T, ctrlId, K, hashFunc, equalKey, style> thisClass;
	typedef TypedListViewCtrl<T, ctrlId, style> baseClass;
	
	struct ParentPair {
		T* parent;
		vector<T*> children;

		void resetChildren() {
			parent->hits = 0;
			children.clear();
		}
	};

	typedef unordered_map<K*, ParentPair, hashFunc, equalKey> ParentMap;

	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, onLButton)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP();


	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		states.CreateFromImage(IDB_STATE, 16, 2, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
		this->SetImageList(states, LVSIL_STATE);

		bHandled = FALSE;
		return 0;
	}

	LRESULT onLButton(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
		//if (style & VIRTUAL_CHILDREN){
		//	bHandled = FALSE;
		//	return SendMessage(GetParent(), uMsg, wParam, lParam);
		//}

		CPoint pt;
		pt.x = GET_X_LPARAM(lParam);
		pt.y = GET_Y_LPARAM(lParam);

		LVHITTESTINFO lvhti;
		lvhti.pt = pt;

		int pos = this->SubItemHitTest(&lvhti);
		if (pos != -1) {
			CRect rect;
			this->GetItemRect(pos, rect, LVIR_ICON);
			
			if (pt.x < rect.left) {
				T* i = this->getItemData(pos);
				if(i->parent == NULL) {
					if(i->collapsed) {
						Expand(i, pos);
					} else {
						Collapse(i, pos);
					}
				}
			}
		}

		bHandled = FALSE;
		return 0;
	} 

	void Collapse(T* parent, int itemPos) {
		this->SetRedraw(false);
		auto& children = findChildren(parent->getGroupCond());
		for(const auto& c: children)
			this->deleteItem(c);

		parent->collapsed = true;
		this->SetItemState(itemPos, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
		this->SetRedraw(true);
	}

	void Expand(T* parent, int itemPos) {
		this->SetRedraw(false);
		
		if (insertF) {
			insertF(parent);
		}
		const auto& children = findChildren(parent->getGroupCond());
		if(children.size() > (size_t)(uniqueParent ? 1 : 0)) {
			LVITEM lvItem;
			lvItem.iItem = itemPos;
			lvItem.iSubItem = 0;
			lvItem.mask = LVIF_GROUPID;
			this->GetItem(&lvItem);

			parent->collapsed = false;
			for (const auto& c : children){
				if (filterF && !filterF(c))
					continue;
					
				insertChild(c, itemPos + 1, lvItem.iGroupId);
			}

			this->SetItemState(itemPos, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
			resort();
		}
		this->SetRedraw(true);
	}

	int insertChild(const T* item, int idx, int groupIndex = 0) {
		LV_ITEM lvi;
		lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE | LVIF_INDENT;
		if (style & LVITEM_GROUPING){
			lvi.mask = lvi.mask | LVIF_GROUPID;
			lvi.iGroupId = groupIndex;
		}
		lvi.iItem = idx;
		lvi.iSubItem = 0;
		lvi.iIndent = 1;
		lvi.pszText = LPSTR_TEXTCALLBACK;
		lvi.iImage = item->getImageIndex();
		lvi.lParam = (LPARAM)item;
		lvi.state = 0;
		lvi.stateMask = 0;
		return this->InsertItem(&lvi);
	}

	inline T* findParent(const K& groupCond) const {
		auto i = parents.find(const_cast<K*>(&groupCond));
		return i != parents.end() ? (*i).second.parent : nullptr;
	}

	static const vector<T*> emptyVector;
	inline const vector<T*>& findChildren(const K& groupCond) const {
		auto i = parents.find(const_cast<K*>(&groupCond));
		return i != parents.end() ? (*i).second.children : emptyVector;
	}

	inline ParentPair* findParentPair(const K& groupCond) {
		auto i = parents.find(const_cast<K*>(&groupCond));
		return i != parents.end() ? &((*i).second) : nullptr;
	}

	void insertGroupedItem(T* item, bool autoExpand, int groupIndex = 0, bool hasVirtualChildren = false) {
		T* parent = nullptr;
		auto pp = findParentPair(item->getGroupCond());

		int pos = -1;

		auto createParentPair = [&](T* parent, T* oldParent) {
			ParentPair newPP = { parent };
			pp = &(parents.emplace(const_cast<K*>(&parent->getGroupCond()), newPP).first->second);

			parent->parent = nullptr; // ensure that parent of this item is really NULL
			if (oldParent) {
				oldParent->parent = parent;
				pp->children.push_back(oldParent); // mark old parent item as a child
			}
			parent->hits++;

			pos = this->insertItem(getSortPos(parent), parent, parent->getImageIndex(), groupIndex);
		};

		auto updateCollapsedState = [&] {
			if (pos != -1) {
				if (autoExpand) {
					this->SetItemState(pos, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
					parent->collapsed = false;
				} else {
					this->SetItemState(pos, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
				}
			}
		};

		if (style & NO_GROUP_UNIQUE_CHILDREN) {
			if (!pp) {
				parent = item;

				ParentPair newPP = { parent };
				parents.emplace(const_cast<K*>(&parent->getGroupCond()), newPP);

				parent->parent = nullptr; // ensure that parent of this item is really NULL
				if (filterF && !filterF(parent))
					return;

				pos = this->insertItem(getSortPos(parent), parent, parent->getImageIndex(), groupIndex);
				if (hasVirtualChildren && (style & VIRTUAL_CHILDREN)) updateCollapsedState();
				return;
			} else if (pp->children.empty()) {
				auto oldParent = pp->parent;
				parent = oldParent->createParent();
				if (parent != oldParent) {
					uniqueParent = true;
					parents.erase(const_cast<K*>(&oldParent->getGroupCond()));
					this->deleteItem(oldParent);

					createParentPair(parent, oldParent);
				} else {
					uniqueParent = false;
					pos = this->findItem(parent);
				}

				updateCollapsedState();
			} else {
				parent = pp->parent;
				pos = this->findItem(parent);
			}
		} else {
			if (!pp) {
				// always create a parent for this item
				parent = item->createParent();
				uniqueParent = false;

				createParentPair(parent, nullptr);
				updateCollapsedState();
			} else {
				parent = pp->parent;
				pos = this->findItem(parent);
			}
		}

		pp->children.push_back(item);
		parent->hits++;
		item->parent = parent;

		if(pos != -1) {
			if(!parent->collapsed) {
				insertChild(item, pos + pp->children.size(), groupIndex);
			}
			this->updateItem(pos);
		}
	}

	void removeParent(T* parent) {
		auto pp = findParentPair(parent->getGroupCond());
		if(pp) {
			for(auto i: pp->children) {
				this->deleteItem(i);
				delete i;
			}
			pp->children.clear();
			parents.erase(const_cast<K*>(&parent->getGroupCond()));
		}
		this->deleteItem(parent);
	}

	bool removeGroupedItem(T* item, bool removeFromMemory = true, int groupIndex = 0) {
		bool parentExists = true;

		if(!item->parent) {
			removeParent(item);
		} else {
			T* parent = item->parent;
			ParentPair* pp = findParentPair(parent->getGroupCond());

			this->deleteItem(item);

			auto n = find(pp->children.begin(), pp->children.end(), item);
			if(n != pp->children.end()) {
				pp->children.erase(n);
				pp->parent->hits--;
			}
	
			if (style & NO_GROUP_UNIQUE_CHILDREN) {
				if(uniqueParent) {
					dcassert(!pp->children.empty());
					if(pp->children.size() == 1) {
						const auto oldParent = parent;
						parent = pp->children.front();

						this->deleteItem(oldParent);
						parents.erase(const_cast<K*>(&oldParent->getGroupCond()));
						delete oldParent;

						ParentPair newPP = { parent };
						parents.emplace(const_cast<K*>(&parent->getGroupCond()), newPP);

						parent->parent = nullptr; // ensure that parent of this item is really NULL
						this->deleteItem(parent);
						this->insertItem(getSortPos(parent), parent, parent->getImageIndex(), groupIndex);
					}
				} else if(pp->children.empty()) {
					this->SetItemState(this->findItem(parent), INDEXTOSTATEIMAGEMASK(0), LVIS_STATEIMAGEMASK);
				}

				this->updateItem(parent);
			} else {
				if (pp->children.empty()) {
					this->deleteItem(parent);
					parents.erase(const_cast<K*>(&parent->getGroupCond()));
					delete parent;
					parentExists = false;
				} else {
					this->updateItem(parent);
				}
			}
		}

		if(removeFromMemory)
			delete item;

		return parentExists;
	}

	void deleteAllItems() {
		// HACK: ugly hack but at least it doesn't crash and there's no memory leak
		for(auto p: parents | views::values) {
			for(auto c: p.children) {
				this->deleteItem(c);
				delete c;
			}
			this->deleteItem(p.parent);
			delete p.parent;
		}
		for(int i = 0; i < this->GetItemCount(); i++) {
			auto si = this->getItemData(i);
			delete si;
		}

 		parents.clear();
		this->DeleteAllItems();
	}

	//count the parents and their children that exist in the list
	size_t getTotalItemCount() {
		size_t ret = 0;
		for (int i = 0; i < this->GetItemCount(); i++) {
			T* si = this->getItemData(i);
			if (si->parent) {
				//only count the parents
				continue;
			} else {
				ret += si->hits+1;
			}
		}
		
		return ret;
	}

	LRESULT onColumnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLISTVIEW* l = (NMLISTVIEW*)pnmh;
		if(l->iSubItem != this->getSortColumn()) {
			this->setAscending(true);
			this->setSortColumn(l->iSubItem);
		} else if (this->isAscending()) {
			this->setAscending(false);
		} else {
			this->setSortColumn(-1);
		}		
		resort();
		return 0;
	}

	void resort() {
		if(this->getSortColumn() != -1) {
			this->SortItems(&compareFunc, (LPARAM)this);
		}
	}

	int getSortPos(const T* a) {
		int high = this->GetItemCount();
		if((this->getSortColumn() == -1) || (high == 0))
			return high;

		high--;

		int low = 0;
		int mid = 0;
		T* b = nullptr;
		int comp = 0;
		while( low <= high ) {
			mid = (low + high) / 2;
			b = this->getItemData(mid);
			comp = compareItems(a, b, static_cast<uint8_t>(this->getSortColumn()));
			
			if(!this->isAscending())
				comp = -comp;

			if(comp == 0) {
				return mid;
			} else if(comp < 0) {
				high = mid - 1;
			} else if(comp > 0) {
				low = mid + 1;
			} else if(comp == 2){
				if(this->isAscending())
					low = mid + 1;
				else
					high = mid -1;
			} else if(comp == -2){
				if(!this->isAscending())
					low = mid + 1;
				else
					high = mid -1;
			}
		}

		comp = compareItems(a, b, static_cast<uint8_t>(this->getSortColumn()));
		if(!this->isAscending())
			comp = -comp;
		if(comp > 0)
			mid++;

		return mid;
	}

	ParentMap& getParents() { return parents; }


	template<class _Function>
	_Function filteredForEachSelectedT(_Function pred) {
		unordered_set<K> doneParents;
		int i = -1;
		while( (i = this->GetNextItem(i, LVNI_SELECTED)) != -1) {
			auto d = this->getItemData(i);
			if (!d->parent) {
				doneParents.insert(d->getGroupCond());
				pred(d);
			} else if (doneParents.find(d->getGroupCond()) == doneParents.end()) {
				pred(d);
			}
		}
		return pred;
	}

	typedef function < void(T*) > InsertFunction;
	void setInsertFunction(InsertFunction&& aF) { insertF = std::move(aF); }

	typedef function < bool(const T*) > FilterFunction;
	void setFilterFunction(FilterFunction&& aF) { filterF = std::move(aF); }
private:

   	/** map of all parent items with their associated children */
	ParentMap parents;

	/** +/- images */
	CImageListManaged states;

	/** is extra item needed for parent items? */
	bool		uniqueParent;

	static int CALLBACK compareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
		thisClass* t = (thisClass*)lParamSort;
		int result = compareItems((T*)lParam1, (T*)lParam2, t->getRealSortColumn());

		if(result == 2)
			result = (t->isAscending() ? 1 : -1);
		else if(result == -2)
			result = (t->isAscending() ? -1 : 1);

		return (t->isAscending() ? result : -result);
	}

	static int compareItems(const T* a, const T* b, uint8_t col) {
		// Copyright (C) Liny, RevConnect

		// both are children
		if(a->parent && b->parent){
			// different parent
			if(a->parent != b->parent)
				return compareItems(a->parent, b->parent, col);			
		}else{
			if(a->parent == b)
				return 2;  // a should be displayed below b

			if(b->parent == a)
				return -2; // b should be displayed below a

			if(a->parent)
				return compareItems(a->parent, b, col);	

			if(b->parent)
				return compareItems(a, b->parent, col);	
		}

		return T::compareItems(a, b, col);
	}

	InsertFunction insertF;
	FilterFunction filterF;
};

template<class T, int ctrlId, class K, class hashFunc, class equalKey, DWORD style>
const vector<T*> TypedTreeListViewCtrl<T, ctrlId, K, hashFunc, equalKey, style>::emptyVector;

}

#endif // !defined(TYPED_LIST_VIEW_CTRL_H)
