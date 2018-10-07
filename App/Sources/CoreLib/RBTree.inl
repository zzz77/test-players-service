#pragma once

#include "RBTree.h"

#include <cassert>
#include <cstdlib>
#include <iterator>
#include <memory>

template<typename TKey, typename TValue>
inline ps::RBTree<TKey, TValue>::RBTree()
	: m_CurrentVersion(0)
{
	ClearCurrentVersion();
}

template<typename TKey, typename TValue>
inline ps::RBTree<TKey, TValue>::~RBTree()
{
	for (auto it = m_RootHistory.rbegin(); it != m_RootHistory.rend(); ++it)
	{
		delete *it;
	}
}

template<typename TKey, typename TValue>
inline void ps::RBTree<TKey, TValue>::Rollback(int delta)
{
	assert(delta > 0 && delta <= m_CurrentVersion);
	m_CurrentVersion -= delta;
}

template<typename TKey, typename TValue>
inline ps::RBNode<TKey, TValue>* ps::RBTree<TKey, TValue>::Insert(const TKey& key)
{
	m_CurrentVersion++;
	assert(m_CurrentVersion > 0);

	// Firstly we need to clear this version (in case of rollback - it could contain rollback'd changes)
	ClearCurrentVersion();

	// Special case - create root
	if (!m_RootHistory[m_CurrentVersion - 1])
	{
		m_RootHistory[m_CurrentVersion] = new ps::RBNode<TKey, TValue>();
		m_RootHistory[m_CurrentVersion]->m_Key = key;
		return m_RootHistory[m_CurrentVersion];
	}

	auto[oldNode, newNode] = ClonePathTo(key);
	if (!oldNode || !newNode)
	{
		// If we didn't found path to that key that means that we're trying to modify root node. Clone it and return.
		m_RootHistory[m_CurrentVersion] = new ps::RBNode<TKey, TValue>(*m_RootHistory[m_CurrentVersion - 1]);
		return m_RootHistory[m_CurrentVersion];
	}

	if (key < newNode->m_Key)
	{
		if (newNode->m_Left)
		{
			// Target node has been found. Clone it and return
			oldNode = oldNode->m_Left.get();
			newNode->m_Left = std::make_shared<ps::RBNode<TKey, TValue>>(*oldNode);
			return newNode->m_Left.get();
		}

		// Create new node
		newNode->m_Left = std::make_shared<ps::RBNode<TKey, TValue>>();
		newNode->m_Left->m_Key = key;
		return newNode->m_Left.get();
	}

	if (newNode->m_Right)
	{
		// Target node has been found. Clone it and return
		oldNode = oldNode->m_Right.get();
		newNode->m_Right = std::make_shared<ps::RBNode<TKey, TValue>>(*oldNode);
		return newNode->m_Right.get();
	}

	// Create new node
	newNode->m_Right = std::make_shared<ps::RBNode<TKey, TValue>>();
	newNode->m_Right->m_Key = key;
	return newNode->m_Right.get();
}

template<typename TKey, typename TValue>
inline ps::RBNode<TKey, TValue>* ps::RBTree<TKey, TValue>::Delete(const TKey& key)
{
	m_CurrentVersion++;
	assert(m_CurrentVersion > 0);

	// Firstly we need to clear this version (in case of rollback - it could contain rollback'd changes)
	ClearCurrentVersion();

	// Find parent of node being deleted and clone all path to this parent

	return NULL;
}

template<typename TKey, typename TValue>
inline ps::RBNode<TKey, TValue>* ps::RBTree<TKey, TValue>::Search(const TKey& key)
{
	Node* root = GetRoot();
	return Search(root, key);
}

template<typename TKey, typename TValue>
inline ps::RBNode<TKey, TValue>* ps::RBTree<TKey, TValue>::Min()
{
	Node* root = GetRoot();
	if (!root)
	{
		return nullptr;
	}

	return Min(root);
}

template<typename TKey, typename TValue>
inline ps::RBNode<TKey, TValue>* ps::RBTree<TKey, TValue>::Max()
{
	Node* root = GetRoot();
	if (!root)
	{
		return nullptr;
	}

	return Max(root);
}

template<typename TKey, typename TValue>
inline ps::RBNode<TKey, TValue>* ps::RBTree<TKey, TValue>::GetRoot()
{
	Node* root = m_RootHistory.size() > m_CurrentVersion ? m_RootHistory[m_CurrentVersion] : nullptr;
	return root;
}

template<typename TKey, typename TValue>
inline void ps::RBTree<TKey, TValue>::ClearCurrentVersion()
{
	if (m_RootHistory.size() > m_CurrentVersion)
	{
		delete m_RootHistory[m_CurrentVersion];
		m_RootHistory[m_CurrentVersion] = nullptr;
	}
	else
	{
		// There shouldn't be any gap!
		assert(m_RootHistory.size() == m_CurrentVersion);
		m_RootHistory.push_back(nullptr);
	}
}

template<typename TKey, typename TValue>
inline std::tuple<ps::RBNode<TKey, TValue>*, ps::RBNode<TKey, TValue>*> ps::RBTree<TKey, TValue>::ClonePathTo(const TKey& key)
{
	// Handle case when root doesn't exist or it is a target node
	ps::RBNode<TKey, TValue>* oldNode = m_RootHistory[m_CurrentVersion - 1];
	if (!oldNode || oldNode->m_Key == key)
	{
		return { nullptr, nullptr };
	}

	ps::RBNode<TKey, TValue>* newNode = new ps::RBNode<TKey, TValue>(*oldNode);
	m_RootHistory[m_CurrentVersion] = newNode;
	while (true)
	{
		assert(newNode->m_Key != key);
		if (newNode->m_Left && newNode->m_Left->m_Key == key)
		{
			// Our left child is node we're looking for
			return { oldNode, newNode };
		}

		if (newNode->m_Right && newNode->m_Right->m_Key == key)
		{
			// Our right child is node we're looking for
			return { oldNode, newNode };
		}

		if (key < newNode->m_Key && !newNode->m_Left)
		{
			// Node we're looking doesn't exist. This node should be a parent for node with specified key
			return { oldNode, newNode };
		}

		if (key > newNode->m_Key && !newNode->m_Right)
		{
			// Node we're looking doesn't exist. This node should be a parent for node with specified key
			return { oldNode, newNode };
		}

		// Continue traversal
		if (key < newNode->m_Key)
		{
			oldNode = oldNode->m_Left.get();
			newNode->m_Left = std::make_shared<ps::RBNode<TKey, TValue>>(*oldNode);
			newNode = newNode->m_Left.get();
		}
		else
		{
			oldNode = oldNode->m_Right.get();
			newNode->m_Right = std::make_shared<ps::RBNode<TKey, TValue>>(*oldNode);
			newNode = newNode->m_Right.get();
		}
	}

	// Should never reach this point
	std::abort();
}

template<typename TKey, typename TValue>
inline ps::RBNode<TKey, TValue>* ps::RBTree<TKey, TValue>::Search(ps::RBNode<TKey, TValue>* node, const TKey& key)
{
	while (node && node->m_Key != key)
	{
		if (key < node->m_Key)
		{
			node = node->m_Left.get();
		}
		else
		{
			node = node->m_Right.get();
		}
	}

	return node;
}

template<typename TKey, typename TValue>
inline ps::RBNode<TKey, TValue>* ps::RBTree<TKey, TValue>::Min(ps::RBNode<TKey, TValue>* node)
{
	while (node->m_Left)
	{
		node = node->m_Left;
	}

	return node;
}

template<typename TKey, typename TValue>
inline ps::RBNode<TKey, TValue>* ps::RBTree<TKey, TValue>::Max(ps::RBNode<TKey, TValue>* node)
{
	while (node->m_Right)
	{
		node = node->m_Right;
	}

	return node;
}

//template<typename TKey, typename TValue>
//inline void ps::RBTree<TKey, TValue>::Transplant(ps::RBNodeWithParent<TKey, TValue>* oldNode, ps::RBNode<TKey, TValue>* newNode)
//{
//	if (!oldNode->m_Parent)
//	{
//		m_Root = newNode;
//	}
//
//	if (newNode)
//	{
//		newNode
//	}
//}
