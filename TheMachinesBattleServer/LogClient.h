#pragma once

#include "RakNet/RakNetTypes.h"

class LogClient
{
public:
	LogClient(const char* ipStr, unsigned short port);
	void Start();

private:
	RakNet::RakPeerInterface* peer;
};
