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

#ifndef CONFIG_TREE_H 
#define CONFIG_TREE_H
 
#include "config.h"
#include <stdint.h>

#include "ConfigTreeNode.h"

// This configured the maximum depth of command tree.
// e.g., "devices.foo.0.bar.5.name" is 6 levels deep.
#define MAX_CONFIGURATION_TREE_DEPTH          8

//
// Configuration Matching Tree class
//
// Implements configuration tree search and traversal without using
// virtual functions or recusion to minimize RAM requirements.
//
class ConfigurationTree
{
public:

  ConfigurationTree();

  ConfigurationTreeNode *FindNode(const char *name);
  ConfigurationTreeNode *FindFirstLeafNode(const ConfigurationTreeNode *search_root_node);
  ConfigurationTreeNode *FindNextLeafNode(const ConfigurationTreeNode *search_root_node);

  ConfigurationTreeNode *GetRootNode() { return &node_array[0]; };

  ConfigurationTreeNode *GetParentNode(const ConfigurationTreeNode *node) const;

  int8_t GetFullName(const ConfigurationTreeNode *node, char *buffer, uint8_t length);
  
private:

  ConfigurationTreeNode *GetFirstChild(const ConfigurationTreeNode *node);
  ConfigurationTreeNode *GetNextChild(const ConfigurationTreeNode *node);
  ConfigurationTreeNode *GetCurrentChild(const ConfigurationTreeNode *node);

  ConfigurationTreeNode node_array[MAX_CONFIGURATION_TREE_DEPTH+1]; // extra node accounts for root node
};

#endif