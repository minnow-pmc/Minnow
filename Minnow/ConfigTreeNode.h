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

#ifndef CONFIG_TREE_NODE_H 
#define CONFIG_TREE_NODE_H
 
#include <stdint.h>

//
// Configuration Tree Node class
//
// Implements configuration tree search and traversal without using
// virtual functions or recusion to minimize RAM requirements.
//
class ConfigurationTreeNode
{
public:

// internal node types
#define NODE_TYPE_INVALID               0
#define NODE_TYPE_CONFIG_ROOT                  1

// top level groups
#define NODE_TYPE_GROUP_SYSTEM          2
#define NODE_TYPE_GROUP_DEVICES         3
#define NODE_TYPE_GROUP_STATISTICS      4
#define NODE_TYPE_GROUP_DEBUG     5

// device types
#define NODE_TYPE_CONFIG_DEVICE_INPUT_SWITCHES   20
#define NODE_TYPE_CONFIG_DEVICE_OUTPUT_SWITCHES  21
#define NODE_TYPE_CONFIG_DEVICE_PWM_OUTPUTS     22
#define NODE_TYPE_CONFIG_DEVICE_HEATERS         23
#define NODE_TYPE_CONFIG_DEVICE_BUZZERS         24

// device type instances
#define NODE_TYPE_CONFIG_DEVICE_INSTANCE_INPUT_SWITCH   40
#define NODE_TYPE_CONFIG_DEVICE_INSTANCE_OUTPUT_SWITCH  41
#define NODE_TYPE_CONFIG_DEVICE_INSTANCE_PWM_OUTPUT     42
#define NODE_TYPE_CONFIG_DEVICE_INSTANCE_HEATER         43
#define NODE_TYPE_CONFIG_DEVICE_INSTANCE_BUZZER         44

// device attributes
#define NODE_TYPE_CONFIG_LEAF_INPUT_SWITCH_FRIENDLY_NAME  60
#define NODE_TYPE_CONFIG_LEAF_INPUT_SWITCH_PIN            61

#define NODE_TYPE_CONFIG_LEAF_OUTPUT_SWITCH_FRIENDLY_NAME 62
#define NODE_TYPE_CONFIG_LEAF_OUTPUT_SWITCH_PIN           63

#define NODE_TYPE_CONFIG_LEAF_PWM_OUTPUT_FRIENDLY_NAME    64
#define NODE_TYPE_CONFIG_LEAF_PWM_OUTPUT_PIN              65

#define NODE_TYPE_CONFIG_LEAF_BUZZER_FRIENDLY_NAME        66
#define NODE_TYPE_CONFIG_LEAF_BUZZER_PIN                  67

#define NODE_TYPE_CONFIG_LEAF_HEATER_FRIENDLY_NAME        68
#define NODE_TYPE_CONFIG_LEAF_HEATER_PIN                  69

// System configuration   
#define NODE_TYPE_CONFIG_LEAF_SYSTEM_HARDWARE_NAME        180
#define NODE_TYPE_CONFIG_LEAF_SYSTEM_HARDWARE_TYPE        181
#define NODE_TYPE_CONFIG_LEAF_SYSTEM_HARDWARE_REV         182
#define NODE_TYPE_CONFIG_LEAF_SYSTEM_BOARD_IDENTITY       183
#define NODE_TYPE_CONFIG_LEAF_SYSTEM_BOARD_SERIAL_NUM     184


// Statistics
#define NODE_TYPE_STATS_LEAF_RX_PACKET_COUNT        200
#define NODE_TYPE_STATS_LEAF_RX_ERROR_COUNT         201
#define NODE_TYPE_STATS_LEAF_QUEUE_MEMORY           202

#define NODE_TYPE_DEBUG_LEAF_STACK_MEMORY           203
#define NODE_TYPE_DEBUG_LEAF_STACK_LOW_WATER_MARK   204

//
// Other defines

#define INVALID_INSTANCE_ID  0xFF
#define INVALID_NODE_INFO_INDEX 0xFF

#define LEAF_CLASS_INVALID     0xFF
#define LEAF_CLASS_CONFIG      0
#define LEAF_CLASS_STATUS      1
#define LEAF_CLASS_STATISTICS  2
#define LEAF_CLASS_FUNCTION    3
#define LEAF_CLASS_DIAGNOSTICS 4

// Operations allows on leaf nodes

#define LEAF_OPERATIONS_INVALID       0
#define LEAF_OPERATIONS_READABLE      1
#define LEAF_OPERATIONS_WRITABLE      2

#define LEAF_OPERATIONS_READWRITABLE  (LEAF_OPERATIONS_READABLE | LEAF_OPERATIONS_WRITABLE)

// Data types of leaf nodes

#define LEAF_SET_DATATYPE_INVALID   0
#define LEAF_SET_DATATYPE_UINT8     1
#define LEAF_SET_DATATYPE_INT16     2
#define LEAF_SET_DATATYPE_BOOL      3
#define LEAF_SET_DATATYPE_STRING    4


  uint8_t GetName(char *buffer, uint8_t buffer_length) const;
  int8_t CompareName(const char *str) const;
  
  uint8_t GetNodeType() const { return node_type; };

  uint8_t GetInstanceId() const { return instance_id; };
  
  bool IsValid() const { return (node_info_index != INVALID_NODE_INFO_INDEX); };
  bool IsInstanceNode() const { return (instance_id != INVALID_INSTANCE_ID); };
  bool IsLeafNode() const { return GetLeafClass() != LEAF_CLASS_INVALID; };
  bool IsInUse() const;

  uint8_t GetLeafClass() const;
  uint8_t GetLeafOperations() const;
  uint8_t GetLeafSetDataType() const;
  
private:

  friend class ConfigurationTree;

  ConfigurationTreeNode() { Clear(); }

  bool InitializeNextChild(ConfigurationTreeNode &child) const;

  void SetAsRootNode(); 
  void Clear();
  uint8_t FindNodeInfoIndex(uint8_t) const;

  uint8_t node_type;
  uint8_t node_info_index; 
  uint8_t instance_id; 
};

#endif