/*
* Copyright (C) 2012-2024 AirDC++ Project
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

#include "stdafx.h"

#include "BrowseDlg.h"

#include <airdcpp/Text.h>
#include <airdcpp/Util.h>


const GUID BrowseDlg::browseGuids[TYPE_LAST] = {
	{ 0 },
	{ 0 },
	{ 0x6316692a, 0x64cb, 0x4571, { 0x83, 0xdc, 0xac, 0x33, 0xd3, 0x92, 0x30, 0xbe } }, // {6316692A-64CB-4571-83DC-AC33D39230BE}
	{ 0xb874be80, 0x7f5c, 0x4af8, { 0xa3, 0x59, 0x6a, 0x51, 0xe5, 0xde, 0xd5, 0x15 } }, // {B874BE80-7F5C-4AF8-A359-6A51E5DED515}
	{ 0x2fe0147c, 0x568, 0x4071, { 0xaa, 0xa8, 0x41, 0xe1, 0x6a, 0xe9, 0x1d, 0xaf } } // {2FE0147C-0568-4071-AAA8-41E16AE91DAF}
};

#define checkinit() if(!initialized) return false;
#define check(x) if(!SUCCEEDED(x)) return false;

BrowseDlg::BrowseDlg(HWND hwnd, RecentType aRecentType, DialogType aDlgType) : m_hwnd(hwnd), type(aDlgType) {
	// CoCreate the File Open Dialog object.
	HRESULT hr = CoCreateInstance(type == DIALOG_SAVE_FILE ? CLSID_FileSaveDialog : CLSID_FileOpenDialog,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pfd));

	if (SUCCEEDED(hr)) {
		initialized = true;

		DWORD dwFlags;

		// Before setting, always get the options first in order 
		// not to override existing options.
		if (SUCCEEDED(pfd->GetOptions(&dwFlags))) {
			if (type == DIALOG_SELECT_FOLDER)
				dwFlags |= FOS_PICKFOLDERS;

			if (aRecentType == TYPE_NOSAVE) {
				dwFlags |= FOS_DONTADDTORECENT;
			} else if (aRecentType != TYPE_GENERIC) {
				pfd->SetClientGuid(browseGuids[aRecentType]);
			}

			// In this case, get shell items only for file system items.
			pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM);
		}

		if (aDlgType == DIALOG_SELECT_FILE) {
			setOkLabel(TSTRING(SELECT));
		}
	}

}

bool BrowseDlg::setTitle(const tstring& aTitle) {
	checkinit();
	check(pfd->SetTitle(aTitle.c_str()));
	return true;
}

bool BrowseDlg::setPath(const tstring& aPath, bool forced) {
	if (aPath.empty())
		return false;

	checkinit();

	auto target = Text::toT(Util::validatePath(Text::fromT(aPath)));

	// Prefill the filename
	auto fileName = Util::getFileName(target);
	if (!fileName.empty()) {
		pfd->SetFileName(fileName.c_str());
	}

	// Set the given directory
	CComPtr<IShellItem> psiFolder;
	if (SUCCEEDED(SHCreateItemFromParsingName(Util::getFilePath(target).c_str(), NULL, IID_PPV_ARGS(&psiFolder)))) {
		if (forced) {
			check(pfd->SetFolder(psiFolder));
		} else {
			check(pfd->SetDefaultFolder(psiFolder));
		}
	}

	return true;
}

bool BrowseDlg::addFlags(int aFlags) {
	DWORD dwFlags;
	check(pfd->GetOptions(&dwFlags));
	dwFlags |= aFlags;
	check(pfd->SetOptions(dwFlags));
	return true;
}

bool BrowseDlg::setTypes(int typeCount, const COMDLG_FILTERSPEC* types) {
	checkinit();
	check(pfd->SetFileTypes(typeCount, types));
	return true;
}

bool BrowseDlg::setFileName(const tstring& aName) {
	checkinit();
	check(pfd->SetFileName(aName.c_str()));
	return true;
}

bool BrowseDlg::setOkLabel(const tstring& aText) {
	checkinit();
	check(pfd->SetOkButtonLabel(aText.c_str()));
	return true;
}

bool BrowseDlg::show(tstring& target) {
	checkinit();

	// Show the dialog
	auto hr = pfd->Show(m_hwnd);
	if (SUCCEEDED(hr)) {
		// Obtain the result once the user clicks 
		// the 'Open' button.
		// The result is an IShellItem object.
		IShellItem *psiResult;
		hr = pfd->GetResult(&psiResult);
		if (SUCCEEDED(hr)) {
			// Copy the path
			PWSTR pszFilePath = NULL;
			hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
			if (SUCCEEDED(hr)) {
				target = Text::toT(Util::validatePath(Text::fromT(pszFilePath), type == DIALOG_SELECT_FOLDER));
			}
			psiResult->Release();
		}
	}

	return SUCCEEDED(hr);
}

bool BrowseDlg::setTypeIndex(int aIndex) {
	checkinit();
	check(pfd->SetFileTypeIndex(aIndex));
	return true;
}

BrowseDlg::~BrowseDlg() {
	if (initialized) {
		pfd->Release();
	}
}