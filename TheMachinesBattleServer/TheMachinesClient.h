#pragma once

#include "RakNet/RakNetTypes.h"

#include <cstdint>
#include <memory>

class Session;

class TheMachinesClient
{
private:
	RakNet::SystemAddress address;
	std::int32_t battleUID;
	std::int32_t lastReportedFrame;
	std::shared_ptr<Session> session;

public:
	TheMachinesClient(const RakNet::SystemAddress& address);
	std::int32_t GetLastReportedFrame() const { return lastReportedFrame; }
	void SetLastReportedFrame(std::int32_t frame) { lastReportedFrame = frame; }
	std::int32_t GetBattleUID() const { return battleUID; }
	Session* GetSession() const;
	void AssignSession(std::shared_ptr<Session> newSession);
};
