#pragma once

#include <memory>
#include <tuple>
#include <vector>

// PersistentMap!
namespace ps
{
	template <typename TKey, typename TValue>
	class RBNode
	{
	public:

		// TODO: Make TKey const
		TKey m_Key;
		bool m_Red;
		TValue m_Value;

		std::shared_ptr<RBNode<TKey, TValue>> m_Left;
		std::shared_ptr<RBNode<TKey, TValue>> m_Right;
	};

	template <typename TKey, typename TValue>
	class RBTree
	{
	public:
		using Node = ps::RBNode<TKey, TValue>;

		RBTree();

		void Rollback(int delta);
		int GetVersion() const;

		// TODO: Should return ref to value probably
		/// Creates new node with specified key. If node already created - returns pointer to it. Creates new version of data.
		Node* Insert(const TKey& key);
		void Delete(const TKey& key);

		// TODO: Should return consts (and be const)!
		Node* Search(const TKey& key);

		// TODO: Remove or add tests for unused functions
		Node* GetMin();
		Node* GetMax();

		bool DEBUG_CheckIfSorted();
		bool DEBUG_CheckIfRB();

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
		/// Returns current version of from and toKey's parent node.
		std::tuple<std::shared_ptr<Node>, Node*> ClonePath(Node* from, const TKey& toKey);

		/// Detaches target from targetParent and makes source child of targetParent. 
		/// TargetParent should be of current version.
		/// Source can be either of old version or of current version. It is responsibility of caller to clone it if necessary
		void Transplant(Node* target, Node* targetParent, std::shared_ptr<Node> source);

		// TODO: Unite Left and Right rotate functions
		/// Rotates subtree to left
		/// Target and one of it's child will NOT be cloned. It is responsibility of caller to clone it if necessary
		/// TargetParent should be of current version.
		void LeftRotate(Node* target, Node* targetParent);

		/// Rotates subtree to right
		/// Target and one of it's child will NOT be cloned. It is responsibility of caller to clone it if necessary
		/// TargetParent should be of current version.
		void RightRotate(Node* target, Node* targetParent);

		/// Finds target node in parent and returns shared_ptr which is stored in parent
		std::shared_ptr<ps::RBNode<TKey, TValue>> GetSharedPtr(ps::RBNode<TKey, TValue>* target, ps::RBNode<TKey, TValue>* targetParent);

		/// Restores RB-tree properties after inserting node.
		void InsertFixup(Node* newNode);

		/// Restores RB-tree properties after deleting node.
		void DeleteFixup(Node* replacementNode);

		/// Returns path [root; toNode) as a vector where root is located at 0 element and toNode's parent at last element. Uses current version
		std::vector<Node*> BuildPath(Node* toNode);

		// TODO: Move to separate class with tests
		bool DEBUG_CheckIfSorted(Node* node);
		bool DEBUG_CheckIfRB(Node* node, int expectedBlackNodes);
		int DEBUG_CountBlackNodes(Node* toNode);

		std::vector<std::shared_ptr<Node>> m_RootHistory;
		int m_CurrentVersion;
	};
}

#include "RBTree.inl"