/*
 * Copyright (C) 2011-2021 AirDC++ Project
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
#include <airdcpp/UserInfoBase.h>
#include <airdcpp/LogManager.h>

#include "ResourceLoader.h"

tstring ResourceLoader::m_IconPath;
ResourceLoader::ImageMap ResourceLoader::fileIndexes;
int ResourceLoader::fileImageCount;
CImageList ResourceLoader::autoSearchStatuses;
CImageList ResourceLoader::searchImages;
CImageList ResourceLoader::settingsTreeImages;
CImageList ResourceLoader::fileImages;
CImageList ResourceLoader::userImages;
CImageList ResourceLoader::flagImages;
CImageList ResourceLoader::arrowImages;
CImageList ResourceLoader::filelistTbImages;
CImageList ResourceLoader::hubImages;
CImageList ResourceLoader::queueTreeImages;
CImageList ResourceLoader::thumbBarImages;
COLORREF ResourceLoader::GrayPalette[256];
CIcon ResourceLoader::infoIcon = NULL;
CIcon ResourceLoader::warningIcon = NULL;
CIcon ResourceLoader::errorIcon = NULL;
CIcon ResourceLoader::iSecureGray = NULL;
CIcon ResourceLoader::iCCPMUnSupported = NULL;

std::multimap<int, ResourceLoader::cachedIcon> ResourceLoader::cachedIcons;

void ResourceLoader::load() {

	m_IconPath = Text::toT(SETTING(ICON_PATH));

	for(int i = 0; i < 256; i++){
		GrayPalette[i] = RGB(255-i, 255-i, 255-i);
	}

	iSecureGray = convertGrayscaleIcon(loadIcon(IDI_SECURE, 16));
	iCCPMUnSupported = mergeIcons(iSecureGray, loadIcon(IDI_USER_NOCONNECT, 16), 16);

}
void ResourceLoader::unload() {
	searchImages.Destroy();
	settingsTreeImages.Destroy();
	fileImages.Destroy();
	userImages.Destroy();
	flagImages.Destroy();
	autoSearchStatuses.Destroy();
	hubImages.Destroy();
	arrowImages.Destroy();
	filelistTbImages.Destroy();
	queueTreeImages.Destroy();
	thumbBarImages.Destroy();
}

CImageList& ResourceLoader::getUserImages() {
	
	if(userImages.IsNull()) {
		userImages.Create(16, 16, ILC_COLOR32 | ILC_MASK,   10, 10);
		const unsigned baseCount = USER_ICON_MOD_START;
		const unsigned modifierCount = USER_ICON_LAST - USER_ICON_MOD_START;

		CIcon bases[baseCount] = { loadIcon(IDI_USER_BASE, 16), loadIcon(IDI_USER_AWAY, 16), loadIcon(IDI_USER_BOT, 16) };
		CIcon modifiers[modifierCount] = { loadIcon(IDI_USER_PASSIVE, 16), loadIcon(IDI_USER_OP, 16), loadIcon(IDI_USER_NOCONNECT, 16)/*, loadIcon(IDI_FAV_USER, 16)*/ };

		for(size_t iBase = 0; iBase < baseCount; ++iBase) {
			for(size_t i = 0, n = modifierCount * modifierCount; i < n; ++i) {
				CImageListManaged icons;
				icons.Create(16, 16, ILC_COLOR32 | ILC_MASK, 3, 3);

				icons.AddIcon(bases[iBase]);

				for(size_t iMod = 0; iMod < modifierCount; ++iMod)
					if(i & (1i64 << iMod))
						icons.AddIcon(modifiers[iMod]);

				const size_t imageCount = icons.GetImageCount();
				if(imageCount > 1) {
					CImageListManaged  tmp;
					tmp.Create(16, 16, ILC_COLOR32 | ILC_MASK, 3, 3);
					tmp.AddIcon(CIcon(MergeImages(icons, 0, icons, 1)));

					for(size_t k = 2; k < imageCount; ++k)
						tmp.ReplaceIcon(0, CIcon(MergeImages(tmp, 0, icons, k)));
			
					userImages.AddIcon(CIcon(tmp.GetIcon(0, ILD_TRANSPARENT)));
				} else {
					userImages.AddIcon(CIcon(icons.GetIcon(0, ILD_TRANSPARENT)));
				}
			}
		}
	}
	return userImages;
}
HICON ResourceLoader::MergeImages(HIMAGELIST hImglst1, int pos, HIMAGELIST hImglst2, int pos2) {
	/* Adding the merge images to a same Imagelist before merging makes the transparency work on xp */
	CImageListManaged tmp;
	tmp.Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 0);
	CIcon base = ImageList_GetIcon(hImglst1, pos, ILD_TRANSPARENT);
	CIcon overlay = ImageList_GetIcon(hImglst2, pos2, ILD_TRANSPARENT);
	tmp.AddIcon(base);
	tmp.AddIcon(overlay);
	CImageListManaged mergelist;
	mergelist.Merge(tmp, 0, tmp, 1, 0, 0);

	return mergelist.GetIcon(0, ILD_TRANSPARENT);

}


HBITMAP ResourceLoader::getBitmapFromIcon(long defaultIcon, COLORREF crBgColor, int xSize /*= 0*/, int ySize /*= 0*/) {
	CIcon hIcon = loadIcon(defaultIcon, xSize);
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
	DeleteObject(hBrush);
	DeleteObject(hOldBitmap);

	return hBitmap;
}

tstring ResourceLoader::getIconPath(const tstring& filename) {
	return m_IconPath.empty() ? Util::emptyStringT : (m_IconPath + _T("\\") + filename);
}

HICON ResourceLoader::loadIcon(int aDefault, int size) {
	tstring icon = getIconPath(getIconName(aDefault));
	HICON iHandle = icon.empty() ? NULL : (HICON)::LoadImage(NULL, icon.c_str(), IMAGE_ICON, size, size, LR_DEFAULTSIZE | LR_DEFAULTCOLOR | LR_LOADFROMFILE);
	if(!iHandle) 
		return loadDefaultIcon(aDefault, size);
	return iHandle;
}

HICON ResourceLoader::loadDefaultIcon(int icon, int size/* = 0*/) {
	return (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(icon), IMAGE_ICON, size, size, LR_DEFAULTSIZE | LR_DEFAULTCOLOR);
}

HICON ResourceLoader::getIcon(int aDefault, int size) {
	auto icons = cachedIcons.equal_range(aDefault);
	for (auto i = icons.first; i != icons.second; ++i) {
		if (i->second.size == size)
			return i->second.handle;
	}

	HICON newIcon = loadIcon(aDefault, size);
	cachedIcons.emplace(aDefault, cachedIcon(size, newIcon));
	return newIcon;

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
		case IDI_FOLLOW:		return _T("ToolbarImages\\Follow.ico");
		case IDI_USERS:			return _T("ToolbarImages\\UsersFrame.ico");
		case IDI_OPEN_LIST:		return _T("ToolbarImages\\OpenFileList.ico");
		case IDI_NOTEPAD:		return _T("ToolbarImages\\Notepad.ico");
		case IDI_OPEN_DOWNLOADS:return _T("ToolbarImages\\OpenDLDir.ico");
		case IDI_LOGS :			return _T("ToolbarImages\\SystemLog.ico");
		case IDI_UPLOAD_QUEUE:	return _T("ToolbarImages\\uQueue.ico");
		case IDI_EXTENSIONS:	return _T("ToolbarImages\\Extensions.ico");
		case IDI_AUTOSEARCH:	return _T("ToolbarImages\\AutoSearch.ico");
		case IDI_LOGDIR:		return _T("ToolbarImages\\LogDir.ico");
		case IDI_RSS:			return _T("ToolbarImages\\RSS.ico");
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
		case IDI_HUBOFF:		return _T("HubOff.ico");
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
		case IDI_SFILE:			return _T("searchTypes\\file.ico");
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
		case IDI_DIR_INC_OL:	return _T("overlay.ico");
		case IDI_USER_BASE:		return _T("UserlistImages\\UserBase.ico");
		case IDI_USER_AWAY:		return _T("UserlistImages\\UserAway.ico");
		case IDI_USER_BOT:		return _T("UserlistImages\\UserBot.ico");
		case IDI_USER_PASSIVE:	return _T("UserlistImages\\UserNoCon.ico");
		case IDI_USER_OP:		return _T("UserlistImages\\UserOP.ico");
		case IDI_SLOTS:			return _T("Slots.ico");
		case IDI_SLOTSFULL:		return _T("SlotsFull.ico");
		case IDR_TRAY_PM:		return _T("PM.ico");
		case IDR_TRAY_HUB:		return _T("HubMessage.ico");
		case IDI_MESSAGE_OVERLAY:		return _T("message_overlay.ico");
		case IDI_TOTAL_UP:		return _T("TotalUp.ico");
		case IDI_TOTAL_DOWN:	return _T("TotalDown.ico");
		case IDI_SHARED:		return _T("Shared.ico");
		case IDI_DIR_LOADING_OL:return _T("ExecMasked.ico");
		case IDI_FAV_USER:		return _T("UsersFrame\\FavUser.ico");
		case IDI_FAV_USER_OFF:	return _T("UsersFrame\\FavUserOff.ico");
		case IDI_GRANT_ON:		return _T("UsersFrame\\grant.ico");
		case IDI_GRANT_OFF:		return _T("UsersFrame\\grantOff.ico");
		case IDI_USER_ON:		return _T("UsersFrame\\online.ico");
		case IDI_USER_OFF:		return _T("UsersFrame\\offline.ico");
		case IDI_EXPAND_UP:		return _T("expand_up.ico");
		case IDI_EXPAND_DOWN:	return _T("expand_down.ico");
		case IDR_UPDATE:		return _T("Update.ico");
		case IDR_EMOTICON:		return _T("Emoticon.ico");
		case IDI_MAGNET:		return _T("Magnet.ico");
		case IDI_SEND_FILE:		return _T("send_file.ico");
		case IDI_PAUSED:		return _T("paused.ico");
		case IDI_STEPBACK:		return _T("Stepback.ico");
		case IDI_SECURE:		return _T("Secure.ico");
		case IDI_TYPING:		return _T("Typing.ico");
		case IDI_SEEN:			return _T("Seen.ico");
		case IDI_SEND_MESSAGE:	return _T("Send_message.ico");

		case IDI_UP:            return _T("BrowserBar\\Up.ico");
		case IDI_NEXT:			return _T("BrowserBar\\NextResult.ico");
		case IDI_PREV:			return _T("BrowserBar\\PreviousResult.ico");
		case IDI_BACK:			return _T("BrowserBar\\Back.ico");
		case IDI_FORWARD:		return _T("BrowserBar\\Forward.ico");
		case IDI_FIND:			return _T("BrowserBar\\Find.ico");
		case IDI_RELOAD:		return _T("BrowserBar\\Reload.ico");

		case IDI_SEARCHING:		return _T("AutoSearch\\Searching.ico");
		case IDI_QUEUED_OK:		return _T("AutoSearch\\QueuedOK.ico");
		case IDI_QUEUED_ERROR:	return _T("AutoSearch\\QueuedError.ico");
		case IDI_SEARCH_ERROR:	return _T("AutoSearch\\SearchError.ico");
		case IDI_WAITING:		return _T("AutoSearch\\Waiting.ico");
		case IDI_MANUAL:		return _T("AutoSearch\\Manual.ico");
		case IDI_DISABLED:		return _T("AutoSearch\\Disabled.ico");
		case IDI_COLLECTING:	return _T("AutoSearch\\Collecting.ico");
		case IDI_POSTSEARCH:	return _T("AutoSearch\\PostSearch.ico");
		case IDI_EXPIRED:		return _T("AutoSearch\\Expired.ico");

		default: return Util::emptyStringT;
	}
}

HICON ResourceLoader::mergeIcons(HICON tmp1, HICON tmp2, int size){
	CImageListManaged tmp;
	tmp.Create(size, size, ILC_COLOR32 | ILC_MASK, 0, 0);
	tmp.AddIcon(tmp1);
	tmp.AddIcon(tmp2);
	CImageListManaged mergelist;
	mergelist.Merge(tmp, 0, tmp, 1, 0, 0);

	return mergelist.GetIcon(0, ILD_TRANSPARENT);
}

CImageList& ResourceLoader::getQueueTreeImages() {
	if (queueTreeImages.IsNull()){
		const int size = 16;
		queueTreeImages.Create(size, size, ILC_COLOR32 | ILC_MASK, 0, 3);
		queueTreeImages.AddIcon(CIcon(loadIcon(IDI_DOWNLOAD, size)));
		queueTreeImages.AddIcon(CIcon(loadIcon(IDI_FINISHED_DL, size)));
		queueTreeImages.AddIcon(CIcon(loadIcon(IDI_QUEUE, size)));
		queueTreeImages.AddIcon(CIcon(loadIcon(IDI_QUEUED_ERROR, size)));
		queueTreeImages.AddIcon(CIcon(loadIcon(IDI_PAUSED, size)));
		queueTreeImages.AddIcon(CIcon(loadIcon(IDI_AUTOSEARCH, size)));
		queueTreeImages.AddIcon(CIcon(getFileImages().GetIcon(DIR_NORMAL)));
		queueTreeImages.AddIcon(CIcon(loadIcon(IDI_OPEN_LIST, size)));
		queueTreeImages.AddIcon(CIcon(loadIcon(IDI_SEND_FILE, size)));
	}
	return queueTreeImages;
}

CImageList& ResourceLoader::getArrowImages() {
	if(arrowImages.IsNull()){
		const int size = 22;
		arrowImages.Create(size, size, ILC_COLOR32 | ILC_MASK,  0, 3);
		arrowImages.AddIcon(CIcon(loadIcon(IDI_UP, size)));
		arrowImages.AddIcon(CIcon(loadIcon(IDI_FORWARD, size)));
		arrowImages.AddIcon(CIcon(loadIcon(IDI_BACK, size)));
	}
	return arrowImages;
}

CImageList& ResourceLoader::getFilelistTbImages() {
	if(filelistTbImages.IsNull()){
		const int size = 16;
		filelistTbImages.Create(size, size, ILC_COLOR32 | ILC_MASK,  0, 4);
		filelistTbImages.AddIcon(CIcon(loadIcon(IDI_RELOAD, size)));
		filelistTbImages.AddIcon(CIcon(loadIcon(IDI_FIND, size)));
		filelistTbImages.AddIcon(CIcon(loadIcon(IDI_PREV, size)));
		filelistTbImages.AddIcon(CIcon(loadIcon(IDI_NEXT, size)));
	}
	return filelistTbImages;
}
CImageList& ResourceLoader::getHubImages() {
	if (hubImages.IsNull()){
		const int size = 16;
		hubImages.Create(size, size, ILC_COLOR32 | ILC_MASK, 0, 3);
		hubImages.AddIcon(CIcon(loadIcon(IDI_HUB, size)));
		hubImages.AddIcon(CIcon(mergeIcons(loadIcon(IDI_HUB, size), loadIcon(IDI_HUBREG, 16), 16)));
		hubImages.AddIcon(CIcon(mergeIcons(loadIcon(IDI_HUB, size), loadIcon(IDI_HUBOP, 16), 16)));
		hubImages.AddIcon(CIcon(loadIcon(IDI_HUBOFF, size)));
	}
	return hubImages;
}

CImageList& ResourceLoader::getFileImages() {

	if(fileImages.IsNull()) {
		fileImages.Create(16, 16, ILC_COLOR32 | ILC_MASK,  0, 4);
		fileImages.AddIcon(CIcon(loadIcon(IDI_FOLDER, 16))); fileImageCount++;
		fileImages.AddIcon(CIcon(loadIcon(IDI_FOLDER_INC, 16))); fileImageCount++;
		fileImages.AddIcon(CIcon(loadIcon(IDI_FOLDER, 16))); fileImageCount++;
		fileImages.AddIcon(CIcon(loadIcon(IDI_STEPBACK, 16))); fileImageCount++;
		fileImages.AddIcon(CIcon(loadIcon(IDI_FILE, 16))); fileImageCount++;

		if(SETTING(USE_SYSTEM_ICONS)) {
			SHFILEINFO fi;
			memzero(&fi, sizeof(SHFILEINFO));
			if(::SHGetFileInfo(_T("./"), FILE_ATTRIBUTE_DIRECTORY, &fi, sizeof(fi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES)) {
				fileImages.ReplaceIcon(DIR_NORMAL, fi.hIcon);

				auto mergeIcon = [&] (int resourceID, int8_t iconIndex) -> void {
					CIcon overlayIcon = loadIcon(resourceID, 16);

					CImageListManaged tmpIcons;
					tmpIcons.Create(16, 16, ILC_COLOR32 | ILC_MASK, 2, 1);
					tmpIcons.AddIcon(fi.hIcon);
					tmpIcons.AddIcon(overlayIcon);

					CImageListManaged mergeList(ImageList_Merge(tmpIcons, 0, tmpIcons, 1, 0, 0));
					CIcon icDirIcon = mergeList.GetIcon(0);
					fileImages.ReplaceIcon(iconIndex, icDirIcon);

				};

				mergeIcon(IDI_DIR_INC_OL, DIR_INCOMPLETE);
				mergeIcon(IDI_DIR_LOADING_OL, DIR_LOADING);
			
				::DestroyIcon(fi.hIcon);
			}
		}

	}
	return fileImages;
}

CImageList& ResourceLoader::getSettingsTreeIcons() {
	if(settingsTreeImages.IsNull()) {
		int size = 16;
		settingsTreeImages.Create(size, size, ILC_COLOR32 | ILC_MASK,  0, 30);
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_GENERAL, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_CONNECTIONS, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_SPEED, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_LIMITS, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_PROXY, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_DOWNLOADS, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_LOCATIONS, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_PREVIEW, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_PRIORITIES, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_QUEUE, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_SHARING, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_SHAREOPTIONS, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_HASHING, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_WEBSERVER, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_APPEARANCE, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_FONTS, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_PROGRESS, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_USERLIST, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_SOUNDS, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_TOOLBAR, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_WINDOWS, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_TABS, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_POPUPS, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_HIGHLIGHTS, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_AIRAPPEARANCE, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_ADVANCED, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_EXPERTS, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_LOGS, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_UC, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_WEB_SHORTCUTS, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_CERTIFICATES, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_MISC, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_IGNORE, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_SEARCH, size)));
		settingsTreeImages.AddIcon(CIcon(loadDefaultIcon(IDI_SEARCHTYPES, size)));
	}
	return settingsTreeImages;
}

CImageList&  ResourceLoader::getSearchTypeIcons() {
	if(searchImages.IsNull()) {
		searchImages.Create(16, 16, ILC_COLOR32 | ILC_MASK,  0, 15);
		searchImages.AddIcon(CIcon(loadIcon(IDI_ANY, 16)));
		searchImages.AddIcon(CIcon(loadIcon(IDI_AUDIO, 16)));
		searchImages.AddIcon(CIcon(loadIcon(IDI_COMPRESSED, 16)));
		searchImages.AddIcon(CIcon(loadIcon(IDI_DOCUMENT, 16)));
		searchImages.AddIcon(CIcon(loadIcon(IDI_EXEC, 16)));
		searchImages.AddIcon(CIcon(loadIcon(IDI_PICTURE, 16)));
		searchImages.AddIcon(CIcon(loadIcon(IDI_VIDEO, 16)));
		searchImages.AddIcon(CIcon(loadIcon(IDI_DIRECTORY,16)));
		searchImages.AddIcon(CIcon(loadIcon(IDI_TTH, 16)));
		searchImages.AddIcon(CIcon(loadIcon(IDI_SFILE, 16)));
		searchImages.AddIcon(CIcon(loadIcon(IDI_CUSTOM, 16)));
	}
	return searchImages;
}

CImageList&  ResourceLoader::getAutoSearchStatuses() {
	if (autoSearchStatuses.IsNull()) {
		autoSearchStatuses.Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 2);
		autoSearchStatuses.AddIcon(CIcon(loadIcon(IDI_DISABLED, 16)));
		autoSearchStatuses.AddIcon(CIcon(loadIcon(IDI_EXPIRED, 16)));
		autoSearchStatuses.AddIcon(CIcon(loadIcon(IDI_MANUAL, 16)));
		autoSearchStatuses.AddIcon(CIcon(loadIcon(IDI_SEARCHING, 16)));
		autoSearchStatuses.AddIcon(CIcon(loadIcon(IDI_COLLECTING, 16)));
		autoSearchStatuses.AddIcon(CIcon(loadIcon(IDI_WAITING, 16)));
		autoSearchStatuses.AddIcon(CIcon(loadIcon(IDI_POSTSEARCH, 16)));
		autoSearchStatuses.AddIcon(CIcon(loadIcon(IDI_QUEUED_OK, 16)));
		autoSearchStatuses.AddIcon(CIcon(loadIcon(IDI_QUEUED_ERROR, 16)));
		autoSearchStatuses.AddIcon(CIcon(loadIcon(IDI_SEARCH_ERROR, 16)));
	}

	return autoSearchStatuses;
}

//Yes, could use settings cmdBar list but, decided to do this anyway...
CImageList& ResourceLoader::getThumbBarImages() {

	if (thumbBarImages.IsNull()) {
		const int size = 16;
		thumbBarImages.Create(size, size, ILC_COLOR32 | ILC_MASK, 0, 3);
		thumbBarImages.AddIcon(CIcon(loadIcon(IDI_OPEN_DOWNLOADS, size)));
		thumbBarImages.AddIcon(CIcon(loadIcon(IDI_SETTINGS, size)));
	}
	return thumbBarImages;
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
		return FILE;
	}
}

void ResourceLoader::loadCmdBarImageList(CImageList& images){
	images.Create(16, 16, ILC_COLOR32 | ILC_MASK,  0, 33);
	//use default ones totally here too?
	images.AddIcon(CIcon(loadIcon(IDI_PUBLICHUBS, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_RECONNECT, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_FOLLOW, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_RECENTS, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_FAVORITEHUBS, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_USERS, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_QUEUE, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_UPLOAD_QUEUE, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_FINISHED_UL, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_SEARCH, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_ADLSEARCH, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_SEARCHSPY, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_OPEN_LIST, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_OWNLIST, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_MATCHLIST, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_REFRESH, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_OPEN_DOWNLOADS, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_QCONNECT, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_SETTINGS, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_GET_TTH, 16)));
	images.AddIcon(CIcon(loadIcon(IDR_UPDATE, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_SHUTDOWN, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_NOTEPAD, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_CDM, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_LOGS, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_AUTOSEARCH, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_EXTENSIONS, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_INDEXING, 16)));
	images.AddIcon(CIcon(loadIcon(IDR_MAINFRAME, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_WIZARD, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_LOGDIR, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_SETTINGS, 16)));
	images.AddIcon(CIcon(loadIcon(IDI_RSS, 16)));
}

void ResourceLoader::loadWinampToolbarIcons(CImageList& winampImages) {
	int size = SETTING(WTB_IMAGE_SIZE);
	winampImages.Create(size, size, ILC_COLOR32 | ILC_MASK,  0, 11);
	winampImages.AddIcon(CIcon(loadIcon(IDI_MPSTART, size)));
	winampImages.AddIcon(CIcon(loadIcon(IDI_MPSPAM, size)));
	winampImages.AddIcon(CIcon(loadIcon(IDI_MPBACK, size)));
	winampImages.AddIcon(CIcon(loadIcon(IDI_MPPLAY, size)));
	winampImages.AddIcon(CIcon(loadIcon(IDI_MPPAUSE, size)));
	winampImages.AddIcon(CIcon(loadIcon(IDI_MPNEXT, size)));
	winampImages.AddIcon(CIcon(loadIcon(IDI_MPSTOP, size)));
	winampImages.AddIcon(CIcon(loadIcon(IDI_MPVOLUMEUP, size)));
	winampImages.AddIcon(CIcon(loadIcon(IDI_MPVOLUME50, size)));
	winampImages.AddIcon(CIcon(loadIcon(IDI_MPVOLUMEDOWN, size)));
}

void ResourceLoader::loadFlagImages() {
	flagImages.CreateFromImage(IDB_FLAGS, 25, 8, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED); 
}

const CIcon& ResourceLoader::getSeverityIcon(uint8_t sev) {
	switch (sev) {
		case LogMessage::SEV_INFO : 
			if (!infoIcon)
				infoIcon = loadIcon(IDI_INFO, 16);
			return infoIcon;
		case LogMessage::SEV_WARNING:
			if (!warningIcon)
				warningIcon = loadIcon(IDI_IWARNING, 16);
			return warningIcon;
		case LogMessage::SEV_ERROR:
			if (!errorIcon)
				errorIcon = loadIcon(IDI_IERROR, 16);
			return errorIcon;
		default :
			break;
	}
	return infoIcon;
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
