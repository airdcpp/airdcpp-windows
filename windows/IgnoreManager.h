///////////////////////////////////////////////////////////////////////////////
//
//	Handles saving and loading of ignorelists
//
///////////////////////////////////////////////////////////////////////////////

#ifndef IGNOREMANAGER_H
#define IGNOREMANAGER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/DCPlusPlus.h"

#include "../client/Singleton.h"
#include "../client/SettingsManager.h"
#include "../client/SimpleXML.h"
#include "../client/User.h"

namespace dcpp {
class IgnoreManager: public Singleton<IgnoreManager>, private SettingsManagerListener
{
public:
	IgnoreManager() { SettingsManager::getInstance()->addListener(this); }
	~IgnoreManager() { SettingsManager::getInstance()->removeListener(this); }

	// store & remove ignores through/from hubframe
	void storeIgnore(const UserPtr& user);
	void removeIgnore(const UserPtr& user);

	// check if user is ignored
	bool isIgnored(const string& aNick);

	// get and put ignorelist (for MiscPage)
	unordered_set<tstring> getIgnoredUsers() { Lock l(cs); return ignoredUsers; }
	void putIgnoredUsers(unordered_set<tstring> ignoreList) { Lock l(cs); ignoredUsers = ignoreList; }

private:
	typedef unordered_set<tstring> TStringHash;
	typedef TStringHash::const_iterator TStringHashIterC;
	CriticalSection cs;

	// save & load
	void load(SimpleXML& aXml);
	void save(SimpleXML& aXml);
	GETSET(UserPtr, user, User);
	// SettingsManagerListener
	virtual void on(SettingsManagerListener::Load, SimpleXML& xml) throw();
	virtual void on(SettingsManagerListener::Save, SimpleXML& xml) throw();

	// contains the ignored nicks and patterns 
	TStringHash ignoredUsers;
};
}
#endif // IGNOREMANAGER_H