/* 
 * 
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

#include <windows/stdafx.h>

#include <airdcpp/core/classes/Exception.h>
#include <airdcpp/settings/SettingsManager.h>
#include <airdcpp/core/io/xml/SimpleXML.h>
#include <airdcpp/util/PathUtil.h>
#include <airdcpp/core/io/File.h>
#include <airdcpp/events/LogManager.h>
#include <airdcpp/util/SystemUtil.h>

#include "boost/algorithm/string/replace.hpp"

#include <windows/EmoticonsManager.h>
#include <windows/util/WinUtil.h>

#include <cmath>

namespace wingui {
Emoticon::Emoticon(const tstring& _emoticonText, const string& _imagePath) : 
	emoticonText(_emoticonText), imagePath(_imagePath)
{	
	if(!emoticonBitmap.IsNull()) 
		emoticonBitmap.Destroy();
	
	if(emoticonBitmap.Load(Text::toT(_imagePath).c_str()) != S_OK) {
		dcdebug("Emoticon load Error: %s \n", SystemUtil::translateError(GetLastError()).c_str());
		return;
	}
		

	if(emoticonBitmap.IsNull()) {
		return;
	}
	
	BITMAP bm = { 0 };
	GetObject(emoticonBitmap, sizeof(bm), &bm);
	
	if(bm.bmBitsPixel == 32) {
		BYTE *pBits = new BYTE[bm.bmWidth * bm.bmHeight * 4];
		GetBitmapBits(emoticonBitmap, bm.bmWidth * bm.bmHeight * 4, pBits);
		
		// fix alpha channel	
		for (int y = 0; y < bm.bmHeight; y++) {
			BYTE * pPixel = (BYTE *) pBits + bm.bmWidth * 4 * y;

			for (int x = 0; x < bm.bmWidth; x++) {
				pPixel[0] = pPixel[0] * pPixel[3] / 255; 
				pPixel[1] = pPixel[1] * pPixel[3] / 255; 
				pPixel[2] = pPixel[2] * pPixel[3] / 255; 

				pPixel += 4;
			}
		}
		SetBitmapBits((HBITMAP)emoticonBitmap, bm.bmWidth * bm.bmHeight * 4, pBits);
	    
		delete[] pBits;
	} else if(bm.bmBitsPixel <= 8) {
		// Let's convert to a bitmap TransparentBlt can handle
		HDC fixDC = CreateCompatibleDC(NULL);

		BITMAPINFO bmi;
		ZeroMemory(&bmi, sizeof(BITMAPINFO));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = bm.bmWidth;
		bmi.bmiHeader.biHeight = bm.bmHeight;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 16;
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biSizeImage = bm.bmWidth * bm.bmHeight * 2;

		VOID *pvBits;
		HBITMAP oldFixBitmap = (HBITMAP)SelectObject(fixDC, CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0));

		emoticonBitmap.BitBlt(fixDC, 0, 0, SRCCOPY);
		emoticonBitmap.Destroy();

		emoticonBitmap.Attach((HBITMAP)SelectObject(fixDC, oldFixBitmap));
		DeleteDC(fixDC);
	}
}

HBITMAP Emoticon::getEmoticonBmp(const COLORREF &clrBkColor) {
	if(emoticonBitmap.IsNull())
		return NULL;

	HDC DirectDC = CreateCompatibleDC(NULL);
	HDC memDC = emoticonBitmap.GetDC();
	
	BITMAP bm = { 0 };
	GetObject((HBITMAP)emoticonBitmap, sizeof(bm), &bm);

	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = bm.bmWidth;
	bmi.bmiHeader.biHeight = bm.bmHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = bm.bmBitsPixel;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = bm.bmWidth * bm.bmHeight * (bm.bmBitsPixel / 8);
	
	VOID *pvBits;
	HBITMAP DirectBitmap = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);

	SelectObject(DirectDC, DirectBitmap);
	SetBkColor(DirectDC, clrBkColor);
	
	RECT rc = { 0, 0, bm.bmWidth, bm.bmHeight };
	ExtTextOut(DirectDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

	if(bm.bmBitsPixel == 32) {
		BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
		AlphaBlend(DirectDC, 0, 0, bm.bmWidth, bm.bmHeight, memDC, 0, 0, bm.bmWidth, bm.bmHeight, bf);
	} else {
		TransparentBlt(DirectDC, 0, 0, bm.bmWidth, bm.bmHeight, memDC, 0, 0, bm.bmWidth, bm.bmHeight, GetPixel(memDC, 0, 0));	
	}

	emoticonBitmap.ReleaseDC();
	DeleteDC(DirectDC);

    return DirectBitmap;
}

void EmoticonsManager::Load() {

	setUseEmoticons(false);
	const auto emojiPath = WinUtil::getPath(WinUtil::PATH_EMOPACKS);

	if((SETTING(EMOTICONS_FILE) == "Disabled") || !PathUtil::fileExists(emojiPath + SETTING(EMOTICONS_FILE) + ".xml" )) {
		return;
	}

	try {
		SimpleXML xml;
		xml.fromXML(File(emojiPath + SETTING(EMOTICONS_FILE) + ".xml", File::READ, File::OPEN).read());
		
		if(xml.findChild("Emoticons")) {
			xml.stepIn();

			while(xml.findChild("Emoticon")) {
				tstring strEmotionText = Text::toT(xml.getChildAttrib("PasteText"));
				if (strEmotionText.empty()) {
					strEmotionText = Text::toT(xml.getChildAttrib("Expression"));
				}
				
				boost::algorithm::replace_all(strEmotionText, " ", "");
				string strEmotionBmpPath = xml.getChildAttrib("Bitmap");
				if (!strEmotionBmpPath.empty()) {

						strEmotionBmpPath = emojiPath + strEmotionBmpPath;
				}

				emoticons.push_back(new Emoticon(strEmotionText, strEmotionBmpPath));
			}
			xml.stepOut();
		}
	} catch(const Exception& e) {
		dcdebug("EmoticonsManager::Create: %s\n", e.getError().c_str());
		return;
	}
	
	setUseEmoticons(true);
}

void EmoticonsManager::Unload() {
	for_each(emoticons.begin(), emoticons.end(), std::default_delete<Emoticon>());
	emoticons.clear();
}
}