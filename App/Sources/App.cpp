#include "CoreLib/PersistentMap.h"
#include "DataModel/PlayersStorage.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <numeric>
#include <random>
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

// TODO: Move to separate files/classes
void RunSimulation()
{
	// Test Persistent Map based on Red Black Tree
	{
		// Test inserting nodes and rollback
		{
			pst::PersistentMap<std::string, int> tree;
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
		}

		// Test inserting nodes, sortness and rollback
		{
			pst::PersistentMap<int, int> tree;
			assert(tree.DEBUG_CheckIfSorted());
			tree.Insert(16);
			tree.Insert(8);
			tree.Insert(4);
			tree.Insert(12);
			tree.Insert(24);
			tree.Insert(20);
			tree.Insert(28);
			assert(tree.DEBUG_CheckIfSorted());
			tree.Rollback(4);
			assert(tree.DEBUG_CheckIfSorted());
			tree.Insert(3);
			tree.Insert(2);
			tree.Insert(1);
			assert(tree.DEBUG_CheckIfSorted());
			tree.Insert(12);
			tree.Insert(24);
			tree.Insert(20);
			tree.Insert(28);
			assert(tree.DEBUG_CheckIfSorted());
		}

		// Test inserting nodes, deleting nodes, sortness and rollback
		{
			const int numberOfNodes = 7;
			std::array<int, numberOfNodes> arr = { 16, 8, 12, 4, 24, 20, 28 };
			for (int i = 0; i < numberOfNodes; i++)
			{
				pst::PersistentMap<int, int> tree;
				for (int key : arr)
				{
					tree.Insert(key)->m_Value = key;
				}

				// Let's delete half of elements and check that tree is still sorted
				const int numberOfRemovals = static_cast<int>(std::ceil(numberOfNodes / 2.0));
				const int step = i >= numberOfRemovals ? -1 : 1;
				for (int j = i, stepsDone = 0; stepsDone < numberOfRemovals; j += step, stepsDone++)
				{
					tree.Delete(arr[j]);
					assert(tree.Search(arr[j]) == nullptr);
					assert(tree.DEBUG_CheckIfSorted());
				}
			}

			{
				pst::PersistentMap<int, int> tree;
				tree.Insert(1)->m_Value = 1;
				tree.Insert(2)->m_Value = 2;
				tree.Delete(1);
				tree.Delete(2);
				tree.Rollback(1);
				assert(tree.Search(1) == nullptr);
				assert(tree.Search(2)->m_Value == 2);
				tree.Insert(2)->m_Value = 22;
				assert(tree.Search(1) == nullptr);
				assert(tree.Search(2)->m_Value == 22);
				tree.Rollback(4);
				assert(tree.Search(1) == nullptr);
				assert(tree.Search(2) == nullptr);
				tree.Insert(2)->m_Value = 222;
				assert(tree.Search(1) == nullptr);
				assert(tree.Search(2)->m_Value == 222);
			}
		}

		// Test with random numbers
		{
			const int numberOfAttempts = 8;
			auto generator = std::default_random_engine{};
			for (int attempt = 0; attempt < numberOfAttempts; attempt++)
			{
				pst::PersistentMap<int, int> tree;
				std::vector<int> randomKeys(5000);
				std::iota(std::begin(randomKeys), std::end(randomKeys), 0);

				// Ideally we need deterministic random generator
				std::shuffle(std::begin(randomKeys), std::end(randomKeys), generator);
				for (int key : randomKeys)
				{
					tree.Insert(key)->m_Value = key;;
				}

				assert(tree.DEBUG_CheckIfSorted());
				assert(tree.DEBUG_CheckIfRB());
				std::shuffle(std::begin(randomKeys), std::end(randomKeys), generator);
				randomKeys.resize(2000);
				for (int key : randomKeys)
				{
					tree.Delete(key);
				}

				assert(tree.DEBUG_CheckIfSorted());
				assert(tree.DEBUG_CheckIfRB());
				tree.Rollback(1000);
				assert(tree.DEBUG_CheckIfSorted());
				assert(tree.DEBUG_CheckIfRB());
				tree.Rollback(3000);
				assert(tree.DEBUG_CheckIfSorted());
				assert(tree.DEBUG_CheckIfRB());
				tree.Rollback(3000);
				for (int key : randomKeys)
				{
					tree.Insert(key);
				}

				tree.Rollback(1000);
				assert(tree.DEBUG_CheckIfSorted());
				assert(tree.DEBUG_CheckIfRB());
			}
		}
	}

	// Test PlayersStorage (composite data structure)
	{
		// Test basic registering/unregistering ratings
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

		// Test rollback for registering/unregistering ratings
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
	}
}
