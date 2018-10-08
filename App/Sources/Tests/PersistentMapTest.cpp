#include "PersistentMapTest.h"

#include "../CoreLib/PersistentMap.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <numeric>
#include <random>
#include <string>

void pst::PersistentMapTest::Run()
{
	TestInsertingAndRollback();
	TestSortness();
	TestDeleting();
	TestRBTreeWithRandomData();
}

void pst::PersistentMapTest::TestInsertingAndRollback()
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

void pst::PersistentMapTest::TestSortness()
{
	pst::PersistentMap<int, int> tree;
	assert(CheckIfTreeIsSorted(&tree));
	tree.Insert(16);
	tree.Insert(8);
	tree.Insert(4);
	tree.Insert(12);
	tree.Insert(24);
	tree.Insert(20);
	tree.Insert(28);
	assert(CheckIfTreeIsSorted(&tree));
	tree.Rollback(4);
	assert(CheckIfTreeIsSorted(&tree));
	tree.Insert(3);
	tree.Insert(2);
	tree.Insert(1);
	assert(CheckIfTreeIsSorted(&tree));
	tree.Insert(12);
	tree.Insert(24);
	tree.Insert(20);
	tree.Insert(28);
	assert(CheckIfTreeIsSorted(&tree));
}

void pst::PersistentMapTest::TestDeleting()
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
			assert(CheckIfTreeIsSorted(&tree));
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

void pst::PersistentMapTest::TestRBTreeWithRandomData()
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

		assert(CheckIfTreeIsSorted(&tree));
		assert(CheckIfTreeIsRB(&tree));
		std::shuffle(std::begin(randomKeys), std::end(randomKeys), generator);
		randomKeys.resize(2000);
		for (int key : randomKeys)
		{
			tree.Delete(key);
		}

		assert(CheckIfTreeIsSorted(&tree));
		assert(CheckIfTreeIsRB(&tree));
		tree.Rollback(1000);
		assert(CheckIfTreeIsSorted(&tree));
		assert(CheckIfTreeIsRB(&tree));
		tree.Rollback(3000);
		assert(CheckIfTreeIsSorted(&tree));
		assert(CheckIfTreeIsRB(&tree));
		tree.Rollback(3000);
		for (int key : randomKeys)
		{
			tree.Insert(key);
		}

		tree.Rollback(1000);
		assert(CheckIfTreeIsSorted(&tree));
		assert(CheckIfTreeIsRB(&tree));
	}
}

template<typename TKey, typename TValue>
bool pst::PersistentMapTest::CheckIfTreeIsSorted(const pst::PersistentMap<TKey, TValue>* map)
{
	return CheckIfTreeIsSorted(map, map->GetRoot());
}

template<typename TKey, typename TValue>
bool pst::PersistentMapTest::CheckIfTreeIsRB(const pst::PersistentMap<TKey, TValue>* map)
{
	const pst::PersistentMapNode<TKey, TValue>* root = map->GetRoot();
	if (!root)
	{
		return true;
	}

	const pst::PersistentMapNode<TKey, TValue>* minNode = map->GetMin();
	int blackNodes = CountBlackNodes(map, minNode);
	return !root->IsRed() && CheckIfTreeIsRB(map, root, blackNodes);
}

template<typename TKey, typename TValue>
bool pst::PersistentMapTest::CheckIfTreeIsSorted(const pst::PersistentMap<TKey, TValue>* map, const pst::PersistentMapNode<TKey, TValue>* node)
{
	if (!node)
	{
		return true;
	}

	if (node->m_Left && node->m_Left->m_Key > node->m_Key)
	{
		return false;
	}

	if (node->m_Right && node->m_Right->m_Key < node->m_Key)
	{
		return false;
	}

	return CheckIfTreeIsSorted(map, node->m_Left.get()) && CheckIfTreeIsSorted(map, node->m_Right.get());
}

template<typename TKey, typename TValue>
bool pst::PersistentMapTest::CheckIfTreeIsRB(const pst::PersistentMap<TKey, TValue>* map, const pst::PersistentMapNode<TKey, TValue>* node, int expectedBlackNodes)
{
	if (!node)
	{
		return true;
	}

	if (node->IsRed() && node->m_Left && node->m_Left->IsRed())
	{
		return false;
	}

	if (node->IsRed() && node->m_Right && node->m_Right->IsRed())
	{
		return false;
	}

	if (!node->m_Left || !node->m_Right)
	{
		int blackNodes = CountBlackNodes(map, node);
		if (blackNodes != expectedBlackNodes)
		{
			return false;
		}
	}

	return CheckIfTreeIsRB(map, node->m_Left.get(), expectedBlackNodes) && CheckIfTreeIsRB(map, node->m_Right.get(), expectedBlackNodes);
}

template<typename TKey, typename TValue>
int pst::PersistentMapTest::CountBlackNodes(const pst::PersistentMap<TKey, TValue>* map, const pst::PersistentMapNode<TKey, TValue>* toNode)
{
	int blackNodes = 0;
	std::vector<const pst::PersistentMapNode<TKey, TValue>*> path = map->BuildPath(toNode);
	for (auto* node : path)
	{
		if (node && !node->IsRed())
		{
			blackNodes++;
		}
	}

	if (!toNode->IsRed())
	{
		blackNodes++;
	}

	return blackNodes;
}
