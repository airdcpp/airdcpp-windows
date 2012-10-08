#ifndef __SHELLEXEC
#define __SHELLEXEC

namespace dcpp {
	int ShellExecAsUser(const TCHAR *pcOperation, const TCHAR *pcFileName, const TCHAR *pcParameters, const HWND parentHwnd);
}

#endif // __SHELLEXEC