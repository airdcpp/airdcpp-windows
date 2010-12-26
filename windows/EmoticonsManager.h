/* 
 * 
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

#ifndef EMOTICONSMANAGER_H
#define EMOTICONSMANAGER_H

#include "../client/FastAlloc.h"
#include "../client/Util.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Emoticon

class Emoticon : public FastAlloc<Emoticon> {
public:
	typedef list<Emoticon*> List;
	typedef List::const_iterator Iter;

	Emoticon(const tstring& _emoticonText, const string& _imagePath);
	~Emoticon() {	}
	
	const tstring& getEmoticonText() const { return emoticonText; }
	HBITMAP getEmoticonBmp() const {	return emoticonBitmap; }
	HBITMAP getEmoticonBmp(const COLORREF &clrBkColor);
	const string& getEmoticonBmpPath() const { return imagePath; }

private:
	tstring		emoticonText;
	string		imagePath;
	HBITMAP		emoticonBitmap;
};

/**
 * Emoticons Manager
 */
class EmoticonsManager : public intrusive_ptr_base<EmoticonsManager> {
public:
	EmoticonsManager() { Load(); }
	~EmoticonsManager() { Unload(); }

	// Variables
	GETSET(bool, useEmoticons, UseEmoticons);

	const Emoticon::List& getEmoticonsList() const { return emoticons; }

	void Load();
	void Unload();
	
private:
	Emoticon::List emoticons;
};

#endif // EMOTICONSMANAGER_H
