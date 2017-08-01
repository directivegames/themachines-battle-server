#pragma once
#include <map>
#include <vector>
#include <cstdint>
#include <memory>
#include <sstream>

class Session;
class TheMachinesClient;
class ClientManager;

class SessionManager
{
private:
	ClientManager& clientManager;	// damn this is ugly


	std::map<std::int32_t, std::shared_ptr<Session>> sessions;

	std::vector<std::shared_ptr<Session>> guestSessions;	// sessions for those who don't have a valid session ID
	std::shared_ptr<Session> freeGuestSession;


	static const int MAX_CLIENTS_PER_GUEST_SESSION = 2;

public:
	SessionManager(ClientManager& _clientManager);

	void AddToSession(TheMachinesClient& client);
	void RemoveFromSession(TheMachinesClient& client);
	std::ostringstream& Write(std::ostringstream& oss) const;

private:
	void AddToSession(TheMachinesClient& client, std::shared_ptr<Session> session);
};

inline std::ostringstream& operator<<(std::ostringstream& oss, const SessionManager& sessionManager)
{
	return sessionManager.Write(oss);
}
