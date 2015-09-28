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

#ifndef AS_SETTINGS_H
#define AS_SETTINGS_H

#include "stdafx.h"
#include "../client/SettingsManager.h"
#include "../client/AutoSearchManager.h"
#include "../client/TargetUtil.h"


//Item settings to be feed into dialog... yeah, open for cleaner methods.. at least rename some of them.
struct AutoSearchItemSettings {

	//Default constructor, for adding auto search
	AutoSearchItemSettings() :
		as(nullptr), fileTypeStr(SETTING(LAST_AS_FILETYPE)), action(0), matcherType(0), remove(false),
		targetType(TargetUtil::TARGET_PATH), curNumber(1), maxNumber(0), startTime(0, 0),
		endTime(23, 59), searchDays("1111111"), checkQueued(true), checkShared(true), matchFullPath(false),
		numberLen(2), useParams(false), searchInterval(180), groupName(Util::emptyString)
	{
		expireTime = SETTING(AUTOSEARCH_EXPIRE_DAYS) > 0 ? GET_TIME() + (SETTING(AUTOSEARCH_EXPIRE_DAYS) * 24 * 60 * 60) : 0;
	}

	//Construct with auto search item, for changing or duplicate.
	AutoSearchItemSettings(const AutoSearchPtr& aAutoSearch, bool isDuplicate) : as(isDuplicate ? nullptr : aAutoSearch),
		searchString(aAutoSearch->getSearchString()),
		excludedWords(aAutoSearch->getExcludedString()),
		fileTypeStr(aAutoSearch->getFileType()),
		action(aAutoSearch->getAction()),
		remove(aAutoSearch->getRemove()),
		target(aAutoSearch->getTarget()),
		userMatch(aAutoSearch->getNickPattern()),
		matcherString(aAutoSearch->getMatcherString()),
		matcherType(aAutoSearch->getMethod()),
		expireTime(aAutoSearch->getExpireTime()),
		searchDays(aAutoSearch->searchDays),
		startTime(aAutoSearch->startTime),
		endTime(aAutoSearch->endTime),
		targetType(aAutoSearch->getTargetType()),
		checkQueued(aAutoSearch->getCheckAlreadyQueued()),
		checkShared(aAutoSearch->getCheckAlreadyShared()),
		matchFullPath(aAutoSearch->getMatchFullPath()),
		searchInterval(aAutoSearch->getSearchInterval()),
		groupName(aAutoSearch->getGroup()),

		curNumber(aAutoSearch->getCurNumber()),
		numberLen(aAutoSearch->getNumberLen()),
		maxNumber(aAutoSearch->getMaxNumber()),
		useParams(aAutoSearch->getUseParams())
	{ }



public:

	//seed the modified properties into auto search
	void setItemProperties(AutoSearchPtr& aAutoSearch, const string& aSearchString) {
		aAutoSearch->setSearchString(aSearchString);
		aAutoSearch->setExcludedString(excludedWords);
		aAutoSearch->setFileType(fileTypeStr);
		aAutoSearch->setAction((AutoSearch::ActionType)action);
		aAutoSearch->setRemove(remove);
		aAutoSearch->setTargetType(targetType);
		aAutoSearch->setTarget(target);
		aAutoSearch->setMethod((StringMatch::Method)matcherType);
		aAutoSearch->setMatcherString(matcherString);
		aAutoSearch->setUserMatcher(userMatch);
		aAutoSearch->setExpireTime(expireTime);
		aAutoSearch->setCheckAlreadyQueued(checkQueued);
		aAutoSearch->setCheckAlreadyShared(checkShared);
		aAutoSearch->setMatchFullPath(matchFullPath);
		aAutoSearch->setSearchInterval(searchInterval);
		aAutoSearch->setGroup(groupName);

		if (aAutoSearch->getCurNumber() != curNumber)
			aAutoSearch->setLastIncFinish(0);

		aAutoSearch->startTime = startTime;
		aAutoSearch->endTime = endTime;
		aAutoSearch->searchDays = searchDays;

		aAutoSearch->setCurNumber(curNumber);
		aAutoSearch->setMaxNumber(maxNumber);
		aAutoSearch->setNumberLen(numberLen);
		aAutoSearch->setUseParams(useParams);
	}


	bool display;
	bool remove;
	bool checkQueued;
	bool checkShared;
	bool matchFullPath;
	bool useParams;

	int searchInterval;
	int numberLen, curNumber, maxNumber;

	string searchString, target, fileTypeStr;
	string comment, userMatch, matcherString, excludedWords;
	uint8_t action;
	TargetUtil::TargetType targetType;
	uint8_t matcherType;
	SearchTime startTime;
	SearchTime endTime;
	bitset<7> searchDays;
	time_t expireTime;
	int searchType;
	string groupName;
	AutoSearchPtr as;

}; 
#pragma once
#endif