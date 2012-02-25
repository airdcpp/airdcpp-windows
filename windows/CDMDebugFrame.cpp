#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "CDMDebugFrame.h"
#include "WinUtil.h"
#include "../client/File.h"

#define MAX_TEXT_LEN 131072

CDMDebugFrame::CDMDebugFrame() : stop(false), closed(false), showCommands(true), showHubCommands(false), showDetection(false), bFilterIp(false),
		detectionContainer(WC_BUTTON, this, DETECTION_MESSAGE_MAP),
		HubCommandContainer(WC_BUTTON, this, HUB_COMMAND_MESSAGE_MAP),
		commandContainer(WC_BUTTON, this, COMMAND_MESSAGE_MAP),
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

int CDMDebugFrame::run() {
	setThreadPriority(Thread::LOW);
	string x = Util::emptyString;
	stop = false;

	while(true) {
		s.wait();
		if(stop)
			break;

		{
			Lock l(cs);
			if(cmdList.empty()) continue;

			x = cmdList.front();
			cmdList.pop_front();
		}
		addLine(x);
	}
		
	stop = false;
	return 0;
}

void CDMDebugFrame::addCmd(const string& cmd) {
	{
		Lock l(cs);
		cmdList.push_back(cmd);
	}
	s.signal();
}
	
void CDMDebugFrame::on(DebugManagerListener::DebugDetection, const string& aLine) noexcept {
	if(!showDetection)
		return;

	addCmd(aLine);
}
void CDMDebugFrame::on(DebugManagerListener::DebugCommand, const string& aLine, int typeDir, const string& ip) noexcept {
		switch(typeDir) {
			case DebugManager::HUB_IN:
				if(!showHubCommands)
					return;
				if(!bFilterIp || Text::toT(ip) == sFilterIp) {
					addCmd("Hub:\t[Incoming][" + ip + "]\t \t" + aLine);
				}
				break;
			case DebugManager::HUB_OUT:
				if(!showHubCommands)
					return;
				if(!bFilterIp || Text::toT(ip) == sFilterIp) {
					addCmd("Hub:\t[Outgoing][" + ip + "]\t \t" + aLine);
				}
				break;
			case DebugManager::CLIENT_IN:
				if(!showCommands)
					return;
				if(!bFilterIp || Text::toT(ip) == sFilterIp) {
					addCmd("Client:\t[Incoming][" + ip + "]\t \t" + aLine);
				}
				break;
			case DebugManager::CLIENT_OUT:
				if(!showCommands)
					return;
				if(!bFilterIp || Text::toT(ip) == sFilterIp) {
					addCmd("Client:\t[Outgoing][" + ip + "]\t \t" + aLine);
				}
				break;
			default: dcassert(0);
		}
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
LRESULT CDMDebugFrame::onSetCheckDetection(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	showDetection = wParam == BST_CHECKED;
	bHandled = FALSE;
	return 0;
}
LRESULT CDMDebugFrame::onSetCheckCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	showCommands = wParam == BST_CHECKED;
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