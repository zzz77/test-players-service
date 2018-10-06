#include "DataModel/PlayersStorage.h"

#include <cassert>
#include <string>

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
	// Test basic registering/unregistering ratings
	{
		ps::PlayersStorage storage;
		std::string nickname1 = "Baron_Von_Awesome";
		std::string nickname2 = "Mrs.Wonderful";
		assert(storage.GetPlayerRating(nickname1) == -1);
		assert(storage.GetPlayerRating(nickname2) == -1);
		storage.RegisterPlayerResult(nickname1, 1000);
		storage.RegisterPlayerResult(nickname2, 2000);
		assert(storage.GetPlayerRating(nickname1) == 1000);
		assert(storage.GetPlayerRating(nickname2) == 2000);
		storage.UnregisterPlayer(nickname1);
		assert(storage.GetPlayerRating(nickname1) == -1);
		assert(storage.GetPlayerRating(nickname2) == 2000);
		storage.RegisterPlayerResult(nickname1, 3000);
		storage.RegisterPlayerResult(nickname2, 4000);
		assert(storage.GetPlayerRating(nickname1) == 3000);
		assert(storage.GetPlayerRating(nickname2) == 4000);
	}

	// Test rollback for registering/unregistering ratings
	{
		ps::PlayersStorage storage;
		std::string nickname1 = "sashagrey720hd";
		std::string nickname2 = "Toastoftheundead";
		storage.RegisterPlayerResult(nickname1, 1000);
		storage.RegisterPlayerResult(nickname2, 2000);
		storage.Rollback(2);
		assert(storage.GetPlayerRating(nickname1) == -1);
		assert(storage.GetPlayerRating(nickname2) == -1);
		storage.RegisterPlayerResult(nickname1, 1000);
		storage.RegisterPlayerResult(nickname2, 2000);
		storage.Rollback(1);
		assert(storage.GetPlayerRating(nickname1) == 1000);
		assert(storage.GetPlayerRating(nickname2) == -1);
		storage.RegisterPlayerResult(nickname1, 3000);
		assert(storage.GetPlayerRating(nickname1) == 3000);
		assert(storage.GetPlayerRating(nickname2) == -1);
		storage.RegisterPlayerResult(nickname2, 4000);
		assert(storage.GetPlayerRating(nickname1) == 3000);
		assert(storage.GetPlayerRating(nickname2) == 4000);
		storage.Rollback(1);
		assert(storage.GetPlayerRating(nickname1) == 3000);
		assert(storage.GetPlayerRating(nickname2) == -1);
		storage.Rollback(2);
		assert(storage.GetPlayerRating(nickname1) == -1);
		assert(storage.GetPlayerRating(nickname2) == -1);
	}
}
