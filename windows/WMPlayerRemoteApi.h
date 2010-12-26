

#if !defined(_REMOTE_API_H_)
#define _REMOTE_API_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <wmp.h>	// WMPSDK file...

class WMPlayerRemoteApi : public CComObjectRootEx<CComSingleThreadModel>, public IServiceProvider, 
    public IWMPRemoteMediaServices  
{
public:
    WMPlayerRemoteApi();
    virtual ~WMPlayerRemoteApi();

    BEGIN_COM_MAP(WMPlayerRemoteApi)
        COM_INTERFACE_ENTRY(IServiceProvider)
        COM_INTERFACE_ENTRY(IWMPRemoteMediaServices)
    END_COM_MAP()

    STDMETHOD(GetServiceType)(BSTR * bstrType);
	STDMETHOD(QueryService)(REFGUID ServiceGuid, REFIID refiid, void ** pvoid);
    STDMETHOD(GetApplicationName)(BSTR * bstrName);
    STDMETHOD(GetScriptableObject)(BSTR * bstrName, IDispatch ** pDispatch);
    STDMETHOD(GetCustomUIMode)(BSTR * bstrFile);

};

#endif // !defined(_REMOTE_API_H_)