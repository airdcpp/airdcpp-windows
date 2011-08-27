/* 
 * Copyright (C) 2006-2011 Crise, crise<at>mail.berlios.de
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef RESOURCE_LOADER_H
#define RESOURCE_LOADER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef __ATLMISC_H__
#define __ATLTYPES_H__
#endif

#include "../client/Singleton.h"
#include "../client/Pointer.h"
#include "../client/FastAlloc.h"
#include <atlimage.h>

class ExCImage : public CImage, public FastAlloc<ExCImage>, public intrusive_ptr_base<ExCImage>, boost::noncopyable
{
public:
	typedef boost::intrusive_ptr<ExCImage> Ptr;

	ExCImage() {
		m_hBuffer = NULL;
	}
	ExCImage(LPCTSTR pszFileName) throw() {
		m_hBuffer = NULL; Load(pszFileName);
	}
	ExCImage(UINT id, LPCTSTR pType = RT_RCDATA, HMODULE hInst = NULL) throw() {
		m_hBuffer = NULL; LoadFromResource(id, pType, hInst);
	}
	ExCImage(UINT id, UINT type, HMODULE hInst = NULL) throw() { 
		m_hBuffer = NULL; LoadFromResource(id, MAKEINTRESOURCE(type), hInst);
	}

	~ExCImage() throw() { Destroy(); }

	bool LoadFromResource(UINT id, LPCTSTR pType = RT_RCDATA, HMODULE hInst = NULL) throw();
	void Destroy() throw();

private:
	HGLOBAL m_hBuffer;
};

inline void ExCImage::Destroy() throw() {
	CImage::Destroy();
	if(m_hBuffer) {
		::GlobalUnlock(m_hBuffer);
		::GlobalFree(m_hBuffer);
		m_hBuffer = NULL;
	}
}

inline bool ExCImage::LoadFromResource(UINT id, LPCTSTR pType, HMODULE hInst) throw() {
	HRSRC hResource = ::FindResource(hInst, MAKEINTRESOURCE(id), pType);
	if(!hResource)
		return false;
	
	DWORD imageSize = ::SizeofResource(hInst, hResource);
	if(!imageSize)
		return false;

	const void* pResourceData = ::LockResource(::LoadResource(hInst, hResource));
	if(!pResourceData)
		return false;

	HRESULT res = E_FAIL;
	m_hBuffer  = ::GlobalAlloc(GMEM_MOVEABLE, imageSize);
	if(m_hBuffer) {
		void* pBuffer = ::GlobalLock(m_hBuffer);
		if(pBuffer) {
			CopyMemory(pBuffer, pResourceData, imageSize);

			IStream* pStream = NULL;
			if(::CreateStreamOnHGlobal(m_hBuffer, FALSE, &pStream) == S_OK) {
				res = Load(pStream);
				pStream->Release();
				pStream = NULL;
			}
			::GlobalUnlock(m_hBuffer);
		}
		::GlobalFree(m_hBuffer);
		m_hBuffer = NULL;
	}
	return (res == S_OK);
}

class ResourceLoader
{
public:
	static void LoadImageList(LPCTSTR pszFileName, CImageList& aImgLst, int cx, int cy) {
		if(cx <= 0 || cy <= 0) return;
		ExCImage img;

		try {
			img.Load(pszFileName);
			aImgLst.Create(cx, cy, ILC_COLOR32 | ILC_MASK, (img.GetWidth()/cy), 0);
			aImgLst.Add(img, img.GetPixel(0, 0));
			img.Destroy();
		} catch(...) {
			dcdebug("ResourceLoader::LoadImageList(): %s\n", pszFileName);
		}
	}

	static void LoadImageList(UINT id, CImageList& aImgLst, int cx, int cy) {
		if(cx <= 0 || cy <= 0) return;
		ExCImage img;

		try {
			img.LoadFromResource(id, _T("PNG"));
			aImgLst.Create(cx, cy, ILC_COLOR32 | ILC_MASK, (img.GetWidth()/cy), 1);
			aImgLst.Add(img);
			img.Destroy();
		} catch(...) {
			dcdebug("ResourceLoader::LoadImageList(): %u\n", id);
		}
	}
};

#endif // RESOURCE_LOADER_H