/*
 * Copyright (C) 2011-2012 AirDC++ Project
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
#include "../client/DCPlusPlus.h"
#include "../client/UserInfoBase.h"

#include "ResourceLoader.h"

tstring ResourceLoader::m_IconPath;
ResourceLoader::ImageMap ResourceLoader::fileIndexes;
int ResourceLoader::fileImageCount;
int ResourceLoader::dirIconIndex = 0;
int ResourceLoader::dirMaskedIndex = 0;
CImageList ResourceLoader::searchImages;
CImageList ResourceLoader::settingsTreeImages;
CImageList ResourceLoader::fileImages;
CImageList ResourceLoader::userImages;
CImageList ResourceLoader::flagImages;
COLORREF ResourceLoader::GrayPalette[256];

void ResourceLoader::load() {

	m_IconPath = Text::toT(SETTING(ICON_PATH));

	loadFileImages();
	loadUserImages();
	loadSearchTypeIcons();
	loadSettingsTreeIcons();

	for(int i = 0; i < 256; i++){
		GrayPalette[i] = RGB(255-i, 255-i, 255-i);
	}

}
void ResourceLoader::unload() {
	searchImages.Destroy();
	settingsTreeImages.Destroy();
	fileImages.Destroy();
	userImages.Destroy();
	flagImages.Destroy();
}
void ResourceLoader::loadUserImages() {

	//first construct the overlay images.
	CImageList overLays;
	overLays.Create(16, 16, ILC_COLOR32 | ILC_MASK, 3, 3);
	overLays.AddIcon(loadIcon(IDI_USER_PASSIVE, 16));
	overLays.AddIcon(loadIcon(IDI_USER_OP, 16));
	overLays.AddIcon(MergeImages(overLays, 0, overLays, 1));

	//start constructing the userlist images from the bases
	userImages.Create(16, 16, ILC_COLOR32 | ILC_MASK,   10, 10);
	int pos = userImages.AddIcon(loadIcon(IDI_USER_BASE, 16));
	for(int i = 0; i < overLays.GetImageCount(); i++){
		userImages.AddIcon(MergeImages(userImages, pos, overLays, i));
	}

	pos = userImages.AddIcon(loadIcon(IDI_USER_AWAY, 16));
	for(int i = 0; i < overLays.GetImageCount(); i++){
		userImages.AddIcon(MergeImages(userImages, pos, overLays, i));
	}

	pos = userImages.AddIcon(loadIcon(IDI_USER_BOT, 16));
	for(int i = 0; i < overLays.GetImageCount(); i++){
		userImages.AddIcon(MergeImages(userImages, pos, overLays, i));
	}
	overLays.Destroy();
	
}
HICON ResourceLoader::MergeImages(HIMAGELIST hImglst1, int pos, HIMAGELIST hImglst2, int pos2) {
	/* Adding the merge images to a same Imagelist before merging makes the transparency work on xp */
	CImageList tmp;
	tmp.Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 0);
	tmp.AddIcon(ImageList_GetIcon(hImglst1, pos, ILD_TRANSPARENT));
	tmp.AddIcon(ImageList_GetIcon(hImglst2, pos2, ILD_TRANSPARENT));
	CImageList mergelist;
	mergelist.Merge(tmp, 0, tmp, 1, 0, 0);

	HICON merged = CopyIcon(mergelist.GetIcon(0, ILD_TRANSPARENT));
	tmp.Destroy();
	mergelist.Destroy();
	return merged;
}


HBITMAP ResourceLoader::getBitmapFromIcon(long defaultIcon, COLORREF crBgColor, int xSize /*= 0*/, int ySize /*= 0*/) {
	HICON hIcon = loadIcon(defaultIcon, xSize);
	if(!hIcon)
		return NULL;

	int cx = xSize;
	int cy = ySize;

	{
		ICONINFO	iconInfo;
		BITMAP		bm;

		GetIconInfo(hIcon, &iconInfo);
		if(iconInfo.hbmColor)
			GetObject(iconInfo.hbmColor, sizeof(bm), &bm);
		else if(iconInfo.hbmMask)
			GetObject(iconInfo.hbmMask, sizeof(bm), &bm);
		else
			return NULL;

		cx = bm.bmWidth;
		cy = bm.bmHeight;

	}

	HDC	crtdc = GetDC(NULL);
	HDC	memdc = CreateCompatibleDC(crtdc);
	HBITMAP hBitmap	= CreateCompatibleBitmap(crtdc, cx, cy);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(memdc, hBitmap);
	HBRUSH hBrush = CreateSolidBrush(crBgColor);
	RECT rect = { 0, 0, cx, cy };

	FillRect(memdc, &rect, hBrush);
	DrawIconEx(memdc, 0, 0, hIcon, cx, cy, 0, NULL, DI_NORMAL); // DI_NORMAL will automatically alpha-blend the icon over the memdc

	SelectObject(memdc, hOldBitmap);

	DeleteDC(crtdc);
	DeleteDC(memdc);
	DeleteObject(hIcon);
	DeleteObject(hBrush);
	DeleteObject(hOldBitmap);

	return hBitmap;
}

tstring ResourceLoader::getIconPath(const tstring& filename) {
	return m_IconPath.empty() ? Util::emptyStringT : (m_IconPath + _T("\\") + filename);
}

HICON ResourceLoader::loadIcon(int aDefault, int size/* = 0*/) {
	tstring icon = getIconPath(getIconName(aDefault));
	HICON iHandle = icon.empty() ? NULL : (HICON)::LoadImage(NULL, icon.c_str(), IMAGE_ICON, size, size, LR_DEFAULTSIZE | LR_DEFAULTCOLOR | LR_LOADFROMFILE);
	if(!iHandle) 
		return loadDefaultIcon(aDefault, size);
	return iHandle;
}

HICON ResourceLoader::loadDefaultIcon(int icon, int size/* = 0*/) {
	return (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(icon), IMAGE_ICON, size, size, LR_SHARED | LR_DEFAULTSIZE | LR_DEFAULTCOLOR);
}

tstring ResourceLoader::getIconName(int aDefault) {
	switch(aDefault) {
		case IDI_ADLSEARCH :	return _T("ToolbarImages\\ADLSearch.ico");
		case IDI_AWAY      :	return _T("ToolbarImages\\Away.ico");
		case IDI_FAVORITEHUBS:  return _T("ToolbarImages\\Favorites.ico");
		case IDI_FINISHED_DL:	return _T("ToolbarImages\\FinishedDL.ico");
		case IDI_FINISHED_UL:   return _T("ToolbarImages\\FinishedUL.ico");
		case IDI_PUBLICHUBS:	return _T("ToolbarImages\\PublicHubs.ico");
		case IDI_QUEUE:			return _T("ToolbarImages\\Queue.ico");
		case IDI_RECENTS:		return _T("ToolbarImages\\Recents.ico");
		case IDI_RECONNECT:		return _T("ToolbarImages\\Reconnect.ico");
		case IDI_REFRESH:		return _T("ToolbarImages\\Refresh.ico");
		case IDI_SEARCH:		return _T("ToolbarImages\\Search.ico");
		case IDI_SEARCHSPY:		return _T("ToolbarImages\\SearchSpy.ico");
		case IDI_SETTINGS:		return _T("ToolbarImages\\Settings.ico");
		case IDI_SHUTDOWN:		return _T("ToolbarImages\\Exit.ico");
		case IDI_NETSTATS:		return _T("ToolbarImages\\NetStats.ico");
		case IDI_FOLLOW:		return _T("ToolbarImages\\Follow.ico");
		case IDI_FAVORITE_USERS:return _T("ToolbarImages\\FavoriteUser.ico");
		case IDI_OPEN_LIST:		return _T("ToolbarImages\\OpenFileList.ico");
		case IDI_NOTEPAD:		return _T("ToolbarImages\\Notepad.ico");
		case IDI_OPEN_DOWNLOADS:return _T("ToolbarImages\\OpenDLDir.ico");
		case IDI_LOGS :			return _T("ToolbarImages\\SystemLog.ico");
		case IDI_UPLOAD_QUEUE:	return _T("ToolbarImages\\uQueue.ico");
		case IDI_SCAN :			return _T("ToolbarImages\\ScanMissing.ico");
		case IDI_AUTOSEARCH:	return _T("ToolbarImages\\AutoSearch.ico");
		case IDI_TEXT:			return _T("text.ico");
		case IDI_IERROR:		return _T("error.ico");
		case IDI_IWARNING:		return _T("warning.ico");
		case IDI_INFO:			return _T("info.ico");
		case IDI_CDM:			return _T("Cdm.ico");
		case IDI_OWNLIST:		return _T("ownlist.ico");
		case IDI_MATCHLIST:		return _T("matchlists.ico");
		case IDI_QCONNECT:		return _T("QConnect.ico");
		case IDI_GET_TTH:		return _T("getTTH.ico");
		case IDI_INDEXING:		return _T("Indexing.ico");
		case IDI_WINDOW_OPTIONS:return _T("WindowOptions.ico");
		case IDI_WIZARD:		return _T("Wizard.ico");
		case IDI_UPLOAD:		return _T("Upload.ico");
		case IDI_DOWNLOAD:		return _T("Download.ico");
		case IDI_SEGMENT:		return _T("transfer_segmented.ico");
		case IDI_D_USER:		return _T("download_user.ico");
		case IDI_U_USER:		return _T("upload_user.ico");
		case IDI_ONLINE:		return _T("Online.ico");
		case IDI_OFFLINE:		return _T("Offline.ico");
		case IDI_HUB:			return _T("Hub.ico");
		case IDI_HUBREG	:		return _T("HubReg.ico");
		case IDI_HUBOP:			return _T("HubOp.ico");
		case IDI_ANY:			return _T("searchTypes\\Any.ico");
		case IDI_AUDIO:			return _T("searchTypes\\Audio.ico");
		case IDI_COMPRESSED:	return _T("searchTypes\\Compressed.ico");
		case IDI_DOCUMENT:		return _T("searchTypes\\Document.ico");
		case IDI_EXEC:			return _T("searchTypes\\Exec.ico");
		case IDI_PICTURE:		return _T("searchTypes\\Pic.ico");
		case IDI_VIDEO:			return _T("searchTypes\\Video.ico");
		case IDI_DIRECTORY:		return _T("searchTypes\\Directory.ico");
		case IDI_TTH:			return _T("searchTypes\\tth.ico");
		case IDI_CUSTOM:		return _T("searchTypes\\Custom.ico");
		case IDI_MPSTART:		return _T("MediaToolbar\\start.ico");
		case IDI_MPSPAM:		return _T("MediaToolbar\\spam.ico");
		case IDI_MPBACK:		return _T("MediaToolbar\\back.ico");
		case IDI_MPPLAY:		return _T("MediaToolbar\\play.ico");
		case IDI_MPPAUSE:		return _T("MediaToolbar\\pause.ico");
		case IDI_MPNEXT:		return _T("MediaToolbar\\next.ico");
		case IDI_MPSTOP:		return _T("MediaToolbar\\stop.ico");
		case IDI_MPVOLUMEUP:	return _T("MediaToolbar\\up.ico");
		case IDI_MPVOLUME50:	return _T("MediaToolbar\\volume50.ico");
		case IDI_MPVOLUMEDOWN:	return _T("MediaToolbar\\down.ico");
		case IDR_MAINFRAME:		return _T("AirDCPlusPlus.ico");
		case IDR_PRIVATE:       return _T("User.ico");
		case IDR_PRIVATE_OFF:   return _T("UserOff.ico");
		case IDI_BOT:			return _T("Bot.ico");
		case IDI_BOT_OFF:		return _T("BotOff.ico");
		case IDI_FOLDER:		return _T("folder.ico");
		case IDI_FOLDER_INC:	return _T("folder_incomplete.ico");
		case IDI_FILE:			return _T("file.ico");
		case IDI_OVERLAY:		return _T("overlay.ico");
		case IDI_USER_BASE:		return _T("UserlistImages\\UserBase.ico");
		case IDI_USER_AWAY:		return _T("UserlistImages\\UserAway.ico");
		case IDI_USER_BOT:		return _T("UserlistImages\\UserBot.ico");
		case IDI_USER_PASSIVE:	return _T("UserlistImages\\UserNoCon.ico");
		case IDI_USER_OP:		return _T("UserlistImages\\UserOP.ico");
		case IDI_SLOTS:			return _T("Slots.ico");
		case IDI_SLOTSFULL:		return _T("SlotsFull.ico");
		case IDR_TRAY_PM:		return _T("PM.ico");
		case IDR_TRAY_HUB:		return _T("HubMessage.ico");
		case IDI_TOTAL_UP:		return _T("TotalUp.ico");
		case IDI_TOTAL_DOWN:	return _T("TotalDown.ico");
		case IDI_SHARED:		return _T("Shared.ico");

		default: return Util::emptyStringT;
	}
}
void ResourceLoader::loadFileImages() {

	fileImages.Create(16, 16, ILC_COLOR32 | ILC_MASK,  0, 3);
	fileImages.AddIcon(loadIcon(IDI_FOLDER, 16));
	fileImages.AddIcon(loadIcon(IDI_FOLDER_INC, 16));
	fileImages.AddIcon(loadIcon(IDI_FILE, 16));
	
	dirIconIndex = fileImageCount++;
	dirMaskedIndex = fileImageCount++;
	fileImageCount++;

	if(SETTING(USE_SYSTEM_ICONS)) {
		SHFILEINFO fi;
		memzero(&fi, sizeof(SHFILEINFO));
		if(::SHGetFileInfo(_T("./"), FILE_ATTRIBUTE_DIRECTORY, &fi, sizeof(fi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES)) {
			fileImages.ReplaceIcon(dirIconIndex, fi.hIcon);

			{
				HICON overlayIcon = loadIcon(IDI_OVERLAY);

				CImageList tmpIcons;
				tmpIcons.Create(16, 16, ILC_COLOR32 | ILC_MASK, 2, 1);
				tmpIcons.AddIcon(fi.hIcon);
				tmpIcons.AddIcon(overlayIcon);

				CImageList mergeList(ImageList_Merge(tmpIcons, 0, tmpIcons, 1, 0, 0));
				HICON icDirIcon = mergeList.GetIcon(0);
				fileImages.ReplaceIcon(dirMaskedIndex, icDirIcon);

				mergeList.Destroy();
				tmpIcons.Destroy();

				::DestroyIcon(icDirIcon);
				::DestroyIcon(overlayIcon);
			}

			
			::DestroyIcon(fi.hIcon);
		}
	}
}

void ResourceLoader::loadSettingsTreeIcons() {
	int size = 16;
	settingsTreeImages.Create(size, size, ILC_COLOR32 | ILC_MASK,  0, 30);
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_GENERAL, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_CONNECTIONS, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_SPEED, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_LIMITS, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_DOWNLOADS, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_LOCATIONS, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_PREVIEW, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_PRIORITIES, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_QUEUE, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_SHARING, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_SHAREOPTIONS, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_APPEARANCE, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_FONTS, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_PROGRESS, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_USERLIST, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_SOUNDS, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_TOOLBAR, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_WINDOWS, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_TABS, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_POPUPS, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_HIGHLIGHTS, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_AIRAPPEARANCE, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_ADVANCED, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_EXPERTS, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_LOGS, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_UC, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_CERTIFICATES, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_MISC, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_IGNORE, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_SEARCH, size));
	settingsTreeImages.AddIcon(loadDefaultIcon(IDI_SEARCHTYPES, size));
}

void ResourceLoader::loadSearchTypeIcons() {
	searchImages.Create(16, 16, ILC_COLOR32 | ILC_MASK,  0, 15);
	searchImages.AddIcon(loadIcon(IDI_ANY, 16));
	searchImages.AddIcon(loadIcon(IDI_AUDIO, 16));
	searchImages.AddIcon(loadIcon(IDI_COMPRESSED, 16));
	searchImages.AddIcon(loadIcon(IDI_DOCUMENT, 16));
	searchImages.AddIcon(loadIcon(IDI_EXEC, 16));
	searchImages.AddIcon(loadIcon(IDI_PICTURE, 16));
	searchImages.AddIcon(loadIcon(IDI_VIDEO, 16));
	searchImages.AddIcon(loadIcon(IDI_DIRECTORY,16));
	searchImages.AddIcon(loadIcon(IDI_TTH, 16));
	searchImages.AddIcon(loadIcon(IDI_CUSTOM, 16));
}

int ResourceLoader::getIconIndex(const tstring& aFileName) {
	if(SETTING(USE_SYSTEM_ICONS)) {
		SHFILEINFO fi;
		tstring x = Text::toLower(Util::getFileExt(aFileName));
		if(!x.empty()) {
			ImageIter j = fileIndexes.find(x);
			if(j != fileIndexes.end())
				return j->second;
		}
		tstring fn = Text::toLower(Util::getFileName(aFileName));
		::SHGetFileInfo(fn.c_str(), FILE_ATTRIBUTE_NORMAL, &fi, sizeof(fi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		fileImages.AddIcon(fi.hIcon);
		::DestroyIcon(fi.hIcon);

		fileIndexes[x] = fileImageCount++;
		return fileImageCount - 1;
	} else {
		return 2;
	}
}

void ResourceLoader::loadCmdBarImageList(CImageList& images){
	images.Create(16, 16, ILC_COLOR32 | ILC_MASK,  0, 33);
	//use default ones totally here too?
	images.AddIcon(ResourceLoader::loadIcon(IDI_PUBLICHUBS, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_RECONNECT, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_FOLLOW, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_RECENTS, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_FAVORITEHUBS, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_FAVORITE_USERS, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_QUEUE, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_FINISHED_DL, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_UPLOAD_QUEUE, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_FINISHED_UL, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_SEARCH, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_ADLSEARCH, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_SEARCHSPY, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_OPEN_LIST, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_OWNLIST, 16)); 
	images.AddIcon(ResourceLoader::loadIcon(IDI_ADLSEARCH, 16)); 
	images.AddIcon(ResourceLoader::loadIcon(IDI_MATCHLIST, 16)); 
	images.AddIcon(ResourceLoader::loadIcon(IDI_REFRESH, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_SCAN, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_OPEN_DOWNLOADS, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_QCONNECT, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_SETTINGS, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_GET_TTH, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDR_UPDATE, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_SHUTDOWN, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_NOTEPAD, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_NETSTATS, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_CDM, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_LOGS, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_AUTOSEARCH, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_INDEXING, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDR_MAINFRAME, 16));
	images.AddIcon(ResourceLoader::loadIcon(IDI_WIZARD, 16));
}

void ResourceLoader::loadWinampToolbarIcons(CImageList& winampImages) {
	int size = SETTING(WTB_IMAGE_SIZE);
	winampImages.Create(size, size, ILC_COLOR32 | ILC_MASK,  0, 11);
	winampImages.AddIcon(loadIcon(IDI_MPSTART, size));
	winampImages.AddIcon(loadIcon(IDI_MPSPAM, size));
	winampImages.AddIcon(loadIcon(IDI_MPBACK, size));
	winampImages.AddIcon(loadIcon(IDI_MPPLAY, size));
	winampImages.AddIcon(loadIcon(IDI_MPPAUSE, size));
	winampImages.AddIcon(loadIcon(IDI_MPNEXT, size));
	winampImages.AddIcon(loadIcon(IDI_MPSTOP, size));
	winampImages.AddIcon(loadIcon(IDI_MPVOLUMEUP, size));
	winampImages.AddIcon(loadIcon(IDI_MPVOLUME50, size));
	winampImages.AddIcon(loadIcon(IDI_MPVOLUMEDOWN, size));
}

void ResourceLoader::loadFlagImages() { 
	flagImages.CreateFromImage(IDB_FLAGS, 25, 8, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED); 
}

//try to keep the icon smoothness but copy it as grayscale.
HICON ResourceLoader::convertGrayscaleIcon( HICON hIcon ) {
	if(!hIcon)
		return NULL;

	HDC hdc = ::GetDC(NULL);
	HICON hGrayIcon = NULL;
	ICONINFO iconInfo = { 0 };
	ICONINFO iconGray = { 0 };
	LPDWORD lpBits = NULL;
	LPBYTE lpBitsPtr = NULL;
	SIZE sz;
	DWORD c1 = 0;
	BITMAPINFO bmpInfo = { 0 };
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	COLORREF* grayPalette = GrayPalette;

	if(::GetIconInfo(hIcon, &iconInfo)) {
		if(::GetDIBits(hdc, iconInfo.hbmColor, 0, 0, NULL, &bmpInfo, DIB_RGB_COLORS) != 0) {
			bmpInfo.bmiHeader.biCompression = BI_RGB;
			sz.cx = bmpInfo.bmiHeader.biWidth;
			sz.cy = bmpInfo.bmiHeader.biHeight;
			c1 = sz.cx * sz.cy;
			lpBits = (LPDWORD)::GlobalAlloc(GMEM_FIXED, (c1) * 4);

			if (lpBits && ::GetDIBits(hdc, iconInfo.hbmColor, 0, sz.cy, lpBits, &bmpInfo, DIB_RGB_COLORS) != 0) {
				lpBitsPtr     = (LPBYTE)lpBits;
				UINT off      = 0;

				for (UINT i = 0; i < c1; i++) {
					off = (UINT)(255-(( lpBitsPtr[0] + lpBitsPtr[1] + lpBitsPtr[2] ) / 3) );
					if (lpBitsPtr[3] != 0 || off != 255) {
						if(off == 0)
							off = 1;
					
						lpBits[i] = grayPalette[off] | ( lpBitsPtr[3] << 24 );
					}

					lpBitsPtr += 4;
				}

				iconGray.hbmColor = ::CreateCompatibleBitmap(hdc, sz.cx, sz.cy);

				if (iconGray.hbmColor != NULL) {
					::SetDIBits(hdc, iconGray.hbmColor, 0, sz.cy, lpBits, &bmpInfo, DIB_RGB_COLORS);
					iconGray.hbmMask = iconInfo.hbmMask;
					iconGray.fIcon   = TRUE;
					hGrayIcon = ::CreateIconIndirect(&iconGray);
					::DeleteObject(iconGray.hbmColor);
				}

				::GlobalFree(lpBits);
				lpBits = NULL;
			}
		}
		::DeleteObject(iconInfo.hbmColor);
		::DeleteObject(iconInfo.hbmMask);
	}
	::ReleaseDC(NULL, hdc);

	return hGrayIcon;
}
