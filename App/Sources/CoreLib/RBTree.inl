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
		m_RootHistory[m_CurrentVersion] = std::make_shared<ps::RBNode<TKey, TValue>>(key, m_CurrentVersion);
		return m_RootHistory[m_CurrentVersion].get();
	}

	ps::RBNode<TKey, TValue>* keyNewParent = ClonePath(key);
	if (!keyNewParent)
	{
		// If we didn't found path to that key that means that we're trying to modify root node. Clone it and return.
		m_RootHistory[m_CurrentVersion] = m_RootHistory[m_CurrentVersion - 1]->Clone(m_CurrentVersion);
		return m_RootHistory[m_CurrentVersion].get();
	}

	if (key < keyNewParent->m_Key)
	{
		if (keyNewParent->m_Left)
		{
			// Target node has been found. Clone it and return
			keyNewParent->m_Left = keyNewParent->m_Left->Clone(m_CurrentVersion);
			return keyNewParent->m_Left.get();
		}

		// Create new node
		keyNewParent->m_Left = std::make_shared<ps::RBNode<TKey, TValue>>(key, m_CurrentVersion);
		keyNewParent->m_Left->SetColor(m_CurrentVersion, true);
		InsertFixup(keyNewParent->m_Left.get());

		// Fixup can invalidate node (by cloning it for instance)
		return Search(key);
	}

	if (keyNewParent->m_Right)
	{
		// Target node has been found. Clone it and return
		keyNewParent->m_Right = keyNewParent->m_Right->Clone(m_CurrentVersion);
		return keyNewParent->m_Right.get();
	}

	// Create new node
	keyNewParent->m_Right = std::make_shared<ps::RBNode<TKey, TValue>>(key, m_CurrentVersion);
	keyNewParent->m_Right->SetColor(m_CurrentVersion, true);
	InsertFixup(keyNewParent->m_Right.get());

	// Fixup can invalidate node (by cloning it for instance)
	return Search(key);
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
	std::shared_ptr<ps::RBNode<TKey, TValue>> nodeToDelete = nullptr;
	if (!nodeToDeleteNewParent)
	{
		nodeToDelete = m_RootHistory[m_CurrentVersion - 1];
	}
	else if (nodeToDeleteNewParent->m_Left && nodeToDeleteNewParent->m_Left->m_Key == key)
	{
		nodeToDelete = nodeToDeleteNewParent->m_Left;
	}
	else
	{
		nodeToDelete = nodeToDeleteNewParent->m_Right;
	}

	bool requiresFixup = !nodeToDelete->IsRed();

	//		Start moving subtrees which effectively deletes node.
	// 1. Case when node which will replace deletable node has 0 or 1 child
	if (!nodeToDelete->m_Left)
	{
		std::shared_ptr<ps::RBNode<TKey, TValue>> replacementNode = nodeToDelete->m_Right ? nodeToDelete->m_Right->Clone(m_CurrentVersion) : nullptr;
		Transplant(nodeToDelete.get(), nodeToDeleteNewParent, replacementNode);
		if (requiresFixup)
		{
			// Special case - if tree is empty now
			if (GetRoot())
			{
				DeleteFixup(replacementNode.get(), nodeToDeleteNewParent);
			}
		}

		return;
	}

	if (!nodeToDelete->m_Right)
	{
		std::shared_ptr<ps::RBNode<TKey, TValue>> replacementNode = nodeToDelete->m_Left ? nodeToDelete->m_Left->Clone(m_CurrentVersion) : nullptr;
		Transplant(nodeToDelete.get(), nodeToDeleteNewParent, replacementNode);
		if (requiresFixup)
		{
			DeleteFixup(replacementNode.get(), nodeToDeleteNewParent);
		}

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
		replacementNodeParent = nodeToDelete.get();
		replacementNode = nodeToDelete->m_Right.get();
	}

	requiresFixup = !replacementNode->IsRed();
	if (replacementNodeParent == nodeToDelete.get())
	{
		// 2a. Case when node which will replace deletable node is deletable node's direct child
		std::shared_ptr<ps::RBNode<TKey, TValue>> clonedReplacementNode = replacementNode->Clone(m_CurrentVersion);
		Transplant(nodeToDelete.get(), nodeToDeleteNewParent, clonedReplacementNode);
		clonedReplacementNode->m_Left = nodeToDelete->m_Left;
		clonedReplacementNode->SetColor(m_CurrentVersion, nodeToDelete->IsRed());
		if (requiresFixup)
		{
			clonedReplacementNode->m_Right = clonedReplacementNode->m_Right ? clonedReplacementNode->m_Right->Clone(m_CurrentVersion) : nullptr;
			DeleteFixup(clonedReplacementNode->m_Right.get(), clonedReplacementNode.get());
		}

		return;
	}

	// 2b. Case when node which will replace deletable node is NOT deletable node's direct child. That means that we need to clone path to this replacementNode
	auto[nodeToDeleteNewRightChild, replacementNodeNewParent] = ClonePath(nodeToDelete->m_Right.get(), replacementNode->m_Key);
	std::shared_ptr<ps::RBNode<TKey, TValue>> clonedReplacementNode = replacementNode->Clone(m_CurrentVersion);
	Transplant(nodeToDelete.get(), nodeToDeleteNewParent, clonedReplacementNode);
	clonedReplacementNode->SetColor(m_CurrentVersion, nodeToDelete->IsRed());
	clonedReplacementNode->m_Left = nodeToDelete->m_Left;
	clonedReplacementNode->m_Right = nodeToDeleteNewRightChild;
	replacementNodeNewParent->m_Left = replacementNode->m_Right;
	if (requiresFixup)
	{
		replacementNodeNewParent->m_Left = replacementNodeNewParent->m_Left ? replacementNodeNewParent->m_Left->Clone(m_CurrentVersion) : nullptr;
		DeleteFixup(replacementNodeNewParent->m_Left.get(), replacementNodeNewParent);
	}
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

	return GetMin(root);
}

template<typename TKey, typename TValue>
inline ps::RBNode<TKey, TValue>* ps::RBTree<TKey, TValue>::GetMax()
{
	Node* root = GetRoot();
	if (!root)
	{
		return nullptr;
	}

	return GetMax(root);
}

template<typename TKey, typename TValue>
inline bool ps::RBTree<TKey, TValue>::DEBUG_CheckIfSorted()
{
	Node* root = GetRoot();
	return DEBUG_CheckIfSorted(root);
}

template<typename TKey, typename TValue>
inline bool ps::RBTree<TKey, TValue>::DEBUG_CheckIfRB()
{
	Node* root = GetRoot();
	if (!root)
	{
		return true;
	}

	ps::RBNode<TKey, TValue>* minNode = GetMin();
	int blackNodes = DEBUG_CountBlackNodes(minNode);
	return !root->IsRed() && DEBUG_CheckIfRB(root, blackNodes);
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
inline void ps::RBTree<TKey, TValue>::InsertFixup(ps::RBNode<TKey, TValue>* fixNode)
{
	// TODO: Consider caching parent and grandparent
	// All parents has been cloned already. Uncles has not.
	std::vector<ps::RBNode<TKey, TValue>*> parents = BuildPath(fixNode);
	auto getParent = [&parents]() { return parents[parents.size() - 1]; };
	auto getGrandParent = [&parents]() { return parents[parents.size() - 2]; };
	while (getParent() && getParent()->IsRed())
	{
		if (getParent() == getGrandParent()->m_Left.get())
		{
			ps::RBNode<TKey, TValue>* uncle = getGrandParent()->m_Right.get();
			if (uncle && uncle->IsRed())
			{
				// Case 1
				getParent()->SetColor(m_CurrentVersion, false);
				getGrandParent()->m_Right = uncle->Clone(m_CurrentVersion);
				uncle = getGrandParent()->m_Right.get();
				uncle->SetColor(m_CurrentVersion, false);
				getGrandParent()->SetColor(m_CurrentVersion, true);
				fixNode = getGrandParent();
				parents.pop_back();
				parents.pop_back();
			}
			else
			{
				if (fixNode == getParent()->m_Right.get())
				{
					// Case 2
					fixNode = getParent();
					parents.pop_back();

					// Clone needed node before rotation, remember what node is being rotated and then restore parents after rotation
					fixNode->m_Right = fixNode->m_Right->Clone(m_CurrentVersion);
					ps::RBNode<TKey, TValue>* willBeNewParent = fixNode->m_Right.get();
					LeftRotate(fixNode, getParent());
					parents.push_back(willBeNewParent);
				}

				// Case 3. No else intended
				getParent()->SetColor(m_CurrentVersion, false);
				getGrandParent()->SetColor(m_CurrentVersion, true);

				// Clone needed node before rotation, remember what node is being rotated and then restore parents after rotation
				getGrandParent()->m_Left = getGrandParent()->m_Left->Clone(m_CurrentVersion);
				ps::RBNode<TKey, TValue>* willBeNewParent = getGrandParent()->m_Left.get();
				RightRotate(getGrandParent(), parents[parents.size() - 3]);
				parents.push_back(willBeNewParent);

				// Rotation should always terminate loop
				assert(!getParent()->IsRed());
			}
		}
		else
		{
			/////////////////////////////////////////
			// TODO: AVOID DUPLICATING THIS LOGIC !!!
			/////////////////////////////////////////

			ps::RBNode<TKey, TValue>* uncle = getGrandParent()->m_Left.get();
			if (uncle && uncle->IsRed())
			{
				// Case 1
				getParent()->SetColor(m_CurrentVersion, false);
				getGrandParent()->m_Left = uncle->Clone(m_CurrentVersion);
				uncle = getGrandParent()->m_Left.get();
				uncle->SetColor(m_CurrentVersion, false);
				getGrandParent()->SetColor(m_CurrentVersion, true);
				fixNode = getGrandParent();
				parents.pop_back();
				parents.pop_back();
			}
			else
			{
				if (fixNode == getParent()->m_Left.get())
				{
					// Case 2
					fixNode = getParent();
					parents.pop_back();

					// Clone needed node before rotation, remember what node is being rotated and then restore parents after rotation
					fixNode->m_Left = fixNode->m_Left->Clone(m_CurrentVersion);
					ps::RBNode<TKey, TValue>* willBeNewParent = fixNode->m_Left.get();
					RightRotate(fixNode, getParent());
					parents.push_back(willBeNewParent);
				}

				// Case 3. No else intended
				getParent()->SetColor(m_CurrentVersion, false);
				getGrandParent()->SetColor(m_CurrentVersion, true);

				// Clone needed node before rotation, remember what node is being rotated and then restore parents after rotation
				getGrandParent()->m_Right = getGrandParent()->m_Right->Clone(m_CurrentVersion);
				ps::RBNode<TKey, TValue>* willBeNewParent = getGrandParent()->m_Right.get();
				LeftRotate(getGrandParent(), parents[parents.size() - 3]);
				parents.push_back(willBeNewParent);

				// Rotation should always terminate loop
				assert(!getParent()->IsRed());
			}
		}
	}

	GetRoot()->SetColor(m_CurrentVersion, false);
}

template<typename TKey, typename TValue>
inline std::vector<ps::RBNode<TKey, TValue>*> ps::RBTree<TKey, TValue>::BuildPath(ps::RBNode<TKey, TValue>* toNode)
{
	assert(toNode);
	std::vector<ps::RBNode<TKey, TValue>*> path;

	// Parent of root is always nullptr
	path.push_back(nullptr);
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
inline int ps::RBTree<TKey, TValue>::DEBUG_CountBlackNodes(ps::RBNode<TKey, TValue>* toNode)
{
	int blackNodes = 0;
	std::vector<ps::RBNode<TKey, TValue>*> path = BuildPath(toNode);
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

template<typename TKey, typename TValue>
inline bool ps::RBTree<TKey, TValue>::DEBUG_CheckIfRB(ps::RBNode<TKey, TValue>* node, int expectedBlackNodes)
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
		int blackNodes = DEBUG_CountBlackNodes(node);
		if (blackNodes != expectedBlackNodes)
		{
			return false;
		}
	}

	return DEBUG_CheckIfRB(node->m_Left.get(), expectedBlackNodes) && DEBUG_CheckIfRB(node->m_Right.get(), expectedBlackNodes);
}

template<typename TKey, typename TValue>
inline void ps::RBTree<TKey, TValue>::DeleteFixup(ps::RBNode<TKey, TValue>* fixNode, ps::RBNode<TKey, TValue>* parentForNullNode)
{
	// All parents has been cloned already. Sublings has not.
	std::vector<ps::RBNode<TKey, TValue>*> parents = fixNode ? BuildPath(fixNode) : BuildPath(parentForNullNode);
	if (!fixNode)
	{
		assert(parentForNullNode);
		parents.push_back(parentForNullNode);
	}

	auto getParent = [&parents]() { return parents[parents.size() - 1]; };
	auto getGrandParent = [&parents]() { return parents[parents.size() - 2]; };
	while (fixNode != GetRoot() && (!fixNode || !fixNode->IsRed()))
	{
		if (fixNode == getParent()->m_Left.get())
		{
			ps::RBNode<TKey, TValue>* subling = getParent()->m_Right.get();
			if (subling && subling->IsRed())
			{
				// Case 1
				getParent()->m_Right = getParent()->m_Right->Clone(m_CurrentVersion);
				subling = getParent()->m_Right.get();
				subling->SetColor(m_CurrentVersion, false);
				getParent()->SetColor(m_CurrentVersion, true);
				LeftRotate(getParent(), getGrandParent());
				
				// Restore parents
				ps::RBNode<TKey, TValue>* parent = parents.back();
				parents.pop_back();
				parents.push_back(subling);
				parents.push_back(parent);

				// Find new subling
				subling = getParent()->m_Right.get();
			}

			if ((!subling->m_Left || !subling->m_Left->IsRed()) && (!subling->m_Right || !subling->m_Right->IsRed()))
			{
				// Case 2. Both subling's children are black
				getParent()->m_Right = getParent()->m_Right->Clone(m_CurrentVersion);
				subling = getParent()->m_Right.get();
				subling->SetColor(m_CurrentVersion, true);
				fixNode = getParent();
				parents.pop_back();
			}
			else
			{
				if (!subling->m_Right || !subling->m_Right->IsRed())
				{
					// Case 3
					// Cloning subling in order to clone its child and rotate them then
					getParent()->m_Right = getParent()->m_Right->Clone(m_CurrentVersion);
					subling = getParent()->m_Right.get();
					subling->m_Left = subling->m_Left->Clone(m_CurrentVersion);
					subling->m_Left->SetColor(m_CurrentVersion, false);
					subling->SetColor(m_CurrentVersion, true);
					RightRotate(subling, getParent());

					// Find new subling
					subling = getParent()->m_Right.get();
				}

				// Case 4. No else intended
				getParent()->m_Right = getParent()->m_Right->Clone(m_CurrentVersion);
				subling = getParent()->m_Right.get();
				subling->m_Right = subling->m_Right->Clone(m_CurrentVersion);
				subling->SetColor(m_CurrentVersion, getParent()->IsRed());
				getParent()->SetColor(m_CurrentVersion, false);
				subling->m_Right->SetColor(m_CurrentVersion, false);
				LeftRotate(getParent(), getGrandParent());

				// Restore parents
				ps::RBNode<TKey, TValue>* parent = parents.back();
				parents.pop_back();
				parents.push_back(subling);
				parents.push_back(parent);

				fixNode = GetRoot();
			}
		}
		else
		{
			ps::RBNode<TKey, TValue>* subling = getParent()->m_Left.get();
			if (subling && subling->IsRed())
			{
				// Case 1
				getParent()->m_Left = getParent()->m_Left->Clone(m_CurrentVersion);
				subling = getParent()->m_Left.get();
				subling->SetColor(m_CurrentVersion, false);
				getParent()->SetColor(m_CurrentVersion, true);
				RightRotate(getParent(), getGrandParent());

				// Restore parents
				ps::RBNode<TKey, TValue>* parent = parents.back();
				parents.pop_back();
				parents.push_back(subling);
				parents.push_back(parent);

				// Find new subling
				subling = getParent()->m_Left.get();
			}

			if ((!subling->m_Left || !subling->m_Left->IsRed()) && (!subling->m_Right || !subling->m_Right->IsRed()))
			{
				// Case 2. Both subling's children are black
				getParent()->m_Left = getParent()->m_Left->Clone(m_CurrentVersion);
				subling = getParent()->m_Left.get();
				subling->SetColor(m_CurrentVersion, true);
				fixNode = getParent();
				parents.pop_back();
			}
			else
			{
				if (!subling->m_Left || !subling->m_Left->IsRed())
				{
					// Case 3
					// Cloning subling in order to clone its child and rotate them then
					getParent()->m_Left = getParent()->m_Left->Clone(m_CurrentVersion);
					subling = getParent()->m_Left.get();
					subling->m_Right = subling->m_Right->Clone(m_CurrentVersion);
					subling->m_Right->SetColor(m_CurrentVersion, false);
					subling->SetColor(m_CurrentVersion, true);
					LeftRotate(subling, getParent());

					// Find new subling
					subling = getParent()->m_Left.get();
				}

				// Case 4. No else intended
				getParent()->m_Left = getParent()->m_Left->Clone(m_CurrentVersion);
				subling = getParent()->m_Left.get();
				subling->m_Left = subling->m_Left->Clone(m_CurrentVersion);
				subling->SetColor(m_CurrentVersion, getParent()->IsRed());
				getParent()->SetColor(m_CurrentVersion, false);
				subling->m_Left->SetColor(m_CurrentVersion, false);
				RightRotate(getParent(), getGrandParent());

				// Restore parents
				ps::RBNode<TKey, TValue>* parent = parents.back();
				parents.pop_back();
				parents.push_back(subling);
				parents.push_back(parent);

				fixNode = GetRoot();
			}
		}
	}

	// Always root node - so it should be cloned already
	fixNode->SetColor(m_CurrentVersion, false);
}

template<typename TKey, typename TValue>
inline std::tuple<std::shared_ptr<ps::RBNode<TKey, TValue>>, ps::RBNode<TKey, TValue>*> ps::RBTree<TKey, TValue>::ClonePath(
	ps::RBNode<TKey, TValue>* from, const TKey& toKey)
{
	assert(from->m_Key != toKey);
	std::shared_ptr<ps::RBNode<TKey, TValue>> newFrom = from->Clone(m_CurrentVersion);
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
			newNode->m_Left = newNode->m_Left->Clone(m_CurrentVersion);
			newNode = newNode->m_Left.get();
		}
		else
		{
			newNode->m_Right = newNode->m_Right->Clone(m_CurrentVersion);
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
		node = node->m_Left.get();
	}

	return node;
}

template<typename TKey, typename TValue>
inline ps::RBNode<TKey, TValue>* ps::RBTree<TKey, TValue>::GetMax(ps::RBNode<TKey, TValue>* node)
{
	while (node->m_Right)
	{
		node = node->m_Right.get();
	}

	return node;
}
