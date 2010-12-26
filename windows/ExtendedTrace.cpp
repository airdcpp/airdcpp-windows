//////////////////////////////////////////////////////////////////////////////////////
//
// Written by Zoltan Csizmadia, zoltan_csizmadia@yahoo.com
// For companies(Austin,TX): If you would like to get my resume, send an email.
//
// The source is free, but if you want to use it, mention my name and e-mail address
//
// History:
//    1.0      Initial version                  Zoltan Csizmadia
//
//////////////////////////////////////////////////////////////////////////////////////
//
// ExtendedTrace.cpp
//

// Include StdAfx.h, if you're using precompiled 
// header through StdAfx.h
#include "stdafx.h"

#if defined(_WIN32)

#include "../client/DCPlusPlus.h"
#include "../client/File.h"
#include "WinUtil.h"

#include <tchar.h>
#include <DbgHelp.h>
#include "ExtendedTrace.h"

#define BUFFERSIZE		0x200
#define LIBCOUNT		47

TCHAR* crashLibs[LIBCOUNT][2] = {
	{ L"Vlsp", L"V-One Smartpass" },
	{ L"mclsp", L"McAfee AV" },
	{ L"Niphk", L"Norman AV" },
	{ L"aslsp", L"Aventail Corporation VPN" },
	{ L"AXShlEx", L"Alcohol 120%" },
	{ L"gdlsphlr", L"McAfee" },
	{ L"mlang", L"IE" },
	{ L"cslsp", L"McAfee" },
	{ L"winsflt", L"PureSight Internet Content Filter" },
	{ L"imslsp", L"ZoneLabs IM Secure" },
	{ L"apitrap", L"Norton Cleansweep [?]" },
	{ L"sockspy", L"BitDefender Antivirus" },
	{ L"imon", L"Eset NOD32" },
	{ L"KvWspXp(_1)", L"Kingsoft Antivirus" },
	{ L"nl_lsp", L"NetLimiter" },
	{ L"OSMIM", L"Marketscore Internet Accelerator" },
	{ L"opls", L"Opinion Square [malware]" },
	{ L"PavTrc", L"Panda Anti-Virus" },
	{ L"pavlsp", L"Panda Anti-Virus" },
	{ L"AppToPort", L"Wyvern Works  Firewall" },
	{ L"SpyDll", L"Nice Spy [malware]" },
	{ L"WBlind", L"Window Blinds" },
	{ L"UPS10", L"Uniscribe Unicode Script Processor Library" },
	{ L"SOCKS32", L"Sockscap [?]" },
	{ L"___j", L"Worm: W32.Maslan.C@mm" },
	{ L"nvappfilter", L"NVidia nForce Network Access Manager" },
	{ L"mshp32", L"Worm: W32.Worm.Feebs" },
	{ L"ProxyFilter", L"Hide My IP 2007" },
	{ L"msui32", L"Malware MSUI32" },
	{ L"fsma32", L"F-Secure Management Agent" },
	{ L"FSLSP", L"F-Secure Antivirus/Internet Security" },
	{ L"msxq32", L"Trojan.Win32.Agent.bi" },
	{ L"CurXP0", L"Stardock CursorXP" },
	{ L"msnq32", L"Trojan" },
	{ L"proxy32", L"FreeCap" },
	{ L"iFW_Xfilter", L"System Mechanic Professional 7's Firewall" },
	{ L"spi", L"Ashampoo Firewall" },
	{ L"haspnt32", L"AdWare.Win32.BHO.cw" },
	{ L"TCompLsp", L"Traffic Compressor" },
	{ L"MSCTF", L"Microsoft Text Service Module" },
	{ L"radhslib", L"Naomi web filter" },
	{ L"msftp", L"Troj/Agent-GNA" },
	{ L"ftp34", L"Troj/Agent-GZF" },
	{ L"imonlsp", L"Internet Monitor Layered Service provider" },
	{ L"McVSSkt", L"McAfee VirusScan Winsock Helper" },
	{ L"adguard", L"Sir AdGuard" },
	{ L"msjetoledb40", L"Microsoft Jet 4.0" }
};

static void checkBuggyLibrary(PCSTR library) {
	for(int i = 0; i < LIBCOUNT; i++) {
		string lib = Text::fromT(crashLibs[i][0]); tstring app = crashLibs[i][1];
		if(stricmp(library, lib) == 0) {
			size_t BUF_SIZE = TSTRING(LIB_CRASH).size() + app.size() + 16;
			
			tstring buf;
			buf.resize(BUF_SIZE);

			snwprintf(&buf[0], buf.size(), CTSTRING(LIB_CRASH), app.c_str());
		
			MessageBox(0, &buf[0], _T("Unhandled exception"), MB_OK);
			exit(1);
		}
	}
}

// Unicode safe char* -> TCHAR* conversion
void PCSTR2LPTSTR( PCSTR lpszIn, LPTSTR lpszOut )
{
#if defined(UNICODE)||defined(_UNICODE)
   ULONG index = 0; 
   PCSTR lpAct = lpszIn;
   
	for( ; ; lpAct++ )
	{
		lpszOut[index++] = (TCHAR)(*lpAct);
		if ( *lpAct == 0 )
			break;
	} 
#else
   // This is trivial :)
	strcpy( lpszOut, lpszIn );
#endif
}

// Let's figure out the path for the symbol files
// Search path= ".;%_NT_SYMBOL_PATH%;%_NT_ALTERNATE_SYMBOL_PATH%;%SYSTEMROOT%;%SYSTEMROOT%\System32;" + lpszIniPath
// Note: There is no size check for lpszSymbolPath!
static void InitSymbolPath( PSTR lpszSymbolPath, PCSTR lpszIniPath )
{
	CHAR lpszPath[BUFFERSIZE];

   // Creating the default path
   // ".;%_NT_SYMBOL_PATH%;%_NT_ALTERNATE_SYMBOL_PATH%;%SYSTEMROOT%;%SYSTEMROOT%\System32;"
	strcpy( lpszSymbolPath, "." );

	// environment variable _NT_SYMBOL_PATH
	if ( GetEnvironmentVariableA( "_NT_SYMBOL_PATH", lpszPath, BUFFERSIZE ) )
	{
	   strcat( lpszSymbolPath, ";" );
		strcat( lpszSymbolPath, lpszPath );
	}

	// environment variable _NT_ALTERNATE_SYMBOL_PATH
	if ( GetEnvironmentVariableA( "_NT_ALTERNATE_SYMBOL_PATH", lpszPath, BUFFERSIZE ) )
	{
	   strcat( lpszSymbolPath, ";" );
		strcat( lpszSymbolPath, lpszPath );
	}

	// environment variable SYSTEMROOT
	if ( GetEnvironmentVariableA( "SYSTEMROOT", lpszPath, BUFFERSIZE ) )
	{
	   strcat( lpszSymbolPath, ";" );
		strcat( lpszSymbolPath, lpszPath );
		strcat( lpszSymbolPath, ";" );

		// SYSTEMROOT\System32
		strcat( lpszSymbolPath, lpszPath );
		strcat( lpszSymbolPath, "\\System32" );
	}

   // Add user defined path
	if ( lpszIniPath != NULL )
		if ( lpszIniPath[0] != '\0' )
		{
		   strcat( lpszSymbolPath, ";" );
			strcat( lpszSymbolPath, lpszIniPath );
		}
}

// Uninitialize the loaded symbol files
BOOL UninitSymInfo() {
	return SymCleanup( GetCurrentProcess() );
}

// Initializes the symbol files
BOOL InitSymInfo( PCSTR lpszInitialSymbolPath )
{
	SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_LOAD_LINES );
	CHAR     lpszSymbolPath[BUFFERSIZE];
	InitSymbolPath( lpszSymbolPath, lpszInitialSymbolPath );

	return SymInitialize( GetCurrentProcess(), lpszSymbolPath, TRUE);
}

// Get the module name from a given address
static BOOL GetModuleNameFromAddress( DWORD64 address, LPTSTR lpszModule )
{
	BOOL              ret = FALSE;
	IMAGEHLP_MODULE   moduleInfo;

	::ZeroMemory( &moduleInfo, sizeof(moduleInfo) );
	moduleInfo.SizeOfStruct = sizeof(moduleInfo);

	if ( SymGetModuleInfo( GetCurrentProcess(), (DWORD)address, &moduleInfo ) )
	{
	   // Got it!
		PCSTR2LPTSTR( moduleInfo.ModuleName, lpszModule );

		checkBuggyLibrary(moduleInfo.ModuleName);

		ret = TRUE;
	}
	else
	   // Not found :(
		_tcscpy( lpszModule, _T("?") );
	
	return ret;
}

// Get function prototype and parameter info from ip address and stack address
static BOOL GetFunctionInfoFromAddresses( DWORD64 fnAddress, DWORD64 stackAddress, LPTSTR lpszSymbol )
{
	BOOL              ret = FALSE;
	DWORD64             dwDisp = 0;
	DWORD             dwSymSize = 1024*16;
   TCHAR             lpszUnDSymbol[BUFFERSIZE]=_T("?");
	CHAR              lpszNonUnicodeUnDSymbol[BUFFERSIZE]="?";
	LPTSTR            lpszParamSep = NULL;
	LPCTSTR           lpszParsed = lpszUnDSymbol;
	PSYMBOL_INFO  pSym = (PSYMBOL_INFO)GlobalAlloc( GMEM_FIXED, dwSymSize );

	::ZeroMemory( pSym, dwSymSize );
	pSym->SizeOfStruct = dwSymSize;
	pSym->MaxNameLen = dwSymSize - sizeof(IMAGEHLP_SYMBOL);

   // Set the default to unknown
	_tcscpy( lpszSymbol, _T("?") );

	// Get symbol info for IP
	if ( SymFromAddr( GetCurrentProcess(), fnAddress, &dwDisp, pSym ) )
	{
	   // Make the symbol readable for humans
		UnDecorateSymbolName( pSym->Name, lpszNonUnicodeUnDSymbol, BUFFERSIZE, 
			UNDNAME_COMPLETE | 
			UNDNAME_NO_THISTYPE |
			UNDNAME_NO_SPECIAL_SYMS |
			UNDNAME_NO_MEMBER_TYPE |
			UNDNAME_NO_MS_KEYWORDS |
			UNDNAME_NO_ACCESS_SPECIFIERS );
		// Symbol information is ANSI string
		PCSTR2LPTSTR( lpszNonUnicodeUnDSymbol, lpszUnDSymbol );

      // I am just smarter than the symbol file :)
		if ( _tcscmp(lpszUnDSymbol, _T("_WinMain@16")) == 0 )
			_tcscpy(lpszUnDSymbol, _T("WinMain(HINSTANCE,HINSTANCE,LPCTSTR,int)"));
		else
		if ( _tcscmp(lpszUnDSymbol, _T("_main")) == 0 )
			_tcscpy(lpszUnDSymbol, _T("main(int,TCHAR * *)"));
		else
		if ( _tcscmp(lpszUnDSymbol, _T("_mainCRTStartup")) == 0 )
			_tcscpy(lpszUnDSymbol, _T("mainCRTStartup()"));
		else
		if ( _tcscmp(lpszUnDSymbol, _T("_wmain")) == 0 )
			_tcscpy(lpszUnDSymbol, _T("wmain(int,TCHAR * *,TCHAR * *)"));
		else
		if ( _tcscmp(lpszUnDSymbol, _T("_wmainCRTStartup")) == 0 )
			_tcscpy(lpszUnDSymbol, _T("wmainCRTStartup()"));

		lpszSymbol[0] = _T('\0');

      // Let's go through the stack, and modify the function prototype, and insert the actual
      // parameter values from the stack
		if ( _tcsstr( lpszUnDSymbol, _T("(void)") ) == NULL && _tcsstr( lpszUnDSymbol, _T("()") ) == NULL)
		{
			ULONG index = 0;
			for( ; ; index++ )
			{
				lpszParamSep = _tcschr( const_cast<wchar_t*>(lpszParsed), _T(',') );
				if ( lpszParamSep == NULL )
					break;

				*lpszParamSep = _T('\0');

				_tcscat( lpszSymbol, lpszParsed );
				_stprintf( lpszSymbol + _tcslen(lpszSymbol), _T("=0x%08lX,"), *((ULONG*)(stackAddress) + 2 + index) );

				lpszParsed = lpszParamSep + 1;
			}

			lpszParamSep = _tcschr( const_cast<wchar_t*>(lpszParsed), _T(')') );
			if ( lpszParamSep != NULL )
			{
				*lpszParamSep = _T('\0');

				_tcscat( lpszSymbol, lpszParsed );
				_stprintf( lpszSymbol + _tcslen(lpszSymbol), _T("=0x%08lX)"), *((ULONG*)(stackAddress) + 2 + index) );

				lpszParsed = lpszParamSep + 1;
			}
		}

		_tcscat( lpszSymbol, lpszParsed );
   
		ret = TRUE;
	} 
	GlobalFree( pSym );

	return ret;
}

// Get source file name and line number from IP address
// The output format is: "sourcefile(linenumber)" or
//                       "modulename!address" or
//                       "address"
#ifndef _WIN64
static BOOL GetSourceInfoFromAddress( DWORD address, LPTSTR lpszSourceInfo )
#else
static BOOL GetSourceInfoFromAddress( DWORD64 address, LPTSTR lpszSourceInfo )
#endif

{
	BOOL           ret = FALSE;
	IMAGEHLP_LINE  lineInfo;
	DWORD          dwDisp;
	TCHAR          lpszFileName[BUFFERSIZE] = _T("");
	TCHAR          lpModuleInfo[BUFFERSIZE] = _T("");

	_tcscpy( lpszSourceInfo, _T("?(?)") );

	::ZeroMemory( &lineInfo, sizeof( lineInfo ) );
	lineInfo.SizeOfStruct = sizeof( lineInfo );

	if ( SymGetLineFromAddr( GetCurrentProcess(), address, &dwDisp, &lineInfo ) )
	{
	   // Got it. Let's use "sourcefile(linenumber)" format
		PCSTR2LPTSTR( lineInfo.FileName, lpszFileName );
		_stprintf( lpszSourceInfo, _T("%s(%d)"), lpszFileName, lineInfo.LineNumber );
		ret = TRUE;
	}
	else
	{
      // There is no source file information. :(
      // Let's use the "modulename!address" format
	  	GetModuleNameFromAddress( address, lpModuleInfo );

		if ( lpModuleInfo[0] == _T('?') || lpModuleInfo[0] == _T('\0'))
		   // There is no modulename information. :((
         // Let's use the "address" format
			_stprintf( lpszSourceInfo, _T("0x%08X"), address );
		else
			_stprintf( lpszSourceInfo, _T("%s!0x%08X"), lpModuleInfo, address );

		ret = FALSE;
	}
	
	return ret;
}

#ifndef _M_AMD64
void StackTrace( HANDLE hThread, LPCTSTR lpszMessage, File& f, DWORD eip, DWORD esp, DWORD ebp )
#else
void StackTrace( HANDLE hThread, LPCTSTR lpszMessage, File& f, DWORD64 eip, DWORD64 esp, DWORD64 ebp )
#endif
{
	STACKFRAME     callStack;
	BOOL           bResult;
	TCHAR          symInfo[BUFFERSIZE] = _T("?");
	TCHAR          srcInfo[BUFFERSIZE] = _T("?");
	HANDLE         hProcess = GetCurrentProcess();

	// If it's not this thread, let's suspend it, and resume it at the end
	if ( hThread != GetCurrentThread() )
		if ( SuspendThread( hThread ) == -1 )
		{
			// whaaat ?!
			f.write(LIT("No call stack\r\n"));
			return;
		}

		::ZeroMemory( &callStack, sizeof(callStack) );
		callStack.AddrPC.Offset    = eip;
		callStack.AddrStack.Offset = esp;
		callStack.AddrFrame.Offset = ebp;
		callStack.AddrPC.Mode      = AddrModeFlat;
		callStack.AddrStack.Mode   = AddrModeFlat;
		callStack.AddrFrame.Mode   = AddrModeFlat;

		f.write(Text::fromT(lpszMessage));

		GetFunctionInfoFromAddresses( callStack.AddrPC.Offset, callStack.AddrFrame.Offset, symInfo );
		GetSourceInfoFromAddress( callStack.AddrPC.Offset, srcInfo );

		f.write(Text::fromT(srcInfo));
		f.write(LIT(": "));
		f.write(Text::fromT(symInfo));
		f.write(LIT("\r\n"));

		// Max 100 stack lines...
		for( ULONG index = 0; index < 100; index++ ) 
		{
			bResult = StackWalk(
#ifdef _M_AMD64
				IMAGE_FILE_MACHINE_AMD64,
#else
				IMAGE_FILE_MACHINE_I386,
#endif
				hProcess,
				hThread,
				&callStack,
				NULL, 
				NULL,
				SymFunctionTableAccess,
				SymGetModuleBase,
				NULL);

			if ( index == 0 )
				continue;

			if( !bResult || callStack.AddrFrame.Offset == 0 ) 
				break;

			GetFunctionInfoFromAddresses( callStack.AddrPC.Offset, callStack.AddrFrame.Offset, symInfo );
			GetSourceInfoFromAddress( callStack.AddrPC.Offset, srcInfo );

			f.write(Text::fromT(srcInfo));
			f.write(LIT(": "));
			f.write(Text::fromT(symInfo));
			f.write(LIT("\r\n"));

		}
		if ( hThread != GetCurrentThread() )
			ResumeThread( hThread );
}

#endif //_DEBUG && _WIN32

/**
* @file
* $Id: ExtendedTrace.cpp 500 2010-06-25 22:08:18Z bigmuscle $
*/
