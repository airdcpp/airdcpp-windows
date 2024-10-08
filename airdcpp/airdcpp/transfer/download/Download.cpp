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
#include <airdcpp/transfer/download/Download.h>

#include <airdcpp/queue/Bundle.h>
#include <airdcpp/core/io/File.h>
#include <airdcpp/core/io/stream/FilteredFile.h>
#include <airdcpp/hash/HashManager.h>
#include <airdcpp/hash/value/MerkleCheckOutputStream.h>
#include <airdcpp/hash/value/MerkleTreeOutputStream.h>
#include <airdcpp/util/PathUtil.h>
#include <airdcpp/queue/QueueItem.h>
#include <airdcpp/settings/SettingsManager.h>
#include <airdcpp/core/io/stream/SharedFileStream.h>
#include <airdcpp/core/io/stream/Streams.h>
#include <airdcpp/connection/UserConnection.h>
#include <airdcpp/core/io/compress/ZUtils.h>

namespace dcpp {

Download::Download(UserConnection& conn, QueueItem& qi) noexcept : Transfer(conn, qi.getTarget(), qi.getTTH()),
	tempTarget(qi.getTempTarget()), listDirectoryPath(qi.isFilelist() ? qi.getListDirectoryPath() : Util::emptyString)
{
	dcassert(!conn.getDownload());
	conn.setDownload(this);
	
	// Type
	if(qi.isSet(QueueItem::FLAG_PARTIAL_LIST)) {
		setType(TYPE_PARTIAL_LIST);
	} else if(qi.isSet(QueueItem::FLAG_USER_LIST)) {
		setType(TYPE_FULL_LIST);
	}

	// Base flags
	initFlags(qi);

	// Bundle
	if (qi.getBundle()) {
		dcassert(!qi.isSet(QueueItem::FLAG_USER_LIST) && !qi.isSet(QueueItem::FLAG_CLIENT_VIEW));
		setBundle(qi.getBundle());
	}
	
	// File
	if(getType() == TYPE_FILE && qi.getSize() != -1) {
		initSegment(qi);
		initOverlapped(qi);
	}
}

Download::~Download() {
	dcassert(getUserConnection().getDownload() == this);
	// dcdebug("Deleting download %s\n", getToken().c_str());
	getUserConnection().setDownload(nullptr);
}

void Download::initSegment(QueueItem& qi) noexcept {
	const auto& conn = getUserConnection();
	auto source = qi.getSource(getUser());
	if (HashManager::getInstance()->getTree(getTTH(), tt)) {
		setTreeValid(true);
		setSegment(qi.getNextSegment(getTigerTree().getBlockSize(), conn.getChunkSize(), conn.getSpeed(), source->getPartsInfo(), true));
		qi.setBlockSize(getTigerTree().getBlockSize());
	} else if(conn.isSet(UserConnection::FLAG_SUPPORTS_TTHL) && !source->isSet(QueueItem::Source::FLAG_NO_TREE) && qi.getSize() > HashManager::getMinBlockSize()) {
		// Get the tree unless the file is small (for small files, we'd probably only get the root anyway)
		setType(TYPE_TREE);
		getTigerTree().setFileSize(qi.getSize());
		setSegment(Segment(0, -1));
	} else {
		// Use the root as tree to get some sort of validation at least...
		getTigerTree() = TigerTree(qi.getSize(), qi.getSize(), getTTH());
		setTreeValid(true);
		setSegment(qi.getNextSegment(getTigerTree().getBlockSize(), 0, 0, source->getPartsInfo(), true));
	}
		
	if ((getStartPos() + getSegmentSize()) != qi.getSize() || (conn.getDownload() && conn.getDownload()->isSet(FLAG_CHUNKED))) {
		setFlag(FLAG_CHUNKED);
	}
}

void Download::initOverlapped(const QueueItem& qi) noexcept {
	if (!getSegment().getOverlapped()) {
		return;
	}

	setFlag(FLAG_OVERLAP);

	// set overlapped flag to original segment
	for (auto d : qi.getDownloads()) {
		if (d->getSegment().contains(getSegment())) {
			d->setOverlapped(true);
			break;
		}
	}
}

void Download::initFlags(const QueueItem& qi) noexcept {
	// Source flags
	if (auto source = qi.getSource(getUser()); source->isSet(QueueItem::Source::FLAG_PARTIAL))
		setFlag(FLAG_PARTIAL);

	// Other
	if (qi.isSet(QueueItem::FLAG_CLIENT_VIEW))
		setFlag(FLAG_VIEW);
	if (qi.isSet(QueueItem::FLAG_MATCH_QUEUE))
		setFlag(FLAG_QUEUE);
	if (qi.isSet(QueueItem::FLAG_RECURSIVE_LIST))
		setFlag(FLAG_RECURSIVE);
	if (qi.isSet(QueueItem::FLAG_TTHLIST_BUNDLE))
		setFlag(FLAG_TTHLIST_BUNDLE);
	if (qi.getPriority() == Priority::HIGHEST)
		setFlag(FLAG_HIGHEST_PRIO);
}

string Download::getBundleStringToken() const noexcept {
	if (!bundle)
		return Util::emptyString;

	return bundle->getStringToken();
}

void Download::flush() const noexcept {
	if (!getOutput()) {
		return;
	}

	if (getActual() > 0) {
		try {
			getOutput()->flushBuffers(false);
		} catch (const Exception&) {
			// ...
		}
	}
}

void Download::appendFlags(OrderedStringSet& flags_) const noexcept {
	if (isSet(Download::FLAG_PARTIAL)) {
		flags_.emplace("P");
	}

	if (isSet(Download::FLAG_TTH_CHECK)) {
		flags_.emplace("T");
	}
	if (isSet(Download::FLAG_ZDOWNLOAD)) {
		flags_.emplace("Z");
	}
	if (isSet(Download::FLAG_CHUNKED)) {
		flags_.emplace("C");
	}

	Transfer::appendFlags(flags_);
}

AdcCommand Download::getCommand(bool zlib, const string& mySID) const noexcept {
	AdcCommand cmd(AdcCommand::CMD_GET);
	
	cmd.addParam(Transfer::names[getType()]);

	if(getType() == TYPE_PARTIAL_LIST || getType() == TYPE_TTH_LIST) {
		cmd.addParam(getListDirectoryPath());
	} else if(getType() == TYPE_FULL_LIST) {
		if(isSet(Download::FLAG_XML_BZ_LIST)) {
			cmd.addParam(USER_LIST_NAME_BZ);
		} else {
			cmd.addParam(USER_LIST_NAME_EXTRACTED);
		}
	} else {
		cmd.addParam("TTH/" + getTTH().toBase32());
	}

	cmd.addParam(Util::toString(getStartPos()));
	cmd.addParam(Util::toString(getSegmentSize()));

	if(!mySID.empty()) //add requester's SID (mySID) to the filelist request, so he can find the hub we are calling from.
		cmd.addParam("ID", mySID); 

	if(zlib && SETTING(COMPRESS_TRANSFERS)) {
		cmd.addParam("ZL1");
	}

	if(isSet(Download::FLAG_RECURSIVE) && getType() == TYPE_PARTIAL_LIST) {
		cmd.addParam("RE1");
	}
	
	if(isSet(Download::FLAG_QUEUE) && getType() == TYPE_PARTIAL_LIST) {	 
		cmd.addParam("TL1");
	}

	return cmd;
}

void Download::getParams(const UserConnection& aSource, ParamMap& params) const noexcept {
	Transfer::getParams(aSource, params);
	params["target"] = (getType() == TYPE_PARTIAL_LIST ? STRING(PARTIAL_FILELIST) + " (" + getListDirectoryPath() + ")" : getPath());
}

string Download::getTargetFileName() const noexcept {
	return PathUtil::getFileName(getPath());
}

const string& Download::getDownloadTarget() const noexcept {
	return (getTempTarget().empty() ? getPath() : getTempTarget());
}

void Download::disconnectOverlappedThrow() {
	if (!getOverlapped() || !bundle) {
		return;
	}

	setOverlapped(false);

	// ok, we got a fast slot, so it's possible to disconnect original user now
	bool found = false;
	for (auto d: bundle->getDownloads()) {
		if (d == this || compare(d->getPath(), getPath()) != 0 || !d->getSegment().contains(getSegment())) {
			continue;
		}

		// overlapping has no sense if segment is going to finish
		if (d->getSecondsLeft() >= 10) {
			found = true;

			// disconnect slow chunk
			d->getUserConnection().disconnect();
		}

		break;
	}

	if (!found) {
		// slow chunk already finished ???
		throw Exception(STRING(DOWNLOAD_FINISHED_IDLE));
	}
}

void Download::open(int64_t bytes, bool z, bool aHasDownloadedBytes) {
	if(getType() == Transfer::TYPE_FILE) {

		disconnectOverlappedThrow();

		auto target = getDownloadTarget();
		auto fullSize = tt.getFileSize();
		if (aHasDownloadedBytes) {
			if(File::getSize(target) != fullSize) {
				// When trying the download the next time, the resume pos will be reset
				throw Exception(CSTRING(TARGET_FILE_MISSING));
			}
		} else {
			File::ensureDirectory(target);
		}

		int fileFlags = File::OPEN | File::CREATE | File::SHARED_WRITE;
		if (getSegment().getEnd() != fullSize) {
			// Segmented download, let Windows decide the buffering
			fileFlags |= File::BUFFER_AUTO;
		}

		auto f = make_unique<SharedFileStream>(target, File::WRITE, fileFlags);

		if(f->getSize() != fullSize) {
			f->setSize(fullSize);
		}

		f->setPos(getSegment().getStart());

		output = std::move(f);
		tempTarget = target;
	} else if(getType() == Transfer::TYPE_FULL_LIST) {
		auto target = getPath();
		File::ensureDirectory(target);

		if(isSet(Download::FLAG_XML_BZ_LIST)) {
			target += ".xml.bz2";
		} else {
			target += ".xml";
		}

		output.reset(new File(target, File::WRITE, File::OPEN | File::TRUNCATE | File::CREATE));
		tempTarget = target;
	} else if(getType() == Transfer::TYPE_PARTIAL_LIST) {
		output.reset(new StringOutputStream(pfs));
	} else if(getType() == Transfer::TYPE_TREE) {
		output.reset(new MerkleTreeOutputStream<TigerTree>(tt));
	}

	if((getType() == Transfer::TYPE_FILE || getType() == Transfer::TYPE_FULL_LIST) && SETTING(BUFFER_SIZE) > 0 ) {
		output.reset(new BufferedOutputStream<true>(output.release()));
	}

	if (getType() == Transfer::TYPE_FILE) {
		using MerkleStream = MerkleCheckOutputStream<TigerTree, true>;

		output.reset(new MerkleStream(tt, output.release(), getStartPos()));
		setFlag(Download::FLAG_TTH_CHECK);
	}

	// Check that we don't get too many bytes
	output.reset(new LimitedOutputStream<true>(output.release(), bytes));

	if(z) {
		setFlag(Download::FLAG_ZDOWNLOAD);
		output.reset(new FilteredOutputStream<UnZFilter, true>(output.release()));
	}
}

void Download::close()
{
	output.reset();
}

} // namespace dcpp
