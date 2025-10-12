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


#ifndef DCPP_WINGUI_FINISHED_FRAME_BASE
#define DCPP_WINGUI_FINISHED_FRAME_BASE

#include <windows/stdafx.h>
#include <windows/Resource.h>

#include <windows/FlatTabCtrl.h>
#include <windows/components/TypedListViewCtrl.h>
#include <windows/components/ShellContextMenu.h>
#include <windows/util/ActionUtil.h>
#include <windows/util/WinUtil.h>
#include <windows/ResourceLoader.h>
#include <windows/frames/text/TextFrame.h>

#include <airdcpp/user/UserInfoBase.h>

#include <airdcpp/favorites/FavoriteUserManager.h>
#include <airdcpp/favorites/ReservedSlotManager.h>

#include <airdcpp/modules/FinishedManager.h>

namespace wingui {

static const ResourceManager::Strings columnNames[] = { 
	ResourceManager::FILENAME, ResourceManager::TIME, ResourceManager::PATH,
	ResourceManager::NICK, ResourceManager::HUB, ResourceManager::SIZE, ResourceManager::SPEED, ResourceManager::TYPE
};

template<class T, int title, int Idc>
class FinishedFrameBase : public MDITabChildWindowImpl<T>, public StaticFrame<T, title, Idc>,
	protected FinishedManagerListener, private SettingsManagerListener, protected Async<FinishedFrameBase<T, title, Idc>>
{

public:
	class ItemInfo : UserInfoBase {
	public:
		explicit ItemInfo(const FinishedItemPtr& aFinishedItem) : finishedItem(aFinishedItem) {}
		~ItemInfo() = default;

		static int compareItems(const ItemInfo* a, const ItemInfo* b, uint8_t col) noexcept {
			switch (col) {
			case COLUMN_SPEED:	return compare(a->finishedItem->getAvgSpeed(), b->finishedItem->getAvgSpeed());
			case COLUMN_SIZE:	return compare(a->finishedItem->getSize(), b->finishedItem->getSize());
			default:			return Util::stricmp(a->getText(col).c_str(), b->getText(col).c_str());
			}
		}

		enum {
			COLUMN_FIRST,
			COLUMN_FILE = COLUMN_FIRST,
			COLUMN_DONE,
			COLUMN_PATH,
			COLUMN_NICK,
			COLUMN_HUB,
			COLUMN_SIZE,
			COLUMN_SPEED,
			COLUMN_TYPE,
			COLUMN_LAST
		};

		const tstring getText(uint8_t col) const noexcept {
			dcassert(col >= 0 && col < COLUMN_LAST);
			switch (col) {
			case COLUMN_FILE: return Text::toT(PathUtil::getFileName(finishedItem->getTarget()));
			case COLUMN_DONE: return FormatUtil::formatDateTimeW(finishedItem->getTime());
			case COLUMN_PATH: return Text::toT(PathUtil::getFilePath(finishedItem->getTarget()));
			case COLUMN_NICK: return Text::toT(ClientManager::getInstance()->getFormattedNicks(finishedItem->getUser()));
			case COLUMN_HUB: {
				if (getUser()->isOnline()) {
					return Text::toT(ClientManager::getInstance()->getFormattedHubNames(finishedItem->getUser()));
				} else {
					auto ofu = ClientManager::getInstance()->getOfflineUser(getUser()->getCID());
					return TSTRING(OFFLINE) + (ofu ? _T(" ( ") + Text::toT(ofu->getUrl()) + _T(" ) ") : _T(""));
				}
			}
			case COLUMN_SIZE: return Util::formatBytesW(finishedItem->getSize());
			case COLUMN_SPEED: return Util::formatBytesW(finishedItem->getAvgSpeed()) + _T("/s");
			case COLUMN_TYPE: {
				tstring filetype = Text::toT(PathUtil::getFileExt(Text::fromT(getText(COLUMN_FILE))));
				if (!filetype.empty() && filetype[0] == _T('.'))
					filetype.erase(0, 1);
				return filetype;
			}
			default: return Util::emptyStringT;
			}
		}

		int getImageIndex() const noexcept {
			return ResourceLoader::getIconIndex(Text::toT(finishedItem->getTarget()));
		}

		const UserPtr& getUser() const noexcept override { return finishedItem->getUser().user; }
		const string& getHubUrl() const noexcept override { return finishedItem->getUser().hint; }

		const FinishedItemPtr finishedItem = nullptr;
	};

	typedef MDITabChildWindowImpl<T> baseClass;

	FinishedFrameBase() { }
	virtual ~FinishedFrameBase() { }

	using ItemInfoMap = unordered_map<FinishedItemToken, unique_ptr<ItemInfo>>;
	ItemInfoMap itemInfos;

	BEGIN_MSG_MAP(T)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_TOTAL, onRemove)
		COMMAND_ID_HANDLER(IDC_VIEW_AS_TEXT, onViewAsText)
		COMMAND_ID_HANDLER(IDC_GETLIST, onGetList)
		COMMAND_ID_HANDLER(IDC_GRANTSLOT, onGrant)		
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		NOTIFY_HANDLER(Idc, LVN_GETDISPINFO, ctrlList.onGetDispInfo)
		NOTIFY_HANDLER(Idc, LVN_COLUMNCLICK, ctrlList.onColumnClick)
		NOTIFY_HANDLER(Idc, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(Idc, NM_DBLCLK, onDoubleClick)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		this->CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
		ctrlStatus.Attach(this->m_hWndStatusBar);

		ctrlList.Create(this->m_hWnd, this->rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
			WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, Idc);
		ctrlList.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

		ctrlList.SetImageList(ResourceLoader::getFileImages(), LVSIL_SMALL);
		ctrlList.SetBkColor(WinUtil::bgColor);
		ctrlList.SetTextBkColor(WinUtil::bgColor);
		ctrlList.SetTextColor(WinUtil::textColor);
		ctrlList.SetFont(WinUtil::listViewFont);

		// Create listview columns
		WinUtil::splitTokens(columnIndexes, SettingsManager::getInstance()->get(columnOrder), ItemInfo::COLUMN_LAST);
		WinUtil::splitTokens(columnSizes, SettingsManager::getInstance()->get(columnWidth), ItemInfo::COLUMN_LAST);

		for(uint8_t j = 0; j < ItemInfo::COLUMN_LAST; j++) {
			int fmt = (j == ItemInfo::COLUMN_SIZE || j == ItemInfo::COLUMN_SPEED) ? LVCFMT_RIGHT : LVCFMT_LEFT;
			ctrlList.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
		}

		ctrlList.SetColumnOrderArray(ItemInfo::COLUMN_LAST, columnIndexes);
		ctrlList.setVisible(SettingsManager::getInstance()->get(columnVisible));
		ctrlList.setSortColumn(ItemInfo::COLUMN_DONE);

		this->UpdateLayout();

		SettingsManager::getInstance()->addListener(this);
		FinishedManager::getInstance()->addListener(this);
		updateList(FinishedManager::getInstance()->lockList());
		FinishedManager::getInstance()->unlockList();

		CRect rc(SETTING(FINISHED_LEFT), SETTING(FINISHED_TOP), SETTING(FINISHED_RIGHT), SETTING(FINISHED_BOTTOM));
		if(! (rc.top == 0 && rc.bottom == 0 && rc.left == 0 && rc.right == 0) )
			this->MoveWindow(rc, TRUE);

		WinUtil::SetIcon(this->m_hWnd, iIcon);
		bHandled = FALSE;
		return TRUE;
	}


	LRESULT onSpeaker(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
		return Async<FinishedFrameBase<T, title, Idc>>::onSpeaker(uMsg, wParam, lParam, bHandled);
	}

LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		this->PostMessage(WM_CLOSE);
		return 0;
	}

LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		if(!closed) {
			FinishedManager::getInstance()->removeListener(this);
			SettingsManager::getInstance()->removeListener(this);

			closed = true;
			WinUtil::setButtonPressed(Idc, false);
			this->PostMessage(WM_CLOSE);
			return 0;
		} else {
			
			ctrlList.saveHeaderOrder(columnOrder, columnWidth, columnVisible);
		
			this->frame = NULL;
			ctrlList.DeleteAllItems();
			CRect rc;
			if(!this->IsIconic()){
				//Get position of window
				this->GetWindowRect(&rc);
					
				//convert the position so it's relative to main window
				::ScreenToClient(this->GetParent(), &rc.TopLeft());
				::ScreenToClient(this->GetParent(), &rc.BottomRight());
				
				//save the position
				SettingsManager::getInstance()->set(SettingsManager::FINISHED_BOTTOM, (rc.bottom > 0 ? rc.bottom : 0));
				SettingsManager::getInstance()->set(SettingsManager::FINISHED_TOP, (rc.top > 0 ? rc.top : 0));
				SettingsManager::getInstance()->set(SettingsManager::FINISHED_LEFT, (rc.left > 0 ? rc.left : 0));
				SettingsManager::getInstance()->set(SettingsManager::FINISHED_RIGHT, (rc.right > 0 ? rc.right : 0));
			}

			bHandled = FALSE;
			return 0;
		}
	}

	LRESULT onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMITEMACTIVATE * const item = (NMITEMACTIVATE*) pnmh;

		if(item->iItem != -1) {
			auto ii = ctrlList.getItemData(item->iItem);
			ActionUtil::openFile(Text::toT(ii->finishedItem->getTarget()));
		}
		return 0;
	}

	void addLine(const FinishedItemPtr& aItem) {
		addEntry(aItem);

		if (SettingsManager::getInstance()->get(boldFinished))
			this->setDirty();

		updateStatus();
	}

	LRESULT onRemove(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		
		switch(wID)
		{
		case IDC_REMOVE:
			{
				vector<ItemInfo*> removeList;
				ctrlList.SetRedraw(FALSE);
				int i = -1;
				while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
					removeList.push_back(ctrlList.getItemData(i));
				}

				for (auto ii : removeList) {
					FinishedManager::getInstance()->remove(ii->finishedItem);
					ctrlList.deleteItem(ii);

					totalBytes -= ii->finishedItem->getSize();
					totalSpeed -= ii->finishedItem->getAvgSpeed();

					itemInfos.erase(ii->finishedItem->getToken());
				}

				updateStatus();
				ctrlList.SetRedraw(TRUE);
				break;

			}
		case IDC_TOTAL:
			ctrlList.SetRedraw(FALSE);
			FinishedManager::getInstance()->removeAll();
			
			ctrlList.DeleteAllItems();

			for( int i = 0; i < ctrlList.GetItemCount(); ++i ) {
					delete ctrlList.getItemData(i);
			}
			
			totalBytes = 0;
			totalSpeed = 0;
			updateStatus();
			ctrlList.SetRedraw(TRUE);
			break;
		}

		return 0;
	}

	LRESULT onViewAsText(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		int i;
		if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			auto ii = ctrlList.getItemData(i);
			if (ii) {
				TextFrame::openFile(ii->finishedItem->getTarget());
			}
		}
		return 0;
	}


	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
		if (reinterpret_cast<HWND>(wParam) == ctrlList && ctrlList.GetSelectedCount() > 0) { 
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

			if(pt.x == -1 && pt.y == -1) {
				WinUtil::getContextMenuPos(ctrlList, pt);
			}

			ShellMenu shellMenu;
			shellMenu.CreatePopupMenu();

			if(ctrlList.GetSelectedCount() == 1) {
				shellMenu.AppendMenu(MF_STRING, IDC_VIEW_AS_TEXT, CTSTRING(VIEW_AS_TEXT));
				shellMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT, CTSTRING(GRANT_EXTRA_SLOT));
				shellMenu.AppendMenu(MF_SEPARATOR);

				auto ii = ctrlList.getSelectedItem();
				auto path = ii->finishedItem->getTarget();
				shellMenu.appendShellMenu({ path });
			}		
			ctrlList.appendCopyMenu(shellMenu);

			shellMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
			shellMenu.AppendMenu(MF_STRING, IDC_TOTAL, CTSTRING(REMOVE_ALL));
			shellMenu.open(this->m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
			return TRUE; 
		}
		bHandled = FALSE;
		return FALSE; 
	}

	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLVKEYDOWN* kd = reinterpret_cast<NMLVKEYDOWN*>(pnmh);

		if(kd->wVKey == VK_DELETE) {
			this->PostMessage(WM_COMMAND, IDC_REMOVE);
		} 
		return 0;
	}

	LRESULT onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		int i;
		if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			auto ii = ctrlList.getItemData(i);
			if(ii->getUser()->isOnline()) {
				ActionUtil::GetList()(ii->getUser(), ii->finishedItem->getUser().hint);
			} else {
				addStatusLine(TSTRING(USER_OFFLINE));
			}
		}
		return 0;
	}

	LRESULT onGrant(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		int i;
		if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			auto ii = ctrlList.getItemData(i);
			if (ii->getUser()->isOnline()) {
				FavoriteUserManager::getInstance()->getReservedSlots().reserveSlot(ii->finishedItem->getUser(), 600);
			} else {
				addStatusLine(TSTRING(USER_OFFLINE));
			}
		}
		return 0;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE) {
		RECT rect;
		this->GetClientRect(&rect);

		// position bars and offset their dimensions
		this->UpdateBarsPosition(rect, bResizeBars);

		if(ctrlStatus.IsWindow()) {
			CRect sr;
			int w[4];
			ctrlStatus.GetClientRect(sr);
			w[3] = sr.right - 16;
			w[2] = max(w[3] - 100, 0);
			w[1] = max(w[2] - 100, 0);
			w[0] = max(w[1] - 100, 0);

			ctrlStatus.SetParts(4, w);
		}

		CRect rc(rect);
		ctrlList.MoveWindow(rc);
	}

	LRESULT onSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) {
		ctrlList.SetFocus();
		return 0;
	}
protected:
	CStatusBarCtrl ctrlStatus;
	
	TypedListViewCtrl<ItemInfo, Idc> ctrlList;

	int64_t totalBytes = 0;
	int64_t totalSpeed = 0;

	bool closed = false;

	bool upload;
	int iIcon;
	SettingsManager::BoolSetting boldFinished;
	SettingsManager::StrSetting columnWidth;
	SettingsManager::StrSetting columnOrder;
	SettingsManager::StrSetting columnVisible;
	

	static int columnSizes[ItemInfo::COLUMN_LAST];
	static int columnIndexes[ItemInfo::COLUMN_LAST];

	void addStatusLine(const tstring& aLine) {
		ctrlStatus.SetText(0, WinUtil::formatMessageWithTimestamp(aLine).c_str());
	}

	void updateStatus() {
		int count = ctrlList.GetItemCount();
		ctrlStatus.SetText(1, (WinUtil::toStringW(count) + _T(" ") + TSTRING(ITEMS)).c_str());
		ctrlStatus.SetText(2, Util::formatBytesW(totalBytes).c_str());
		ctrlStatus.SetText(3, (Util::formatBytesW(count > 0 ? totalSpeed / count : 0) + _T("/s")).c_str());
	}

	void updateList(const FinishedItemList& fl) {
		ctrlList.SetRedraw(FALSE);
		for(const auto& fi: fl) {
			addEntry(fi);
		}
		ctrlList.SetRedraw(TRUE);
		ctrlList.Invalidate();
		updateStatus();
	}

	void addEntry(const FinishedItemPtr& entry) {
		totalBytes += entry->getSize();
		totalSpeed += entry->getAvgSpeed();

		auto ii = std::make_unique<ItemInfo>(entry);
		auto image = ii->getImageIndex();
		ctrlList.insertItem(ii.get(), image);

		itemInfos.emplace(entry->getToken(), std::move(ii));
	}

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
		bool refresh = false;
		if(ctrlList.GetBkColor() != WinUtil::bgColor) {
			ctrlList.SetBkColor(WinUtil::bgColor);
			ctrlList.SetTextBkColor(WinUtil::bgColor);
			refresh = true;
		}
		if(ctrlList.GetTextColor() != WinUtil::textColor) {
			ctrlList.SetTextColor(WinUtil::textColor);
			refresh = true;
		}

		if (ctrlList.GetFont() != WinUtil::listViewFont){
			ctrlList.SetFont(WinUtil::listViewFont);
			refresh = true;
		}

		if(refresh == true) {
			this->RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
		}
	}

};

template <class T, int title, int Idc>
int FinishedFrameBase<T, title, Idc>::columnIndexes[] = { 
	FinishedFrameBase::ItemInfo::COLUMN_DONE, FinishedFrameBase::ItemInfo::COLUMN_FILE,
	FinishedFrameBase::ItemInfo::COLUMN_PATH, FinishedFrameBase::ItemInfo::COLUMN_NICK,
	FinishedFrameBase::ItemInfo::COLUMN_HUB, FinishedFrameBase::ItemInfo::COLUMN_SIZE,
	FinishedFrameBase::ItemInfo::COLUMN_SPEED, FinishedFrameBase::ItemInfo::COLUMN_TYPE  
};

template <class T, int title, int Idc>
int FinishedFrameBase<T, title, Idc>::columnSizes[] = { 100, 110, 290, 125, 80, 80 };

}

#endif // !defined(DCPP_WINGUI_FINISHED_FRAME_BASE)