/*
  Name:

    DimEdit

  Author:

    Paul A. Howes

  Description:

    This object is a subclass of a normal edit control, which displays a
    "dimmed" string when the control is empty and does not have the
    keyboard focus.  When the control gains the focus, the user can type
    normally.  When the control loses the focus again, it determines
    whether or not the text of the control is empty or not, and if it is
    empty, replaces the "dimmed" string.

    The "dimmed" string and the color in which it is displayed can be
    specified by the user.

    One of the nice features of this Dim Edit control is that the window
    text is not modified by the control in any way.  This way, the
    GetWindowText and SetWindowText functions (and message handlers for
    WM_SETTEXT and WM_GETTEXT) do not need to be modified get the text
    "right" for the client.  All of the work is done in the WM_PAINT
    message handler.  The focus handlers are there only to keep the edit
    field in the right state.
*/

#ifndef DIMEDIT
#define DIMEDIT
#pragma once

#include <string>
#include <AtlCrack.h>
#include "../client/typedefs.h"

class CDimEdit : public CWindowImpl< CDimEdit, CEdit > {
public:

	// Constructors and destructors.

  //  Default constructor
  CDimEdit( )
  : CWindowImpl< CDimEdit, CEdit >( ),
    m_getDlgCodeHandled( false ),
    m_isDim( true ),
		m_dimText(_T("<Click here to enter value>")),
    m_dimColor( RGB( 128, 128, 128 ) ) {
  }

  //  Constructor that initializes the dim text and the color.
  CDimEdit( const tstring& dimText,
            const COLORREF dimColor = RGB( 128, 128, 128 ) )
  : CWindowImpl< CDimEdit, CEdit >( ),
    m_getDlgCodeHandled( false ),
    m_isDim( true ),
    m_dimText( dimText ),
    m_dimColor( dimColor ) {
  }

  //  Another constructor that initializes the text and color.
  CDimEdit( const tstring&      dimText,
            const unsigned char red,
            const unsigned char green,
            const unsigned char blue )
  : CWindowImpl< CDimEdit, CEdit >( ),
    m_getDlgCodeHandled( false ),
    m_isDim( true ),
    m_dimText( dimText ),
    m_dimColor( RGB( red, green, blue ) ) {
  }

  //  Destructor
  ~CDimEdit( ) {
  }

	// Actions.

  //  Subclass the control.
	BOOL SubclassWindow( HWND hWnd ) {
    return( CWindowImpl< CDimEdit, CEdit >::SubclassWindow( hWnd ) );
  }

  //  Set the text to display when the control is empty (the "dim" text)
	CDimEdit& SetDimText( const tstring& dimText ) {
    m_dimText = dimText;
    return( *this );
  }

  //  Set the color to display the dim text in.
	CDimEdit& SetDimColor( const COLORREF dimColor ) {
    m_dimColor = dimColor;
    return( *this );
  }

  //  Another way to set the "dim" color.
  CDimEdit& SetDimColor( const unsigned char red,
                         const unsigned char green,
		const unsigned char blue ) {
    m_dimColor = RGB( red, green, blue );
    return( *this );
  }

	// Message map and message handlers.

	BEGIN_MSG_MAP_EX( CDimEdit )
    MSG_WM_PAINT( OnPaint )
    MSG_WM_SETFOCUS( OnSetFocus )
    MSG_WM_KILLFOCUS( OnKillFocus )
  END_MSG_MAP( )

  //  WM_PAINT message handler.
  //  NOTE:  The device context passed in does not exist.  This is a WTL bug.
	void OnPaint( HDC ) {
    //  Start the painting operation.
    PAINTSTRUCT paint;

    //  CDCHandle is used, because this object is not responsible for
    //  releasing the device context.
    CDCHandle dc( BeginPaint( &paint ) );

    //  Same idea here -- It's a system brush, and doesn't need to be deleted.
    CBrushHandle brush;
    brush.CreateSysColorBrush( COLOR_WINDOW );

    //  Paint the background of the edit control's client area.
    dc.FillRect( &paint.rcPaint, brush );

    //  Use the system default font.
    dc.SelectFont( HFONT( GetStockObject( DEFAULT_GUI_FONT ) ) );

    //  If the control is in "dim" mode, display the text centered and dimmed.
		if( m_isDim == true ) {
      dc.SetTextColor( m_dimColor );
      dc.SetTextAlign( TA_CENTER );
      dc.TextOut( ( paint.rcPaint.right - paint.rcPaint.left ) / 2, 1, m_dimText.c_str( ) );
		} else { 	//  If the control is "normal", display the text on the left.
      dc.SetTextColor( GetSysColor( COLOR_BTNTEXT ) );
      dc.SetTextAlign( TA_LEFT );
      int length = GetWindowTextLength( ) + 1;
      TCHAR* text = new TCHAR[length];
      GetWindowText( text, length );
      dc.TextOut( 1, 1, text );
      delete[] text;
    }

    //  End the paint operation.  This releases the DC internally.
    EndPaint( &paint );
  }

  //  The edit control received the keyboard focus.
	void OnSetFocus( HWND /*hWnd*/ ) {
    //  Allow Windows to process everything normally.
    DefWindowProc( );

    //  If the text in the edit control is the dim text, then erase it.
		if( GetWindowTextLength( ) == 0 ) {
      m_isDim = false;
      Invalidate( );
    }
  }

  //  The edit control lost the keyboard focus.
	void OnKillFocus( HWND /*hWnd*/ ) {
    //  Allow Windows to process everything normally.
    DefWindowProc( );

    //  If the text length is zero, then the field is empty.  Put the dim text back.
		if( GetWindowTextLength( ) == 0 ) {
      m_isDim = true;
      Invalidate( );
    }
  }

private:
  //  A flag telling the code if WM_GETDLGCODE has already been handled.
  bool m_getDlgCodeHandled;

  //  A flag telling the paint code if the control is dim or active.
  bool m_isDim;

  //  The text to display when the edit control is empty.
  tstring m_dimText;

  //  The color to display it in.
  DWORD m_dimColor;
};

#endif // DIMEDIT