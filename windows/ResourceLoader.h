/*
 * Copyright (C) 2011-2013 AirDC++ Project
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

#include "../client/Util.h"
#include "../client/SettingsManager.h"
#include "../client/Singleton.h"

#include "resource.h"
#include "ExCImage.h"

class ResourceLoader
{
public:
	static void load();
	static void unload();
	static void loadWinampToolbarIcons(CImageList& winampImages);
	static void loadCmdBarImageList(CImageList& images);
	static void loadFlagImages();

	static CImageList& getSettingsTreeIcons();
	static CImageList& getSearchTypeIcons();
	static CImageList& getUserImages();
	static CImageList& getFileImages();
	static CImageList& getArrowImages();
	static CImageList& getFilelistTbImages();
	
	static HICON loadIcon(int aDefault, int size = 0);
	static HICON convertGrayscaleIcon(HICON hIcon);
	static HBITMAP getBitmapFromIcon(long defaultIcon, COLORREF crBgColor, int xSize = 0, int ySize = 0);
	
	static tstring getIconPath(const tstring& filename);

	static CImageList flagImages;

	enum {
		DIR_NORMAL,
		DIR_INCOMPLETE,
		DIR_LOADING,
		FILE
	};

	static int getIconIndex(const tstring& aFileName);

private:
	
	typedef std::map<tstring, int> ImageMap;
	typedef ImageMap::const_iterator ImageIter;
	static ImageMap fileIndexes;

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
	
	static tstring m_IconPath;


};

#endif // RESOURCE_LOADER_H