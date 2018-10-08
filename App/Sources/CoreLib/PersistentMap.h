#pragma once

#include <cassert>
#include <memory>
#include <tuple>
#include <vector>

namespace pst
{
	template <typename TKey, typename TValue>
	class PersistentMapNode
	{
	public:
		PersistentMapNode(const TKey& key, int currentVersion)
			: m_Key(key)
			, m_CreateVersion(currentVersion)
			, m_Red(false)
			, m_Value(TValue())
			, m_Left(nullptr)
			, m_Right(nullptr)
		{
		}

		PersistentMapNode(const PersistentMapNode<TKey, TValue>& other, int currentVersion)
			: m_Key(other.m_Key)
			, m_CreateVersion(currentVersion)
			, m_Red(other.m_Red)
			, m_Value(other.m_Value)
			, m_Left(other.m_Left)
			, m_Right(other.m_Right)
		{
		}

		std::shared_ptr<PersistentMapNode> Clone(int currentVersion)
		{
			return std::make_shared<PersistentMapNode>(*this, currentVersion);
		}

		void SetIsRed([[maybe_unused]] int currentVersion, bool red)
		{
			// TODO: Extend version check to changing other node's data and to track duplicate clones calls
			assert(m_CreateVersion == currentVersion);
			m_Red = red;
		}

		bool IsRed() { return m_Red; }

		const TKey m_Key;
		TValue m_Value;
		std::shared_ptr<PersistentMapNode> m_Left;
		std::shared_ptr<PersistentMapNode> m_Right;

	private:
		const int m_CreateVersion;
		bool m_Red;
	};

	template <typename TKey, typename TValue>
	class PersistentMap
	{
	public:
		PersistentMap();

		void Rollback(int delta);
		int GetVersion() const;

		// TODO: Should return ref to value probably
		/// Creates new node with specified key. If node already created - returns pointer to it. Creates new version of data.
		PersistentMapNode<TKey, TValue>* Insert(const TKey& key);
		void Delete(const TKey& key);

		// TODO: Should return consts (and be const)!
		PersistentMapNode<TKey, TValue>* Search(const TKey& key);
		PersistentMapNode<TKey, TValue>* GetMin();
		PersistentMapNode<TKey, TValue>* GetMax();

		bool DEBUG_CheckIfSorted();
		bool DEBUG_CheckIfRB();

	private:
		PersistentMapNode<TKey, TValue>* Search(PersistentMapNode<TKey, TValue>* node, const TKey& key);
		PersistentMapNode<TKey, TValue>* GetMin(PersistentMapNode<TKey, TValue>* node);
		PersistentMapNode<TKey, TValue>* GetMax(PersistentMapNode<TKey, TValue>* node);

		// TODO: Think of better API for traversal!
		PersistentMapNode<TKey, TValue>* GetMinParent(PersistentMapNode<TKey, TValue>* node);

		PersistentMapNode<TKey, TValue>* GetRoot();
		void ClearCurrentVersion();

		/// Clones previous version of [root; toKey)-nodes and insertes it into current version root.
		/// Node with m_Key == toKey is not cloned if it exists.
		/// Returns current version of toKey's parent node.
		PersistentMapNode<TKey, TValue>* ClonePath(const TKey& toKey);

		/// Clones previous version of [from; toKey)-nodes.
		/// Node with m_Key == toKey is not cloned if it exists.
		/// Returns current version of from and toKey's parent node.
		std::tuple<std::shared_ptr<PersistentMapNode<TKey, TValue>>, PersistentMapNode<TKey, TValue>*> ClonePath(PersistentMapNode<TKey, TValue>* from, const TKey& toKey);

		/// Detaches target from targetParent and makes source child of targetParent. 
		/// TargetParent should be of current version.
		/// Source can be either of old version or of current version. It is responsibility of caller to clone it if necessary
		void Transplant(PersistentMapNode<TKey, TValue>* target, PersistentMapNode<TKey, TValue>* targetParent, std::shared_ptr<PersistentMapNode<TKey, TValue>> source);

		/// Rotates subtree to left
		/// Target and one of it's child will NOT be cloned. It is responsibility of caller to clone it if necessary
		/// TargetParent should be of current version.
		void LeftRotate(PersistentMapNode<TKey, TValue>* target, PersistentMapNode<TKey, TValue>* targetParent);

		/// Rotates subtree to right
		/// Target and one of it's child will NOT be cloned. It is responsibility of caller to clone it if necessary
		/// TargetParent should be of current version.
		void RightRotate(PersistentMapNode<TKey, TValue>* target, PersistentMapNode<TKey, TValue>* targetParent);

		/// Finds target node in parent and returns shared_ptr which is stored in parent
		std::shared_ptr<PersistentMapNode<TKey, TValue>> GetSharedPtr(PersistentMapNode<TKey, TValue>* target, PersistentMapNode<TKey, TValue>* targetParent);

		/// Restores RB-tree properties after inserting node.
		void InsertFixup(PersistentMapNode<TKey, TValue>* fixNode);

		/// Restores RB-tree properties after deleting node.
		void DeleteFixup(PersistentMapNode<TKey, TValue>* fixNode, PersistentMapNode<TKey, TValue>* parentForNullNode);

		/// Returns path [root; toNode) as a vector where root is located at 0 element and toNode's parent at last element. Uses current version
		std::vector<PersistentMapNode<TKey, TValue>*> BuildPath(PersistentMapNode<TKey, TValue>* toNode);

		// TODO: Move to separate class with tests
		bool DEBUG_CheckIfSorted(PersistentMapNode<TKey, TValue>* node);
		bool DEBUG_CheckIfRB(PersistentMapNode<TKey, TValue>* node, int expectedBlackNodes);
		int DEBUG_CountBlackNodes(PersistentMapNode<TKey, TValue>* toNode);

		std::vector<std::shared_ptr<PersistentMapNode<TKey, TValue>>> m_RootHistory;
		int m_CurrentVersion;
	};
}

#include "PersistentMap.inl"