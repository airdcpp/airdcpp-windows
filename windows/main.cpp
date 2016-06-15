/* 
 * Copyright (C) 2001-2016 Jacek Sieka, arnetheduck on gmail point com
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

#include "ExtendedTrace.h"
#include "MainFrm.h"
#include "PopupManager.h"
#include "Resource.h"
#include "ResourceLoader.h"
#include "ShellExecAsUser.h"
#include "SingleInstance.h"
#include "SplashWindow.h"
#include "WinUtil.h"
#include "Wizard.h"

#include <airdcpp/DCPlusPlus.h>
#include <airdcpp/LogManager.h>
#include <airdcpp/MerkleTree.h>
#include <airdcpp/Updater.h>
#include <airdcpp/version.h>

#include <web-server/WebServerManager.h>

#include <delayimp.h>
#include <future>

CAppModule _Module;

CriticalSection cs;
enum { DEBUG_BUFSIZE = 8192 };
static int recursion = 0;
static char exeTTH[192*8/(5*8)+2];
static bool firstException = true;

static char debugBuf[DEBUG_BUFSIZE];


#include <airdcpp/SSLSocket.h>

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

LONG handleCrash(unsigned long aCode, const string& aError, PCONTEXT aContext)
{	
	Lock l(cs);

	if(recursion++ > 30)
		exit(-1);

#ifndef _DEBUG
	// The release version loads the dll and pdb:s here...
	EXTENDEDTRACEINITIALIZE(Util::getAppFilePath().c_str());
#endif

	if(File::getSize(Util::getAppFilePath() + "AirDC.pdb") == -1) {
		// No debug symbols, we're not interested...
		::MessageBox(WinUtil::mainWnd, _T("AirDC++ has crashed and you don't have AirDC.pdb file installed. Hence, I can't find out why it crashed, so don't report this as a bug unless you find a solution..."), _T("AirDC++ has crashed"), MB_OK);
#ifndef _DEBUG
		exit(1);
#else
		return EXCEPTION_CONTINUE_SEARCH;
#endif
	}

	if ((!SETTING(SOUND_EXC).empty()) && (!SETTING(SOUNDS_DISABLED)))
		WinUtil::playSound(Text::toT(SETTING(SOUND_EXC)));

	if (MainFrame::getMainFrame()) {
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
	}

	auto exceptionFilePath = Util::getPath(Util::PATH_USER_CONFIG) + "exceptioninfo.txt";

	if (firstException) {
		File::deleteFile(exceptionFilePath);
		firstException = false;
	}

	try {
		File f(exceptionFilePath, File::WRITE, File::OPEN | File::CREATE);
		f.setEndPos(0);

		string archStr = "x86";
#ifdef _WIN64
		archStr = "x64";
#endif

		sprintf(debugBuf, "Code: %x ( %s )\r\nVersion: %s %s\r\n",
			aCode, aError.c_str(), shortVersionString.c_str(), archStr.c_str());

		f.write(debugBuf, strlen(debugBuf));
		sprintf(debugBuf, "Build: %s\r\n",
			BUILD_NUMBER_STR.c_str());
		f.write(debugBuf, strlen(debugBuf));

		OSVERSIONINFOEX ver;
		WinUtil::getVersionInfo(ver);

		sprintf(debugBuf, "Major: %d\r\nMinor: %d\r\nBuild: %d\r\nSP: %d\r\nType: %d\r\n",
			(DWORD)ver.dwMajorVersion, (DWORD)ver.dwMinorVersion, (DWORD)ver.dwBuildNumber,
			(DWORD)ver.wServicePackMajor, (DWORD)ver.wProductType);

		f.write(debugBuf, strlen(debugBuf));
		time_t now;
		time(&now);
		strftime(debugBuf, DEBUG_BUFSIZE, "Time: %Y-%m-%d %H:%M:%S\r\n", localtime(&now));

		f.write(debugBuf, strlen(debugBuf));

		f.write(LIT("TTH: "));
		f.write(exeTTH, strlen(exeTTH));
		f.write(LIT("\r\n"));

		f.write(LIT("\r\n"));

		STACKTRACE(f, aContext);

		f.write(LIT("\r\n"));

		f.close();

		auto msg = "AirDC++ just encountered a fatal bug and details have been written to " + exceptionFilePath + "\n\nYou can upload this file at http://www.airdcpp.net to help us find out what happened. Go there now?";
		if (::MessageBox(WinUtil::mainWnd, Text::toT(msg).c_str(), _T("AirDC++ has crashed"), MB_YESNO | MB_ICONERROR) == IDYES) {
			WinUtil::openLink(_T("http://crash.airdcpp.net"));
		}
	} catch (const FileException& e) {
		auto msg = "Crash details could not be written to " + exceptionFilePath + " (" + e.what() + "). Ensure that the directory is writable.";
		::MessageBox(WinUtil::mainWnd, Text::toT(msg).c_str(), _T("AirDC++ has crashed"), MB_OK);
	}

#ifndef _DEBUG
	EXTENDEDTRACEUNINITIALIZE();
	
	exit(-1);
#else
	return EXCEPTION_CONTINUE_SEARCH;
#endif
}

LONG __stdcall DCUnhandledExceptionFilter(LPEXCEPTION_POINTERS e) {
	auto code = e->ExceptionRecord->ExceptionCode;
	return handleCrash(code, getExceptionName(code), e->ContextRecord);
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

class CFindDialogMessageFilter : public CMessageFilter
{
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		if (WinUtil::findDialog) {
			return IsDialogMessage(WinUtil::findDialog, pMsg);
		}
		return FALSE;
	}
};

static int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	static unique_ptr<MainFrame> wndMain;

	CMessageLoop theLoop;	

	CFindDialogMessageFilter findDialogFilter;
	theLoop.AddMessageFilter(&findDialogFilter);
	_Module.AddMessageLoop(&theLoop);

	WinUtil::preInit();

	SplashWindow::create();
	WinUtil::splash->update("Starting up");

	auto loader = std::async(std::launch::async, [&] {
		unique_ptr<SetupWizard> wizard = nullptr;
		try {
			startup(
				[&](const string& str) { WinUtil::splash->update(str); },
				[&](const string& str, bool isQuestion, bool isError) {
					auto ret = ::MessageBox(WinUtil::splash->m_hWnd, Text::toT(str).c_str(), Text::toT(shortVersionString).c_str(), MB_SETFOREGROUND | (isQuestion ? MB_YESNO : MB_OK) | (isError ? MB_ICONEXCLAMATION : MB_ICONQUESTION));
					return isQuestion ? ret == IDYES : true;
			},
				[&]() {
					Semaphore s;
					WinUtil::splash->callAsync([&] {
						wizard = make_unique<SetupWizard>(true);
						if (wizard->DoModal(/*m_hWnd*/) != IDOK) {
							//the wizard was cancelled
							wizard.reset(nullptr);
						}
						s.signal();
					});

					// wait for the wizard to finish
					s.wait();
			},
				[=](float progress) { WinUtil::splash->update(progress); }
			);

			webserver::WebServerManager::newInstance();
			if (webserver::WebServerManager::getInstance()->load()) {
				auto webResourcePath = Util::getStartupParam("--web-resources");
#ifdef _DEBUG
				if (!webResourcePath) {
					webResourcePath = Util::getParentDir(Util::getParentDir(Util::getAppFilePath())) + "installer\\Web-resources\\";
				}
#endif

				auto started = webserver::WebServerManager::getInstance()->start([](const string& aError) {
					LogManager::getInstance()->message(aError, LogMessage::SEV_ERROR);
				}, webResourcePath ? *webResourcePath : Util::emptyString);
				
				if (started) {
					LogManager::getInstance()->message("Web server started", LogMessage::SEV_INFO);
				}
			}

			WinUtil::splash->update(STRING(LOADING_GUI));
		} catch (...) {
			ExitProcess(1);
		}

		//apply the share changes from wizard (if available)
		if (wizard) {
			PropPage::TaskList tasks;
			wizard->getTasks(tasks);
			for(auto& t: tasks) {
				t();
			}
			wizard.reset(nullptr);
			SettingsManager::getInstance()->save();
		}

		PopupManager::newInstance();

		WinUtil::splash->callAsync([=] {
			if(SETTING(PASSWD_PROTECT) && !WinUtil::checkClientPassword()) {
				ExitProcess(1);
			}

			if(ResourceManager::getInstance()->isRTL()) {
				SetProcessDefaultLayout(LAYOUT_RTL);
			}

			wndMain.reset(new MainFrame);

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
				return;
			}

			if(SETTING(MINIMIZE_ON_STARTUP)) {
				wndMain->ShowWindow(SW_SHOWMINIMIZED);
			} else {
				WinUtil::isAppActive = true;
				wndMain->ShowWindow(((nCmdShow == SW_SHOWDEFAULT) || (nCmdShow == SW_SHOWNORMAL)) ? SETTING(MAIN_WINDOW_STATE) : nCmdShow);
			}
		});
	});

	int nRet = theLoop.Run();

	dcassert(WinUtil::splash);
	loader = std::async(std::launch::async, [=] {
		PopupManager::deleteInstance();
		webserver::WebServerManager::getInstance()->stop();

		shutdown(
			[&](const string& str) { WinUtil::splash->update(str); },
			[=](float progress) { WinUtil::splash->update(progress); }
		);

		webserver::WebServerManager::getInstance()->save();
		webserver::WebServerManager::deleteInstance();

		WinUtil::splash->callAsync([=] { PostQuitMessage(0); });
	});

	nRet = theLoop.Run();
	_Module.RemoveMessageLoop();

	wndMain.reset(nullptr);
	WinUtil::splash->destroy();

	WinUtil::runPendingUpdate();
	return nRet;
}

// this would need better handling that works with multiple threads... handle the common memory-intensive operations separately for now

/*void no_memory_handler() {
	tstring msg = _T("AirDC++ ran out of memory and will now quit. To avoid such issues in future, prefer using partial file lists and don't queue too large amount of files.");
#ifndef _WIN64
	msg += _T(" If you have a 64 bit operating system, switching to the 64 bit version of AirDC++ is heavily recommended.");
#endif
	MessageBox(NULL, msg.c_str(), _T("Out of memory"), MB_OK | MB_ICONERROR);
	std::exit(1);
}*/

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow) {
	SingleInstance dcapp(_T(INST_NAME));

	//std::set_new_handler(no_memory_handler);

#ifndef _DEBUG
	std::set_terminate([]() {
		handleCrash(0, "std::terminate was called", nullptr);
	});
#endif
	
	LPTSTR* argv = __targv;
	int argc = --__argc;
	Util::initialize();

	auto checkParams = [&argc, &argv] () -> void {
		while (argc > 0) {
			Util::addStartupParam(Text::fromT(*argv));
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
				SplashWindow::create();
				WinUtil::splash->update("Updating");
				string sourcePath = Util::getAppFilePath();
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
							Text::toT(shortVersionString).c_str(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_TOPMOST) == IDYES) {
								continue;
						}
					}
					break;
				}

				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
				SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

				//add new startup params
				Util::addStartupParam(success ? "/updated" : "/updatefailed");
				Util::addStartupParam("/silent");

				//append the passed params (but leave out the update commands...)
				argv++;
				checkParams();

				//start the updated instance
				auto path = Text::toT(installPath + Util::getAppFileName());
				auto startupParams = Text::toT(Util::getStartupParams(true));
				if (!startElevated) {
					ShellExecAsUser(NULL, path.c_str(), startupParams.c_str(), NULL);
				} else {
					ShellExecute(NULL, NULL, path.c_str(), startupParams.c_str(), NULL, SW_SHOWNORMAL);
				}

				WinUtil::splash->destroy();
				return FALSE;
			}
			return FALSE;
		} else {
			checkParams();
		}
	}

	string updaterFile;
	auto updated = Util::hasStartupParam("/updated") || Util::hasStartupParam("/updatefailed");
	if (Updater::checkPendingUpdates(Util::getAppFilePath(), updaterFile, updated)) {
		WinUtil::addUpdate(updaterFile);
		WinUtil::runPendingUpdate();
		return FALSE;
	}

	bool multiple = Util::hasStartupParam("/silent");

	if(dcapp.IsAnotherInstanceRunning()) {
		// Allow for more than one instance...
		if(_tcslen(lpstrCmdLine) == 0) {
			if (::MessageBox(NULL, _T("There is already an instance of AirDC++ running.\nDo you want to launch another instance anyway?"), 
				Text::toT(shortVersionString).c_str(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_TOPMOST) == IDYES) {
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
	EXTENDEDTRACEINITIALIZE(Util::getAppFilePath().c_str());
#endif
	LPTOP_LEVEL_EXCEPTION_FILTER pOldSEHFilter = NULL;
	pOldSEHFilter = SetUnhandledExceptionFilter(&DCUnhandledExceptionFilter);
	
	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES |
		ICC_TAB_CLASSES | ICC_UPDOWN_CLASS | ICC_USEREX_CLASSES);	// add flags to support other controls
	
	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));
	
	try {		
		File f(Util::getAppPath(), File::READ, File::OPEN);
		TigerTree tth(TigerTree::calcBlockSize(f.getSize(), 1));
		size_t n = 0;
		size_t n2 = DEBUG_BUFSIZE;
		while( (n = f.read(debugBuf, n2)) > 0) {
			tth.update(debugBuf, n);
			n2 = DEBUG_BUFSIZE;
		}
		tth.finalize();
		strcpy(exeTTH, tth.getRoot().toBase32().c_str());
		WinUtil::tth = Text::toT(exeTTH);
	} catch(const FileException&) {
		dcdebug("Failed reading exe\n");
	}	

	HINSTANCE hInstRich = ::LoadLibrary(RichTextBox::GetLibraryName());

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
