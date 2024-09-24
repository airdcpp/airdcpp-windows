// ImageDataObject.h: Impementation for IDataObject Interface to be used 
//					     in inserting bitmap to the RichEdit Control.
//
// Author : Hani Atassi  (atassi@arabteam2000.com)
//
// How to use : Just call the static member InsertBitmap with 
//				the appropriate parrameters. 
//
// Known bugs :
//
//
//////////////////////////////////////////////////////////////////////
#if !defined(AFX_IMAGEDATAOBJECT_H__7E162227_62B8_49E3_A35B_FEC3F241A78F__INCLUDED_)
#define AFX_IMAGEDATAOBJECT_H__7E162227_62B8_49E3_A35B_FEC3F241A78F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

namespace wingui {
class CImageDataObject : IDataObject
{
public:

	static void InsertBitmap(IRichEditOle* pRichEditOle, HBITMAP hBitmap, bool deleteHandle = true);

private:
	ULONG	m_ulRefCnt;
	BOOL	m_bRelease;
	bool duplicate;

	// The data being bassed to the richedit
	//
	STGMEDIUM m_stgmed;
	FORMATETC m_fromat;


public:
	CImageDataObject() : m_ulRefCnt(0), duplicate(false)
	{
		m_bRelease = FALSE;
	}

	~CImageDataObject()
	{
		if (m_bRelease)
			::ReleaseStgMedium(&m_stgmed);
	}

	// Methods of the IUnknown interface
	// 
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{
		if (iid == IID_IUnknown || iid == IID_IDataObject)
		{
			*ppvObject = this;
			AddRef();
			return S_OK;
		}
		else
			return E_NOINTERFACE;
	}
	STDMETHOD_(ULONG, AddRef)(void)
	{
		m_ulRefCnt++;
		return m_ulRefCnt;
	}
	STDMETHOD_(ULONG, Release)(void)
	{
		if (--m_ulRefCnt == 0)
		{
			delete this;
		}

		return m_ulRefCnt;
	}

	// Methods of the IDataObject Interface
	//
	STDMETHOD(GetData)(FORMATETC* /*pformatetcIn*/, STGMEDIUM *pmedium) {
		if(duplicate) {
			HANDLE hDst;
			hDst = ::OleDuplicateData(m_stgmed.hBitmap, CF_BITMAP, NULL);
			if (hDst == NULL)
			{
				return E_HANDLE;
			}
		
		pmedium->tymed = TYMED_GDI;
		pmedium->hBitmap = (HBITMAP)hDst;
		} else {
			pmedium->tymed = TYMED_GDI;
			pmedium->hBitmap = m_stgmed.hBitmap;
		}
		pmedium->pUnkForRelease = NULL;

		return S_OK;
	}

	STDMETHOD(SetData)(FORMATETC* pformatetc , STGMEDIUM*  pmedium , BOOL  /*fRelease*/ ) {
		m_fromat = *pformatetc;
		m_stgmed = *pmedium;

		return S_OK;
	}

	STDMETHOD(GetDataHere)(FORMATETC* /*pformatetc*/, STGMEDIUM*  /*pmedium*/ ) { return E_NOTIMPL; }
	STDMETHOD(QueryGetData)(FORMATETC*  /*pformatetc*/ ) { return E_NOTIMPL; }
	STDMETHOD(GetCanonicalFormatEtc)(FORMATETC*  /*pformatectIn*/ ,FORMATETC* /*pformatetcOut*/ ) 	{ return E_NOTIMPL; }
	STDMETHOD(EnumFormatEtc)(DWORD  /*dwDirection*/ , IEnumFORMATETC**  /*ppenumFormatEtc*/ ) { return E_NOTIMPL; }
	STDMETHOD(DAdvise)(FORMATETC* /*pformatetc*/, DWORD /*advf*/, IAdviseSink* /*pAdvSink*/, DWORD* /*pdwConnection*/) { return E_NOTIMPL; }
	STDMETHOD(DUnadvise)(DWORD /*dwConnection*/) { return E_NOTIMPL; }
	STDMETHOD(EnumDAdvise)(IEnumSTATDATA** /*ppenumAdvise*/) { return E_NOTIMPL; }

	// Some Other helper functions
	//
	SCODE SetBitmap(HBITMAP hBitmap);
	SCODE GetOleObject(IOleClientSite *pOleClientSite, IStorage *pStorage, IOleObject** pOleObject);
	SCODE createStorage(IStorage **pStorage, LPLOCKBYTES *lpLockBytes);

};
}

#endif // !defined(AFX_IMAGEDATAOBJECT_H__7E162227_62B8_49E3_A35B_FEC3F241A78F__INCLUDED_)
