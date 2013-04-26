/* 
 * Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
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

#include "stdafx.h"

#ifdef _DEBUG
/** 
	Memory leak detector
	You can remove following 3 lines if you don't want to detect memory leaks.
	Ignore STL pseudo-leaks, we can avoid them with _STLP_LEAKS_PEDANTIC, but it only slows down everything.
 */
#define VLD_MAX_DATA_DUMP 0
#define VLD_AGGREGATE_DUPLICATES
//#include <vld.h>
#endif

#include "../client/DCPlusPlus.h"
#include "SingleInstance.h"
#include "WinUtil.h"

#include "../client/MerkleTree.h"
#include "../client/MappingManager.h"
#include "../client/UpdateManager.h"
#include "../client/Updater.h"
#include "../client/version.h"

#include "SplashWindow.h"
#include "ShellExecAsUser.h"
#include "Resource.h"
#include "ExtendedTrace.h"
#include "ResourceLoader.h"
#include "MainFrm.h"
#include "PopupManager.h"
#include "LineDlg.h"
#include "Wizard.h"
#include <delayimp.h>
#include <future>

#include "IgnoreManager.h"

CAppModule _Module;

CriticalSection cs;
enum { DEBUG_BUFSIZE = 8192 };
static char guard[DEBUG_BUFSIZE];
static int recursion = 0;
static char tth[192*8/(5*8)+2];
static bool firstException = true;

static char buf[DEBUG_BUFSIZE];


#ifndef _DEBUG

FARPROC WINAPI FailHook(unsigned /* dliNotify */, PDelayLoadInfo  pdli) {
	char buf[DEBUG_BUFSIZE];
	sprintf(buf, "AirDC++ just encountered and unhandled exception and will terminate.\nPlease do not report this as a bug. The error was caused by library %s.", pdli->szDll);
	MessageBox(WinUtil::mainWnd, Text::toT(buf).c_str(), _T("AirDC++ Has Crashed"), MB_OK | MB_ICONERROR);
	exit(-1);
}

#endif

#include "../client/SSLSocket.h"

string getExceptionName(DWORD code) {
	switch(code)
    { 
		case EXCEPTION_ACCESS_VIOLATION:
			return "Access violation"; break; 
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			return "Array out of range"; break; 
		case EXCEPTION_BREAKPOINT:
			return "Breakpoint"; break; 
		case EXCEPTION_DATATYPE_MISALIGNMENT:
			return "Read or write error"; break; 
		case EXCEPTION_FLT_DENORMAL_OPERAND:
			return "Floating-point error"; break; 
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			return "Floating-point division by zero"; break; 
		case EXCEPTION_FLT_INEXACT_RESULT:
			return "Floating-point inexact result"; break; 
		case EXCEPTION_FLT_INVALID_OPERATION:
			return "Unknown floating-point error"; break; 
		case EXCEPTION_FLT_OVERFLOW:
			return "Floating-point overflow"; break; 
		case EXCEPTION_FLT_STACK_CHECK:
			return "Floating-point operation caused stack overflow"; break; 
		case EXCEPTION_FLT_UNDERFLOW:
			return "Floating-point underflow"; break; 
		case EXCEPTION_ILLEGAL_INSTRUCTION:
			return "Illegal instruction"; break; 
		case EXCEPTION_IN_PAGE_ERROR:
			return "Page error"; break; 
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			return "Integer division by zero"; break; 
		case EXCEPTION_INT_OVERFLOW:
			return "Integer overflow"; break; 
		case EXCEPTION_INVALID_DISPOSITION:
			return "Invalid disposition"; break; 
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:
			return "Noncontinueable exception"; break; 
		case EXCEPTION_PRIV_INSTRUCTION:
			return "Invalid instruction"; break; 
		case EXCEPTION_SINGLE_STEP:
			return "Single step executed"; break; 
		case EXCEPTION_STACK_OVERFLOW:
			return "Stack overflow"; break; 
	}
	return "Unknown";
}
LONG __stdcall DCUnhandledExceptionFilter( LPEXCEPTION_POINTERS e )
{	
	Lock l(cs);

	if(recursion++ > 30)
		exit(-1);

#ifndef _DEBUG
#if _MSC_VER >= 1600
	__pfnDliFailureHook2 = FailHook;
#else
#error Unknown Compiler version
#endif

	// The release version loads the dll and pdb:s here...
	EXTENDEDTRACEINITIALIZE( Util::getPath(Util::PATH_RESOURCES).c_str() );

#endif

	if(firstException) {
		File::deleteFile(Util::getPath(Util::PATH_RESOURCES) + "exceptioninfo.txt");
		firstException = false;
	}

	if(File::getSize(Util::getPath(Util::PATH_RESOURCES) + "AirDC.pdb") == -1) {
		// No debug symbols, we're not interested...
		::MessageBox(WinUtil::mainWnd, _T("AirDC++ has crashed and you don't have AirDC.pdb file installed. Hence, I can't find out why it crashed, so don't report this as a bug unless you find a solution..."), _T("AirDC++ has crashed"), MB_OK);
#ifndef _DEBUG
		exit(1);
#else
		return EXCEPTION_CONTINUE_SEARCH;
#endif
	}

	File f(Util::getPath(Util::PATH_RESOURCES) + "exceptioninfo.txt", File::WRITE, File::OPEN | File::CREATE);
	f.setEndPos(0);
	
	DWORD exceptionCode = e->ExceptionRecord->ExceptionCode ;
	string archStr = "x86";
#ifdef _WIN64
	archStr = "x64";
#endif

	sprintf(buf, "Code: %x ( %s )\r\nVersion: %s %s\r\n", 
		exceptionCode, getExceptionName(exceptionCode).c_str(), VERSIONSTRING, archStr.c_str());

	f.write(buf, strlen(buf));
	sprintf(buf, "Build: %s\r\n", 
		SVNVERSION);	
	f.write(buf, strlen(buf));
	
	OSVERSIONINFOEX ver;
	WinUtil::getVersionInfo(ver);

	sprintf(buf, "Major: %d\r\nMinor: %d\r\nBuild: %d\r\nSP: %d\r\nType: %d\r\n",
		(DWORD)ver.dwMajorVersion, (DWORD)ver.dwMinorVersion, (DWORD)ver.dwBuildNumber,
		(DWORD)ver.wServicePackMajor, (DWORD)ver.wProductType);

	f.write(buf, strlen(buf));
	time_t now;
	time(&now);
	strftime(buf, DEBUG_BUFSIZE, "Time: %Y-%m-%d %H:%M:%S\r\n", localtime(&now));

	f.write(buf, strlen(buf));

	f.write(LIT("TTH: "));
	f.write(tth, strlen(tth));
	f.write(LIT("\r\n"));

    f.write(LIT("\r\n"));

	STACKTRACE(f, e->ContextRecord);

	f.write(LIT("\r\n"));

	f.close();

	if ((!SETTING(SOUND_EXC).empty()) && (!SETTING(SOUNDS_DISABLED)))
		WinUtil::playSound(Text::toT(SETTING(SOUND_EXC)));

	NOTIFYICONDATA m_nid;
	m_nid.cbSize = sizeof(NOTIFYICONDATA);
	m_nid.hWnd = MainFrame::getMainFrame()->m_hWnd;
	m_nid.uID = 0;
	m_nid.uFlags = NIF_INFO;
	m_nid.uTimeout = 5000;
	m_nid.dwInfoFlags = NIIF_WARNING;
	_tcscpy(m_nid.szInfo, _T("exceptioninfo.txt was generated"));
	_tcscpy(m_nid.szInfoTitle, _T("AirDC++ has crashed"));
	Shell_NotifyIcon(NIM_MODIFY, &m_nid);

	if(::MessageBox(WinUtil::mainWnd, _T("AirDC++ just encountered a fatal bug and should have written an exceptioninfo.txt the same directory as the executable. You can upload this file at http://www.airdcpp.net to help us find out what happened. Go there now?"), _T("AirDC++ Has Crashed"), MB_YESNO | MB_ICONERROR) == IDYES) {
		WinUtil::openLink(_T("http://crash.airdcpp.net"));
	}

#ifndef _DEBUG
	EXTENDEDTRACEUNINITIALIZE();
	
	exit(-1);
#else
	return EXCEPTION_CONTINUE_SEARCH;
#endif
}

static void sendCmdLine(HWND hOther, LPTSTR lpstrCmdLine)
{
	tstring cmdLine = lpstrCmdLine;
	LRESULT result;

	COPYDATASTRUCT cpd;
	cpd.dwData = 0;
	cpd.cbData = sizeof(TCHAR)*(cmdLine.length() + 1);
	cpd.lpData = (void *)cmdLine.c_str();
	result = SendMessage(hOther, WM_COPYDATA, NULL,	(LPARAM)&cpd);
}

BOOL CALLBACK searchOtherInstance(HWND hWnd, LPARAM lParam) {
	ULONG_PTR result;
	LRESULT ok = ::SendMessageTimeout(hWnd, WMU_WHERE_ARE_YOU, 0, 0,
		SMTO_BLOCK | SMTO_ABORTIFHUNG, 5000, &result);
	if(ok == 0)
		return TRUE;
	if(result == WMU_WHERE_ARE_YOU) {
		// found it
		HWND *target = (HWND *)lParam;
		*target = hWnd;
		return FALSE;
	}
	return TRUE;
}

static void checkCommonControls() {
#define PACKVERSION(major,minor) MAKELONG(minor,major)

	HINSTANCE hinstDll;
	DWORD dwVersion = 0;
	
	hinstDll = LoadLibrary(_T("comctl32.dll"));
	
	if(hinstDll)
	{
		DLLGETVERSIONPROC pDllGetVersion;
	
		pDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hinstDll, "DllGetVersion");
		
		if(pDllGetVersion)
		{
			DLLVERSIONINFO dvi;
			HRESULT hr;
			
			memzero(&dvi, sizeof(dvi));
			dvi.cbSize = sizeof(dvi);
			
			hr = (*pDllGetVersion)(&dvi);
			
			if(SUCCEEDED(hr))
			{
				dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
			}
		}
		
		FreeLibrary(hinstDll);
	}

	if(dwVersion < PACKVERSION(5,80)) {
		MessageBox(NULL, _T("Your version of windows common controls is too old for AirDC++ to run correctly, and you will most probably experience problems with the user interface. You should download version 5.80 or higher from the DC++ homepage or from Microsoft directly."), _T("User Interface Warning"), MB_OK);
	}

    // InitCommonControls() is required on Windows XP if an application
    // manifest specifies use of ComCtl32.dll version 6 or later to enable
    // visual styles.  Otherwise, any window creation will fail.

    INITCOMMONCONTROLSEX used = {
        sizeof(INITCOMMONCONTROLSEX),
            ICC_WIN95_CLASSES
    };
    InitCommonControlsEx(&used);
}

class CFindDialogMessageFilter : public CMessageFilter
{
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		if (WinUtil::findDialog) {
			return IsDialogMessage(WinUtil::findDialog, pMsg);
		} else if (pMsg->message == WM_SPEAKER && pMsg->wParam == 5000) {
			auto f = reinterpret_cast<Dispatcher::F*>(pMsg->lParam);
			(*f)();
			delete f;
			return TRUE;
		}
		return FALSE;
	}
};

static unique_ptr<MainFrame> mainWindow;
static int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	checkCommonControls();

	CMessageLoop theLoop;	

	CFindDialogMessageFilter findDialogFilter;
	theLoop.AddMessageFilter(&findDialogFilter);
	_Module.AddMessageLoop(&theLoop);

	WinUtil::preInit();

	WinUtil::splash = unique_ptr<SplashWindow>(new SplashWindow());
	(*WinUtil::splash)("Starting up");

	std::future<void> loader;
	loader = std::async([=] {
		startup(
			[&](const string& str) { (*WinUtil::splash)(str); },
			[&](const string& str, bool isQuestion, bool isError) { 
				auto ret = ::MessageBox(WinUtil::splash->getHWND(), Text::toT(str).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), (isQuestion ? MB_YESNO : MB_OK) | (isError ? MB_ICONEXCLAMATION : MB_ICONQUESTION)); 
				return isQuestion ? ret == IDYES : true;
		},
			[&]() { 
				WizardDlg dlg;
				dlg.DoModal(/*m_hWnd*/);
		},
			[=](float progress) { (*WinUtil::splash)(progress); }
		);


		PopupManager::newInstance();
		IgnoreManager::newInstance();

		WinUtil::splash->callAsync([=] {
			if(SETTING(PASSWD_PROTECT)) {
				PassDlg dlg;
				dlg.description = TSTRING(PASSWORD_DESC);
				dlg.title = TSTRING(PASSWORD_TITLE);
				dlg.ok = TSTRING(UNLOCK);
				if(dlg.DoModal(/*m_hWnd*/) == IDOK){
					tstring tmp = dlg.line;
					if (tmp != Text::toT(Util::base64_decode(SETTING(PASSWORD)))) {
						ExitProcess(1);
					}
				}
			}

			if(ResourceManager::getInstance()->isRTL()) {
				SetProcessDefaultLayout(LAYOUT_RTL);
			}

			MainFrame* wndMain = new MainFrame;
			mainWindow.reset(wndMain);

			CRect rc = wndMain->rcDefault;
			if( (SETTING(MAIN_WINDOW_POS_X) != CW_USEDEFAULT) &&
				(SETTING(MAIN_WINDOW_POS_Y) != CW_USEDEFAULT) &&
				(SETTING(MAIN_WINDOW_SIZE_X) != CW_USEDEFAULT) &&
				(SETTING(MAIN_WINDOW_SIZE_Y) != CW_USEDEFAULT) ) {

					rc.left = SETTING(MAIN_WINDOW_POS_X);
					rc.top = SETTING(MAIN_WINDOW_POS_Y);
					rc.right = rc.left + SETTING(MAIN_WINDOW_SIZE_X);
					rc.bottom = rc.top + SETTING(MAIN_WINDOW_SIZE_Y);
					// Now, let's ensure we have sane values here...
					if( (rc.left < 0 ) || (rc.top < 0) || (rc.right - rc.left < 10) || ((rc.bottom - rc.top) < 10) ) {
						rc = wndMain->rcDefault;
					}
			}

			int rtl = ResourceManager::getInstance()->isRTL() ? WS_EX_RTLREADING : 0;
			if(wndMain->CreateEx(NULL, rc, 0, rtl | WS_EX_APPWINDOW | WS_EX_WINDOWEDGE) == NULL) {
				ATLTRACE(_T("Main window creation failed!\n"));
				//throw("Main window creation failed");
				//return 0;
			}

			if(SETTING(MINIMIZE_ON_STARTUP)) {
				wndMain->ShowWindow(SW_SHOWMINIMIZED);
			} else {
				wndMain->ShowWindow(((nCmdShow == SW_SHOWDEFAULT) || (nCmdShow == SW_SHOWNORMAL)) ? SETTING(MAIN_WINDOW_STATE) : nCmdShow);
			}

			WinUtil::splash.reset();
		});
	});

	int nRet = theLoop.Run();

	dcassert(WinUtil::splash);
	loader = std::async([=] {
		IgnoreManager::deleteInstance();
		PopupManager::deleteInstance();

		shutdown(
			[&](const string& str) { (*WinUtil::splash)(str); },
			[=](float progress) { (*WinUtil::splash)(progress); }
		);
		WinUtil::splash->callAsync([=] { PostQuitMessage(0); });
	});

	nRet = theLoop.Run();
	_Module.RemoveMessageLoop();

	WinUtil::runPendingUpdate();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow) {
	SingleInstance dcapp(_T(INST_NAME));
	LPTSTR* argv = __targv;
	int argc = --__argc;

	auto checkParams = [&argc, &argv] () -> void {
		while (argc > 0) {
			Util::addParam(Text::fromT(*argv));
			argc--;
			argv++;
		}
	};

	if (argc > 0) {
		argv++;
		if(_tcscmp(*argv, _T("/createupdate")) == 0) {
			Updater::createUpdate();
			return FALSE;
		} else if(_tcscmp(*argv, _T("/sign")) == 0 && --argc >= 2) {
			string xmlPath = Text::fromT(*++argv);
			string keyPath = Text::fromT(*++argv);
			bool genHeader = (argc > 2 && (_tcscmp(*++argv, _T("-pubout")) == 0));
			if(Util::fileExists(xmlPath) && Util::fileExists(keyPath))
				Updater::signVersionFile(xmlPath, keyPath, genHeader);
			return FALSE;
		} else if(_tcscmp(*argv, _T("/update")) == 0) {
			if(--argc >= 1) {
				WinUtil::splash = unique_ptr<SplashWindow>(new SplashWindow());
				(*WinUtil::splash)("Updating");
				string sourcePath = Util::getFilePath(Util::getAppName());
				string installPath = Text::fromT(*++argv); argc--;

				bool startElevated = false;
				if (argc > 0) {
					if (Text::fromT((*++argv)) == "/elevation") {
						startElevated = true;
						argc--;
					} else {
						--argv;
					}
				}

				SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);

				bool success = false;

				for (;;) {
					string error;
					for(int i = 0; i < 10 && (success = Updater::applyUpdate(sourcePath, installPath, error)) == false; ++i)
						Thread::sleep(1000);

					if (!success) {
						if (::MessageBox(NULL, Text::toT("Updating failed:\n\n" + error + "\n\nDo you want to retry installing the update?").c_str(), 
							_T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_TOPMOST) == IDYES) {
								continue;
						}
					}
					break;
				}

				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
				SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

				//add new startup params
				Util::addParam(success ? "/updated" : "/updatefailed");
				Util::addParam("/silent");

				//append the passed params (but leave out the update commands...)
				argv++;
				checkParams();

				//start the updated instance
				if (!startElevated) {
					ShellExecAsUser(NULL, Text::toT(installPath + Util::getFileName(Util::getAppName())).c_str(), Util::getParams(true).c_str(), NULL);
				} else {
					ShellExecute(NULL, NULL, Text::toT(installPath + Util::getFileName(Util::getAppName())).c_str(), Util::getParams(true).c_str(), NULL, SW_SHOWNORMAL);
				}

				return FALSE;
			}
			return FALSE;
		} else {
			checkParams();
		}
	}

	string updaterFile;
	auto updated = Util::hasParam("/updated") || Util::hasParam("/updatefailed");
	if (UpdateManager::checkPendingUpdates(Util::getFilePath(Util::getAppName()), updaterFile, updated)) {
		WinUtil::addUpdate(updaterFile);
		WinUtil::runPendingUpdate();
		return FALSE;
	}

	bool multiple = Util::hasParam("/silent");

	if(dcapp.IsAnotherInstanceRunning()) {
		// Allow for more than one instance...
		if(_tcslen(lpstrCmdLine) == 0) {
			if (::MessageBox(NULL, _T("There is already an instance of AirDC++ running.\nDo you want to launch another instance anyway?"), 
				_T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_TOPMOST) == IDYES) {
					multiple = true;
			}
		}

		if(multiple == false) {
			HWND hOther = NULL;
			EnumWindows(searchOtherInstance, (LPARAM)&hOther);

			if( hOther != NULL ) {
				// pop up
				::SetForegroundWindow(hOther);

				if( IsIconic(hOther)) {
					// restore
					::ShowWindow(hOther, SW_RESTORE);
				}
				sendCmdLine(hOther, lpstrCmdLine);
			}
			return FALSE;
		}
	}
	
	
	// For SHBrowseForFolder, UPnP_COM
	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); 
#ifdef _DEBUG
	EXTENDEDTRACEINITIALIZE(Util::getPath(Util::PATH_RESOURCES).c_str());
#endif
	LPTOP_LEVEL_EXCEPTION_FILTER pOldSEHFilter = NULL;
	pOldSEHFilter = SetUnhandledExceptionFilter(&DCUnhandledExceptionFilter);
	
	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES |
		ICC_TAB_CLASSES | ICC_UPDOWN_CLASS | ICC_USEREX_CLASSES);	// add flags to support other controls
	
	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));
	
	try {		
		File f(Util::getAppName(), File::READ, File::OPEN);
		TigerTree tth(TigerTree::calcBlockSize(f.getSize(), 1));
		size_t n = 0;
		size_t n2 = DEBUG_BUFSIZE;
		while( (n = f.read(buf, n2)) > 0) {
			tth.update(buf, n);
			n2 = DEBUG_BUFSIZE;
		}
		tth.finalize();
		strcpy(::tth, tth.getRoot().toBase32().c_str());
		WinUtil::tth = Text::toT(::tth);
	} catch(const FileException&) {
		dcdebug("Failed reading exe\n");
	}	

	HINSTANCE hInstRich = ::LoadLibrary(_T("MSFTEDIT.DLL"));
	if (hInstRich == NULL) {
		MessageBox(NULL, _T("AirDC++ isn't supported in operating systems older than Microsoft Windows XP3"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	int nRet = Run(lpstrCmdLine, nCmdShow);
 
	if ( hInstRich ) {
		::FreeLibrary(hInstRich);
	}
	
	// Return back old VS SEH handler
	if (pOldSEHFilter != NULL)
		SetUnhandledExceptionFilter(pOldSEHFilter);

	_Module.Term();
	::CoUninitialize();

#ifdef _DEBUG
	EXTENDEDTRACEUNINITIALIZE();
#endif

	return nRet;
}
