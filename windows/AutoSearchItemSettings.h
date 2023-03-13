/*
* Copyright (C) 2011-2023 AirDC++ Project
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

#include <airdcpp/modules/AutoSearchManager.h>
#include <airdcpp/Search.h>
#include <airdcpp/SettingsManager.h>


//Item settings to be feed into dialog... yeah, open for cleaner methods.. at least rename some of them.
struct AutoSearchItemSettings {

	//Default constructor, for adding auto search
	AutoSearchItemSettings() :
		as(nullptr), fileTypeId(SETTING(LAST_AS_FILETYPE)), action(0), matcherType(0), remove(false), target(SETTING(DOWNLOAD_DIRECTORY)),
		curNumber(1), maxNumber(0), startTime(0, 0),
		endTime(23, 59), searchDays("1111111"), checkQueued(true), checkShared(true), matchFullPath(false),
		numberLen(2), useParams(false), groupName(Util::emptyString), userMatcherExclude(false)
	{
		expireTime = SETTING(AUTOSEARCH_EXPIRE_DAYS) > 0 ? GET_TIME() + (SETTING(AUTOSEARCH_EXPIRE_DAYS) * 24 * 60 * 60) : 0;
	}

	//Construct with auto search item, for changing or duplicate.
	AutoSearchItemSettings(const AutoSearchPtr& aAutoSearch, bool isDuplicate) : as(isDuplicate ? nullptr : aAutoSearch),
		searchString(aAutoSearch->getSearchString()),
		excludedWords(aAutoSearch->getExcludedString()),
		fileTypeId(aAutoSearch->getFileType()),
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
		checkQueued(aAutoSearch->getCheckAlreadyQueued()),
		checkShared(aAutoSearch->getCheckAlreadyShared()),
		matchFullPath(aAutoSearch->getMatchFullPath()),
		groupName(aAutoSearch->getGroup()),
		userMatcherExclude(aAutoSearch->getUserMatcherExclude()),

		curNumber(aAutoSearch->getCurNumber()),
		numberLen(aAutoSearch->getNumberLen()),
		maxNumber(aAutoSearch->getMaxNumber()),
		useParams(aAutoSearch->getUseParams())
	{ }



public:

	//seed the modified properties into auto search
	void setItemProperties(AutoSearchPtr& aAutoSearch, string& aSearchString) {
		
		if (aSearchString[0] == '$') {
			aAutoSearch->setEnabled(false);
			aSearchString = aSearchString.substr(1);
		}


		aAutoSearch->setSearchString(aSearchString);
		aAutoSearch->setExcludedString(excludedWords);
		aAutoSearch->setFileType(fileTypeId);
		aAutoSearch->setAction((AutoSearch::ActionType)action);
		aAutoSearch->setRemove(remove);
		aAutoSearch->setTarget(target);
		aAutoSearch->setMethod((StringMatch::Method)matcherType);
		aAutoSearch->setMatcherString(matcherString);
		aAutoSearch->setUserMatcher(userMatch);
		aAutoSearch->setExpireTime(expireTime);
		aAutoSearch->setCheckAlreadyQueued(checkQueued);
		aAutoSearch->setCheckAlreadyShared(checkShared);
		aAutoSearch->setMatchFullPath(matchFullPath);
		aAutoSearch->setGroup(groupName);
		aAutoSearch->setUserMatcherExclude(userMatcherExclude);

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
	bool userMatcherExclude;

	int numberLen, curNumber, maxNumber;

	string searchString, target, fileTypeId;
	string comment, userMatch, matcherString, excludedWords;
	uint8_t action;
	uint8_t matcherType;
	SearchTime startTime;
	SearchTime endTime;
	bitset<7> searchDays;
	time_t expireTime;
	Search::TypeModes searchType;
	string groupName;
	AutoSearchPtr as;

}; 
#pragma once
#endif