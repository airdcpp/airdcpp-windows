#ifndef _MEMDC_H_
#define _MEMDC_H_

//////////////////////////////////////////////////
// CMemDC - memory DC
//
// Author: Keith Rule
// Email:  keithr@europa.com
// Copyright 1996-2002, Keith Rule
//
// You may freely use or modify this code provided this
// Copyright is included in all derived versions.
//
// History - 10/3/97 Fixed scrolling bug.
//                   Added print support. - KR
//
//           11/3/99 Fixed most common complaint. Added
//                   background color fill. - KR
//
//           11/3/99 Added support for mapping modes other than
//                   MM_TEXT as suggested by Lee Sang Hun. - KR
//
//           02/11/02 Added support for CScrollView as supplied
//                    by Gary Kirkham. - KR
//
// This class implements a memory Device Context which allows
// flicker free drawing.
//
// Modified by Trem to work with wtl
// Removed the print support

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "stdafx.h"
#include "../client/DCPlusPlus.h"

class CMemDC : public CDC {
private:	
	CBitmap		m_bitmap;		// Offscreen bitmap
	HBITMAP		m_oldBitmap;	// bitmap originally found in CMemDC
	CDC*		m_pDC;			// Saves CDC passed in constructor
	CRect		m_rect;			// Rectangle of drawing area.
	bool		managed;		// Determine if m_pDC should be deleted

	void Init(const CRect* pRect) {
		// Get the rectangle to draw
		if (pRect == NULL) {
			m_pDC->GetClipBox(&m_rect);
		} else {
			m_rect = *pRect;
		}

		// Create a Memory DC
		CreateCompatibleDC(*m_pDC);
		m_pDC->LPtoDP(&m_rect);

		m_bitmap.CreateCompatibleBitmap(*m_pDC, m_rect.Width(), m_rect.Height());
		m_oldBitmap = SelectBitmap(m_bitmap);

		SetMapMode(m_pDC->GetMapMode());

		SelectFont(m_pDC->GetCurrentFont());

		SIZE size = {0, 0};
		if( 0 != m_pDC->GetWindowExt(&size))
			SetWindowExt(size);

		if( 0 != m_pDC->GetViewportExt(&size))
			SetViewportExt(size);

		m_pDC->DPtoLP(&m_rect);
		SetWindowOrg(m_rect.left, m_rect.top);

		// Fill background 
		FillSolidRect(m_rect, m_pDC->GetBkColor());
	}
public:
	
	CMemDC(CDC* pDC, const CRect* pRect = NULL) : CDC()
	{
		dcassert(pDC != NULL); 

		// Some initialization
		m_pDC = pDC;
		m_oldBitmap = NULL;

		Init(pRect);
		
		managed = false;
	}

	CMemDC(HDC& dc, const CRect* pRect = NULL) : CDC()
	{
		dcassert(dc != NULL); 

		// Some initialization
		m_pDC = new CDC(dc);
		m_oldBitmap = NULL;

		Init(pRect);

		managed = true;
	}

	//Do not attempt to use an CMemDC object after calling Paint
	void Paint(){
		if(m_pDC == NULL)
			return;

		// Copy the offscreen bitmap onto the screen.
		m_pDC->BitBlt(m_rect.left, m_rect.top, m_rect.Width(), m_rect.Height(),
			*this, m_rect.left, m_rect.top, SRCCOPY);			

		//Swap back the original bitmap.
		SelectBitmap(m_oldBitmap);	

		if(managed){
			m_pDC->Detach();
			delete m_pDC;
		}

		m_pDC = NULL;
	}
	
	~CMemDC()	
	{		
		Paint();	
	}
};

#endif