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
		Node* Delete(const TKey& key);

		// TODO: Should return consts (and be const)!
		Node* Search(const TKey& key);
		Node* Min();
		Node* Max();

		bool DEBUG_CheckIfSorted();

	private:
		Node* Search(Node* node, const TKey& key);
		Node* Min(Node* node);
		Node* Max(Node* node);

		Node* GetRoot();
		void ClearCurrentVersion();
		/// Node with m_Key == key is not cloned if it exists. Parent node for key is returned (for current version and previous one). 
		std::tuple<Node*, Node*> ClonePathTo(const TKey& key);

		bool DEBUG_CheckIfSorted(Node* node);
		
		//void Transplant(RBNodeWithParent<TKey, TValue>* oldNode, RBNode<TKey, TValue>* newNode);

		// TODO: make unique_ptr?
		std::vector<Node*> m_RootHistory;
		int m_CurrentVersion;
	};
}

#include "RBTree.inl"