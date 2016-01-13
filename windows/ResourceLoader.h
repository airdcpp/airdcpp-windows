/*
 * Copyright (C) 2011-2015 AirDC++ Project
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

#ifndef RESOURCE_LOADER_H
#define RESOURCE_LOADER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef __ATLMISC_H__
#define __ATLTYPES_H__
#endif

#include <airdcpp/Util.h>
#include <airdcpp/SettingsManager.h>
#include <airdcpp/Singleton.h>

#include "resource.h"
#include "ExCImage.h"

#define GET_ICON(id, size) ResourceLoader::getIcon(id, size)

class ResourceLoader
{
public:
	static void load();
	static void unload();
	static void loadWinampToolbarIcons(CImageList& winampImages);
	static void loadCmdBarImageList(CImageList& images);
	static void loadFlagImages();

	static CImageList& getAutoSearchStatuses();
	static CImageList& getSettingsTreeIcons();
	static CImageList& getSearchTypeIcons();
	static CImageList& getUserImages();
	static CImageList& getFileImages();
	static CImageList& getArrowImages();
	static CImageList& getFilelistTbImages();
	static CImageList& getHubImages();
	static CImageList& getQueueTreeImages();

	//Loads icon with resource id, returned handle is unmanaged.
	static HICON loadIcon(int aDefault, int size);
	//Get icon with resource id, loads if necessary. Manages the handles by adding them in cache.
	static HICON getIcon(int aDefault, int size);

	static HICON convertGrayscaleIcon(HICON hIcon);

	static HBITMAP getBitmapFromIcon(long defaultIcon, COLORREF crBgColor, int xSize = 0, int ySize = 0);
	
	static tstring getIconPath(const tstring& filename);

	static CImageList flagImages;

	static const CIcon& getSeverityIcon(uint8_t sev);

	static CIcon iSecureGray;
	static CIcon iCCPMUnSupported;

	enum {
		DIR_NORMAL,
		DIR_INCOMPLETE,
		DIR_LOADING,
		DIR_STEPBACK,
		FILE
	};

	static int getIconIndex(const tstring& aFileName);
	static HICON mergeIcons(HICON tmp1, HICON tmp2, int size);

private:
	
	typedef std::map<tstring, int> ImageMap;
	typedef ImageMap::const_iterator ImageIter;
	static ImageMap fileIndexes;

	struct cachedIcon {
		cachedIcon(int aSize, HICON aHandle) : size(aSize), handle(aHandle) {}
		int size;
		HICON handle;
	};

	//Store loaded icons in all different loaded sizes by resource id
	static std::multimap<int, cachedIcon> cachedIcons;

	static int fileImageCount;
	static COLORREF GrayPalette[256];
	static HICON MergeImages(HIMAGELIST hImglst1, int pos, HIMAGELIST hImglst2, int pos2);
	static tstring getIconName(int aDefault);
	static HICON loadDefaultIcon(int icon, int size=0);

	static CImageList settingsTreeImages;
	static CImageList searchImages;
	static CImageList userImages;
	static CImageList fileImages;
	static CImageList arrowImages;
	static CImageList filelistTbImages;
	static CImageList autoSearchStatuses;
	static CImageList hubImages;
	static CImageList queueTreeImages;
	
	static tstring m_IconPath;

	static CIcon infoIcon;
	static CIcon warningIcon;
	static CIcon errorIcon;

};

#endif // RESOURCE_LOADER_H