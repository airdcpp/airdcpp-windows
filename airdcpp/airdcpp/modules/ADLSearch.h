/*
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
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

/*
 * Automatic Directory Listing Search
 * Henrik Engstr�m, henrikengstrom at home se
 */

#ifndef DCPLUSPLUS_DCPP_A_D_L_SEARCH_H
#define DCPLUSPLUS_DCPP_A_D_L_SEARCH_H

#include <airdcpp/filelist/DirectoryListingDirectory.h>
#include <airdcpp/message/Message.h>
#include <airdcpp/core/Singleton.h>
#include <airdcpp/util/text/StringMatch.h>

namespace dcpp {

class AdlSearchManager;

///	Class that represent an ADL search
class ADLSearch
{
public:

	ADLSearch();								 

	// Active search
	bool isActive = true;

	// Some Comment
	string adlsComment;

	// Auto Queue Results
	bool isAutoQueue = false;

	// Search source type
	enum SourceType {
		TypeFirst = 0,
		OnlyFile = TypeFirst,
		OnlyDirectory,
		FullPath,
		TypeLast
	};
	
	SourceType sourceType = OnlyFile;

	SourceType StringToSourceType(const string& s);

	string SourceTypeToString(SourceType t);

	tstring SourceTypeToDisplayString(SourceType t);

	// Maximum & minimum file sizes (in bytes). 
	// Negative values means do not check.
	int64_t minFileSize = -1;
	int64_t maxFileSize = -1;

	enum SizeType {
		SizeBytes     = TypeFirst,
		SizeKiloBytes,
		SizeMegaBytes,
		SizeGigaBytes
	};

	SizeType typeFileSize = SizeBytes;

	SizeType StringToSizeType(const string& s);
	string SizeTypeToString(SizeType t);
	tstring SizeTypeToDisplayString(SizeType t);
	int64_t GetSizeBase();

	// Name of the destination directory (empty = 'ADLSearch') and its index
	//string destDir;
	unsigned long ddIndex = 0;

	bool isRegEx() const;
	void setRegEx(bool b);
	string getPattern();
	void setPattern(const string& aPattern);

	void setDestDir(const string& aDestDir) noexcept;
	const string& getDestDir() const noexcept { return name; }
private:
	string name;

	friend class ADLSearchManager;

	StringMatch match;

	/// Prepare search
	void prepare();

	/// Search for file match
	bool matchesFile(const string& f, const string& fp, int64_t size);
	/// Search for directory match
	bool matchesDirectory(const string& d);

	bool searchAll(const string& s);
};


class ADLSearchManager : public Singleton<ADLSearchManager>
{
public:
	// Destination directory indexing
	struct DestDir {
		DestDir(const string& aName, const DirectoryListing::Directory::Ptr& aDir) :
			name(aName), dir(aDir) { }

		const string name;
		DirectoryListing::Directory::Ptr dir = nullptr;
		DirectoryListing::Directory* subdir = nullptr;
		bool fileAdded = false;
	};
	using DestDirList = vector<DestDir>;

	ADLSearchManager();
	~ADLSearchManager();

	// Search collections
	using SearchCollection = vector<ADLSearch>;
	SearchCollection collection;


	// Load/save search collection to XML file
	void load() noexcept;
	void save(bool forced = false) noexcept;

	// Settings
	GETSET(bool, breakOnFirst, BreakOnFirst)
	GETSET(HintedUser, user, User)

	// @remarks Used to add ADLSearch directories to an existing DirectoryListing
	// Throws AbortException
	void matchListing(DirectoryListing& aList);
	bool addCollection(ADLSearch& search, int index) noexcept;
	bool removeCollection(int index) noexcept;
	bool changeState(int index, bool enabled) noexcept;
	bool updateCollection(ADLSearch& search, int index) noexcept;
	int8_t getRunning() const noexcept { return running; }

	static void log(const string& aMsg, LogMessage::Severity aSeverity) noexcept;
private:
	ADLSearch loadSearch(SimpleXML& xml);

	ADLSearch::SourceType StringToSourceType(const string& s);
	bool dirty = false;

	// @internal
	// Throws AbortException
	void matchRecurse(DestDirList& /*aDestList*/, const DirectoryListing::Directory::Ptr& /*aDir*/, const string& aAdcPath, DirectoryListing& /*aDirList*/);
	// Search for file match
	void MatchesFile(DestDirList& destDirVector, const DirectoryListing::File::Ptr& currentFile, const string& aAdcPath) noexcept;
	// Search for directory match
	void MatchesDirectory(DestDirList& destDirVector, const DirectoryListing::Directory::Ptr& currentDir, const string& aAdcPath) noexcept;
	// Step up directory
	void stepUpDirectory(DestDirList& destDirVector) noexcept;

	// Prepare destination directory indexing
	void PrepareDestinationDirectories(DestDirList& destDirVector, DirectoryListing::Directory::Ptr& root) noexcept;
	static void addRootDirectory(const string& aName, DestDirList& destDirs_, DirectoryListing::Directory::Ptr& root) noexcept;
	// Finalize destination directories
	void FinalizeDestinationDirectories(DestDirList& destDirVector, DirectoryListing::Directory::Ptr& root) noexcept;

	int8_t running = 0;
};

} // namespace dcpp

#endif // !defined(DCPLUSPLUS_DCPP_A_D_L_SEARCH_H)