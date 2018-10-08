#include "PlayerStorageTest.h"

#include "../DataModel/PlayersStorage.h"

#include <cassert>
#include <string>

void pst::PlayerStorageTest::Run()
{
	TestRegistration();
	TestRollback();
}

void pst::PlayerStorageTest::TestRegistration()
{
	pst::PlayersStorage storage;
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

void pst::PlayerStorageTest::TestRollback()
{
	pst::PlayersStorage storage;
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
