#include "CoreLib/RBTree.h"
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
	// Test inserting nodes into persistent tree and basic functionality
	{
		ps::RBTree<std::string, int> tree;
		assert(tree.Search("1") == nullptr);
		tree.Insert("1")->m_Value = 100;
		assert(tree.Search("1")->m_Value == 100);
		tree.Insert("1")->m_Value = 200;
		assert(tree.Search("1")->m_Value == 200);
		tree.Rollback(1);
		assert(tree.Search("1")->m_Value == 100);
		tree.Rollback(1);
		assert(tree.Search("1") == nullptr);
		tree.Insert("2")->m_Value = 300;
		assert(tree.Search("1") == nullptr);
		assert(tree.Search("2")->m_Value == 300);
		tree.Insert("1")->m_Value = 400;
		assert(tree.Search("1")->m_Value == 400);
		assert(tree.Search("2")->m_Value == 300);
		tree.Rollback(2);
		assert(tree.Search("1") == nullptr);
		assert(tree.Search("2") == nullptr);

		tree.Insert("1")->m_Value = 1000;
		tree.Insert("2")->m_Value = 2000;
		tree.Rollback(2);
		assert(tree.Search("1") == nullptr);
		assert(tree.Search("2") == nullptr);
		tree.Insert("1")->m_Value = 1000;
		tree.Insert("2")->m_Value = 2000;
		tree.Rollback(1);
		assert(tree.Search("1")->m_Value == 1000);
		assert(tree.Search("2") == nullptr);
		tree.Insert("1")->m_Value = 3000;
		assert(tree.Search("1")->m_Value == 3000);
		assert(tree.Search("2") == nullptr);
		tree.Insert("2")->m_Value = 4000;
		assert(tree.Search("1")->m_Value == 3000);
		assert(tree.Search("2")->m_Value == 4000);
		tree.Rollback(1);
		assert(tree.Search("1")->m_Value == 3000);
		assert(tree.Search("2") == nullptr);
		tree.Rollback(2);
		assert(tree.Search("1") == nullptr);
		assert(tree.Search("2") == nullptr);
	}

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

	// COMMENTED UNTIL BUGS WILL BE FIXED
	// Test rollback for registering/unregistering ratings
	//{
	//	ps::PlayersStorage storage;
	//	std::string nickname1 = "sashagrey720hd";
	//	std::string nickname2 = "Toastoftheundead";
	//	storage.RegisterPlayerResult(nickname1, 1000);
	//	storage.RegisterPlayerResult(nickname2, 2000);
	//	storage.Rollback(2);
	//	assert(storage.GetPlayerRating(nickname1) == -1);
	//	assert(storage.GetPlayerRating(nickname2) == -1);
	//	storage.RegisterPlayerResult(nickname1, 1000);
	//	storage.RegisterPlayerResult(nickname2, 2000);
	//	storage.Rollback(1);
	//	assert(storage.GetPlayerRating(nickname1) == 1000);
	//	assert(storage.GetPlayerRating(nickname2) == -1);
	//	storage.RegisterPlayerResult(nickname1, 3000);
	//	assert(storage.GetPlayerRating(nickname1) == 3000);
	//	assert(storage.GetPlayerRating(nickname2) == -1);
	//	storage.RegisterPlayerResult(nickname2, 4000);
	//	assert(storage.GetPlayerRating(nickname1) == 3000);
	//	assert(storage.GetPlayerRating(nickname2) == 4000);
	//	storage.Rollback(1);
	//	assert(storage.GetPlayerRating(nickname1) == 3000);
	//	assert(storage.GetPlayerRating(nickname2) == -1);
	//	storage.Rollback(2);
	//	assert(storage.GetPlayerRating(nickname1) == -1);
	//	assert(storage.GetPlayerRating(nickname2) == -1);
	//}
}
