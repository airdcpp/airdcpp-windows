
#if !defined __CDMDEBUGFRAME_H
#define __CDMDEBUGFRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define COMMAND_MESSAGE_MAP 14
#define DETECTION_MESSAGE_MAP 15
#define HUB_COMMAND_MESSAGE_MAP 16
#define DEBUG_FILTER_MESSAGE_MAP 17
#define DEBUG_FILTER_TEXT_MESSAGE_MAP 18
#define CLEAR_MESSAGE_MAP 19

#include "FlatTabCtrl.h"
#include "WinUtil.h"

#include "../client/DebugManager.h"
#include "../client/Semaphore.h"

class CDMDebugFrame : private DebugManagerListener, public Thread,
	public MDITabChildWindowImpl<CDMDebugFrame>,
	public StaticFrame<CDMDebugFrame, ResourceManager::MENU_CDMDEBUG_MESSAGES>
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("CDMDebugFrame"), IDR_CDM, 0, COLOR_3DFACE);

	CDMDebugFrame() : stop(false), closed(false), showCommands(true), showHubCommands(false), showDetection(false), bFilterIp(false),
		detectionContainer(WC_BUTTON, this, DETECTION_MESSAGE_MAP),
		HubCommandContainer(WC_BUTTON, this, HUB_COMMAND_MESSAGE_MAP),
		commandContainer(WC_BUTTON, this, COMMAND_MESSAGE_MAP),
		cFilterContainer(WC_BUTTON, this, DEBUG_FILTER_MESSAGE_MAP),
		eFilterContainer(WC_EDIT, this, DEBUG_FILTER_TEXT_MESSAGE_MAP),
		clearContainer(WC_BUTTON, this, CLEAR_MESSAGE_MAP),
		statusContainer(STATUSCLASSNAME, this, CLEAR_MESSAGE_MAP)
	 { 
	 }
	
	~CDMDebugFrame() { }
	void OnFinalMessage(HWND /*hWnd*/) { delete this; }

	typedef MDITabChildWindowImpl<CDMDebugFrame> baseClass;
	BEGIN_MSG_MAP(CDMDebugFrame)
		MESSAGE_HANDLER(WM_SETFOCUS, OnFocus)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(DETECTION_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onSetCheckDetection)
	ALT_MSG_MAP(COMMAND_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onSetCheckCommand)
	ALT_MSG_MAP(HUB_COMMAND_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onSetCheckHubCommand)
	ALT_MSG_MAP(DEBUG_FILTER_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onSetCheckFilter)
	ALT_MSG_MAP(DEBUG_FILTER_TEXT_MESSAGE_MAP)
		COMMAND_HANDLER(IDC_DEBUG_FILTER_TEXT, EN_CHANGE, onChange)
 	ALT_MSG_MAP(CLEAR_MESSAGE_MAP)
		COMMAND_ID_HANDLER(IDC_CLEAR, onClear)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClear(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	void UpdateLayout(BOOL bResizeBars = TRUE);
	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
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
	LRESULT OnFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlPad.SetFocus();
		return 0;
	}
	LRESULT onSetCheckDetection(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		showDetection = wParam == BST_CHECKED;
		bHandled = FALSE;
		return 0;
	}
	LRESULT onSetCheckCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		showCommands = wParam == BST_CHECKED;
		bHandled = FALSE;
		return 0;
	}
	LRESULT onSetCheckHubCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		showHubCommands = wParam == BST_CHECKED;
		bHandled = FALSE;
		return 0;
	}
	LRESULT onSetCheckFilter(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		bFilterIp = wParam == BST_CHECKED;
		UpdateLayout();
		bHandled = FALSE;
		return 0;
	}
	LRESULT onChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		sFilterIp.resize(ctrlFilterText.GetWindowTextLength());

		ctrlFilterText.GetWindowText(&sFilterIp[0], sFilterIp.size() + 1);

		UpdateLayout();
		return 0;
	}

	void addLine(const string& aLine);
	
private:
	bool stop;
	CriticalSection cs;
	Semaphore s;
	deque<string> cmdList;

	int run() {
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

	void addCmd(const string& cmd) {
		{
			Lock l(cs);
			cmdList.push_back(cmd);
		}
		s.signal();
	}

	CEdit ctrlPad, ctrlFilterText;
	CStatusBarCtrl ctrlStatus;
	CButton ctrlClear, ctrlCommands, ctrlHubCommands, ctrlDetection, ctrlFilterIp;
	CContainedWindow clearContainer, statusContainer, detectionContainer, commandContainer, HubCommandContainer, cFilterContainer, eFilterContainer;

	bool showCommands, showHubCommands, showDetection, bFilterIp;
	tstring sFilterIp;
	bool closed;
	
	void on(DebugManagerListener::DebugDetection, const string& aLine) noexcept {
		if(!showDetection)
			return;

		addCmd(aLine);
	}
	void on(DebugManagerListener::DebugCommand, const string& aLine, int typeDir, const string& ip) noexcept {
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
};

#endif // __CDMDEBUGFRAME_H