#include "DataModel/PlayersStorage.h"

#include <cassert>

#if defined(_WIN32) || defined(_WIN64)
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

void RunSimulation();

int main()
{
	RunSimulation();
#if defined(_WIN32) || defined(_WIN64)
	_CrtDumpMemoryLeaks();
#endif
}

void RunSimulation()
{
	// Test that basic registering/unregistering ratings works as expected
	{
		ps::PlayersStorage storage;
		std::string nickname1 = "Baron_Von_Awesome";
		std::string nickname2 = "Mr.Wonderful";
		assert(storage.GetPlayerRating(nickname1) == -1);
		assert(storage.GetPlayerRating(nickname2) == -1);
		storage.RegisterPlayerResult(nickname1, 1000);
		storage.RegisterPlayerResult(nickname2, 2000);
		assert(storage.GetPlayerRating(nickname1) == 1000);
		assert(storage.GetPlayerRating(nickname2) == 2000);
		storage.UnregisterPlayer(nickname1);
		assert(storage.GetPlayerRating(nickname1) == -1);
		assert(storage.GetPlayerRating(nickname2) == 2000);
	}
}
