/*
 * Copyright (C) 2010 Crise, crise<at>mail.berlios.de
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

#ifndef EX_MESSAGE_BOX
#define EX_MESSAGE_BOX

class ExMessageBox
{
	private:
		struct MessageBoxValues
		{
			HHOOK hHook;
			HWND  hWnd;
			WNDPROC lpMsgBoxProc;
			UINT typeFlags;
			LPCTSTR lpName;
			void* userdata;
		} static __declspec(thread) mbv;
		
		static LRESULT CALLBACK CbtHookProc(int nCode, WPARAM wParam, LPARAM lParam)
		{
			if (nCode < 0)
				return CallNextHookEx(mbv.hHook, nCode, wParam, lParam);
				
			switch (nCode)
			{
				case HCBT_CREATEWND:
				{
					LPCBT_CREATEWND lpCbtCreate = (LPCBT_CREATEWND)lParam;
					// MSDN says that lpszClass can't be trusted but this seems to be an exception
					if (lpCbtCreate->lpcs->lpszClass == WC_DIALOG && wcscmp(lpCbtCreate->lpcs->lpszName, mbv.lpName) == 0)
					{
						mbv.hWnd = (HWND)wParam;
						mbv.lpMsgBoxProc = (WNDPROC)SetWindowLongPtr(mbv.hWnd, GWLP_WNDPROC, (LONG_PTR)mbv.lpMsgBoxProc);
					}
				}
				break;
				case HCBT_DESTROYWND:
				{
					if (mbv.hWnd == (HWND)wParam)
						mbv.lpMsgBoxProc = (WNDPROC)SetWindowLongPtr(mbv.hWnd, GWLP_WNDPROC, (LONG_PTR)mbv.lpMsgBoxProc);
				}
				break;
			}
			
			return CallNextHookEx(mbv.hHook, nCode, wParam, lParam);
		}
		
	public:
	
		// Get the default WindowProc for the message box (for CallWindowProc)
		static WNDPROC GetMessageBoxProc()
		{
			return mbv.lpMsgBoxProc;
		}
		
		// Userdata manipulation
		static void SetUserData(void* data)
		{
			mbv.userdata = data;
		}
		static void* GetUserData()
		{
			return mbv.userdata;
		}
		
		// Get the MessageBox flags
		static int GetTypeFlags()
		{
			return mbv.typeFlags;
		}
		
		// Show the message box, see overloads below
		static int Show(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType, WNDPROC wndProc)
		{
			mbv.hHook = NULL;
			mbv.hWnd = NULL;
			mbv.lpMsgBoxProc = wndProc;
			mbv.typeFlags = uType;
			mbv.lpName = lpCaption;
			
			// Let's set up a CBT hook for this thread, and then call the standard MessageBox
			mbv.hHook = SetWindowsHookEx(WH_CBT, CbtHookProc, GetModuleHandle(NULL), GetCurrentThreadId());
			
			int nRet = MessageBox(hWnd, lpText, lpCaption, uType);
			
			// And we're done
			UnhookWindowsHookEx(mbv.hHook);
			
			return nRet;
		}
};

// Overload the standard MessageBox for convenience
int WINAPI MessageBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType, WNDPROC wndProc);
int WINAPI MessageBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, LPCTSTR lpQuestion, UINT uType, UINT& bCheck);

#endif // EX_MESSAGE_BOX