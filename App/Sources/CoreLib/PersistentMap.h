#pragma once

#include <cassert>
#include <memory>
#include <tuple>
#include <vector>

namespace pst
{
	class PersistentMapTest;

	template <typename TKey, typename TValue>
	class PersistentMapNode
	{
	public:
		PersistentMapNode(const TKey& key, int currentVersion);

		PersistentMapNode(const PersistentMapNode<TKey, TValue>& other, int currentVersion);

		std::shared_ptr<PersistentMapNode> Clone(int currentVersion) const;

		void SetIsRed([[maybe_unused]] int currentVersion, bool red)
		{
			// TODO: Extend version check to changing other node's data and to track duplicate clones calls
			assert(m_CreateVersion == currentVersion);
			m_Red = red;
		}

		bool IsRed() const { return m_Red; }

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
		// TODO: Not cool but for proper testing without friend class more comprehensive API is needed
		friend class PersistentMapTest;
	public:
		PersistentMap();

		void Rollback(int delta);
		int GetVersion() const;

		// TODO: Return wrapper over key and value, not the node itself
		/// Creates new node with specified key. If node already created - returns pointer to it. Creates new version of data.
		PersistentMapNode<TKey, TValue>* Insert(const TKey& key);
		void Delete(const TKey& key);

		const PersistentMapNode<TKey, TValue>* Search(const TKey& key) const;
		const PersistentMapNode<TKey, TValue>* GetMin() const;
		const PersistentMapNode<TKey, TValue>* GetMax() const;

	private:
		const PersistentMapNode<TKey, TValue>* Search(const PersistentMapNode<TKey, TValue>* node, const TKey& key) const;
		PersistentMapNode<TKey, TValue>* Search(PersistentMapNode<TKey, TValue>* node, const TKey& key);
		const PersistentMapNode<TKey, TValue>* GetMin(const PersistentMapNode<TKey, TValue>* node) const;
		PersistentMapNode<TKey, TValue>* GetMin(PersistentMapNode<TKey, TValue>* node);
		const PersistentMapNode<TKey, TValue>* GetMax(const PersistentMapNode<TKey, TValue>* node) const;
		PersistentMapNode<TKey, TValue>* GetMax(PersistentMapNode<TKey, TValue>* node);

		/// Returns parent of minimal node right after specified node
		PersistentMapNode<TKey, TValue>* GetMinParent(PersistentMapNode<TKey, TValue>* node);

		const PersistentMapNode<TKey, TValue>* GetRoot() const;
		PersistentMapNode<TKey, TValue>* GetRoot();

		/// Resets root for current version
		void ClearCurrentVersion();

		/// Clones previous version of [root; toKey)-nodes and insertes it into current version root.
		/// Node with m_Key == toKey is not cloned if it exists.
		/// Returns current version of toKey's parent node.
		PersistentMapNode<TKey, TValue>* ClonePath(const TKey& toKey);

		/// Clones [from; toKey)-nodes.
		/// Node with m_Key == toKey is not cloned if it exists.
		/// Returns current version of from and toKey's parent node.
		std::tuple<std::shared_ptr<PersistentMapNode<TKey, TValue>>, PersistentMapNode<TKey, TValue>*> ClonePath(const PersistentMapNode<TKey, TValue>* from, const TKey& toKey) const;

		/// Detaches target from targetParent and makes source child of targetParent. 
		/// TargetParent should be of current version.
		/// Source can be either of old version or of current version. It is responsibility of caller to clone it if necessary
		void Transplant(const PersistentMapNode<TKey, TValue>* target, PersistentMapNode<TKey, TValue>* targetParent, std::shared_ptr<PersistentMapNode<TKey, TValue>> source);

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

		/// Returns path [root; toNode) as a vector where root is located at 0 element and toNode's parent at last element. Uses current version
		std::vector<const PersistentMapNode<TKey, TValue>*> BuildPath(const PersistentMapNode<TKey, TValue>* toNode) const;

		std::vector<std::shared_ptr<PersistentMapNode<TKey, TValue>>> m_RootHistory;
		int m_CurrentVersion;
	};
}

#include "PersistentMap.inl"