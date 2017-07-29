// Copyright 2017 Directive Games Limited - All Rights Reserved

#include <stdio.h>
#include <string.h>
#include <iostream>
#include "RakNet/RakPeerInterface.h"
#include "RakNet/MessageIdentifiers.h"
#include "RakNet/BitStream.h"
#include "RakNet/RakNetTypes.h"  // MessageID
#include "BattleSession.h"

#include <vector>
#include <map>
#include <memory>
#include <algorithm>

#include "TheMachinesBattleServer.h"

int main2()
{
	printf("============================================\n");
	printf("The Machines(TM) battle server has started at port %d\n", 12345);
	printf("Session capacity: %d\n", 2);
	printf("Max connections: %d\n", 2);
	printf("============================================\n\n");

	TheMachinesBattleServer server;

	server.Update();

	return 0;
}
