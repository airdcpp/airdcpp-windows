
#if !defined __CDMDEBUGFRAME_H
#define __CDMDEBUGFRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define TCP_COMMAND_MESSAGE_MAP 14
#define UDP_COMMAND_MESSAGE_MAP 15
#define HUB_COMMAND_MESSAGE_MAP 17
#define DEBUG_FILTER_MESSAGE_MAP 18
#define DEBUG_FILTER_TEXT_MESSAGE_MAP 19
#define CLEAR_MESSAGE_MAP 20
#define API_COMMAND_MESSAGE_MAP 21

#include "FlatTabCtrl.h"

#include <airdcpp/DebugManager.h>
#include <airdcpp-webapi/web-server/WebServerManagerListener.h>

#include <airdcpp/Semaphore.h>
#include <airdcpp/Thread.h>

#include <boost/lockfree/queue.hpp>

using namespace webserver;
class CDMDebugFrame : private DebugManagerListener, private WebServerManagerListener, public Thread,
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
	ALT_MSG_MAP(TCP_COMMAND_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onSetTCPCheckCommand)
	ALT_MSG_MAP(UDP_COMMAND_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onSetUDPCheckCommand)
	ALT_MSG_MAP(HUB_COMMAND_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onSetCheckHubCommand)
	ALT_MSG_MAP(API_COMMAND_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onSetCheckApiCommand)
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

	LRESULT onSetTCPCheckCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSetUDPCheckCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSetCheckHubCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSetCheckApiCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);

	LRESULT onSetCheckFilter(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);

	LRESULT onChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void addLine(const string& aLine);
	static string id;

private:

	bool stop = false;
	Semaphore s;
	boost::lockfree::queue<string*> cmdList;

	int run();

	void addCmd(const string& cmd);

	CEdit ctrlPad, ctrlFilterText;
	CStatusBarCtrl ctrlStatus;
	CButton ctrlTCPCommands, ctrlUDPCommands, ctrlHubCommands, ctrlApiCommands;
	CButton ctrlClear, ctrlFilterIp;
	CContainedWindow commandTCPContainer, commandUDPContainer, commandHubContainer, commandApiContainer;
	CContainedWindow clearContainer, statusContainer, cFilterContainer, eFilterContainer;

	bool showTCPCommands = true, showUDPCommands = true, showHubCommands = false, showApiCommands = false, bFilterIp = false;
	tstring sFilterIp;
	bool closed = false;
	
	void on(DebugManagerListener::DebugCommand, const string& aLine, uint8_t aType, uint8_t aDirection, const string& ip) noexcept override;
	void on(WebServerManagerListener::Data, const string& aData, TransportType aType, Direction aDirection, const string& aIP) noexcept override;

	static string formatMessage(const string& aType, bool aIncoming, const string& aData, const string& aIP) noexcept;
};

#endif // __CDMDEBUGFRAME_H