
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

	CDMDebugFrame();
	
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
	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onSetCheckDetection(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSetCheckCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSetCheckHubCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSetCheckFilter(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void addLine(const string& aLine);
	
private:
	bool stop;
	CriticalSection cs;
	Semaphore s;
	deque<string> cmdList;

	int run();

	void addCmd(const string& cmd);

	CEdit ctrlPad, ctrlFilterText;
	CStatusBarCtrl ctrlStatus;
	CButton ctrlClear, ctrlCommands, ctrlHubCommands, ctrlDetection, ctrlFilterIp;
	CContainedWindow clearContainer, statusContainer, detectionContainer, commandContainer, HubCommandContainer, cFilterContainer, eFilterContainer;

	bool showCommands, showHubCommands, showDetection, bFilterIp;
	tstring sFilterIp;
	bool closed;
	
	void on(DebugManagerListener::DebugDetection, const string& aLine) noexcept;
	void on(DebugManagerListener::DebugCommand, const string& aLine, int typeDir, const string& ip) noexcept;
};

#endif // __CDMDEBUGFRAME_H