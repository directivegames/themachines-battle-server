#pragma once
#include <map>
#include <list>
#include <cstdint>
#include <memory>

class Session;
class TheMachinesClient;

class SessionManager
{
private:
	std::map<std::int32_t, std::weak_ptr<Session>> sessions;

	std::list<std::weak_ptr<Session>> guestSessions;	// sessions for those who don't have a valid session ID
	std::shared_ptr<Session> freeGuestSession;

	static const int MAX_CLIENTS_PER_GUEST_SESSION = 2;

public:
	SessionManager() {};
	void AssignSession(TheMachinesClient& client);
};
