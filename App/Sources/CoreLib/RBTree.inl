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
inline void ps::RBTree<TKey, TValue>::Rollback(int delta)
{
	assert(delta > 0 && delta <= m_CurrentVersion);
	m_CurrentVersion -= delta;
}

template<typename TKey, typename TValue>
inline int ps::RBTree<TKey, TValue>::GetVersion() const
{ 
	return m_CurrentVersion; 
}

template<typename TKey, typename TValue>
inline ps::RBNode<TKey, TValue>* ps::RBTree<TKey, TValue>::Insert(const TKey& key)
{
	assert(m_CurrentVersion >= 0);
	m_CurrentVersion++;

	// Firstly we need to clear this version (in case of rollback - it could contain rollback'd changes)
	ClearCurrentVersion();

	// Special case - create root
	if (!m_RootHistory[m_CurrentVersion - 1])
	{
		m_RootHistory[m_CurrentVersion] = std::make_shared<ps::RBNode<TKey, TValue>>();
		m_RootHistory[m_CurrentVersion]->m_Key = key;
		return m_RootHistory[m_CurrentVersion].get();
	}

	ps::RBNode<TKey, TValue>* keyNewParent = ClonePath(key);
	if (!keyNewParent)
	{
		// If we didn't found path to that key that means that we're trying to modify root node. Clone it and return.
		m_RootHistory[m_CurrentVersion] = std::make_shared<ps::RBNode<TKey, TValue>>(*m_RootHistory[m_CurrentVersion - 1]);
		return m_RootHistory[m_CurrentVersion].get();
	}

	if (key < keyNewParent->m_Key)
	{
		if (keyNewParent->m_Left)
		{
			// Target node has been found. Clone it and return
			keyNewParent->m_Left = std::make_shared<ps::RBNode<TKey, TValue>>(*keyNewParent->m_Left);
			return keyNewParent->m_Left.get();
		}

		// Create new node
		keyNewParent->m_Left = std::make_shared<ps::RBNode<TKey, TValue>>();
		keyNewParent->m_Left->m_Key = key;
		keyNewParent->m_Left->m_Red = true;
		InsertFixup(keyNewParent->m_Left.get());
		return keyNewParent->m_Left.get();
	}

	if (keyNewParent->m_Right)
	{
		// Target node has been found. Clone it and return
		keyNewParent->m_Right = std::make_shared<ps::RBNode<TKey, TValue>>(*keyNewParent->m_Right);
		return keyNewParent->m_Right.get();
	}

	// Create new node
	keyNewParent->m_Right = std::make_shared<ps::RBNode<TKey, TValue>>();
	keyNewParent->m_Right->m_Key = key;
	keyNewParent->m_Right->m_Red = true;
	InsertFixup(keyNewParent->m_Right.get());
	return keyNewParent->m_Right.get();
}

template<typename TKey, typename TValue>
inline void ps::RBTree<TKey, TValue>::Delete(const TKey& key)
{
	assert(m_CurrentVersion >= 0);
	assert(Search(key) != nullptr);
	m_CurrentVersion++;

	// Firstly we need to clear this version (in case of rollback - it could contain rollback'd changes)
	ClearCurrentVersion();

	// Find parent of node being deleted and clone all path to this parent (including parent itself)
	ps::RBNode<TKey, TValue>* nodeToDeleteNewParent = ClonePath(key);
	ps::RBNode<TKey, TValue>* nodeToDelete = nullptr;
	if (!nodeToDeleteNewParent)
	{
		nodeToDelete = m_RootHistory[m_CurrentVersion - 1].get();
	}
	else if (nodeToDeleteNewParent->m_Left && nodeToDeleteNewParent->m_Left->m_Key == key)
	{
		nodeToDelete = nodeToDeleteNewParent->m_Left.get();
	}
	else
	{
		nodeToDelete = nodeToDeleteNewParent->m_Right.get();
	}

	//		Start moving subtrees which effectively deletes node.
	// 1. Case when node which will replace deletable node has 0 or 1 child
	if (!nodeToDelete->m_Left)
	{
		Transplant(nodeToDelete, nodeToDeleteNewParent, nodeToDelete->m_Right);
		return;
	}

	if (!nodeToDelete->m_Right)
	{
		Transplant(nodeToDelete, nodeToDeleteNewParent, nodeToDelete->m_Left);
		return;
	}

	// 2. Case when node which will replace deletable node has 2 childs
	ps::RBNode<TKey, TValue>* replacementNode = nullptr;
	ps::RBNode<TKey, TValue>* replacementNodeParent = GetMinParent(nodeToDelete->m_Right.get());
	if (replacementNodeParent)
	{
		replacementNode = replacementNodeParent->m_Left.get();
	}
	else
	{
		replacementNodeParent = nodeToDelete;
		replacementNode = nodeToDelete->m_Right.get();
	}

	if (replacementNodeParent == nodeToDelete)
	{
		// 2a. Case when node which will replace deletable node is deletable node's direct child
		std::shared_ptr<ps::RBNode<TKey, TValue>> clonedReplacementNode = std::make_shared<ps::RBNode<TKey, TValue>>(*replacementNode);
		Transplant(nodeToDelete, nodeToDeleteNewParent, clonedReplacementNode);
		clonedReplacementNode->m_Left = nodeToDelete->m_Left;
		return;
	}

	// 2b. Case when node which will replace deletable node is NOT deletable node's direct child. That means that we need to clone path to this replacementNode
	auto[nodeToDeleteNewRightChild, replacementNodeNewParent] = ClonePath(nodeToDelete->m_Right.get(), replacementNode->m_Key);
	std::shared_ptr<ps::RBNode<TKey, TValue>> clonedReplacementNode = std::make_shared<ps::RBNode<TKey, TValue>>(*replacementNode);
	Transplant(nodeToDelete, nodeToDeleteNewParent, clonedReplacementNode);
	clonedReplacementNode->m_Left = nodeToDelete->m_Left;
	clonedReplacementNode->m_Right = nodeToDeleteNewRightChild;
	replacementNodeNewParent->m_Left = replacementNode->m_Right;
}

template<typename TKey, typename TValue>
inline ps::RBNode<TKey, TValue>* ps::RBTree<TKey, TValue>::Search(const TKey& key)
{
	Node* root = GetRoot();
	return Search(root, key);
}

template<typename TKey, typename TValue>
inline ps::RBNode<TKey, TValue>* ps::RBTree<TKey, TValue>::GetMin()
{
	Node* root = GetRoot();
	if (!root)
	{
		return nullptr;
	}

	return Min(root);
}

template<typename TKey, typename TValue>
inline ps::RBNode<TKey, TValue>* ps::RBTree<TKey, TValue>::GetMax()
{
	Node* root = GetRoot();
	if (!root)
	{
		return nullptr;
	}

	return Max(root);
}

template<typename TKey, typename TValue>
inline bool ps::RBTree<TKey, TValue>::DEBUG_CheckIfSorted()
{
	Node* root = GetRoot();
	return DEBUG_CheckIfSorted(root);
}

template<typename TKey, typename TValue>
inline ps::RBNode<TKey, TValue>* ps::RBTree<TKey, TValue>::GetMinParent(ps::RBNode<TKey, TValue>* node)
{
	if (!node->m_Left)
	{
		// This is minimal node and we cannot aqcuire its parent
		return nullptr;
	}

	while (node->m_Left->m_Left)
	{
		node = node->m_Left.get();
	}

	return node;
}

template<typename TKey, typename TValue>
inline ps::RBNode<TKey, TValue>* ps::RBTree<TKey, TValue>::GetRoot()
{
	Node* root = m_RootHistory.size() > m_CurrentVersion ? m_RootHistory[m_CurrentVersion].get() : nullptr;
	return root;
}

template<typename TKey, typename TValue>
inline void ps::RBTree<TKey, TValue>::ClearCurrentVersion()
{
	if (m_RootHistory.size() > m_CurrentVersion)
	{
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
inline ps::RBNode<TKey, TValue>* ps::RBTree<TKey, TValue>::ClonePath(const TKey& toKey)
{
	// Handle case when root doesn't exist or it is a target node
	ps::RBNode<TKey, TValue>* oldRoot = m_RootHistory[m_CurrentVersion - 1].get();
	if (!oldRoot || oldRoot->m_Key == toKey)
	{
		return nullptr;
	}

	auto[newRoot, newKeyParent] = ClonePath(oldRoot, toKey);
	m_RootHistory[m_CurrentVersion] = newRoot;
	return newKeyParent;
}

template<typename TKey, typename TValue>
inline void ps::RBTree<TKey, TValue>::InsertFixup(ps::RBNode<TKey, TValue>* newNode)
{
	// All parents has been cloned already. Uncles might not be cloned.
	std::vector<ps::RBNode<TKey, TValue>*> parents = BuildPath(newNode);
	auto getParent = [&parents]() { return parents[parents.size() - 1]; };
	auto getGrandParent = [&parents]() { return parents[parents.size() - 2]; };
	while (getParent()->m_Red)
	{
		if (getParent() == getGrandParent()->m_Left.get())
		{
			ps::RBNode<TKey, TValue>* uncle = getGrandParent()->m_Right.get();
			if (uncle->m_Red)
			{
				// Case 1
				getParent()->m_Red = false;
				getGrandParent()->m_Right = std::make_shared<ps::RBNode<TKey, TValue>>(*uncle);
				uncle = getGrandParent()->m_Right.get();
				uncle->m_Red = false;
				getGrandParent()->m_Red = true;
				newNode = getGrandParent();
				parents.pop_back();
				parents.pop_back();
			}
			else
			{
				if (newNode == getParent()->m_Right.get())
				{
					// Case 2
					newNode = getParent();
					parents.pop_back();

					// Clone needed node before rotation, remember what node is being rotated and then restore parents after rotation
					newNode->m_Right = std::make_shared<ps::RBNode<TKey, TValue>>(*newNode->m_Right);
					ps::RBNode<TKey, TValue>* willBeNewParent = newNode->m_Right.get();
					LeftRotate(newNode, getParent());
					parents.push_back(willBeNewParent);
				}

				// Case 3. No else intended
				getParent()->m_Red = false;
				getGrandParent()->m_Red = true;

				// Clone needed node before rotation, remember what node is being rotated and then restore parents after rotation
				getGrandParent()->m_Left = std::make_shared<ps::RBNode<TKey, TValue>>(*getGrandParent()->m_Left);
				ps::RBNode<TKey, TValue>* willBeNewParent = getGrandParent()->m_Left.get();
				RightRotate(getGrandParent(), parents[parents.size() - 3]);
				parents.push_back(willBeNewParent);

				// Rotation should always terminate loop
				assert(!getParent()->m_Red);
			}
		}
		else
		{
			/////////////////////////////////////////
			// TODO: AVOID DUPLICATING THIS LOGIC !!!
			/////////////////////////////////////////

			ps::RBNode<TKey, TValue>* uncle = getGrandParent()->m_Left.get();
			if (uncle->m_Red)
			{
				// Case 1
				getParent()->m_Red = false;
				getGrandParent()->m_Left = std::make_shared<ps::RBNode<TKey, TValue>>(*uncle);
				uncle = getGrandParent()->m_Left.get();
				uncle->m_Red = false;
				getGrandParent()->m_Red = true;
				newNode = getGrandParent();
				parents.pop_back();
				parents.pop_back();
			}
			else
			{
				if (newNode == getParent()->m_Left.get())
				{
					// Case 2
					newNode = getParent();
					parents.pop_back();

					// Clone needed node before rotation, remember what node is being rotated and then restore parents after rotation
					newNode->m_Right = std::make_shared<ps::RBNode<TKey, TValue>>(*newNode->m_Right);
					ps::RBNode<TKey, TValue>* willBeNewParent = newNode->m_Right.get();
					LeftRotate(newNode, getParent());
					parents.push_back(willBeNewParent);
				}

				// Case 3. No else intended
				getParent()->m_Red = false;
				getGrandParent()->m_Red = true;

				// Clone needed node before rotation, remember what node is being rotated and then restore parents after rotation
				getGrandParent()->m_Left = std::make_shared<ps::RBNode<TKey, TValue>>(*getGrandParent()->m_Left);
				ps::RBNode<TKey, TValue>* willBeNewParent = getGrandParent()->m_Left.get();
				RightRotate(getGrandParent(), parents[parents.size() - 3]);
				parents.push_back(willBeNewParent);

				// Rotation should always terminate loop
				assert(!getParent()->m_Red);
			}
		}
	}

	GetRoot()->m_Red = false;
}

template<typename TKey, typename TValue>
inline std::vector<ps::RBNode<TKey, TValue>*> ps::RBTree<TKey, TValue>::BuildPath(ps::RBNode<TKey, TValue>* toNode)
{
	assert(toNode);
	std::vector<ps::RBNode<TKey, TValue>*> path;
	ps::RBNode<TKey, TValue>* node = GetRoot();
	while (node && node->m_Key != toNode->m_Key)
	{
		path.push_back(node);
		if (toNode->m_Key < node->m_Key)
		{
			node = node->m_Left.get();
		}
		else
		{
			node = node->m_Right.get();
		}
	}

	if (!node)
	{
		return std::vector<ps::RBNode<TKey, TValue>*>();
	}

	return path;
}

template<typename TKey, typename TValue>
inline void ps::RBTree<TKey, TValue>::RightRotate(ps::RBNode<TKey, TValue>* target, ps::RBNode<TKey, TValue>* targetParent)
{
	// Important to keep shared_ptrs there. This way object won't be removed during swapping pointers
	std::shared_ptr<ps::RBNode<TKey, TValue>> targetNode = GetSharedPtr(target, targetParent);
	std::shared_ptr<ps::RBNode<TKey, TValue>> childNode = target->m_Left;
	targetNode->m_Left = childNode->m_Right;
	if (!targetParent)
	{
		m_RootHistory[m_CurrentVersion] = childNode;
	}
	else if (targetParent->m_Left.get() == target)
	{
		targetParent->m_Left = childNode;
	}
	else
	{
		targetParent->m_Right = childNode;
	}

	childNode->m_Right = targetNode;
}

template<typename TKey, typename TValue>
inline void ps::RBTree<TKey, TValue>::LeftRotate(ps::RBNode<TKey, TValue>* target, ps::RBNode<TKey, TValue>* targetParent)
{
	// Important to keep shared_ptr there. This way object won't be removed during swapping pointers
	std::shared_ptr<ps::RBNode<TKey, TValue>> targetNode = GetSharedPtr(target, targetParent);
	std::shared_ptr<ps::RBNode<TKey, TValue>> childNode = target->m_Right;
	targetNode->m_Right = childNode->m_Left;
	if (!targetParent)
	{
		m_RootHistory[m_CurrentVersion] = childNode;
	}
	else if (targetParent->m_Left.get() == target)
	{
		targetParent->m_Left = childNode;
	}
	else
	{
		targetParent->m_Right = childNode;
	}

	childNode->m_Left = targetNode;
}

template<typename TKey, typename TValue>
inline std::shared_ptr<ps::RBNode<TKey, TValue>> ps::RBTree<TKey, TValue>::GetSharedPtr(ps::RBNode<TKey, TValue>* target, ps::RBNode<TKey, TValue>* targetParent)
{
	if (!targetParent)
	{
		return m_RootHistory[m_CurrentVersion];
	}
	
	if (targetParent->m_Left.get() == target)
	{
		return targetParent->m_Left;
	}
	
	return targetParent->m_Right;
}

template<typename TKey, typename TValue>
inline std::tuple<std::shared_ptr<ps::RBNode<TKey, TValue>>, ps::RBNode<TKey, TValue>*> ps::RBTree<TKey, TValue>::ClonePath(
	ps::RBNode<TKey, TValue>* from, const TKey& toKey)
{
	assert(from->m_Key != toKey);
	std::shared_ptr<ps::RBNode<TKey, TValue>> newFrom = std::make_shared<ps::RBNode<TKey, TValue>>(*from);
	ps::RBNode<TKey, TValue>* newNode = newFrom.get();
	while (true)
	{
		assert(newNode->m_Key != toKey);
		if (newNode->m_Left && newNode->m_Left->m_Key == toKey)
		{
			// Our left child is node we're looking for
			return { newFrom, newNode };
		}

		if (newNode->m_Right && newNode->m_Right->m_Key == toKey)
		{
			// Our right child is node we're looking for
			return { newFrom, newNode };
		}

		if (toKey < newNode->m_Key && !newNode->m_Left)
		{
			// Node we're looking doesn't exist. This node should be a parent for node with specified key
			return { newFrom, newNode };
		}

		if (toKey > newNode->m_Key && !newNode->m_Right)
		{
			// Node we're looking doesn't exist. This node should be a parent for node with specified key
			return { newFrom, newNode };
		}

		// Continue traversal
		if (toKey < newNode->m_Key)
		{
			newNode->m_Left = std::make_shared<ps::RBNode<TKey, TValue>>(*newNode->m_Left);
			newNode = newNode->m_Left.get();
		}
		else
		{
			newNode->m_Right = std::make_shared<ps::RBNode<TKey, TValue>>(*newNode->m_Right);
			newNode = newNode->m_Right.get();
		}
	}

	// Should never reach this point
	std::abort();
}

template<typename TKey, typename TValue>
inline void ps::RBTree<TKey, TValue>::Transplant(ps::RBNode<TKey, TValue>* target, ps::RBNode<TKey, TValue>* targetParent, 
	std::shared_ptr<ps::RBNode<TKey, TValue>> source)
{
	if (!targetParent)
	{
		m_RootHistory[m_CurrentVersion] = source;
		return;
	}

	if (targetParent->m_Left.get() == target)
	{
		targetParent->m_Left = source;
		return;
	}

	assert(targetParent->m_Right.get() == target);
	targetParent->m_Right = source;
}

template<typename TKey, typename TValue>
inline bool ps::RBTree<TKey, TValue>::DEBUG_CheckIfSorted(ps::RBNode<TKey, TValue>* node)
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

	return DEBUG_CheckIfSorted(node->m_Left.get()) && DEBUG_CheckIfSorted(node->m_Right.get());
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
inline ps::RBNode<TKey, TValue>* ps::RBTree<TKey, TValue>::GetMin(ps::RBNode<TKey, TValue>* node)
{
	while (node->m_Left)
	{
		node = node->m_Left;
	}

	return node;
}

template<typename TKey, typename TValue>
inline ps::RBNode<TKey, TValue>* ps::RBTree<TKey, TValue>::GetMax(ps::RBNode<TKey, TValue>* node)
{
	while (node->m_Right)
	{
		node = node->m_Right;
	}

	return node;
}
