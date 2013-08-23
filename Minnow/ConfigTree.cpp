/*
 Minnow Pacemaker client firmware.
    
 Copyright (C) 2013 Robert Fairlie-Cuninghame

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ConfigTree.h"
#include "ConfigTreeNode.h"

#include "Minnow.h"

//
// Public Methods
//

ConfigurationTree::ConfigurationTree()
{
  node_array[0].SetAsRootNode();
}

ConfigurationTreeNode *
ConfigurationTree::FindNode(const char *name)
{
  ConfigurationTreeNode *node = &node_array[0];
  while (true)
  {
    ConfigurationTreeNode *child = GetFirstChild(node, false);
    while (child != 0)
    {
      int8_t child_name_length = child->CompareName(name);
      if (child_name_length > 0)
      {
        if (name[child_name_length] == '\0')
        {
          // found a match but just to be safe, clear child of match
          // in case its not a leaf node
          node = child;
          child = node + 1;
          if ((uint8_t*)node < (uint8_t*)node_array + sizeof(node_array))
            child->Clear();
          return node;
        }
        else if (name[child_name_length] == '.')
        {
          name += (child_name_length + 1);
          node = child;
          break;
        }
      }
      child = GetNextChild(node, false);
    }
    if (child == 0)
      return 0;
    node = child;
  }
}

ConfigurationTreeNode *
ConfigurationTree::FindFirstLeafNode(const ConfigurationTreeNode *search_root_node, bool inUseOnly)
{
  if ((uint8_t*)search_root_node >= (uint8_t*)node_array + sizeof(node_array) - sizeof(ConfigurationTreeNode))
    return 0;
  ConfigurationTreeNode *child = (ConfigurationTreeNode *)search_root_node + 1;;
  child->Clear(); 
  return FindNextLeafNode(search_root_node, inUseOnly);
}

ConfigurationTreeNode *
ConfigurationTree::FindNextLeafNode(const ConfigurationTreeNode *search_root_node, bool inUseOnly)
{
  // return previous previous position
  ConfigurationTreeNode *node = (ConfigurationTreeNode *)search_root_node;
  ConfigurationTreeNode *child;
  do
  {
    child = GetCurrentChild(node);
    if (child != 0)
      node = child;
  }
  while (child != 0);

  bool firstNode = true;
  while (true)
  {
    if (!(firstNode && node->IsLeafNode()))
    {
      // traverse down tree to find first leaf node
      while ((child = GetFirstChild(node, inUseOnly)) != 0)
      {
        firstNode = false;
        node = child;
      }
      // is this a leaf node? (remember you can reach a terminus of the search which is not a leaf node)
      if (node->IsLeafNode())
        return node;
    }
    firstNode = false;
    
    // keep traversing up tree and locating next child
    do
    {
      if (node == search_root_node)    
        return 0;
      node = GetParentNode(node);
    }
    while ((child = GetNextChild(node, inUseOnly)) == 0);
    node = child;
  }
}


ConfigurationTreeNode *
ConfigurationTree::GetParentNode(const ConfigurationTreeNode *node) const
{
  if ((uint8_t*)node < (uint8_t*)node_array + sizeof(ConfigurationTreeNode) 
      || (uint8_t*)node >= (uint8_t*)node_array + sizeof(node_array))
    return 0;
 return ((ConfigurationTreeNode *)node) - 1;
}

int8_t ConfigurationTree::GetFullName(const ConfigurationTreeNode *target_node, bool &inUse, char *buffer, uint8_t length)
{
  const ConfigurationTreeNode *node = GetCurrentChild(GetRootNode());
  uint8_t used = 0;

  inUse = true;
  
  while (node != 0)
  {
    used += node->GetName(&buffer[used], length - used);
    inUse &= node->IsInUse();
    if (used < length && node != target_node)
      buffer[used++] = '.';
    else if (used >= length)
      return -1;
    else
      return (int8_t)((used > 127) ? 127 : used);
    node=GetCurrentChild(node);    
  }
  if (used < length)
    buffer[used] = '\0';
  return 0;
}


//
// Private Methods
//

ConfigurationTreeNode *
ConfigurationTree::GetFirstChild(const ConfigurationTreeNode *node, bool inUseOnly)
{
  if ((uint8_t*)node >= (uint8_t*)node_array + sizeof(node_array) - sizeof(ConfigurationTreeNode))
    return 0;
  
  ConfigurationTreeNode *child = (ConfigurationTreeNode *)node + 1;
  child->Clear();
  while (node->InitializeNextChild(*child))
  {
    if (inUseOnly && !child->IsInUse())
      continue;
    return child;
  }
  return 0;
}

ConfigurationTreeNode *
ConfigurationTree::GetNextChild(const ConfigurationTreeNode *node, bool inUseOnly)
{
  if ((uint8_t*)node >= (uint8_t*)node_array + sizeof(node_array) - sizeof(ConfigurationTreeNode))
    return 0;
  
  ConfigurationTreeNode *child = (ConfigurationTreeNode *)node + 1;
  while (node->InitializeNextChild(*child))
  {
    if (inUseOnly && !child->IsInUse())
      continue;
    return child;
  }
  return 0;
}

ConfigurationTreeNode *
ConfigurationTree::GetCurrentChild(const ConfigurationTreeNode *node)
{
  if ((uint8_t*)node >= (uint8_t*)node_array + sizeof(node_array) - sizeof(ConfigurationTreeNode))
    return 0;

  ConfigurationTreeNode *child = (ConfigurationTreeNode *)node + 1;
  if (child->IsValid())
    return child;
  else
    return 0;
}


