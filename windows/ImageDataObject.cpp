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
void CImageDataObject::InsertBitmap(IRichEditOle* pRichEditOle, HBITMAP hBitmap, bool deleteHandle/*true*/ )
{
	if(!pRichEditOle || !hBitmap)
		return;

	LPDATAOBJECT lpDataObject = NULL;
	IOleClientSite *pOleClientSite = NULL;	
	IStorage *pStorage  = NULL;	
	LPLOCKBYTES lpLockBytes = NULL;
	
	SCODE sc = NULL;
	CLSID clsid = CLSID_NULL;

	// Get the image data object
	//
	CImageDataObject *pods = new CImageDataObject;
	pods->duplicate = !deleteHandle;
	pods->QueryInterface(IID_IDataObject, (void **)&lpDataObject);
	pods->SetBitmap(hBitmap);

	// Get the RichEdit container site
	//
	pRichEditOle->GetClientSite(&pOleClientSite);

	// Initialize a Storage Object
	//
	sc = ::CreateILockBytesOnHGlobal(NULL, TRUE, &lpLockBytes);
	if(sc != S_OK || lpLockBytes == NULL) {
		if(deleteHandle) DeleteObject(hBitmap);
		pRichEditOle->Release();	
		return;
	}
	
	sc = ::StgCreateDocfileOnILockBytes(lpLockBytes,
		STGM_SHARE_EXCLUSIVE|STGM_CREATE|STGM_READWRITE, 0, &pStorage);
	if (sc != S_OK) {
		if(deleteHandle) DeleteObject(hBitmap);
		pRichEditOle->Release();
		lpLockBytes->Release();
		lpLockBytes = NULL;
		return;
	}

	// The final ole object which will be inserted in the richedit control
	//
	IOleObject *pOleObject = pods->GetOleObject(pOleClientSite, pStorage);
		
	if(pOleObject != NULL) {

		// all items are "contained" -- this makes our reference to this object
		//  weak -- which is needed for links to embedding silent update.
		OleSetContainedObject(pOleObject, TRUE);
		// Now Add the object to the RichEdit 
		//
		REOBJECT reobject;
		ZeroMemory(&reobject, sizeof(REOBJECT));
		reobject.cbStruct = sizeof(REOBJECT);
	
		sc = pOleObject->GetUserClassID(&clsid);
		if (sc != S_OK) {
			pOleObject->Release();
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
		//
		pRichEditOle->InsertObject(&reobject);
	}

	// Release all unnecessary interfaces
	//
	//TODO: rewrite usage of these handles, these should be used with the richedit instance rather than with every icon!
	if(pOleObject) pOleObject->Release();
	if(pOleClientSite) pOleClientSite->Release();
	if(pStorage) pStorage->Release();
	if(lpDataObject) lpDataObject->Release();
	if(lpLockBytes) lpLockBytes->Release();
	if(deleteHandle) DeleteObject(hBitmap);
		
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void CImageDataObject::SetBitmap(HBITMAP hBitmap)
{
	
	STGMEDIUM stgm;
	stgm.tymed = TYMED_GDI;					// Storage medium = HBITMAP handle		
	stgm.hBitmap = hBitmap;
	stgm.pUnkForRelease = NULL;				// Use ReleaseStgMedium

	FORMATETC fm;
	fm.cfFormat = CF_BITMAP;				// Clipboard format = CF_BITMAP
	fm.ptd = NULL;							// Target Device = Screen
	fm.dwAspect = DVASPECT_CONTENT;			// Level of detail = Full content
	fm.lindex = -1;							// Index = Not applicaple
	fm.tymed = TYMED_GDI;					// Storage medium = HBITMAP handle

	this->SetData(&fm, &stgm, TRUE);		
}

IOleObject *CImageDataObject::GetOleObject(IOleClientSite *pOleClientSite, IStorage *pStorage)
{
	SCODE sc;
	IOleObject *pOleObject;
	sc = ::OleCreateStaticFromData(this, IID_IOleObject, OLERENDER_FORMAT, 
			&m_fromat, pOleClientSite, pStorage, (void **)&pOleObject);
	if (sc != S_OK)
		return NULL;

	return pOleObject;
}
