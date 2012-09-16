/************************************
  REVISION LOG ENTRY
  Revision By: Mihai Filimon 
  Revised on 11/24/99 9:45:45 AM
  Comments: PictureWindow.h: interface for the CPictureWindow class.
 ************************************/

#if !defined(AFX_PICTUREWINDOW_H__44323373_9E89_11D3_A393_00C0DFC59237__INCLUDED_)
#define AFX_PICTUREWINDOW_H__44323373_9E89_11D3_A393_00C0DFC59237__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#include <atlbase.h>
//#include <atlwin.h>

class CPictureWindow  : public CWindowImpl<CPictureWindow>
{
public:
	typedef enum HandlerTypeEnum
	{
		ClientPaint = WM_PAINT,
		BackGroundPaint = WM_ERASEBKGND
	} HandlerTypeEnum;

	CPictureWindow()
	{
		m_nMessageHandler = ClientPaint;
	};
	virtual ~CPictureWindow()
	{
	};

	BEGIN_MSG_MAP(CPictureWindow)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkGnd)
	END_MSG_MAP()

	// Function name	: Load
	// Description	    : Loads the picture into memory, and then display it!
	// Return type		: virtual BOOL 
	// Argument         : LPCTSTR szFileName
	virtual BOOL Load( LPCTSTR szFileName )
	{
		BOOL bResult = FALSE;
		Close();
		if ( szFileName )
		{
			OFSTRUCT of;
			HANDLE hFile = NULL;
			if ( (hFile = (HANDLE)OpenFile( Text::fromT(szFileName).c_str(), &of, OF_READ | OF_SHARE_COMPAT)) != (HANDLE)HFILE_ERROR )
			{
				DWORD dwHighWord = NULL, dwSizeLow = GetFileSize( hFile, &dwHighWord );
				DWORD dwFileSize = dwSizeLow;
				HRESULT hResult = NULL;
				if ( HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, dwFileSize) )
					if ( void* pvData = GlobalLock( hGlobal ) )
					{
						DWORD dwReadBytes = NULL;
						BOOL bRead = ReadFile( hFile, pvData, dwFileSize, &dwReadBytes, NULL );
						GlobalUnlock( hGlobal );
						if ( bRead )
						{
							CComPtr<IStream> spStream;
							_ASSERTE( dwFileSize == dwReadBytes );
							if ( SUCCEEDED( CreateStreamOnHGlobal( hGlobal, TRUE, &spStream) ) )
								if ( SUCCEEDED( hResult = OleLoadPicture( spStream, 0, FALSE, IID_IPicture, (void**)&m_spPicture ) ) )
									bResult = TRUE;
						}
					}
				CloseHandle( hFile );
			}
		}
		Invalidate();
		return bResult;
	}

	HandlerTypeEnum m_nMessageHandler;

protected:

	inline virtual BOOL IsHandlerMessage( UINT uMsg )
	{
		return m_nMessageHandler == (HandlerTypeEnum)uMsg ;
	}

	void PutPicture( IPicture* pPicture, HDC hDC, RECT rPicture )
	{
		OLE_XSIZE_HIMETRIC nWidth = NULL; OLE_YSIZE_HIMETRIC nHeight = NULL;
		pPicture->get_Width( &nWidth ); pPicture->get_Height( &nHeight );
		pPicture->Render( hDC, rPicture.left ,rPicture.top, rPicture.right - rPicture.left, rPicture.bottom - rPicture.top, 0, nHeight, nWidth, -nHeight, NULL );
	};

	LRESULT OnEraseBkGnd(UINT uMsg, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if ( IsHandlerMessage( uMsg ) )
		{
			if ( m_spPicture )
			{
				BeginPaint( NULL );
					RECT r ; GetClientRect( &r );
					HDC hDC = GetDC();
					HWND hWndChild = GetWindow( GW_CHILD );
					while ( ::IsWindow( hWndChild ) )
					{
						if ( ::IsWindowVisible( hWndChild ) )
						{
							RECT rChild; ::GetWindowRect( hWndChild, &rChild );
							ScreenToClient( &rChild );
							ExcludeClipRect( hDC, rChild.left, rChild.top, rChild.right, rChild.bottom );
						}
						hWndChild = ::GetWindow( hWndChild, GW_HWNDNEXT );
					}
					PutPicture( m_spPicture, hDC, r );
					ReleaseDC( hDC );
				EndPaint( NULL );
				return TRUE;
			}
		}
		bHandled = FALSE;
		return FALSE;
	};

	LRESULT OnPaint(UINT uMsg, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if ( IsHandlerMessage( uMsg ) )
		{
			if ( m_spPicture )
			{
				BeginPaint( NULL );
				RECT r ; GetClientRect( &r );
				HDC hDC = GetDC();
				PutPicture( m_spPicture, hDC, r );
				ReleaseDC( hDC );
				EndPaint( NULL );
			}
		}
		bHandled = FALSE;
		return NULL;
	};

	void Close()
	{
		m_spPicture = NULL;
	}

	//Attributes
	CComPtr<IPicture> m_spPicture;
};

#endif // !defined(AFX_PICTUREWINDOW_H__44323373_9E89_11D3_A393_00C0DFC59237__INCLUDED_)
