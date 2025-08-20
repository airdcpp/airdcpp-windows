/* 
 * Copyright (C) 2006-2011 Crise, crise<at>mail.berlios.de
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#ifndef EXCIMAGE_H
#define EXCIMAGE_H

#pragma once

#ifdef __ATLMISC_H__
#define __ATLTYPES_H__
#endif

#include <airdcpp/core/classes/FastAlloc.h>

#define byte BYTE // 'byte': ambiguous symbol (C++17)
#include <atlimage.h>
#undef byte


namespace wingui {
class ExCImage : public CImage, public FastAlloc<ExCImage>, boost::noncopyable
{
public:
	typedef std::shared_ptr<ExCImage> Ptr;

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
}

#endif //EXCIMAGE