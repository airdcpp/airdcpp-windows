#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "CDMDebugFrame.h"
#include "WinUtil.h"
#include "../client/File.h"

#define MAX_TEXT_LEN 131072

LRESULT CDMDebugFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	ctrlPad.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY, WS_EX_CLIENTEDGE);
	ctrlPad.LimitText(0);
	ctrlPad.SetFont(WinUtil::font);
	
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	statusContainer.SubclassWindow(ctrlStatus.m_hWnd);

	ctrlClear.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_PUSHBUTTON, 0, IDC_CLEAR);
	ctrlClear.SetWindowText(_T("Clear"));
	ctrlClear.SetFont(WinUtil::systemFont);
	clearContainer.SubclassWindow(ctrlClear.m_hWnd);

	ctrlHubCommands.Create(ctrlStatus.m_hWnd, rcDefault, _T("Hub Commands"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_STATICEDGE);
	ctrlHubCommands.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlHubCommands.SetFont(WinUtil::systemFont);
	ctrlHubCommands.SetCheck(showHubCommands ? BST_CHECKED : BST_UNCHECKED);
	HubCommandContainer.SubclassWindow(ctrlHubCommands.m_hWnd);

	ctrlCommands.Create(ctrlStatus.m_hWnd, rcDefault, _T("Client Commands"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_STATICEDGE);
	ctrlCommands.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlCommands.SetFont(WinUtil::systemFont);
	ctrlCommands.SetCheck(showCommands ? BST_CHECKED : BST_UNCHECKED);
	commandContainer.SubclassWindow(ctrlCommands.m_hWnd);

	ctrlDetection.Create(ctrlStatus.m_hWnd, rcDefault, _T("Detection"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_STATICEDGE);
	ctrlDetection.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlDetection.SetFont(WinUtil::systemFont);
	ctrlDetection.SetCheck(showDetection ? BST_CHECKED : BST_UNCHECKED);
	detectionContainer.SubclassWindow(ctrlDetection.m_hWnd);

	ctrlFilterIp.Create(ctrlStatus.m_hWnd, rcDefault, _T("Filter"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_STATICEDGE);
	ctrlFilterIp.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlFilterIp.SetFont(WinUtil::systemFont);
	ctrlFilterIp.SetCheck(bFilterIp ? BST_CHECKED : BST_UNCHECKED);
	cFilterContainer.SubclassWindow(ctrlFilterIp.m_hWnd);
	
	ctrlFilterText.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_NOHIDESEL, WS_EX_STATICEDGE, IDC_DEBUG_FILTER_TEXT);
	ctrlFilterText.LimitText(0);
	ctrlFilterText.SetFont(WinUtil::font);
	eFilterContainer.SubclassWindow(ctrlStatus.m_hWnd);
	
	m_hWndClient = ctrlPad;
	m_hMenu = WinUtil::mainMenu;

	WinUtil::SetIcon(m_hWnd, _T("CDM.ico"));
	start();
	DebugManager::getInstance()->addListener(this);
		
	bHandled = FALSE;
	return 1;
}

LRESULT CDMDebugFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		DebugManager::getInstance()->removeListener(this);
		
		closed = true;
		stop = true;
		s.signal();

		PostMessage(WM_CLOSE);
	} else {
		bHandled = FALSE;
	}
	return 0;
}

void CDMDebugFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */)
{
	RECT rect = { 0 };
	GetClientRect(&rect);

	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
	
	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[7];
		ctrlStatus.GetClientRect(sr);

		//int clearButtonWidth = 50;
		int tmp = ((sr.Width() - 50) / 6) - 4;
		w[0] = 50;
		w[1] = w[0] + tmp;
		w[2] = w[1] + tmp;
		w[3] = w[2] + tmp;
		w[4] = w[3] + tmp;
		w[5] = w[4] + tmp;
		w[6] = w[5] + tmp;
		
		ctrlStatus.SetParts(7, w);

		ctrlStatus.GetRect(0, sr);
		ctrlClear.MoveWindow(sr);
		ctrlStatus.GetRect(1, sr);
		ctrlCommands.MoveWindow(sr);
		ctrlStatus.GetRect(2, sr);
		ctrlHubCommands.MoveWindow(sr);
		ctrlStatus.GetRect(3, sr);
		ctrlDetection.MoveWindow(sr);
		ctrlStatus.GetRect(4, sr);
		ctrlFilterIp.MoveWindow(sr);
		ctrlStatus.GetRect(5, sr);
		ctrlFilterText.MoveWindow(sr);
		tstring msg;
		if(bFilterIp)
			msg = Text::toT("Watching IP: ") + sFilterIp;
		else
			msg = _T("Watching all IPs");
		ctrlStatus.SetText(6, msg.c_str());
	}
	
	// resize client window
	if(m_hWndClient != NULL)
		::SetWindowPos(m_hWndClient, NULL, rect.left, rect.top,
			rect.right - rect.left, rect.bottom - rect.top,
			SWP_NOZORDER | SWP_NOACTIVATE);
}

void CDMDebugFrame::addLine(const string& aLine) {
	if(ctrlPad.GetWindowTextLength() > MAX_TEXT_LEN) {
		ctrlPad.SetRedraw(FALSE);
		ctrlPad.SetSel(0, ctrlPad.LineIndex(ctrlPad.LineFromChar(2000)));
		ctrlPad.ReplaceSel(_T(""));
		ctrlPad.SetRedraw(TRUE);
	}
	BOOL noscroll = TRUE;
	POINT p = ctrlPad.PosFromChar(ctrlPad.GetWindowTextLength() - 1);
	CRect r;
	ctrlPad.GetClientRect(r);
		
	if( r.PtInRect(p) || MDIGetActive() != m_hWnd)
		noscroll = FALSE;
	else {
		ctrlPad.SetRedraw(FALSE); // Strange!! This disables the scrolling...????
	}
	ctrlPad.AppendText((Text::toT(aLine) + _T("\r\n")).c_str());
	if(noscroll) {
		ctrlPad.SetRedraw(TRUE);
	}
	//setDirty();
}

LRESULT CDMDebugFrame::onClear(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlPad.SetWindowText(_T(""));
	ctrlPad.SetFocus();
	return 0;
}
