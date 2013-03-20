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

#if !defined(TYPED_LIST_VIEW_CTRL_H)
#define TYPED_LIST_VIEW_CTRL_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../Client/SettingsManager.h"
#include "../client/StringTokenizer.h"
#include "ListViewArrows.h"

enum ColumnType{
	COLUMN_TEXT,
	COLUMN_NUMERIC,
	COLUMN_DATES
};

class ColumnInfo {
public:
	ColumnInfo(const tstring &aName, int aPos, int aFormat, int aWidth, ColumnType aColType): name(aName), pos(aPos), width(aWidth), 
		format(aFormat), visible(true), colType(aColType) {}
	~ColumnInfo() {}
		tstring name;
		bool visible;
		int pos;
		int width;
		int format;
		ColumnType colType;
};

template<class T, int ctrlId>
class TypedListViewCtrl : public CWindowImpl<TypedListViewCtrl<T, ctrlId>, CListViewCtrl, CControlWinTraits>,
	public ListViewArrows<TypedListViewCtrl<T, ctrlId> >
{
public:
	TypedListViewCtrl() : sortColumn(-1), sortAscending(true), hBrBg(WinUtil::bgBrush), leftMargin(0), noDefaultItemImages(false) { }
	~TypedListViewCtrl() { for_each(columnList.begin(), columnList.end(), DeleteFunction()); }

	typedef TypedListViewCtrl<T, ctrlId> thisClass;
	typedef CListViewCtrl baseClass;
	typedef ListViewArrows<thisClass> arrowBase;

	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_MENUCOMMAND, onHeaderMenu)
		MESSAGE_HANDLER(WM_CHAR, onChar)
		MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkgnd)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_ENABLE, onEnable)
		CHAIN_MSG_MAP(arrowBase)
	END_MSG_MAP();

	class iterator : public ::iterator<random_access_iterator_tag, T*> {
	public:
		iterator() : typedList(NULL), cur(0), cnt(0) { }
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
		thisClass* typedList;
		int cur;
		int cnt;
	};

	LRESULT onChar(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		if((GetKeyState(VkKeyScan('A') & 0xFF) & 0xFF00) > 0 && (GetKeyState(VK_CONTROL) & 0xFF00) > 0){
			int count = GetItemCount();
			for(int i = 0; i < count; ++i)
				ListView_SetItemState(m_hWnd, i, LVIS_SELECTED, LVIS_SELECTED);

			return 0;
		}
		
		bHandled = FALSE;
		return 1;
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

		tstring InfoTip = GetColumnTexts(pInfoTip->iItem);
		
		pInfoTip->cchTextMax = InfoTip.size();

 		_tcsncpy(pInfoTip->pszText, InfoTip.c_str(), INFOTIPSIZE);
		pInfoTip->pszText[INFOTIPSIZE - 1] = NULL;

		return 0;
	}

	tstring GetColumnTexts(int row) {
#define BUF_SIZE 300
		BOOL NoColumnHeader = (BOOL)(GetWindowLongPtr(GWL_STYLE) & LVS_NOCOLUMNHEADER);
		tstring InfoTip;
		tstring buffer;
		buffer.resize(BUF_SIZE);
		
		int indexes[32];
		GetColumnOrderArray(GetHeader().GetItemCount(), indexes);
		LV_COLUMN lvCol;

		for (int i = 0; i < GetHeader().GetItemCount(); ++i)
		{
			if (!NoColumnHeader) {
				lvCol.mask = LVCF_TEXT;
				lvCol.pszText = &buffer[0];
				lvCol.cchTextMax = BUF_SIZE;
				GetColumn(indexes[i], &lvCol);
				InfoTip += lvCol.pszText;
				InfoTip += _T(": ");
			}
			GetItemText(row, indexes[i],  &buffer[0], BUF_SIZE);

			InfoTip += &buffer[0];
			InfoTip += _T("\r\n");
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
		updateArrow();
		resort();
		return 0;
	}
	void resort() {
		if(sortColumn != -1) {
			SortItems(&compareFunc, (LPARAM)this);
		}
	}

	int insertItem(const T* item, int image) {
		return insertItem(getSortPos(item), item, image);
	}
	int insertItem(int i, const T* item, int image) {
		return InsertItem(LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE, i, 
			LPSTR_TEXTCALLBACK, 0, 0, image, (LPARAM)item);
	}

	T* getItemData(int iItem) const { return (T*)GetItemData(iItem); }
	const T* getSelectedItem() const { return (GetSelectedCount() > 0 ? getItemData(GetNextItem(-1, LVNI_SELECTED)) : NULL); }

	int findItem(const T* item) const { 
		LVFINDINFO fi = { LVFI_PARAM, NULL, (LPARAM)item };
		return FindItem(&fi, -1);
	}
	struct CompFirst {
		CompFirst() { } 
		bool operator()(T& a, const tstring& b) {
			return stricmp(a.getText(0), b) < 0;
		}
	};
	int findItem(const tstring& b, int start = -1, bool aPartial = false) const {
		LVFINDINFO fi = { aPartial ? LVFI_PARTIAL : LVFI_STRING, b.c_str() };
		return FindItem(&fi, start);
	}

	void forEach(void (T::*func)()) {
		int n = GetItemCount();
		for(int i = 0; i < n; ++i)
			(getItemData(i)->*func)();
	}
	void forEachSelected(void (T::*func)()) {
		int i = -1;
		while( (i = GetNextItem(i, LVNI_SELECTED)) != -1)
			(getItemData(i)->*func)();
	}
	template<class _Function>
	_Function forEachT(_Function pred) {
		int n = GetItemCount();
		for(int i = 0; i < n; ++i)
			pred(getItemData(i));
		return pred;
	}
	template<class _Function>
	_Function forEachSelectedT(_Function pred) {
		int i = -1;
		while( (i = GetNextItem(i, LVNI_SELECTED)) != -1)
			pred(getItemData(i));
		return pred;
	}
	void forEachAtPos(int iIndex, void (T::*func)()) {
		(getItemData(iIndex)->*func)();
	}

	void updateItem(int i) {
		int k = GetHeader().GetItemCount();
		for(int j = 0; j < k; ++j)
			SetItemText(i, j, LPSTR_TEXTCALLBACK);
	}
	
	void updateItem(int i, int col) { SetItemText(i, col, LPSTR_TEXTCALLBACK); }
	void updateItem(const T* item) { int i = findItem(item); if(i != -1) updateItem(i); }
	void deleteItem(const T* item) { int i = findItem(item); if(i != -1) DeleteItem(i); }

	int getSortPos(const T* a) const {
		int high = GetItemCount();
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
		updateArrow();
	}
	int getSortColumn() const { return sortColumn; }
	uint8_t getRealSortColumn() const { return findColumn(sortColumn); }
	bool isAscending() const { return sortAscending; }
	void setAscending(bool s) {
		sortAscending = s;
		updateArrow();
	}

	iterator begin() { return iterator(this); }
	iterator end() { return iterator(this, GetItemCount()); }

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
		headerMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
	}
		
	void DestroyAllItems() {
		const int count = GetItemCount();
		for(int i = 0; i < count; i++) {
			T* p = getItemData(i);
			delete p;
		}
			DeleteAllItems();
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
		GetHeader().GetWindowRect(&rc);

		if (PtInRect(&rc, pt)) {
			showMenu(pt);
			return 0;
		}
		bHandled = FALSE;
		return 0;
	}
	
	LRESULT onHeaderMenu(UINT /*msg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ColumnInfo * ci = columnList[wParam];
		ci->visible = ! ci->visible;

		SetRedraw(FALSE);

		if(!ci->visible){
			removeColumn(ci);
		} else {
			if(ci->width == 0) ci->width = 80;
			CListViewCtrl::InsertColumn(ci->pos, ci->name.c_str(), ci->format, ci->width, static_cast<int>(wParam));
			LVCOLUMN lvcl = { 0 };
			lvcl.mask = LVCF_ORDER;
			lvcl.iOrder = ci->pos;
			SetColumn(ci->pos, &lvcl);
			for(int i = 0; i < GetItemCount(); ++i) {
				LVITEM lvItem;
				lvItem.iItem = i;
				lvItem.iSubItem = 0;
				lvItem.mask = LVIF_PARAM | (noDefaultItemImages ? 0 : LVIF_IMAGE);
				GetItem(&lvItem);
				if(!noDefaultItemImages)
					lvItem.iImage = ((T*)lvItem.lParam)->getImageIndex();
				SetItem(&lvItem);
				updateItem(i);
			}
		}

		updateColumnIndexes();

		SetRedraw();
		Invalidate();
		UpdateWindow();

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
		int size = GetHeader().GetItemCount();
		for(int i = 0; i < size; ++i) {
			LVCOLUMN lvc;
			lvc.mask = LVCF_TEXT | LVCF_ORDER | LVCF_WIDTH;
			lvc.cchTextMax = 512;
			lvc.pszText = buf;
			GetColumn(i, &lvc);
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

		StringIter i = l.begin();
		for(ColumnIter j = columnList.begin(); j != columnList.end() && i != l.end(); ++i, ++j) {

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
				SetColumn(i, &lvc);

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

	typedef vector< ColumnInfo* > ColumnList;
	ColumnList& getColumnList() { return columnList; }
	bool noDefaultItemImages;

private:
	int sortColumn;
	bool sortAscending;
	int leftMargin;
	HBRUSH hBrBg;
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

		for(int k = 0; k < GetHeader().GetItemCount(); ++k){
			GetColumn(k, &lvcl);
			if(_tcscmp(ci->name.c_str(), lvcl.pszText) == 0){
				ci->width = lvcl.cx;
				ci->pos = lvcl.iOrder;

				int itemCount = GetHeader().GetItemCount();
				if(itemCount >= 0 && sortColumn > itemCount - 2)
					setSortColumn(0);

				if(sortColumn == ci->pos)
					setSortColumn(0);
	
				DeleteColumn(k);

				for(int i = 0; i < GetItemCount(); ++i) {
					LVITEM lvItem;
					lvItem.iItem = i;
					lvItem.iSubItem = 0;
					lvItem.mask = LVIF_PARAM | (noDefaultItemImages ? 0 : LVIF_IMAGE);
					GetItem(&lvItem);
					if(!noDefaultItemImages)
						lvItem.iImage = ((T*)lvItem.lParam)->getImageIndex();
					SetItem(&lvItem);
				}
				break;
			}
		}
	}

	void updateColumnIndexes() {
		columnIndexes.clear();
		
		int columns = GetHeader().GetItemCount();
		
		columnIndexes.reserve(columns);

		TCHAR buf[128];
		LVCOLUMN lvcl;

		for(int i = 0; i < columns; ++i) {
			lvcl.mask = LVCF_TEXT;
			lvcl.pszText = buf;
			lvcl.cchTextMax = 128;
			GetColumn(i, &lvcl);
			
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
template<class T, int ctrlId, class K, class hashFunc, class equalKey>
class TypedTreeListViewCtrl : public TypedListViewCtrl<T, ctrlId> 
{
public:

	TypedTreeListViewCtrl() { }
	~TypedTreeListViewCtrl() { states.Destroy(); }

	typedef TypedTreeListViewCtrl<T, ctrlId, K, hashFunc, equalKey> thisClass;
	typedef TypedListViewCtrl<T, ctrlId> baseClass;
	
	struct ParentPair {
		T* parent;
		vector<T*> children;
	};

	typedef unordered_map<K*, ParentPair, hashFunc, equalKey> ParentMap;

	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, onLButton)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP();


	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		states.CreateFromImage(IDB_STATE, 16, 2, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
		SetImageList(states, LVSIL_STATE); 

		bHandled = FALSE;
		return 0;
	}

	LRESULT onLButton(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
		CPoint pt;
		pt.x = GET_X_LPARAM(lParam);
		pt.y = GET_Y_LPARAM(lParam);

		LVHITTESTINFO lvhti;
		lvhti.pt = pt;

		int pos = SubItemHitTest(&lvhti);
		if (pos != -1) {
			CRect rect;
			GetItemRect(pos, rect, LVIR_ICON);
			
			if (pt.x < rect.left) {
				T* i = getItemData(pos);
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
		SetRedraw(false);
		auto& children = findChildren(parent->getGroupCond());
		for(const auto c: children)
			deleteItem(c);

		parent->collapsed = true;
		SetItemState(itemPos, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
		SetRedraw(true);
	}

	void Expand(T* parent, int itemPos) {
		SetRedraw(false);
		const auto& children = findChildren(parent->getGroupCond());
		if(children.size() > (size_t)(uniqueParent ? 1 : 0)) {
			parent->collapsed = false;
			for(const auto c: children)
				insertChild(c, itemPos + 1);

			SetItemState(itemPos, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
			resort();
		}
		SetRedraw(true);
	}

	void insertChild(const T* item, int idx) {
		LV_ITEM lvi;
		lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE | LVIF_INDENT;
		lvi.iItem = idx;
		lvi.iSubItem = 0;
		lvi.iIndent = 1;
		lvi.pszText = LPSTR_TEXTCALLBACK;
		lvi.iImage = item->getImageIndex();
		lvi.lParam = (LPARAM)item;
		lvi.state = 0;
		lvi.stateMask = 0;
		InsertItem(&lvi);
	}

	inline T* findParent(const K& groupCond) const {
		ParentMap::const_iterator i = parents.find(const_cast<K*>(&groupCond));
		return i != parents.end() ? (*i).second.parent : NULL;
	}

	static const vector<T*> emptyVector;
	inline const vector<T*>& findChildren(const K& groupCond) const {
		ParentMap::const_iterator i = parents.find(const_cast<K*>(&groupCond));
		return i != parents.end() ? (*i).second.children : emptyVector;
	}

	inline ParentPair* findParentPair(const K& groupCond) {
		ParentMap::iterator i = parents.find(const_cast<K*>(&groupCond));
		return i != parents.end() ? &((*i).second) : NULL;
	}

	void insertGroupedItem(T* item, bool autoExpand) {
		T* parent = NULL;
		ParentPair* pp = findParentPair(item->getGroupCond());

		int pos = -1;

		if(pp == NULL) {
			parent = item;

			ParentPair newPP = { parent };
			parents.emplace(const_cast<K*>(&parent->getGroupCond()), newPP);

			parent->parent = NULL; // ensure that parent of this item is really NULL
			insertItem(getSortPos(parent), parent, parent->getImageIndex());
			return;
		} else if(pp->children.empty()) {
			T* oldParent = pp->parent;
			parent = oldParent->createParent();
			if(parent != oldParent) {
				uniqueParent = true;
				parents.erase(const_cast<K*>(&oldParent->getGroupCond()));
				deleteItem(oldParent);

				ParentPair newPP = { parent };
				pp = &(parents.emplace(const_cast<K*>(&parent->getGroupCond()), newPP).first->second);

				parent->parent = NULL; // ensure that parent of this item is really NULL
				oldParent->parent = parent;
				pp->children.push_back(oldParent); // mark old parent item as a child
				parent->hits++;

				pos = insertItem(getSortPos(parent), parent, parent->getImageIndex());
			} else {
				uniqueParent = false;
				pos = findItem(parent);
			}

			if(pos != -1)
			{
				if(autoExpand){
					SetItemState(pos, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
					parent->collapsed = false;
				} else {
					SetItemState(pos, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
				}
			}
		} else {
			parent = pp->parent;
			pos = findItem(parent);
		}

		pp->children.push_back(item);
		parent->hits++;
		item->parent = parent;

		if(pos != -1) {
			if(!parent->collapsed) {
				insertChild(item, pos + pp->children.size());
			}
			updateItem(pos);
		}
	}


	void insertBundle(T* item, bool autoExpand) {
		T* parent = NULL;
		ParentPair* pp = findParentPair(item->getGroupCond());

		int pos = -1;

		if(pp == NULL) {
			//LogManager::getInstance()->message("insertGroupedItem pp NULL BUNDLE");
			parent  = item->createParent();
			uniqueParent = false;

			ParentPair newPP = { parent };
			pp = &(parents.emplace(const_cast<K*>(&parent->getGroupCond()), newPP).first->second);

			parent->parent = NULL; // ensure that parent of this item is really NULL
			parent->hits++;

			pos = insertItem(getSortPos(parent), parent, parent->getImageIndex());

			if(pos != -1) {
				if(autoExpand){
					SetItemState(pos, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
					parent->collapsed = false;
				} else {
					SetItemState(pos, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
				}
			}
		} else {
			//LogManager::getInstance()->message("insertGroupedItem else");
			parent = pp->parent;
			pos = findItem(parent);
		}

		pp->children.push_back(item);
		parent->hits++;
		item->parent = parent;

		if(pos != -1) {
			if(!parent->collapsed) {
				insertChild(item, pos + pp->children.size());
			}
			updateItem(pos);
		}
	}


	bool removeBundle(T* item, bool removeFromMemory = true) {
		bool parentExists=true;
		if(!item->parent) {
			//LogManager::getInstance()->message("remove if");
			removeParent(item);
		} else {
			//LogManager::getInstance()->message("remove else");
			T* parent = item->parent;
			ParentPair* pp = findParentPair(parent->getGroupCond());

			deleteItem(item);

			vector<T*>::iterator n = find(pp->children.begin(), pp->children.end(), item);
			if(n != pp->children.end()) {
				pp->children.erase(n);
				pp->parent->hits--;
			}
	

			if (pp->children.empty()) {
				//LogManager::getInstance()->message("remove unique parent else");
				deleteItem(parent);
				parents.erase(const_cast<K*>(&parent->getGroupCond()));
				delete parent;
				parentExists=false;
			} else {
				updateItem(parent);
			}
		}

		if(removeFromMemory)
			delete item;
		return parentExists;
	}


	void removeParent(T* parent) {
		ParentPair* pp = findParentPair(parent->getGroupCond());
		if(pp) {
			for(auto i: pp->children) {
				deleteItem(i);
				delete i;
			}
			pp->children.clear();
			parents.erase(const_cast<K*>(&parent->getGroupCond()));
		}
		deleteItem(parent);
	}

	void removeGroupedItem(T* item, bool removeFromMemory = true) {
		if(!item->parent) {
			removeParent(item);
		} else {
			T* parent = item->parent;
			ParentPair* pp = findParentPair(parent->getGroupCond());

			deleteItem(item);

			vector<T*>::iterator n = find(pp->children.begin(), pp->children.end(), item);
			if(n != pp->children.end()) {
				pp->children.erase(n);
				pp->parent->hits--;
			}
	
			if(uniqueParent) {
				dcassert(!pp->children.empty());
				if(pp->children.size() == 1) {
					const T* oldParent = parent;
					parent = pp->children.front();

					deleteItem(oldParent);
					parents.erase(const_cast<K*>(&oldParent->getGroupCond()));
					delete oldParent;

					ParentPair newPP = { parent };
					parents.emplace(const_cast<K*>(&parent->getGroupCond()), newPP);

					parent->parent = NULL; // ensure that parent of this item is really NULL
					deleteItem(parent);
					insertItem(getSortPos(parent), parent, parent->getImageIndex());
				}
			} else {
				if(pp->children.empty()) {
					SetItemState(findItem(parent), INDEXTOSTATEIMAGEMASK(0), LVIS_STATEIMAGEMASK);
				}
			}

			updateItem(parent);
		}

		if(removeFromMemory)
			delete item;
	}

	void deleteAllItems() {
		// HACK: ugly hack but at least it doesn't crash and there's no memory leak
		for(auto p: parents | map_values) {
			for(auto c: p.children) {
				deleteItem(c);
				delete c;
			}
			deleteItem(p.parent);
			delete p.parent;
		}
		for(int i = 0; i < GetItemCount(); i++) {
			T* si = getItemData(i);
			delete si;
		}

 		parents.clear();
		DeleteAllItems();
	}

	LRESULT onColumnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLISTVIEW* l = (NMLISTVIEW*)pnmh;
		if(l->iSubItem != getSortColumn()) {
			setAscending(true);
			setSortColumn(l->iSubItem);
		} else if(isAscending()) {
			setAscending(false);
		} else {
			setSortColumn(-1);
		}		
		resort();
		return 0;
	}

	void resort() {
		if(getSortColumn() != -1) {
			SortItems(&compareFunc, (LPARAM)this);
		}
	}

	int getSortPos(const T* a) {
		int high = GetItemCount();
		if((getSortColumn() == -1) || (high == 0))
			return high;

		high--;

		int low = 0;
		int mid = 0;
		T* b = NULL;
		int comp = 0;
		while( low <= high ) {
			mid = (low + high) / 2;
			b = getItemData(mid);
			comp = compareItems(a, b, static_cast<uint8_t>(getSortColumn()));
			
			if(!isAscending())
				comp = -comp;

			if(comp == 0) {
				return mid;
			} else if(comp < 0) {
				high = mid - 1;
			} else if(comp > 0) {
				low = mid + 1;
			} else if(comp == 2){
				if(isAscending())
					low = mid + 1;
				else
					high = mid -1;
			} else if(comp == -2){
				if(!isAscending())
					low = mid + 1;
				else
					high = mid -1;
			}
		}

		comp = compareItems(a, b, static_cast<uint8_t>(getSortColumn()));
		if(!isAscending())
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
		while( (i = GetNextItem(i, LVNI_SELECTED)) != -1) {
			auto d = getItemData(i);
			if (!d->parent) {
				doneParents.insert(d->getGroupCond());
				pred(d);
			} else if (doneParents.find(d->getGroupCond()) == doneParents.end()) {
				pred(d);
			}
		}
		return pred;
	}
private:

   	/** map of all parent items with their associated children */
	ParentMap	parents;

	/** +/- images */
	CImageList	states;

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
};

template<class T, int ctrlId, class K, class hashFunc, class equalKey>
const vector<T*> TypedTreeListViewCtrl<T, ctrlId, K, hashFunc, equalKey>::emptyVector;

#endif // !defined(TYPED_LIST_VIEW_CTRL_H)
