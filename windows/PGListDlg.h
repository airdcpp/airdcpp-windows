#if !defined(PG_LIST_DLG_H)
#define PG_LIST_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/PGManager.h"
#include "WinUtil.h"
#include <atlctrlx.h>

class PGListDlg : public CDialogImpl<PGListDlg>
{
public:
	enum { IDD = IDD_PG_LIST };

	PGListDlg() : m_hIcon(NULL) { };
	~PGListDlg() {
		if(m_hIcon)
			DeleteObject((HGDIOBJ)m_hIcon);
	}

	BEGIN_MSG_MAP(PGListDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColor)
		MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
		COMMAND_ID_HANDLER(IDC_TEST, OnTest)
		COMMAND_ID_HANDLER(IDC_CURRENT_LIST_BROWSE, OnListBrowse)
		COMMAND_ID_HANDLER(IDC_COPY_DESCRIPTION, OnCopy)
		COMMAND_ID_HANDLER(IDC_COPY_BEGIN, OnCopy)
		COMMAND_ID_HANDLER(IDC_COPY_END, OnCopy)
		COMMAND_ID_HANDLER(IDC_COPY_ALL, OnCopy)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		// load icon
		m_hIcon = ::LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDR_PEERGUARDIAN));
		SetIcon(m_hIcon, FALSE);
		SetIcon(m_hIcon, TRUE);

		// translate dialog
		SetWindowText(CTSTRING(PG_LIST_VIEWER));
		SetDlgItemText(IDC_LIST_VIEWER_GB, CTSTRING(PG_LIST_VIEWER));
		SetDlgItemText(IDC_LIST_OPTIONS, CTSTRING(PG_LIST_OPTIONS));
		SetDlgItemText(IDC_CURRENT_LIST_STR, CTSTRING(PG_CURRENT_LIST));
		SetDlgItemText(IDC_CURRENT_LIST_BROWSE, CTSTRING(BROWSE));
		SetDlgItemText(IDC_UPDATE_LOCATION_STR, CTSTRING(PG_UPDATE_LOCATION));
		SetDlgItemText(IDC_LOADING, CTSTRING(PG_VIEWER_LOADING));
		SetDlgItemText(IDC_TEST, CTSTRING(FIND));
		SetDlgItemText(IDC_COUNT, Text::toT(STRING(PG_RANGE_COUNT) + ": 0").c_str());
		SetDlgItemText(IDC_LIST_NAME, Text::toT(Util::getFileName(SETTING(PG_FILE))).c_str());
		SetDlgItemText(IDCANCEL, CTSTRING(CANCEL));
		SetDlgItemText(IDOK, CTSTRING(OK));

		// set values from settings
		SetDlgItemText(IDC_CURRENT_LIST, Text::toT(SETTING(PG_FILE)).c_str());
		SetDlgItemText(IDC_UPDATE_LOCATION, Text::toT(SETTING(PG_UPDATE_URL)).c_str());

		CRect rc;
		ctrlList.Attach(GetDlgItem(IDC_PG_LIST));
		ctrlList.GetClientRect(rc);
		ctrlList.InsertColumn(0, CTSTRING(DESCRIPTION), LVCFMT_LEFT, (rc.Width() / 3) - 6, 0);
		ctrlList.InsertColumn(1, CTSTRING(BEGIN), LVCFMT_LEFT, (rc.Width() / 3) - 6, 1);
		ctrlList.InsertColumn(2, CTSTRING(END), LVCFMT_LEFT, (rc.Width() / 3) - 6, 2);
		ctrlList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);
		ctrlList.setSort(0, ExListViewCtrl::SORT_STRING_NOCASE);

		populated = false;
		loading = false;
		abort = false;

		ThreadedPGList* tpgl = new ThreadedPGList(this);
		tpgl->start();

		CenterWindow(GetParent());
		return TRUE;
	}
		
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(loading && !populated) {
			abort = true;
			return 0;
		}

		if(wID == IDOK) {
			TCHAR buf[512];

			GetDlgItemText(IDC_CURRENT_LIST, buf, sizeof(buf));
			SettingsManager::getInstance()->set(SettingsManager::PG_FILE, Text::fromT(buf));
			GetDlgItemText(IDC_UPDATE_LOCATION, buf, sizeof(buf));
			SettingsManager::getInstance()->set(SettingsManager::PG_UPDATE_URL, Text::fromT(buf));
		}

		ctrlList.DeleteAllItems();
		ctrlList.Detach();
		EndDialog(wID);
		return 0;
	}

	LRESULT OnCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
		HWND hWnd = (HWND)lParam;
		HDC hDC = (HDC)wParam;

		if(hWnd == GetDlgItem(IDC_LOADING)) {
			::SetBkColor(hDC, ::GetSysColor(COLOR_WINDOW));
			::SetTextColor(hDC, ::GetSysColor(COLOR_WINDOWTEXT));
			return (LRESULT)::GetSysColorBrush(COLOR_WINDOW);
		} else {
			::SetBkColor(hDC, ::GetSysColor(COLOR_3DFACE));
			::SetTextColor(hDC, ::GetSysColor(COLOR_BTNTEXT));
			return (LRESULT)::GetSysColorBrush(COLOR_3DFACE);
		}
	}

	LRESULT OnTest(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		TCHAR buf[16];
		GetDlgItemText(IDC_SEARCH_IP, buf, 16);
		tstring tmp = PGManager::getInstance()->getIPBlock(buf);
		if(!tmp.empty()) {
			int i = -1;
			while((i = ctrlList.find(tmp, i)) != -1) {
				unsigned int input, begin, end;
				GetDlgItemText(IDC_SEARCH_IP, buf, 16);
				input = inet_addr(Text::fromT(buf).c_str());
				ctrlList.GetItemText(i, 1, buf, 16);
				begin = inet_addr(Text::fromT(buf).c_str());
				ctrlList.GetItemText(i, 2, buf, 16);
				end = inet_addr(Text::fromT(buf).c_str());

				if(input >= begin && input <= end) {
					ctrlList.SelectItem(i);
					ctrlList.SetFocus();
					break;
				}
			}
		} else {
			MessageBox(CTSTRING(PG_TESTER_NO_MATCH), CTSTRING(PG_TESTER), 0);
		}
		return 0;
	}

	LRESULT OnListBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		const TCHAR types[] = _T("PeerGuardian Blocklists (.p2b .p2p .txt)\0*.p2b;*.p2p;*.txt\0All Files\0*.*\0");
		TCHAR buf[MAX_PATH];

		GetDlgItemText(IDC_CURRENT_LIST, buf, MAX_PATH);
		tstring x = buf;

		if(WinUtil::browseFile(x, m_hWnd, false, Util::emptyStringT, types) == IDOK) {
			SetDlgItemText(IDC_CURRENT_LIST, x.c_str());
			SetDlgItemText(IDC_LIST_NAME, Util::getFileName(x).c_str());
		}
		return 0;
	}

	LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
		if(reinterpret_cast<HWND>(wParam) == ctrlList && ctrlList.GetSelectedCount() == 1 && ::IsWindowVisible(GetDlgItem(IDC_LOADING)) == false) {
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

			if(pt.x == -1 && pt.y == -1) {
				WinUtil::getContextMenuPos(ctrlList, pt);
			}

			CMenu mnu;
			mnu.CreatePopupMenu();
			mnu.AppendMenu(MF_STRING, IDC_COPY_DESCRIPTION, CTSTRING(COPY_DESCRIPTION));
			mnu.AppendMenu(MF_STRING, IDC_COPY_BEGIN, Text::toT(STRING(COPY) + " " + STRING(BEGIN)).c_str());
			mnu.AppendMenu(MF_STRING, IDC_COPY_END, Text::toT(STRING(COPY) + " " + STRING(END)).c_str());
			mnu.AppendMenu(MF_SEPARATOR);
			mnu.AppendMenu(MF_STRING, IDC_COPY_ALL, CTSTRING(COPY_ALL));

			mnu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

			return TRUE;
		}

		bHandled = FALSE;
		return FALSE; 
	}

	LRESULT OnCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		tstring sCopy;
		TCHAR buf[512];
		int sel = ctrlList.GetNextItem(-1, LVNI_SELECTED);

		if(sel != -1) {
			switch (wID) {
				case IDC_COPY_DESCRIPTION:
					ctrlList.GetItemText(sel, 0, buf, 512);
					sCopy += buf;
					break;
				case IDC_COPY_BEGIN:
					ctrlList.GetItemText(sel, 1, buf, 512);
					sCopy += buf;
					break;
				case IDC_COPY_END:
					ctrlList.GetItemText(sel, 2, buf, 512);
					sCopy += buf;
					break;
				case IDC_COPY_ALL:
					ctrlList.GetItemText(sel, 0, buf, 512);
					sCopy += buf; 
					sCopy += _T(":");
					ctrlList.GetItemText(sel, 1, buf, 512);
					sCopy += buf;
					sCopy += _T("-");
					ctrlList.GetItemText(sel, 2, buf, 512);
					sCopy += buf;
					sCopy += _T("\r\n");
					break;
				default:
					dcdebug("We don't go here! (PGListDlg)\n");
					return 0;
			}
			if(!sCopy.empty())
				WinUtil::setClipboard(sCopy);
		}

		return 0;
	}

private:
	PGListDlg(const PGListDlg&) { dcassert(0); };
	ExListViewCtrl ctrlList;
	HICON m_hIcon;
	bool populated;
	bool loading;
	bool abort;

	void updateList(const PGManager::range::list* ranges) {
		ctrlList.SetRedraw(FALSE);
		int quarter = ranges->size() / 4;
		int status = (quarter >= 5000) ? 0 : -1;
		TStringList lst;

		for(PGManager::range::iter i = ranges->begin(); i != ranges->end(); ++i) {
			if(abort || !loading) {
				throw AbortException();
			}

			lst.push_back((*i)->name);
			lst.push_back(PGManager::getInstance()->getIPTStr((*i)->start));
			lst.push_back(PGManager::getInstance()->getIPTStr((*i)->end));
			ctrlList.insert(ctrlList.GetItemCount(), lst);
			lst.clear();

			if(ctrlList.GetItemCount() >= (quarter * 4) && status == 4) {
				SetDlgItemText(IDC_LOADING, Text::toT(STRING(PG_VIEWER_LOADING) + " (100%)").c_str());
				status = 5;
			} else if(ctrlList.GetItemCount() >= (quarter * 3) && status == 3) {
				SetDlgItemText(IDC_LOADING, Text::toT(STRING(PG_VIEWER_LOADING) + " (75%)").c_str());
				status = 4;
			} else if(ctrlList.GetItemCount() >= (quarter * 2) && status == 2) {
				SetDlgItemText(IDC_LOADING, Text::toT(STRING(PG_VIEWER_LOADING) + " (50%)").c_str());
				status = 3;
			} else if(ctrlList.GetItemCount() >= quarter && status == 0) {
				SetDlgItemText(IDC_LOADING, Text::toT(STRING(PG_VIEWER_LOADING) + " (25%)").c_str());
				status = 2;
			} else if(ctrlList.GetItemCount() <= 2) {
				// this is done twice here to ensure that the control really is visible
				SetDlgItemText(IDC_LOADING, CTSTRING(PG_VIEWER_LOADING));
			}
		}

		ctrlList.resort();
		ctrlList.SetRedraw(TRUE);
	}

	/*
	 * Used for "background" loading
	 * simply calls above updateList()
	 */
	class ThreadedPGList : public Thread
	{
	public:
		ThreadedPGList(PGListDlg* pDlg) : mDlg(pDlg) { }
	protected:
		PGListDlg* mDlg;
	private:
		int run() {
			setThreadPriority(Thread::LOW);
			try {
				mDlg->loading = true;
				mDlg->updateList(PGManager::getInstance()->getRanges());
				mDlg->populated = true;
		
				// update some texts
				if(mDlg->ctrlList.GetItemCount() != 0) {
					mDlg->SetDlgItemText(IDC_COUNT, (TSTRING(PG_RANGE_COUNT) + _T(": ") + PGManager::getInstance()->getTotalIPRangesTStr()).c_str());
					::EnableWindow(mDlg->GetDlgItem(IDC_TEST), true);
					::EnableWindow(mDlg->GetDlgItem(IDC_SEARCH_IP), true);
					::ShowWindow(mDlg->GetDlgItem(IDC_LOADING), false);
				} else {
					mDlg->SetDlgItemText(IDC_LOADING, CTSTRING(PG_VIEWER_EMPTY));
				}
			} catch(const AbortException) {
				mDlg->loading = false;
				mDlg->PostMessage(WM_COMMAND, IDOK);
			} catch(const Exception&) {
				mDlg->loading = false;
				mDlg->PostMessage(WM_COMMAND, IDOK);
			}

			delete this;
			return 0;
		}
	};

};

#endif // !defined(PG_LIST_DLG_H)
