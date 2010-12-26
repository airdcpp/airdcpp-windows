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

#include "stdafx.h"
#include "ImageDataObject.h"

// Static member functions
void CImageDataObject::InsertBitmap(IRichEditOle* pRichEditOle, HBITMAP hBitmap) {
	
	if(!pRichEditOle || !hBitmap)
		return;

	LPLOCKBYTES lpLockBytes = NULL;
	SCODE sc = ::CreateILockBytesOnHGlobal(NULL, TRUE, &lpLockBytes);

	if(sc != S_OK || lpLockBytes == NULL) {
		DeleteObject(hBitmap);
		pRichEditOle->Release();	
		dcdebug("Thrown OLE Exception: %d\n", sc);
		return;
	}

	// Initialize a Storage Object
	IStorage *pStorage = NULL;	
	sc = ::StgCreateDocfileOnILockBytes(lpLockBytes,
		STGM_SHARE_EXCLUSIVE|STGM_CREATE|STGM_READWRITE, 0, &pStorage);
	
	if(sc != S_OK) {
		DeleteObject(hBitmap);
		pRichEditOle->Release();	
		lpLockBytes->Release();
		lpLockBytes = NULL;
		dcdebug("Thrown OLE Exception: %d\n", sc);
		return;
	}
	dcassert(pStorage != NULL);

	CImageDataObject pods;
	pods.SetBitmap(hBitmap);
	
	// Get the RichEdit container site
	IOleClientSite *pOleClientSite;	
	pRichEditOle->GetClientSite(&pOleClientSite);

	if(pOleClientSite == NULL) {
		DeleteObject(hBitmap);
		pRichEditOle->Release();	
		lpLockBytes->Release();
		lpLockBytes = NULL;
		dcdebug("pOleClientSite is NULL\n");
		return;
	}
		
	// The final ole object which will be inserted in the richedit control
	IOleObject* pOleObject = pods.GetOleObject(pOleClientSite, pStorage);

	if(pOleObject != NULL) {
		// all items are "contained" -- this makes our reference to this object
		//  weak -- which is needed for links to embedding silent update.
		OleSetContainedObject(pOleObject, TRUE);		

		// Now Add the object to the RichEdit 
		REOBJECT reobject = { 0 };
		reobject.cbStruct = sizeof(REOBJECT);
	
		CLSID clsid;
		sc = pOleObject->GetUserClassID(&clsid);
		
		if(sc != S_OK) {
			pRichEditOle->Release();
			DeleteObject(hBitmap);
			dcdebug("Thrown OLE Exception: %d\n", sc);
			return;
		}

		reobject.clsid = clsid;
		reobject.cp = REO_CP_SELECTION;
		reobject.dvaspect = DVASPECT_CONTENT;
		reobject.dwFlags = REO_BELOWBASELINE;
		reobject.poleobj = pOleObject;
		reobject.polesite = pOleClientSite;
		reobject.pstg = pStorage;

		// Insert the bitmap at the current location in the richedit control
		pRichEditOle->InsertObject(&reobject);

		// Release all unnecessary interfaces
		pOleObject->Release();
	}

	pOleClientSite->Release();
	lpLockBytes->Release();
	pStorage->Release();
	pRichEditOle->Release();
	DeleteObject(hBitmap);
}

void CImageDataObject::SetBitmap(HBITMAP hBitmap) {
	dcassert(hBitmap);

	STGMEDIUM stgm;
	stgm.tymed = TYMED_GDI;
	stgm.hBitmap = hBitmap;
	stgm.pUnkForRelease = NULL;

	FORMATETC fm;
	fm.cfFormat = CF_BITMAP;
	fm.ptd = NULL;
	fm.dwAspect = DVASPECT_CONTENT;
	fm.lindex = -1;
	fm.tymed = TYMED_GDI;

	this->SetData(&fm, &stgm, TRUE);		
}

IOleObject *CImageDataObject::GetOleObject(IOleClientSite *pOleClientSite, IStorage *pStorage) {
	dcassert(m_stgmed.hBitmap);

	IOleObject *pOleObject;
	
	SCODE sc = ::OleCreateStaticFromData(this, IID_IOleObject, OLERENDER_FORMAT, 
			&m_fromat, pOleClientSite, pStorage, (void **)&pOleObject);
	
	if (sc != S_OK) {
		dcdebug("Thrown OLE Exception: %d\n", sc);
		return NULL;
	}
	return pOleObject;
}
