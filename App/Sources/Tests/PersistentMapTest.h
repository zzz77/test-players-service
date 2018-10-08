#pragma once

namespace pst
{
	template<typename TKey, typename TValue>
	class PersistentMap;

	template<typename TKey, typename TValue>
	class PersistentMapNode;

	class PersistentMapTest
	{
	public:
		static void Run();

	private:

		static void TestInsertingAndRollback();
		static void TestSortness();
		static void TestDeleting();
		static void TestRBTreeWithRandomData();

		// Helper methods to inspect map
		template<typename TKey, typename TValue>
		static bool CheckIfTreeIsSorted(const PersistentMap<TKey, TValue>* map);

		template<typename TKey, typename TValue>
		static bool CheckIfTreeIsRB(const PersistentMap<TKey, TValue>* map);

		template<typename TKey, typename TValue>
		static bool CheckIfTreeIsSorted(const PersistentMap<TKey, TValue>* map, const PersistentMapNode<TKey, TValue>* node);

		template<typename TKey, typename TValue>
		static bool CheckIfTreeIsRB(const PersistentMap<TKey, TValue>* map, const PersistentMapNode<TKey, TValue>* node, int expectedBlackNodes);

		template<typename TKey, typename TValue>
		static int CountBlackNodes(const PersistentMap<TKey, TValue>* map, const PersistentMapNode<TKey, TValue>* toNode);
	};
}
