#pragma once

#include "RakNet/RakNetTypes.h"

#include <cstdint>
#include <memory>
#include <chrono>
#include <sstream>

class Session;

class TheMachinesClient
{
private:
	RakNet::SystemAddress address;
	std::int32_t battleUID;
	std::int32_t lastReportedFrame;
	std::chrono::steady_clock::time_point lastFrameReportTimeStamp;
	std::int32_t catchupTargetFrame;
	std::weak_ptr<Session> session;

public:
	TheMachinesClient(const RakNet::SystemAddress& address);

	std::int32_t GetLastReportedFrame() const { return lastReportedFrame; }
	void SetLastReportedFrame(std::int32_t frame);

	std::int32_t GetCatchupTargetFrame() const { return catchupTargetFrame; }
	void SetCatchupTargetFrame(std::int32_t targetFrame) { catchupTargetFrame = targetFrame; }
	std::chrono::milliseconds TimeSinceLastFrameReported() const;

	std::int32_t GetBattleUID() const { return battleUID; }

	Session* GetSession() const;
	void AssignSession(std::shared_ptr<Session> newSession);

	RakNet::SystemAddress GetAddress() const { return address; }

	std::ostringstream& Write(std::ostringstream& oss) const;

};

inline std::ostream& operator <<(std::ostringstream& oss, const TheMachinesClient& client) { return client.Write(oss); }
