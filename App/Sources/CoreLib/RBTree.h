#pragma once

#include <memory>
#include <tuple>
#include <vector>

// PersistentRBTree!
namespace ps
{
	//template<typename TKey, typename TValue>
	//class RBTree;

	template <typename TKey, typename TValue>
	class RBNode
	{
	public:

		// TODO: Make TKey const
		TKey m_Key;
		TValue m_Value;

		// TODO: Check assignments and use make_shared
		std::shared_ptr<RBNode<TKey, TValue>> m_Left;
		std::shared_ptr<RBNode<TKey, TValue>> m_Right;

		// TODO: Add version number in order to be able to understand in which version this node has been created
	};

	// Helper and temporary struct for tree traversal
	//template <typename TKey, typename TValue>
	//struct RBNodeWithParent
	//{
	//	RBNode<TKey, TValue>* m_Node;
	//	RBNodeWithParent<TKey, TValue>* m_Parent;
	//};

	template <typename TKey, typename TValue>
	class RBTree
	{
	public:
		using Node = ps::RBNode<TKey, TValue>;

		RBTree();
		~RBTree();

		void Rollback(int delta);

		// TODO: Should return ref to value probably
		Node* Insert(const TKey& key);
		void Delete(const TKey& key);

		// TODO: Should return consts (and be const)!
		Node* Search(const TKey& key);

		// TODO: Remove or add tests for unused functions
		Node* GetMin();
		Node* GetMax();

		bool DEBUG_CheckIfSorted();

	private:
		Node* Search(Node* node, const TKey& key);
		Node* GetMin(Node* node);
		Node* GetMax(Node* node);

		// TODO: Think of better API for traversal!
		Node* GetMinParent(Node* node);

		Node* GetRoot();
		void ClearCurrentVersion();

		/// Clones previous version of [root; toKey)-nodes and insertes it into current version root.
		/// Node with m_Key == toKey is not cloned if it exists.
		/// Returns current version of toKey's parent node.
		Node* ClonePath(const TKey& toKey);

		/// Clones previous version of [from; toKey)-nodes.
		/// Node with m_Key == toKey is not cloned if it exists.
		/// Returns current version of from and toKey's parent.
		std::tuple<std::shared_ptr<Node>, Node*> ClonePath(Node* from, const TKey& toKey);

		/// Detaches target from targetParent and makes source child of targetParent. 
		/// TargetParent should be of current version.
		/// Source can be either of old version or of current version. It is responsibility of caller to clone it if necessary
		void Transplant(Node* target, Node* targetParent, std::shared_ptr<Node> source);

		// TODO: Move to separate class with tests
		bool DEBUG_CheckIfSorted(Node* node);

		std::vector<std::shared_ptr<Node>> m_RootHistory;
		int m_CurrentVersion;
	};
}

#include "RBTree.inl"