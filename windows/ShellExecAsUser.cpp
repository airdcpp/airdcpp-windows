
#include "stdafx.h"
#include "../client/stdinc.h"
#include "ShellExecAsUser.h"

#include <exdisp.h>
#include <Shobjidl.h>
#include <Shlwapi.h>
#include <SHLGUID.h>

#include "../client/Text.h"
#include "WinUtil.h"

#include <sddl.h>

namespace dcpp {

#undef ShellExecute


static wchar_t *ansi_to_utf16(const char *input)
{
	wchar_t *Buffer;
	int BuffSize, Result;
	BuffSize = MultiByteToWideChar(CP_ACP, 0, input, -1, NULL, 0);
	if(BuffSize > 0)
	{
		Buffer = new wchar_t[BuffSize];
		Result = MultiByteToWideChar(CP_UTF8, 0, input, -1, Buffer, BuffSize);
		return ((Result > 0) && (Result <= BuffSize)) ? Buffer : NULL;
	}
	return NULL;
}

#ifdef _UNICODE
	#define ALLOC_STRING(STR) SysAllocString(STR)
#else
	static inline BSTR ALLOC_STRING(const char *STR)
	{
		BSTR result = NULL;
		wchar_t *temp = ansi_to_utf16(STR);
		if(temp)
		{
			result = SysAllocString(temp);
			delete [] temp;
		}
		return result;
	}
#endif

class variant_t
{
public:
	variant_t(void) { VariantInit(&data); }
	variant_t(const TCHAR *str) { VariantInit(&data); if(str != NULL) setString(str); }
	variant_t(const LONG value) { VariantInit(&data); setIValue(value); }
	~variant_t(void) { VariantClear(&data); }
	void setIValue(const LONG value) { VariantClear(&data); data.vt = VT_I4; data.lVal = value; }
	void setOleStr(const BSTR value) { VariantClear(&data); if(value != NULL) { data.vt = VT_BSTR; data.bstrVal = value; } }
	void setString(const TCHAR *str) { 
		VariantClear(&data); 
		if(str != NULL) { 
			setOleStr(ALLOC_STRING(str)); 
		} 
	}
	VARIANT &get(void) { return data; }
private:
	VARIANT data;
};

int ShellExecAsUser(const TCHAR *pcOperation, const TCHAR *pcFileName, const TCHAR *pcParameters, const HWND parentHwnd)
{
	/*BOOL bRet;
	HANDLE hToken;
	HANDLE hNewToken;

	// Notepad is used as an example
	//WCHAR wszProcessName[MAX_PATH] = L"C:\\Windows\\Notepad.exe";

	// Low integrity SID: 0x1000 = 4096. To use Medium integrity, use 0x2000 = 8192
	WCHAR wszIntegritySid[20] = L"S-1-16-4096";
	PSID pIntegritySid = NULL;

	TOKEN_MANDATORY_LABEL TIL = {0};
	PROCESS_INFORMATION ProcInfo = {0};
	STARTUPINFO StartupInfo = {0};
	ULONG ExitCode = 0;

	if (OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &hToken)) {
		if (DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &hNewToken)) {
			if (ConvertStringSidToSid(wszIntegritySid, &pIntegritySid)) {
				TIL.Label.Attributes = SE_GROUP_INTEGRITY;
				TIL.Label.Sid = pIntegritySid;

				// Set the process integrity level
				if (SetTokenInformation(hNewToken, TokenIntegrityLevel, &TIL, sizeof(TOKEN_MANDATORY_LABEL) + GetLengthSid(pIntegritySid))) {
					// Create the new process at Low integrity
					bRet = CreateProcessAsUser(hNewToken, NULL, (LPWSTR)pcFileName, NULL, NULL, FALSE, 0, NULL, NULL, &StartupInfo, &ProcInfo);
				}

				LocalFree(pIntegritySid);
			}
			CloseHandle(hNewToken);
		}
		CloseHandle(hToken);
	}

	return bRet;*/

	int bSuccess = 0;

	if (Util::getOsMajor() >= 6) {
		HRESULT hr = CoInitialize(NULL);
		if((hr == S_FALSE) || (hr == S_OK))
		{
			IShellWindows *psw = NULL;
			hr = CoCreateInstance(CLSID_ShellWindows, NULL, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&psw));
			if(SUCCEEDED(hr))
			{
				HWND hwnd = 0;
				IDispatch* pdisp = NULL;
				variant_t vEmpty;
				if(S_OK == psw->FindWindowSW(&vEmpty.get(), &vEmpty.get(), SWC_DESKTOP, (long*)&hwnd, SWFO_NEEDDISPATCH, &pdisp))
				{
					if((hwnd != NULL) && (hwnd != INVALID_HANDLE_VALUE))
					{
						IShellBrowser *psb;
						hr = IUnknown_QueryService(pdisp, SID_STopLevelBrowser, IID_PPV_ARGS(&psb));
						if(SUCCEEDED(hr))
						{
							IShellView *psv = NULL;
							hr = psb->QueryActiveShellView(&psv);
							if(SUCCEEDED(hr))
							{
								IDispatch *pdispBackground = NULL;
								HRESULT hr = psv->GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARGS(&pdispBackground));
								if (SUCCEEDED(hr))
								{
									IShellFolderViewDual *psfvd = NULL;
									hr = pdispBackground->QueryInterface(IID_PPV_ARGS(&psfvd));
									if (SUCCEEDED(hr))
									{
										IDispatch *pdisp = NULL;
										hr = psfvd->get_Application(&pdisp);
										if (SUCCEEDED(hr))
										{
											IShellDispatch2 *psd;
											hr = pdisp->QueryInterface(IID_PPV_ARGS(&psd));
											if(SUCCEEDED(hr))
											{
												variant_t verb(pcOperation);
												variant_t file(pcFileName);
												variant_t para(pcParameters);
												variant_t show(SW_SHOWNORMAL);
												hr = psd->ShellExecute(file.get().bstrVal, para.get(), vEmpty.get(), verb.get(), show.get());
												if(SUCCEEDED(hr)) bSuccess = 1;
												psd->Release();
												psd = NULL;
											}
											pdisp->Release();
											pdisp = NULL;
										}
									}
									pdispBackground->Release();
									pdispBackground = NULL;
								}
								psv->Release();
								psv = NULL;
							}
							psb->Release();
							psb = NULL;
						}
					}
					pdisp->Release();
					pdisp = NULL;
				}
				psw->Release();
				psw = NULL;
			}
			CoUninitialize();
		}
	}

	if(bSuccess < 1)
	{
		dcassert(0);
		HINSTANCE hInst = ShellExecuteW(parentHwnd, pcOperation, pcFileName, pcParameters, NULL, SW_SHOWNORMAL);
		if(((int) hInst) <= 32) bSuccess = -1;
	}

	return bSuccess;
}

}