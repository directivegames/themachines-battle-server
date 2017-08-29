// Copyright 2017 Directive Games Limited - All Rights Reserved

#include "TheMachinesBattleServer.h"
#include "LogClient.h"
#include <stdlib.h>

#include <iostream>

char* getCmdOption(char ** begin, char ** end, const std::string & option)
{
	char ** itr = std::find(begin, end, option);
	if (itr != end && ++itr != end)
	{
		return *itr;
	}
	return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option)
{
	return std::find(begin, end, option) != end;
}

int main(int argc, char * argv[])
{
	if (cmdOptionExists(argv, argv + argc, "-client"))
	{
		const auto serverIp = getCmdOption(argv, argv + argc, "-ip");
		const auto serverPort = atoi(getCmdOption(argv, argv + argc, "-port"));
		printf("Starting LogClient, connecting %s:%d...\n", serverIp, serverPort);
		LogClient logClient(serverIp, serverPort);
		logClient.Start();
	}
	else
	{
		const auto listenPort = getCmdOption(argv, argv + argc, "-port");
		TheMachinesBattleServer server(listenPort ? atoi(listenPort) : 0);
		server.Update();
	}

	return 0;
}
