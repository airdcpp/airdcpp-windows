/* 
* Copyright (C) 2003 Twink,  spm7@waikato.ac.nz
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

#include "stdafx.h"
#include "Resource.h"

#include "PreviewDlg.h"
#include "WinUtil.h"

WinUtil::TextItem PreviewDlg::texts[] = {
	{ IDOK, ResourceManager::OK },
	{ IDCANCEL, ResourceManager::CANCEL },
	{ IDC_PREVIEW_BROWSE, ResourceManager::BROWSE },
	{ IDC_PREVIEW_NAME_TEXT, ResourceManager::PREVIEW_NAME },
	{ IDC_PREVIEW_APPLICATION_TEXT, ResourceManager::PREVIEW_APPLICATION },
	{ IDC_PREVIEW_ARGUMENTS_TEXT, ResourceManager::PREVIEW_ARGUMENTS },
	{ IDC_PREVIEW_EXTENSION_TEXT, ResourceManager::PREVIEW_EXTENSION },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT PreviewDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	#define ATTACH(id, var) var.Attach(GetDlgItem(id))
	ATTACH(IDC_PREVIEW_NAME, ctrlName);
	ATTACH(IDC_PREVIEW_APPLICATION, ctrlApplication);
	ATTACH(IDC_PREVIEW_ARGUMENTS, ctrlArguments);
	ATTACH(IDC_PREVIEW_EXTENSION, ctrlExtensions);
	
	WinUtil::translate(*this, texts);

	ctrlName.SetWindowText(name.c_str());
	ctrlApplication.SetWindowText(application.c_str());
	ctrlArguments.SetWindowText(argument.c_str());
	ctrlExtensions.SetWindowText(extensions.c_str());
	SetWindowText(CTSTRING(PREVIEW_DLG));

	return 0;
}

LRESULT PreviewDlg::OnBrowse(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/) {
	TCHAR buf[MAX_PATH];

	GetDlgItemText(IDC_PREVIEW_APPLICATION, buf, MAX_PATH);
	tstring x = buf;

	if (WinUtil::browseFile(x, m_hWnd, false, Util::emptyStringT, Util::emptyStringT, _T("Programs\0*.exe")) == IDOK) {
		SetDlgItemText(IDC_PREVIEW_APPLICATION, x.c_str());
	}

	return 0;
}