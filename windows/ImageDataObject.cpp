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
SCODE CImageDataObject::createStorage(IStorage **pStorage, LPLOCKBYTES *lpLockBytes){
	// Initialize a Storage Object

	SCODE sc = ::CreateILockBytesOnHGlobal(NULL, TRUE, lpLockBytes);
	if(sc != S_OK || lpLockBytes == NULL) 
		return NULL;
	
	sc = ::StgCreateDocfileOnILockBytes(*lpLockBytes,
		STGM_SHARE_EXCLUSIVE|STGM_CREATE|STGM_READWRITE, 0, pStorage);
	if (sc != S_OK) 
		return NULL;
	
	return sc;
}
void CImageDataObject::InsertBitmap(IRichEditOle* pRichEditOle, HBITMAP hBitmap, bool deleteHandle/*true*/ )
{
	if(!pRichEditOle || !hBitmap)
		return;

	LPDATAOBJECT lpDataObject = NULL;
	IOleClientSite *pOleClientSite = NULL;	
	IStorage *pStorage  = NULL;	
	LPLOCKBYTES lpLockBytes = NULL;
	IOleObject *pOleObject = NULL;
	SCODE sc = NULL;
	CLSID clsid = CLSID_NULL;

	// Get the image data object
	CImageDataObject *pods = new CImageDataObject;
	pods->duplicate = !deleteHandle;

	try {
		// Get the RichEdit container site
		if((sc = pRichEditOle->GetClientSite(&pOleClientSite) != S_OK)) throw sc;
		if((sc = pods->QueryInterface(IID_IDataObject, (void **)&lpDataObject)) != S_OK) throw sc;
		if((sc = pods->SetBitmap(hBitmap) != S_OK)) throw sc;
		if((sc = pods->createStorage(&pStorage, &lpLockBytes)) != S_OK) throw sc;
		// The final ole object which will be inserted in the richedit control
		if((sc = pods->GetOleObject(pOleClientSite, pStorage, &pOleObject)) != S_OK) throw sc;
		
		if((sc = pOleObject->GetUserClassID(&clsid)) != S_OK) throw sc;
		// Now Add the object to the RichEdit 
		REOBJECT reobject;
		ZeroMemory(&reobject, sizeof(REOBJECT));
		reobject.cbStruct = sizeof(REOBJECT);
		reobject.clsid = clsid;
		reobject.cp = REO_CP_SELECTION;
		reobject.dvaspect = DVASPECT_CONTENT;
		reobject.dwFlags = REO_BELOWBASELINE;
		reobject.poleobj = pOleObject;
		reobject.polesite = pOleClientSite;
		reobject.pstg = pStorage;

		// Insert the bitmap at the current location in the richedit control
		pRichEditOle->InsertObject(&reobject);
	
	} catch(...) { }

	// Release all unnecessary interfaces
	
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

SCODE CImageDataObject::SetBitmap(HBITMAP hBitmap)
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

	return this->SetData(&fm, &stgm, TRUE);		
}

SCODE CImageDataObject::GetOleObject(IOleClientSite *pOleClientSite, IStorage *pStorage, IOleObject** pOleObject)
{
	SCODE sc;
	if((sc = ::OleCreateStaticFromData(this, IID_IOleObject, OLERENDER_FORMAT, &m_fromat, pOleClientSite, pStorage, (void **)pOleObject)) != S_OK)
		return sc;

	// all items are "contained" -- this makes our reference to this object
	//  weak -- which is needed for links to embedding silent update.
	sc = OleSetContainedObject(*pOleObject, TRUE);
		
	return sc;
}
