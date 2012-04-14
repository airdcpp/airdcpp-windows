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
// ExtendedTrace.h
//

#ifndef EXTENDEDTRACE_H_INCLUDED
#define EXTENDEDTRACE_H_INCLUDED

#if defined(_WIN32)

#include <windows.h>
#include <tchar.h>

#pragma comment( lib, "dbghelp.lib" )

#define EXTENDEDTRACEINITIALIZE( IniSymbolPath )	InitSymInfo( IniSymbolPath )
#define EXTENDEDTRACEUNINITIALIZE()			        UninitSymInfo()
#define STACKTRACE(file, ctx)                       StackTrace(GetCurrentThread(), file, ctx)

BOOL InitSymInfo( PCSTR );
BOOL UninitSymInfo();

#ifndef _WIN64
void StackTrace( HANDLE hThread, File& f, const PCONTEXT pCtx);
#else
void StackTrace( HANDLE hThread, File& f, const PCONTEXT pCtx);
#endif

#else

#define EXTENDEDTRACEINITIALIZE( IniSymbolPath )   ((void)0)
#define EXTENDEDTRACEUNINITIALIZE()			         ((void)0)
#define STACKTRACE(file, ctx)                        ((void)0)

#endif

#endif

/**
 * @file
 * $Id: ExtendedTrace.h 394 2008-06-28 22:28:44Z BigMuscle $
 */
