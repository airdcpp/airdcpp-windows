// InputBox.h: interface for the CInputBox class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_INPUTBOX_H__0BE6B01B_C74A_45FE_AF35_D6E8E4B65A1B__INCLUDED_)
#define AFX_INPUTBOX_H__0BE6B01B_C74A_45FE_AF35_D6E8E4B65A1B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#define INPUTBOX_WIDTH 400
#define INPUTBOX_HEIGHT 220



/*
Author      : mah
Date        : 13.06.2002
Description : 
    similar to Visual Basic InputBox
*/
class CInputBox  
{
    static HFONT m_hFont;
    static HWND  m_hWndInputBox;
    static HWND  m_hWndParent;
    static HWND  m_hWndEdit;
    static HWND  m_hWndEdit1;
    static HWND  m_hWndOK;
    static HWND  m_hWndPrompt;
    static HWND  m_hWndPrompt1;
    static HWND  m_hWndPrompt2;

    static HINSTANCE m_hInst;

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
public:
    // text from InputBox
	LPTSTR Text;
    BOOL DoModal(LPCTSTR szCaption, LPCTSTR szPrompt, LPCTSTR szText, LPCTSTR szText1);

	CInputBox(HWND hWndParent);
	~CInputBox();

};

#endif // !defined(AFX_INPUTBOX_H__0BE6B01B_C74A_45FE_AF35_D6E8E4B65A1B__INCLUDED_)
