#pragma once

#include "PersistentMap.h"

#include <cassert>
#include <cstdlib>
#include <iterator>
#include <memory>

template<typename TKey, typename TValue>
inline pst::PersistentMapNode<TKey, TValue>::PersistentMapNode(const TKey& key, int currentVersion)
	: m_Key(key)
	, m_CreateVersion(currentVersion)
	, m_Red(false)
	, m_Value(TValue())
	, m_Left(nullptr)
	, m_Right(nullptr)
{
}

template<typename TKey, typename TValue>
inline pst::PersistentMapNode<TKey, TValue>::PersistentMapNode(const PersistentMapNode<TKey, TValue>& other, int currentVersion)
	: m_Key(other.m_Key)
	, m_CreateVersion(currentVersion)
	, m_Red(other.m_Red)
	, m_Value(other.m_Value)
	, m_Left(other.m_Left)
	, m_Right(other.m_Right)
{
}

template<typename TKey, typename TValue>
inline std::shared_ptr<pst::PersistentMapNode<TKey, TValue>> pst::PersistentMapNode<TKey, TValue>::Clone(int currentVersion) const
{
	return std::make_shared<PersistentMapNode<TKey, TValue>>(*this, currentVersion);
}

template<typename TKey, typename TValue>
inline pst::PersistentMap<TKey, TValue>::PersistentMap()
	: m_CurrentVersion(0)
{
	ClearCurrentVersion();
}

template<typename TKey, typename TValue>
inline void pst::PersistentMap<TKey, TValue>::Rollback(int delta)
{
	assert(delta > 0 && delta <= m_CurrentVersion);
	m_CurrentVersion -= delta;
}

template<typename TKey, typename TValue>
inline int pst::PersistentMap<TKey, TValue>::GetVersion() const
{ 
	return m_CurrentVersion; 
}

template<typename TKey, typename TValue>
inline pst::PersistentMapNode<TKey, TValue>* pst::PersistentMap<TKey, TValue>::Insert(const TKey& key)
{
	assert(m_CurrentVersion >= 0);
	m_CurrentVersion++;

	// Firstly we need to clear this version (in case of rollback - it could contain rollback'd changes)
	ClearCurrentVersion();

	// Special case - create root
	if (!m_RootHistory[m_CurrentVersion - 1])
	{
		m_RootHistory[m_CurrentVersion] = std::make_shared<pst::PersistentMapNode<TKey, TValue>>(key, m_CurrentVersion);
		return m_RootHistory[m_CurrentVersion].get();
	}

	pst::PersistentMapNode<TKey, TValue>* keyNewParent = ClonePath(key);
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
		keyNewParent->m_Left = std::make_shared<pst::PersistentMapNode<TKey, TValue>>(key, m_CurrentVersion);
		keyNewParent->m_Left->SetIsRed(m_CurrentVersion, true);
		InsertFixup(keyNewParent->m_Left.get());

		// Fixup can invalidate node (by cloning it for instance)
		return Search(GetRoot(), key);
	}

	if (keyNewParent->m_Right)
	{
		// Target node has been found. Clone it and return
		keyNewParent->m_Right = keyNewParent->m_Right->Clone(m_CurrentVersion);
		return keyNewParent->m_Right.get();
	}

	// Create new node
	keyNewParent->m_Right = std::make_shared<pst::PersistentMapNode<TKey, TValue>>(key, m_CurrentVersion);
	keyNewParent->m_Right->SetIsRed(m_CurrentVersion, true);
	InsertFixup(keyNewParent->m_Right.get());

	// Fixup can invalidate node (by cloning it for instance)
	return Search(GetRoot(), key);
}

template<typename TKey, typename TValue>
inline void pst::PersistentMap<TKey, TValue>::Delete(const TKey& key)
{
	if (!Search(key))
	{
		// TODO: Can be optimized
		// There is nothing to delete
		return;
	}

	assert(m_CurrentVersion >= 0);
	m_CurrentVersion++;

	// Firstly we need to clear this version (in case of rollback - it could contain rollback'd changes)
	ClearCurrentVersion();

	// Find parent of node being deleted and clone all path to this parent (including parent itself)
	pst::PersistentMapNode<TKey, TValue>* nodeToDeleteNewParent = ClonePath(key);
	std::shared_ptr<pst::PersistentMapNode<TKey, TValue>> nodeToDelete = nullptr;
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
		std::shared_ptr<pst::PersistentMapNode<TKey, TValue>> replacementNode = nodeToDelete->m_Right ? nodeToDelete->m_Right->Clone(m_CurrentVersion) : nullptr;
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
		std::shared_ptr<pst::PersistentMapNode<TKey, TValue>> replacementNode = nodeToDelete->m_Left ? nodeToDelete->m_Left->Clone(m_CurrentVersion) : nullptr;
		Transplant(nodeToDelete.get(), nodeToDeleteNewParent, replacementNode);
		if (requiresFixup)
		{
			DeleteFixup(replacementNode.get(), nodeToDeleteNewParent);
		}

		return;
	}

	// 2. Case when node which will replace deletable node has 2 childs
	pst::PersistentMapNode<TKey, TValue>* replacementNode = nullptr;
	pst::PersistentMapNode<TKey, TValue>* replacementNodeParent = GetMinParent(nodeToDelete->m_Right.get());;
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
		std::shared_ptr<pst::PersistentMapNode<TKey, TValue>> clonedReplacementNode = replacementNode->Clone(m_CurrentVersion);
		Transplant(nodeToDelete.get(), nodeToDeleteNewParent, clonedReplacementNode);
		clonedReplacementNode->m_Left = nodeToDelete->m_Left;
		clonedReplacementNode->SetIsRed(m_CurrentVersion, nodeToDelete->IsRed());
		if (requiresFixup)
		{
			clonedReplacementNode->m_Right = clonedReplacementNode->m_Right ? clonedReplacementNode->m_Right->Clone(m_CurrentVersion) : nullptr;
			DeleteFixup(clonedReplacementNode->m_Right.get(), clonedReplacementNode.get());
		}

		return;
	}

	// 2b. Case when node which will replace deletable node is NOT deletable node's direct child. That means that we need to clone path to this replacementNode
	auto[nodeToDeleteNewRightChild, replacementNodeNewParent] = ClonePath(nodeToDelete->m_Right.get(), replacementNode->m_Key);
	std::shared_ptr<pst::PersistentMapNode<TKey, TValue>> clonedReplacementNode = replacementNode->Clone(m_CurrentVersion);
	Transplant(nodeToDelete.get(), nodeToDeleteNewParent, clonedReplacementNode);
	clonedReplacementNode->SetIsRed(m_CurrentVersion, nodeToDelete->IsRed());
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
inline const pst::PersistentMapNode<TKey, TValue>* pst::PersistentMap<TKey, TValue>::Search(const TKey& key) const
{
	return Search(GetRoot(), key);
}

template<typename TKey, typename TValue>
inline const pst::PersistentMapNode<TKey, TValue>* pst::PersistentMap<TKey, TValue>::GetMin() const
{
	const pst::PersistentMapNode<TKey, TValue>* root = GetRoot();
	if (!root)
	{
		return nullptr;
	}

	return GetMin(root);
}

template<typename TKey, typename TValue>
inline const pst::PersistentMapNode<TKey, TValue>* pst::PersistentMap<TKey, TValue>::GetMax() const
{
	pst::PersistentMapNode<TKey, TValue>* root = GetRoot();
	if (!root)
	{
		return nullptr;
	}

	return GetMax(root);
}

template<typename TKey, typename TValue>
inline const pst::PersistentMapNode<TKey, TValue>* pst::PersistentMap<TKey, TValue>::GetRoot() const
{
	pst::PersistentMapNode<TKey, TValue>* root = m_RootHistory.size() > m_CurrentVersion ? m_RootHistory[m_CurrentVersion].get() : nullptr;
	return root;
}

template<typename TKey, typename TValue>
inline pst::PersistentMapNode<TKey, TValue>* pst::PersistentMap<TKey, TValue>::GetRoot()
{
	pst::PersistentMapNode<TKey, TValue>* root = m_RootHistory.size() > m_CurrentVersion ? m_RootHistory[m_CurrentVersion].get() : nullptr;
	return root;
}

template<typename TKey, typename TValue>
inline void pst::PersistentMap<TKey, TValue>::ClearCurrentVersion()
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
inline pst::PersistentMapNode<TKey, TValue>* pst::PersistentMap<TKey, TValue>::ClonePath(const TKey& toKey)
{
	// Handle case when root doesn't exist or it is a target node
	pst::PersistentMapNode<TKey, TValue>* oldRoot = m_RootHistory[m_CurrentVersion - 1].get();
	if (!oldRoot || oldRoot->m_Key == toKey)
	{
		return nullptr;
	}

	auto[newRoot, newKeyParent] = ClonePath(oldRoot, toKey);
	m_RootHistory[m_CurrentVersion] = newRoot;
	return newKeyParent;
}

template<typename TKey, typename TValue>
inline void pst::PersistentMap<TKey, TValue>::InsertFixup(pst::PersistentMapNode<TKey, TValue>* fixNode)
{
	// TODO: Consider caching parent and grandparent

	// All parents has been cloned already. Uncles has not.
	std::vector<pst::PersistentMapNode<TKey, TValue>*> parents = BuildPath(fixNode);
	auto getParent = [&parents]() { return parents[parents.size() - 1]; };
	auto getGrandParent = [&parents]() { return parents[parents.size() - 2]; };
	while (getParent() && getParent()->IsRed())
	{
		if (getParent() == getGrandParent()->m_Left.get())
		{
			pst::PersistentMapNode<TKey, TValue>* uncle = getGrandParent()->m_Right.get();
			if (uncle && uncle->IsRed())
			{
				// Case 1
				getParent()->SetIsRed(m_CurrentVersion, false);
				getGrandParent()->m_Right = uncle->Clone(m_CurrentVersion);
				uncle = getGrandParent()->m_Right.get();
				uncle->SetIsRed(m_CurrentVersion, false);
				getGrandParent()->SetIsRed(m_CurrentVersion, true);
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
					pst::PersistentMapNode<TKey, TValue>* willBeNewParent = fixNode->m_Right.get();
					LeftRotate(fixNode, getParent());
					parents.push_back(willBeNewParent);
				}

				// Case 3. No else intended
				getParent()->SetIsRed(m_CurrentVersion, false);
				getGrandParent()->SetIsRed(m_CurrentVersion, true);

				// Clone needed node before rotation, remember what node is being rotated and then restore parents after rotation
				getGrandParent()->m_Left = getGrandParent()->m_Left->Clone(m_CurrentVersion);
				pst::PersistentMapNode<TKey, TValue>* willBeNewParent = getGrandParent()->m_Left.get();
				RightRotate(getGrandParent(), parents[parents.size() - 3]);
				parents.push_back(willBeNewParent);

				// Rotation should always terminate loop
				assert(!getParent()->IsRed());
			}
		}
		else
		{
			// TODO: Try to find way to avoid this symmetric logic. Same for DeleteFixup!

			pst::PersistentMapNode<TKey, TValue>* uncle = getGrandParent()->m_Left.get();
			if (uncle && uncle->IsRed())
			{
				// Case 1
				getParent()->SetIsRed(m_CurrentVersion, false);
				getGrandParent()->m_Left = uncle->Clone(m_CurrentVersion);
				uncle = getGrandParent()->m_Left.get();
				uncle->SetIsRed(m_CurrentVersion, false);
				getGrandParent()->SetIsRed(m_CurrentVersion, true);
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
					pst::PersistentMapNode<TKey, TValue>* willBeNewParent = fixNode->m_Left.get();
					RightRotate(fixNode, getParent());
					parents.push_back(willBeNewParent);
				}

				// Case 3. No else intended
				getParent()->SetIsRed(m_CurrentVersion, false);
				getGrandParent()->SetIsRed(m_CurrentVersion, true);

				// Clone needed node before rotation, remember what node is being rotated and then restore parents after rotation
				getGrandParent()->m_Right = getGrandParent()->m_Right->Clone(m_CurrentVersion);
				pst::PersistentMapNode<TKey, TValue>* willBeNewParent = getGrandParent()->m_Right.get();
				LeftRotate(getGrandParent(), parents[parents.size() - 3]);
				parents.push_back(willBeNewParent);

				// Rotation should always terminate loop
				assert(!getParent()->IsRed());
			}
		}
	}

	GetRoot()->SetIsRed(m_CurrentVersion, false);
}

template<typename TKey, typename TValue>
inline std::vector<pst::PersistentMapNode<TKey, TValue>*> pst::PersistentMap<TKey, TValue>::BuildPath(pst::PersistentMapNode<TKey, TValue>* toNode)
{
	// TODO: Try to avoid duplicating logic with const-method
	assert(toNode);
	std::vector<pst::PersistentMapNode<TKey, TValue>*> path;

	// Parent of root is always nullptr
	path.push_back(nullptr);
	pst::PersistentMapNode<TKey, TValue>* node = GetRoot();
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
		return std::vector<pst::PersistentMapNode<TKey, TValue>*>();
	}

	return path;
}

template<typename TKey, typename TValue>
inline std::vector<const pst::PersistentMapNode<TKey, TValue>*> pst::PersistentMap<TKey, TValue>::BuildPath(const pst::PersistentMapNode<TKey, TValue>* toNode) const
{
	assert(toNode);
	std::vector<const pst::PersistentMapNode<TKey, TValue>*> path;

	// Parent of root is always nullptr
	path.push_back(nullptr);
	const pst::PersistentMapNode<TKey, TValue>* node = GetRoot();
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
		return std::vector<const pst::PersistentMapNode<TKey, TValue>*>();
	}

	return path;
}

template<typename TKey, typename TValue>
inline void pst::PersistentMap<TKey, TValue>::RightRotate(pst::PersistentMapNode<TKey, TValue>* target, pst::PersistentMapNode<TKey, TValue>* targetParent)
{
	// TODO: Unite Left and Right rotate functions?

	// Important to keep shared_ptrs there. This way object won't be removed during swapping pointers
	std::shared_ptr<pst::PersistentMapNode<TKey, TValue>> targetNode = GetSharedPtr(target, targetParent);
	std::shared_ptr<pst::PersistentMapNode<TKey, TValue>> childNode = target->m_Left;
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
inline void pst::PersistentMap<TKey, TValue>::LeftRotate(pst::PersistentMapNode<TKey, TValue>* target, pst::PersistentMapNode<TKey, TValue>* targetParent)
{
	// Important to keep shared_ptr there. This way object won't be removed during swapping pointers
	std::shared_ptr<pst::PersistentMapNode<TKey, TValue>> targetNode = GetSharedPtr(target, targetParent);
	std::shared_ptr<pst::PersistentMapNode<TKey, TValue>> childNode = target->m_Right;
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
inline std::shared_ptr<pst::PersistentMapNode<TKey, TValue>> pst::PersistentMap<TKey, TValue>::GetSharedPtr(pst::PersistentMapNode<TKey, TValue>* target, pst::PersistentMapNode<TKey, TValue>* targetParent)
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
inline void pst::PersistentMap<TKey, TValue>::DeleteFixup(pst::PersistentMapNode<TKey, TValue>* fixNode, pst::PersistentMapNode<TKey, TValue>* parentForNullNode)
{
	// All parents has been cloned already. Sublings has not.
	std::vector<pst::PersistentMapNode<TKey, TValue>*> parents = fixNode ? BuildPath(fixNode) : BuildPath(parentForNullNode);
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
			pst::PersistentMapNode<TKey, TValue>* subling = getParent()->m_Right.get();
			if (subling && subling->IsRed())
			{
				// Case 1
				getParent()->m_Right = getParent()->m_Right->Clone(m_CurrentVersion);
				subling = getParent()->m_Right.get();
				subling->SetIsRed(m_CurrentVersion, false);
				getParent()->SetIsRed(m_CurrentVersion, true);
				LeftRotate(getParent(), getGrandParent());
				
				// Restore parents
				pst::PersistentMapNode<TKey, TValue>* parent = parents.back();
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
				subling->SetIsRed(m_CurrentVersion, true);
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
					subling->m_Left->SetIsRed(m_CurrentVersion, false);
					subling->SetIsRed(m_CurrentVersion, true);
					RightRotate(subling, getParent());

					// Find new subling
					subling = getParent()->m_Right.get();
				}

				// Case 4. No else intended
				getParent()->m_Right = getParent()->m_Right->Clone(m_CurrentVersion);
				subling = getParent()->m_Right.get();
				subling->m_Right = subling->m_Right->Clone(m_CurrentVersion);
				subling->SetIsRed(m_CurrentVersion, getParent()->IsRed());
				getParent()->SetIsRed(m_CurrentVersion, false);
				subling->m_Right->SetIsRed(m_CurrentVersion, false);
				LeftRotate(getParent(), getGrandParent());

				// Restore parents
				pst::PersistentMapNode<TKey, TValue>* parent = parents.back();
				parents.pop_back();
				parents.push_back(subling);
				parents.push_back(parent);

				fixNode = GetRoot();
			}
		}
		else
		{
			pst::PersistentMapNode<TKey, TValue>* subling = getParent()->m_Left.get();
			if (subling && subling->IsRed())
			{
				// Case 1
				getParent()->m_Left = getParent()->m_Left->Clone(m_CurrentVersion);
				subling = getParent()->m_Left.get();
				subling->SetIsRed(m_CurrentVersion, false);
				getParent()->SetIsRed(m_CurrentVersion, true);
				RightRotate(getParent(), getGrandParent());

				// Restore parents
				pst::PersistentMapNode<TKey, TValue>* parent = parents.back();
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
				subling->SetIsRed(m_CurrentVersion, true);
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
					subling->m_Right->SetIsRed(m_CurrentVersion, false);
					subling->SetIsRed(m_CurrentVersion, true);
					LeftRotate(subling, getParent());

					// Find new subling
					subling = getParent()->m_Left.get();
				}

				// Case 4. No else intended
				getParent()->m_Left = getParent()->m_Left->Clone(m_CurrentVersion);
				subling = getParent()->m_Left.get();
				subling->m_Left = subling->m_Left->Clone(m_CurrentVersion);
				subling->SetIsRed(m_CurrentVersion, getParent()->IsRed());
				getParent()->SetIsRed(m_CurrentVersion, false);
				subling->m_Left->SetIsRed(m_CurrentVersion, false);
				RightRotate(getParent(), getGrandParent());

				// Restore parents
				pst::PersistentMapNode<TKey, TValue>* parent = parents.back();
				parents.pop_back();
				parents.push_back(subling);
				parents.push_back(parent);

				fixNode = GetRoot();
			}
		}
	}

	// Always root node - so it should be cloned already
	fixNode->SetIsRed(m_CurrentVersion, false);
}

template<typename TKey, typename TValue>
inline std::tuple<std::shared_ptr<pst::PersistentMapNode<TKey, TValue>>, pst::PersistentMapNode<TKey, TValue>*> pst::PersistentMap<TKey, TValue>::ClonePath(
	const pst::PersistentMapNode<TKey, TValue>* from, const TKey& toKey) const
{
	assert(from->m_Key != toKey);
	std::shared_ptr<pst::PersistentMapNode<TKey, TValue>> newFrom = from->Clone(m_CurrentVersion);
	pst::PersistentMapNode<TKey, TValue>* newNode = newFrom.get();
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
inline void pst::PersistentMap<TKey, TValue>::Transplant(const pst::PersistentMapNode<TKey, TValue>* target, pst::PersistentMapNode<TKey, TValue>* targetParent,
	std::shared_ptr<pst::PersistentMapNode<TKey, TValue>> source)
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
inline const pst::PersistentMapNode<TKey, TValue>* pst::PersistentMap<TKey, TValue>::Search(const pst::PersistentMapNode<TKey, TValue>* node, const TKey& key) const
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
inline pst::PersistentMapNode<TKey, TValue>* pst::PersistentMap<TKey, TValue>::Search(pst::PersistentMapNode<TKey, TValue>* node, const TKey& key)
{
	return const_cast<pst::PersistentMapNode<TKey, TValue>*>(const_cast<const pst::PersistentMap<TKey, TValue>&>(*this).Search(node, key));
}

template<typename TKey, typename TValue>
inline const pst::PersistentMapNode<TKey, TValue>* pst::PersistentMap<TKey, TValue>::GetMin(const pst::PersistentMapNode<TKey, TValue>* node) const
{
	while (node->m_Left)
	{
		node = node->m_Left.get();
	}

	return node;
}

template<typename TKey, typename TValue>
inline pst::PersistentMapNode<TKey, TValue>* pst::PersistentMap<TKey, TValue>::GetMin(pst::PersistentMapNode<TKey, TValue>* node)
{
	return const_cast<pst::PersistentMapNode<TKey, TValue>*>(const_cast<const pst::PersistentMap<TKey, TValue>&>(*this).GetMin(node));
}

template<typename TKey, typename TValue>
inline const pst::PersistentMapNode<TKey, TValue>* pst::PersistentMap<TKey, TValue>::GetMax(const pst::PersistentMapNode<TKey, TValue>* node) const
{
	while (node->m_Right)
	{
		node = node->m_Right.get();
	}

	return node;
}

template<typename TKey, typename TValue>
inline pst::PersistentMapNode<TKey, TValue>* pst::PersistentMap<TKey, TValue>::GetMax(pst::PersistentMapNode<TKey, TValue>* node)
{
	return const_cast<pst::PersistentMapNode<TKey, TValue>*>(const_cast<const pst::PersistentMap<TKey, TValue>&>(*this).GetMax(node));
}

template<typename TKey, typename TValue>
inline pst::PersistentMapNode<TKey, TValue>* pst::PersistentMap<TKey, TValue>::GetMinParent(pst::PersistentMapNode<TKey, TValue>* node)
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
