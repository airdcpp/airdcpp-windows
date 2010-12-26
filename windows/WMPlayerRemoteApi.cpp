
#include "stdafx.h"
#include "WMPlayerRemoteApi.h"
#include "../client/version.h"

WMPlayerRemoteApi::WMPlayerRemoteApi() { }
WMPlayerRemoteApi::~WMPlayerRemoteApi() { }

HRESULT WMPlayerRemoteApi::GetServiceType(BSTR * bstrType) {
    HRESULT hresult = E_POINTER;
    if(bstrType) {
        *bstrType = ::SysAllocString(L"Remote");
        hresult = *bstrType? S_OK : E_POINTER;
    }
    return hresult;
}

HRESULT WMPlayerRemoteApi::QueryService(REFGUID /*ServiceGuid*/, REFIID refiid, void ** pvoid) {
    return pvoid? QueryInterface(refiid, pvoid) : E_POINTER;
}

HRESULT WMPlayerRemoteApi::GetApplicationName(BSTR * bstrName) {
    HRESULT hresult = E_POINTER;
    if(bstrName) {
        CComBSTR bstrApplicationName = _T(APPNAME);
		*bstrName = bstrApplicationName.Detach();
        hresult = *bstrName? S_OK : E_POINTER;
    }
    return hresult;
}

HRESULT WMPlayerRemoteApi::GetScriptableObject(BSTR * bstrName, IDispatch ** pDispatch) {
    if(bstrName) {
        *bstrName = NULL;
    }
    if(pDispatch) {
        *pDispatch = NULL;
    }
    return E_NOTIMPL;
}

HRESULT WMPlayerRemoteApi::GetCustomUIMode(BSTR * bstrFile) {
	if(bstrFile) {
		*bstrFile = NULL;
	}
    return E_NOTIMPL;
}
