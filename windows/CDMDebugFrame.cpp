#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "CDMDebugFrame.h"
#include "WinUtil.h"
#include "../client/File.h"

#define MAX_TEXT_LEN 131072

CDMDebugFrame::CDMDebugFrame() : stop(false), closed(false), showTCPCommands(true), showUDPCommands(true), showHubCommands(false), bFilterIp(false), cmdList(1024),
		HubCommandContainer(WC_BUTTON, this, HUB_COMMAND_MESSAGE_MAP),
		commandTCPContainer(WC_BUTTON, this, TCP_COMMAND_MESSAGE_MAP),
		commandUDPContainer(WC_BUTTON, this, UDP_COMMAND_MESSAGE_MAP),
		cFilterContainer(WC_BUTTON, this, DEBUG_FILTER_MESSAGE_MAP),
		eFilterContainer(WC_EDIT, this, DEBUG_FILTER_TEXT_MESSAGE_MAP),
		clearContainer(WC_BUTTON, this, CLEAR_MESSAGE_MAP),
		statusContainer(STATUSCLASSNAME, this, CLEAR_MESSAGE_MAP)
	 { }

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

	ctrlTCPCommands.Create(ctrlStatus.m_hWnd, rcDefault, _T("Client Commands (TCP)"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_STATICEDGE);
	ctrlTCPCommands.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlTCPCommands.SetFont(WinUtil::systemFont);
	ctrlTCPCommands.SetCheck(showTCPCommands ? BST_CHECKED : BST_UNCHECKED);
	commandTCPContainer.SubclassWindow(ctrlTCPCommands.m_hWnd);

	ctrlUDPCommands.Create(ctrlStatus.m_hWnd, rcDefault, _T("Client Commands (UDP)"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_STATICEDGE);
	ctrlUDPCommands.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlUDPCommands.SetFont(WinUtil::systemFont);
	ctrlUDPCommands.SetCheck(showUDPCommands ? BST_CHECKED : BST_UNCHECKED);
	commandUDPContainer.SubclassWindow(ctrlUDPCommands.m_hWnd);

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

	WinUtil::SetIcon(m_hWnd, IDI_CDM);
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
		int tmp = ((sr.Width() - 50) / 7) - 4;
		w[0] = 50;
		w[1] = w[0] + tmp;
		w[2] = w[1] + tmp;
		w[3] = w[2] + tmp;
		w[4] = w[3] + tmp;
		w[5] = w[4] + tmp;
		w[6] = w[5] + tmp;
		
		ctrlStatus.SetParts(8, w);

		ctrlStatus.GetRect(0, sr);
		ctrlClear.MoveWindow(sr);
		ctrlStatus.GetRect(1, sr);

		ctrlTCPCommands.MoveWindow(sr);
		ctrlStatus.GetRect(2, sr);

		ctrlUDPCommands.MoveWindow(sr);
		ctrlStatus.GetRect(3, sr);

		ctrlHubCommands.MoveWindow(sr);
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

int CDMDebugFrame::run() {
	setThreadPriority(Thread::LOW);
	string x;
	stop = false;

	while(true) {
		s.wait();
		if(stop)
			break;

		if(!cmdList.pop(x)) {
			continue;
		}
		addLine(x);
	}
		
	stop = false;
	return 0;
}

void CDMDebugFrame::addCmd(const string& cmd) {
	cmdList.push(cmd);
	s.signal();
}

void CDMDebugFrame::on(DebugManagerListener::DebugCommand, const string& aLine, uint8_t aType, uint8_t aDirection, const string& ip) noexcept {
	if(bFilterIp && Text::toT(ip).find(sFilterIp) == tstring::npos) {
		return;
	}

	string cmd;
	switch(aType) {
		case DebugManager::TYPE_HUB:
			if(!showHubCommands)
				return;
			cmd = "Hub:";
			break;
		case DebugManager::TYPE_CLIENT:
			if(!showTCPCommands)
				return;
			cmd = "Client (TCP):";
			break;
		case DebugManager::TYPE_CLIENT_UDP:
			if(!showUDPCommands)
				return;
			cmd = "Client (UDP):";
			break;
		default: dcassert(0);
	}

	cmd += "\t\t";

	if (aDirection == DebugManager::INCOMING) {
		cmd += "[Incoming]";
	} else {
		cmd += "[Outgoing]";
	}

	cmd += "[" + ip + "]\t \t" + aLine;

	addCmd(cmd);
}

LRESULT CDMDebugFrame::onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	HWND hWnd = (HWND)lParam;
	HDC hDC = (HDC)wParam;
	if(hWnd == ctrlPad.m_hWnd) {
		::SetBkColor(hDC, WinUtil::bgColor);
		::SetTextColor(hDC, WinUtil::textColor);
		return (LRESULT)WinUtil::bgBrush;
	}
	bHandled = FALSE;
	return FALSE;
};
LRESULT CDMDebugFrame::OnFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ctrlPad.SetFocus();
	return 0;
}
LRESULT CDMDebugFrame::onSetTCPCheckCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	showTCPCommands = wParam == BST_CHECKED;
	bHandled = FALSE;
	return 0;
}
LRESULT CDMDebugFrame::onSetUDPCheckCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	showUDPCommands = wParam == BST_CHECKED;
	bHandled = FALSE;
	return 0;
}
LRESULT CDMDebugFrame::onSetCheckHubCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	showHubCommands = wParam == BST_CHECKED;
	bHandled = FALSE;
	return 0;
}
LRESULT CDMDebugFrame::onSetCheckFilter(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	bFilterIp = wParam == BST_CHECKED;
	UpdateLayout();
	bHandled = FALSE;
	return 0;
}
LRESULT CDMDebugFrame::onChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	sFilterIp.resize(ctrlFilterText.GetWindowTextLength());

	ctrlFilterText.GetWindowText(&sFilterIp[0], sFilterIp.size() + 1);

	UpdateLayout();
	return 0;
}