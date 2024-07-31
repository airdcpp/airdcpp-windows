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

#include "stdinc.h"
#include "ShareTasks.h"

#include "Exception.h"
#include "LogManager.h"
#include "HashManager.h"
#include "ResourceManager.h"
#include "ScopedFunctor.h"

#include "concurrency.h"

namespace dcpp {

#ifdef ATOMIC_FLAG_INIT
atomic_flag ShareTasks::tasksRunning = ATOMIC_FLAG_INIT;
#else
atomic_flag ShareTasks::tasksRunning;
#endif


void ShareTasks::log(const string& aMsg, LogMessage::Severity aSeverity) noexcept {
	LogManager::getInstance()->message(aMsg, aSeverity, STRING(SHARE));
}

ShareTasks::ShareTasks(ShareTasksManager* const aManager) : manager(aManager) {

}

ShareTasks::~ShareTasks() {
}

ShareRefreshInfo::ShareRefreshInfo::~ShareRefreshInfo() {

}

ShareRefreshInfo::ShareRefreshInfo(const string& aPath, const ShareDirectory::Ptr& aOldShareDirectory, time_t aLastWrite, ShareBloom& bloom_) :
	path(aPath), oldShareDirectory(aOldShareDirectory), bloom(bloom_) {

	// Use a different directory for building the tree
	if (aOldShareDirectory && aOldShareDirectory->isRoot()) {
		newShareDirectory = ShareDirectory::createRoot(
			aPath, aOldShareDirectory->getVirtualName(), aOldShareDirectory->getRoot()->getRootProfiles(), aOldShareDirectory->getRoot()->getIncoming(),
			aLastWrite, rootPathsNew, lowerDirNameMapNew, bloom_, aOldShareDirectory->getRoot()->getLastRefreshTime()
		);
	} else {
		// We'll set the parent later
		newShareDirectory = ShareDirectory::createNormal(Util::getLastDir(aPath), nullptr, aLastWrite, lowerDirNameMapNew, bloom_);
	}
}


bool ShareRefreshInfo::checkContent(const ShareDirectory::Ptr& aDirectory) noexcept {
	if (SETTING(SKIP_EMPTY_DIRS_SHARE) && aDirectory->getDirectories().empty() && aDirectory->getFiles().empty()) {
		// Remove from parent
		ShareDirectory::cleanIndices(*aDirectory.get(), stats.addedSize, tthIndexNew, lowerDirNameMapNew);
		return false;
	}

	return true;
}


void ShareRefreshInfo::applyRefreshChanges(ShareDirectory::MultiMap& lowerDirNameMap_, ShareDirectory::Map& rootPaths_, ShareDirectory::File::TTHMap& tthIndex_, int64_t& sharedBytes_, ProfileTokenSet* dirtyProfiles_) noexcept {
#ifdef _DEBUG
	for (const auto& d : lowerDirNameMapNew | views::values) {
		ShareDirectory::checkAddedDirNameDebug(d, lowerDirNameMap_);
	}

	for (const auto& f : tthIndexNew | views::values) {
		ShareDirectory::File::checkAddedTTHDebug(f, tthIndex_);
	}
#endif

	lowerDirNameMap_.insert(lowerDirNameMapNew.begin(), lowerDirNameMapNew.end());
	tthIndex_.insert(tthIndexNew.begin(), tthIndexNew.end());

	for (const auto& rp : rootPathsNew) {
		//dcassert(rootPaths_.find(rp.first) == rootPaths_.end());
		rootPaths_[rp.first] = rp.second;
	}

	sharedBytes_ += stats.addedSize;

	if (dirtyProfiles_) {
		newShareDirectory->copyRootProfiles(*dirtyProfiles_, true);
	}

	// Save some memory
	lowerDirNameMapNew.clear();
	tthIndexNew.clear();
	oldShareDirectory = nullptr;
	newShareDirectory = nullptr;
}

void ShareRefreshStats::merge(const ShareRefreshStats& aOther) noexcept {
	hashSize += aOther.hashSize;
	addedSize += aOther.addedSize;

	newDirectoryCount += aOther.newDirectoryCount;
	newFileCount += aOther.newFileCount;

	skippedFileCount += aOther.skippedFileCount;
	skippedDirectoryCount += aOther.skippedDirectoryCount;

	existingFileCount += aOther.existingFileCount;
	existingDirectoryCount += aOther.existingDirectoryCount;
}

void ShareTasks::shutdown() noexcept {
	join();
}

ShareRefreshTask::ShareRefreshTask(ShareRefreshTaskToken aToken, const RefreshPathList& aDirs, const string& aDisplayName, ShareRefreshType aRefreshType, ShareRefreshPriority aPriority) :
	token(aToken), dirs(aDirs), displayName(aDisplayName), type(aRefreshType), priority(aPriority) { }

void ShareTasks::validateRefreshTask(StringList& dirs_) noexcept {
	Lock l(tasks.cs);
	const auto& tq = tasks.getTasks();

	// Remove the exact directories that have already been queued for refreshing
	for (const auto& i : tq) {
		auto t = static_cast<ShareRefreshTask*>(i.second.get());
		if (!t->canceled) {
			auto [first, last] = ranges::remove_if(dirs_, [t](const string& p) {
				return ranges::find(t->dirs, p) != t->dirs.end();
			});
			dirs_.erase(first, last);
		}
	}
}

void ShareTasks::reportPendingRefresh(ShareRefreshType aType, const RefreshPathList& aDirectories, const string& aDisplayName) const noexcept {
	string msg;
	switch (aType) {
		case(ShareRefreshType::REFRESH_ALL) :
			msg = STRING(REFRESH_QUEUED);
			break;
		case(ShareRefreshType::REFRESH_DIRS) :
			if (!aDisplayName.empty()) {
				msg = STRING_F(VIRTUAL_REFRESH_QUEUED, aDisplayName);
			} else if (aDirectories.size() == 1) {
				msg = STRING_F(DIRECTORY_REFRESH_QUEUED, *aDirectories.begin());
			}
			break;
		case(ShareRefreshType::ADD_DIR) :
			if (aDirectories.size() == 1) {
				msg = STRING_F(ADD_DIRECTORY_QUEUED, *aDirectories.begin());
			} else {
				msg = STRING_F(ADD_DIRECTORIES_QUEUED, aDirectories.size());
			}
			break;
		case(ShareRefreshType::REFRESH_INCOMING) :
			msg = STRING(INCOMING_REFRESH_QUEUED);
			break;
		default:
			break;
	};

	if (!msg.empty()) {
		log(msg, LogMessage::SEV_INFO);
	}
}

RefreshTaskQueueInfo ShareTasks::addRefreshTask(ShareRefreshPriority aPriority, const StringList& aDirs, ShareRefreshType aRefreshType, const string& aDisplayName, function<void(float)> aProgressF) noexcept {
	auto dirs = aDirs;
	validateRefreshTask(dirs);

	if (dirs.empty()) {
		return {
			nullopt,
			RefreshTaskQueueResult::EXISTS
		};
	}

	auto token = Util::rand();
	RefreshPathList paths(dirs.begin(), dirs.end());

	auto task = make_unique<ShareRefreshTask>(token, paths, aDisplayName, aRefreshType, aPriority);

	// Manager
	manager->onRefreshQueued(*task.get());

	tasks.add(RefreshTaskType::REFRESH, std::move(task));

	if (tasksRunning.test_and_set()) {
		if (aRefreshType != ShareRefreshType::STARTUP) {
			// This is always called from the task thread...
			reportPendingRefresh(aRefreshType, paths, aDisplayName);
		}

		return {
			token,
			RefreshTaskQueueResult::QUEUED
		};
	}

	if (aPriority == ShareRefreshPriority::BLOCKING) {
		runTasks(aProgressF);
	} else {
		try {
			start();
		} catch(const ThreadException& e) {
			log(STRING(FILE_LIST_REFRESH_FAILED) + " " + e.getError(), LogMessage::SEV_WARNING);
			tasksRunning.clear();
		}
	}

	return {
		token,
		RefreshTaskQueueResult::STARTED
	};
}

void ShareTasks::reportTaskStatus(const ShareRefreshTask& aTask, bool aFinished, const ShareRefreshStats* aStats) const noexcept {
	string msg;
	switch (aTask.type) {
		case (ShareRefreshType::STARTUP):
		case (ShareRefreshType::REFRESH_ALL):
			msg = aFinished ? STRING(FILE_LIST_REFRESH_FINISHED) : STRING(FILE_LIST_REFRESH_INITIATED);
			break;
		case (ShareRefreshType::REFRESH_DIRS):
			if (!aTask.displayName.empty()) {
				msg = aFinished ? STRING_F(VIRTUAL_DIRECTORY_REFRESHED, aTask.displayName) : STRING_F(FILE_LIST_REFRESH_INITIATED_VPATH, aTask.displayName);
			} else if (aTask.dirs.size() == 1) {
				msg = aFinished ? STRING_F(DIRECTORY_REFRESHED, *aTask.dirs.begin()) : STRING_F(FILE_LIST_REFRESH_INITIATED_RPATH, *aTask.dirs.begin());
			} else {
				msg = aFinished ? STRING_F(X_DIRECTORIES_REFRESHED, aTask.dirs.size()) : STRING_F(FILE_LIST_REFRESH_INITIATED_X_PATHS, aTask.dirs.size());
				if (aTask.dirs.size() < 30) {
					StringList dirNames;
					for (const auto& d : aTask.dirs) {
						dirNames.push_back(Util::getLastDir(d));
					}

					msg += " (" + Util::listToString(dirNames) + ")";
				}
			}
			break;
		case(ShareRefreshType::ADD_DIR):
			if (aTask.dirs.size() == 1) {
				msg = aFinished ? STRING_F(DIRECTORY_ADDED, *aTask.dirs.begin()) : STRING_F(ADDING_SHARED_DIR, *aTask.dirs.begin());
			} else {
				msg = aFinished ? STRING_F(ADDING_X_SHARED_DIRS, aTask.dirs.size()) : STRING_F(DIRECTORIES_ADDED, aTask.dirs.size());
			}
			break;
		case(ShareRefreshType::REFRESH_INCOMING):
			msg = aFinished ? STRING(INCOMING_REFRESHED) : STRING(FILE_LIST_REFRESH_INITIATED_INCOMING);
			break;
		case(ShareRefreshType::BUNDLE):
			if (aFinished)
				msg = STRING_F(BUNDLE_X_SHARED, aTask.displayName); //show the whole path so that it can be opened from the system log
			break;
	};

	if (!msg.empty()) {
		if (aStats && aStats->hashSize > 0) {
			msg += " " + STRING_F(FILES_ADDED_FOR_HASH, Util::formatBytes(aStats->hashSize));
		} else if (aTask.priority == ShareRefreshPriority::SCHEDULED && !SETTING(LOG_SCHEDULED_REFRESHES)) {
			return;
		}

		log(msg, LogMessage::SEV_INFO);
	}
}

int ShareTasks::run() {
	runTasks();
	return 0;
}

void ShareTasks::runTasks(function<void (float)> progressF /*nullptr*/) noexcept {
	unique_ptr<HashManager::HashPauser> pauser = nullptr;
	ScopedFunctor([this] { tasksRunning.clear(); });

	for (;;) {
		TaskQueue::TaskPair t;
		if (!tasks.getFront(t)) {
			break;
		}

		ScopedFunctor([this] { tasks.pop_front(); });

		if (t.first == RefreshTaskType::REFRESH) {
			auto task = static_cast<ShareRefreshTask*>(t.second);
			if (task->type == ShareRefreshType::STARTUP && task->priority != ShareRefreshPriority::BLOCKING) {
				Thread::sleep(5000); // let the client start first
			}

			task->running = true;

			setThreadPriority(task->priority == ShareRefreshPriority::MANUAL ? Thread::NORMAL : Thread::IDLE);
			if (!pauser) {
				pauser.reset(new HashManager::HashPauser());
			}

			runRefreshTask(*task, progressF);
		}

	}
}

void ShareTasks::runRefreshTask(const ShareRefreshTask& aTask, function<void(float)> progressF) noexcept {

	refreshRunning = true;
	ScopedFunctor([this] { refreshRunning = false; });

	auto refreshPaths = aTask.dirs;
	if (refreshPaths.empty()) {
		return;
	}

	auto taskHandler = manager->startRefresh(aTask);

	reportTaskStatus(aTask, false, nullptr);

	// Refresh
	atomic<long> progressCounter(0);

	ShareRefreshStats totalStats;
	bool allBuildersSucceed = true;

	auto doRefresh = [&](const string& aRefreshPath) {
		if (aTask.canceled || !taskHandler->refreshPath(aRefreshPath, aTask, totalStats)) {
			allBuildersSucceed = false;
		}

		if (progressF) {
			progressF(static_cast<float>(progressCounter++) / static_cast<float>(refreshPaths.size()));
		}
	};

	try {
		if (SETTING(REFRESH_THREADING) == SettingsManager::MULTITHREAD_ALWAYS || (SETTING(REFRESH_THREADING) == SettingsManager::MULTITHREAD_MANUAL && aTask.priority == ShareRefreshPriority::MANUAL)) {
			TaskScheduler s;
			parallel_for_each(refreshPaths.begin(), refreshPaths.end(), doRefresh);
		} else {
			ranges::for_each(refreshPaths, doRefresh);
		}
	} catch (const std::exception& e) {
		log(STRING(FILE_LIST_REFRESH_FAILED) + string(e.what()), LogMessage::SEV_ERROR);
		return;
	}

	if (allBuildersSucceed) {
		reportTaskStatus(aTask, true, &totalStats);
	}

	taskHandler->refreshCompleted(allBuildersSucceed, aTask, totalStats);
}

ShareRefreshTaskList ShareTasks::getRefreshTasks() const noexcept {
	ShareRefreshTaskList ret;

	{
		Lock l(tasks.cs);
		for (const auto& t : tasks.getTasks()) {
			if (t.first == RefreshTaskType::REFRESH) {
				auto refreshTask = static_cast<ShareRefreshTask*>(t.second.get());
				ret.push_back(*refreshTask);
			}
		}
	}

	return ret;
}

RefreshPathList ShareTasks::abortRefresh(optional<ShareRefreshTaskToken> aToken) noexcept {
	RefreshPathList paths;

	{
		Lock l(tasks.cs);

		auto& tl = tasks.getTasks();

		for (const auto& t : tl) {
			if (t.first == RefreshTaskType::REFRESH) {
				auto refreshTask = static_cast<ShareRefreshTask*>(t.second.get());
				if (!aToken || refreshTask->token == *aToken) {
					refreshTask->canceled = true;
					ranges::copy(refreshTask->dirs, inserter(paths, paths.begin()));
				}
			}
		}
	}

	return paths;
}
} // namespace dcpp
