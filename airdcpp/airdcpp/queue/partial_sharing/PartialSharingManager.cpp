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

#include <airdcpp/queue/partial_sharing/PartialSharingManager.h>

#include <airdcpp/user/HintedUser.h>
#include <airdcpp/queue/QueueManager.h>
#include <airdcpp/search/SearchResult.h>
#include <airdcpp/share/ShareManager.h>
#include <airdcpp/core/timer/TimerManager.h>
#include <airdcpp/transfer/upload/Upload.h>
#include <airdcpp/transfer/upload/UploadFileParser.h>
#include <airdcpp/transfer/upload/UploadManager.h>
#include <airdcpp/connection/UserConnection.h>
#include <airdcpp/util/Util.h>
#include <airdcpp/util/ValueGenerator.h>

namespace dcpp {

PartialSharingManager::PartialSharingManager() {
	ShareManager::getInstance()->registerUploadFileProvider(this);

	UploadManager::getInstance()->slotTypeHook.addSubscriber(ActionHookSubscriber(providerName, "Partial sharing", nullptr), HOOK_CALLBACK(PartialSharingManager::onSlotType));
}

ActionHookResult<OptionalTransferSlot> PartialSharingManager::onSlotType(const UserConnection& aUserConnection, const ParsedUpload& aUpload, const ActionHookResultGetter<OptionalTransferSlot>& aResultGetter) const noexcept {
	if (aUpload.provider == providerName) {
		auto partialFree = aUserConnection.hasSlotSource(providerName) || (extraPartial < SETTING(EXTRA_PARTIAL_SLOTS));
		if (partialFree) {
			dcdebug("PartialSharingManager::onSlotType: assign partial slot for %s\n", aUserConnection.getConnectToken().c_str());
			return aResultGetter.getData(TransferSlot(TransferSlot::Type::FILESLOT, providerName));
		}
	}

	return aResultGetter.getData(nullopt);
}

void PartialSharingManager::on(UploadManagerListener::Created, Upload* aUpload, const TransferSlot& aNewSlot) noexcept {
	// Previous slot
	if (aUpload->getUserConnection().hasSlotSource(providerName)) {
		dcassert(extraPartial > 0);
		extraPartial--;
	}

	// New slot
	if (aNewSlot.source == providerName) {
		dcassert(extraPartial >= 0);
		extraPartial++;
	}
}

void PartialSharingManager::on(UploadManagerListener::Failed, const Upload* aUpload, const string&) noexcept {
	if (!aUpload->getUserConnection().hasSlotSource(providerName)) {
		return;
	}

	dcassert(extraPartial > 0);
	extraPartial--;
}

bool PartialSharingManager::toRealWithSize(const UploadFileQuery& aQuery, string& path_, int64_t& size_, bool&) const noexcept {
	if (!SETTING(USE_PARTIAL_SHARING)) {
		return false;
	}

	if (QueueManager::getInstance()->isChunkDownloaded(aQuery.tth, aQuery.segment, size_, path_)) {
		return true;
	}

	return false;
}

void PartialSharingManager::getRealPaths(const TTHValue& aTTH, StringList& paths_) const noexcept {
	auto qiList = QueueManager::getInstance()->findFiles(aTTH);
	for (const auto& qi: qiList) {
		if (qi->isDownloaded()) {
			paths_.push_back(qi->getTarget());
		}
	}
}

void PartialSharingManager::getBloom(ProfileToken, HashBloom& bloom_) const noexcept {
	if (!SETTING(USE_PARTIAL_SHARING)) {
		return;
	}

	for (const auto qi : getBloomFiles()) {
		bloom_.add(qi->getTTH());
	}
}

void PartialSharingManager::getBloomFileCount(ProfileToken, size_t& fileCount_) const noexcept {
	if (!SETTING(USE_PARTIAL_SHARING)) {
		return;
	}

	fileCount_ += getBloomFiles().size();
}

QueueItemList PartialSharingManager::getBloomFiles() const noexcept {
	QueueItemList bloomFiles;

	{
		RLock l(QueueManager::getInstance()->getCS());
		for (const auto qi : QueueManager::getInstance()->getFileQueueUnsafe() | views::values) {
			if (qi->getBundle()) {
				bloomFiles.push_back(qi);
			}
		}
	}

	return bloomFiles;
}

}