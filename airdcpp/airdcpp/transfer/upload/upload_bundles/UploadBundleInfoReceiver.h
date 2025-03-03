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

#ifndef DCPLUSPLUS_DCPP_UPLOAD_BUNDLE_RECEIVER_H
#define DCPLUSPLUS_DCPP_UPLOAD_BUNDLE_RECEIVER_H

#include <airdcpp/forward.h>

#include <airdcpp/protocol/AdcCommand.h>
#include <airdcpp/core/thread/CriticalSection.h>
#include <airdcpp/protocol/ProtocolCommandManager.h>
#include <airdcpp/user/HintedUser.h>
#include <airdcpp/message/Message.h>
#include <airdcpp/core/Speaker.h>
#include <airdcpp/core/timer/TimerManagerListener.h>
#include <airdcpp/transfer/upload/upload_bundles/UploadBundle.h>
#include <airdcpp/transfer/upload/upload_bundles/UploadBundleInfoReceiverListener.h>
#include <airdcpp/transfer/upload/UploadManagerListener.h>

namespace dcpp {

class UploadBundleInfoReceiver : public Speaker<UploadBundleInfoReceiverListener>, private TimerManagerListener, private UploadManagerListener, private ProtocolCommandManagerListener
{
public:
	void onUBD(const AdcCommand& cmd);
	void onUBN(const AdcCommand& cmd);

	UploadBundlePtr findByBundleToken(const string& aBundleToken) const noexcept;
	UploadBundlePtr findByConnectionToken(const string& aConnectionToken) const noexcept;

	size_t getRunningBundleCount() const noexcept;

	UploadBundleInfoReceiver() noexcept;
	~UploadBundleInfoReceiver() override;
private:
	mutable SharedMutex cs;
	void dbgMsg(const string& aMsg, LogMessage::Severity aSeverity) noexcept;
	static string formatDebugBundle(const UploadBundlePtr& u) noexcept;

	static double parseSpeed(const string& aSpeedStr) noexcept;

	void createBundle(const AdcCommand& cmd);
	void changeBundle(const AdcCommand& cmd);
	void updateBundleInfo(const AdcCommand& cmd);
	void finishBundle(const AdcCommand& cmd);
	void removeBundleConnection(const AdcCommand& cmd);

	friend class Singleton<UploadBundleInfoReceiver>;

	void handleAddBundleConnection(const string& aConnectionToken, const UploadBundlePtr& aBundle) noexcept;
	void handleRemoveBundleConnection(const string& aConnectionToken, const UploadBundlePtr& aBundle) noexcept;

	void addBundleConnectionUnsafe(const Upload* aUpload, const UploadBundlePtr& aBundle) noexcept;
	void removeBundleConnectionUnsafe(const Upload* aUpload, const UploadBundlePtr& aBundle) noexcept;

	void removeIdleBundles() noexcept;

	using UploadCallback = std::function<void (Upload *)> &&;
	bool callAsync(const string& aToken, UploadCallback&& aCallback) const noexcept;

	// Listeners
	void on(TimerManagerListener::Second, uint64_t aTick) noexcept override;

	void on(UploadManagerListener::Created, Upload*, const TransferSlot&) noexcept override;
	void on(UploadManagerListener::Removed, const Upload*) noexcept override;

	void on(ProtocolCommandManagerListener::IncomingUDPCommand, const AdcCommand&, const string&) noexcept override;

	unordered_map<string, UploadBundlePtr> connections;

	using RemoteBundleTokenMap = unordered_map<string, UploadBundlePtr>;
	RemoteBundleTokenMap bundles;
};

} // namespace dcpp

#endif // !defined(UPLOAD_MANAGER_H)
