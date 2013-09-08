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

//
// Implementation of ConfigurationTreeNode class for navigating the 
// configuration tree. 
 
#include "ConfigTreeNode.h"

#include "Minnow.h"

#include "Device_InputSwitch.h"
#include "Device_OutputSwitch.h"
#include "Device_PwmOutput.h"
#include "Device_Buzzer.h"
#include "Device_Heater.h"
#include "Device_TemperatureSensor.h"

#include <avr/pgmspace.h>
#include "language.h"

//
// Internal Configuration Structures (defines tree structure)
//

typedef struct _ConfigNodeInfo
{
  uint8_t node_type;
  const char *name; // or 0 if this is an instance node
  const uint8_t *named_child_types; 
  uint8_t instance_child_type;
  uint8_t (*num_children)(); // stores either a functor (for instance node) or the raw count (for group nodes)
  bool (*child_in_use_functor)(uint8_t);
  uint8_t leaf_node_class;
  uint8_t leaf_operations; 
  uint8_t leaf_datatype; // the datatype is only needed for writable leaf nodes
} ConfigNodeInfo;

typedef bool (*in_use_functor_type)(uint8_t);
typedef uint8_t (*num_children_functor_type)();


// Initializer macro for named non-leaf nodes with named children
#define GROUP_NODE(node_type) \
    { node_type, name_of_##node_type, children_of_##node_type, NODE_TYPE_INVALID, \
      (num_children_functor_type)sizeof(children_of_##node_type), 0, LEAF_CLASS_INVALID, \
      LEAF_OPERATIONS_INVALID, LEAF_SET_DATATYPE_INVALID }

// Initializer macro for unnamed non-leaf nodes with named children
#define UNNAMED_GROUP_NODE(node_type) \
    { node_type, 0, children_of_##node_type, NODE_TYPE_INVALID, \
      (num_children_functor_type)sizeof(children_of_##node_type), 0, LEAF_CLASS_INVALID, \
      LEAF_OPERATIONS_INVALID, LEAF_SET_DATATYPE_INVALID }

// Initializer macro for named non-leaf nodes with instance children
#define INSTANCE_CHILDREN_NODE(node_type, child_type, num_children_functor, is_in_use_functor) \
    { node_type, name_of_##node_type, 0, child_type, num_children_functor, is_in_use_functor, \
      LEAF_CLASS_INVALID, LEAF_OPERATIONS_INVALID, LEAF_SET_DATATYPE_INVALID }

// Initializer macro for leaf nodes
#define LEAF_NODE(node_type, leaf_node_class, operations, data_type) \
    { node_type, name_of_##node_type, 0, NODE_TYPE_INVALID, 0, 0, \
      leaf_node_class, operations, data_type }

// Configuration node names
PROGMEM static const char pstr_PIN[] = CONFIG_STR(PIN);
PROGMEM static const char pstr_NAME[] = CONFIG_STR(NAME);
PROGMEM static const char pstr_TYPE[] = CONFIG_STR(TYPE);
PROGMEM static const char pstr_USE_SOFT_PWM[] = CONFIG_STR(USE_SOFT_PWM);

PROGMEM static const char name_of_NODE_TYPE_CONFIG_ROOT[] = "";
PROGMEM static const char name_of_NODE_TYPE_GROUP_SYSTEM[] = CONFIG_STR(SYSTEM);
PROGMEM static const char name_of_NODE_TYPE_GROUP_DEVICES[] = CONFIG_STR(DEVICES);
PROGMEM static const char name_of_NODE_TYPE_GROUP_STATISTICS[] = CONFIG_STR(STATS);
PROGMEM static const char name_of_NODE_TYPE_GROUP_DEBUG[] = CONFIG_STR(DEBUG);

PROGMEM static const char name_of_NODE_TYPE_CONFIG_LEAF_SYSTEM_HARDWARE_NAME[] = CONFIG_STR(HARDWARE_NAME);
PROGMEM static const char name_of_NODE_TYPE_CONFIG_LEAF_SYSTEM_HARDWARE_TYPE[] = CONFIG_STR(HARDWARE_TYPE);
PROGMEM static const char name_of_NODE_TYPE_CONFIG_LEAF_SYSTEM_HARDWARE_REV[] = CONFIG_STR(HARDWARE_REV);
PROGMEM static const char name_of_NODE_TYPE_CONFIG_LEAF_SYSTEM_BOARD_IDENTITY[] = CONFIG_STR(BOARD_IDENTITY);
PROGMEM static const char name_of_NODE_TYPE_CONFIG_LEAF_SYSTEM_BOARD_SERIAL_NUM[] = CONFIG_STR(BOARD_SERIAL_NUM);
PROGMEM static const char name_of_NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_INPUT_SWITCHES[] = CONFIG_STR(NUM_DIGITAL_INPUTS);
PROGMEM static const char name_of_NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_OUTPUT_SWITCHES[] = CONFIG_STR(NUM_DIGITAL_OUTPUTS);
PROGMEM static const char name_of_NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_PWM_OUTPUTS[] = CONFIG_STR(NUM_PWM_OUTPUTS);
PROGMEM static const char name_of_NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_BUZZERS[] = CONFIG_STR(NUM_BUZZERS);
PROGMEM static const char name_of_NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_HEATERS[] = CONFIG_STR(NUM_HEATERS);
PROGMEM static const char name_of_NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_TEMP_SENSORS[] = CONFIG_STR(NUM_TEMP_SENSORS);

PROGMEM static const char name_of_NODE_TYPE_STATS_LEAF_RX_PACKET_COUNT[] = CONFIG_STR(RX_COUNT);
PROGMEM static const char name_of_NODE_TYPE_STATS_LEAF_RX_ERROR_COUNT[] = CONFIG_STR(RX_ERROR);
PROGMEM static const char name_of_NODE_TYPE_STATS_LEAF_QUEUE_MEMORY[] = CONFIG_STR(QUEUE_MEMORY);

PROGMEM static const char name_of_NODE_TYPE_DEBUG_LEAF_STACK_MEMORY[] = CONFIG_STR(STACK_MEMORY);
PROGMEM static const char name_of_NODE_TYPE_DEBUG_LEAF_STACK_LOW_WATER_MARK[] = CONFIG_STR(STACK_LOW_WATER_MARK);

PROGMEM static const char name_of_NODE_TYPE_CONFIG_DEVICE_INPUT_SWITCHES[] = CONFIG_STR(DIGITAL_INPUT);
PROGMEM static const char name_of_NODE_TYPE_CONFIG_DEVICE_OUTPUT_SWITCHES[] = CONFIG_STR(DIGITAL_OUTPUT);
PROGMEM static const char name_of_NODE_TYPE_CONFIG_DEVICE_PWM_OUTPUTS[] = CONFIG_STR(PWM_OUTPUT);
PROGMEM static const char name_of_NODE_TYPE_CONFIG_DEVICE_BUZZERS[] = CONFIG_STR(BUZZER);
PROGMEM static const char name_of_NODE_TYPE_CONFIG_DEVICE_TEMP_SENSORS[] = CONFIG_STR(TEMP_SENSOR);
PROGMEM static const char name_of_NODE_TYPE_CONFIG_DEVICE_HEATERS[] = CONFIG_STR(HEATER);

// ALIASES to generic attribute names
#define name_of_NODE_TYPE_CONFIG_LEAF_INPUT_SWITCH_FRIENDLY_NAME pstr_NAME
#define name_of_NODE_TYPE_CONFIG_LEAF_INPUT_SWITCH_PIN pstr_PIN
#define name_of_NODE_TYPE_CONFIG_LEAF_OUTPUT_SWITCH_FRIENDLY_NAME pstr_NAME
#define name_of_NODE_TYPE_CONFIG_LEAF_OUTPUT_SWITCH_PIN pstr_PIN
#define name_of_NODE_TYPE_CONFIG_LEAF_PWM_OUTPUT_FRIENDLY_NAME pstr_NAME
#define name_of_NODE_TYPE_CONFIG_LEAF_PWM_OUTPUT_PIN pstr_PIN
#define name_of_NODE_TYPE_CONFIG_LEAF_PWM_OUTPUT_USE_SOFT_PWM pstr_USE_SOFT_PWM
#define name_of_NODE_TYPE_CONFIG_LEAF_BUZZER_FRIENDLY_NAME pstr_NAME
#define name_of_NODE_TYPE_CONFIG_LEAF_BUZZER_PIN pstr_PIN
#define name_of_NODE_TYPE_CONFIG_LEAF_BUZZER_USE_SOFT_PWM pstr_USE_SOFT_PWM
#define name_of_NODE_TYPE_CONFIG_LEAF_TEMP_SENSOR_FRIENDLY_NAME pstr_NAME
#define name_of_NODE_TYPE_CONFIG_LEAF_TEMP_SENSOR_PIN pstr_PIN
#define name_of_NODE_TYPE_CONFIG_LEAF_TEMP_SENSOR_TYPE pstr_TYPE
#define name_of_NODE_TYPE_CONFIG_LEAF_HEATER_FRIENDLY_NAME pstr_NAME
#define name_of_NODE_TYPE_CONFIG_LEAF_HEATER_PIN pstr_PIN
#define name_of_NODE_TYPE_CONFIG_LEAF_HEATER_USE_SOFT_PWM pstr_USE_SOFT_PWM


// Arrays of named children
PROGMEM static const uint8_t children_of_NODE_TYPE_CONFIG_ROOT[] = 
{
  NODE_TYPE_GROUP_SYSTEM,
  NODE_TYPE_GROUP_DEVICES,
  NODE_TYPE_GROUP_STATISTICS,
  NODE_TYPE_GROUP_DEBUG
};

PROGMEM static const uint8_t children_of_NODE_TYPE_GROUP_SYSTEM[] = 
{
  NODE_TYPE_CONFIG_LEAF_SYSTEM_HARDWARE_NAME,
  NODE_TYPE_CONFIG_LEAF_SYSTEM_HARDWARE_TYPE,
  NODE_TYPE_CONFIG_LEAF_SYSTEM_HARDWARE_REV,
  NODE_TYPE_CONFIG_LEAF_SYSTEM_BOARD_IDENTITY,
  NODE_TYPE_CONFIG_LEAF_SYSTEM_BOARD_SERIAL_NUM,
  NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_INPUT_SWITCHES,
  NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_OUTPUT_SWITCHES,
  NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_PWM_OUTPUTS,
  NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_BUZZERS,
  NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_HEATERS,
  NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_TEMP_SENSORS
};
PROGMEM static const uint8_t children_of_NODE_TYPE_GROUP_DEVICES[] = 
{
  NODE_TYPE_CONFIG_DEVICE_INPUT_SWITCHES,
  NODE_TYPE_CONFIG_DEVICE_OUTPUT_SWITCHES,
  NODE_TYPE_CONFIG_DEVICE_PWM_OUTPUTS,
  NODE_TYPE_CONFIG_DEVICE_BUZZERS,
  NODE_TYPE_CONFIG_DEVICE_TEMP_SENSORS,
  NODE_TYPE_CONFIG_DEVICE_HEATERS
};
PROGMEM static const uint8_t children_of_NODE_TYPE_GROUP_STATISTICS[] = 
{
  NODE_TYPE_STATS_LEAF_RX_PACKET_COUNT,
  NODE_TYPE_STATS_LEAF_RX_ERROR_COUNT,
  NODE_TYPE_STATS_LEAF_QUEUE_MEMORY
};
PROGMEM static const uint8_t children_of_NODE_TYPE_GROUP_DEBUG[] = 
{
  NODE_TYPE_DEBUG_LEAF_STACK_MEMORY,
  NODE_TYPE_DEBUG_LEAF_STACK_LOW_WATER_MARK
};
PROGMEM static const uint8_t children_of_NODE_TYPE_CONFIG_DEVICE_INSTANCE_INPUT_SWITCH[] = 
{
  NODE_TYPE_CONFIG_LEAF_INPUT_SWITCH_FRIENDLY_NAME,
  NODE_TYPE_CONFIG_LEAF_INPUT_SWITCH_PIN
};
PROGMEM static const uint8_t children_of_NODE_TYPE_CONFIG_DEVICE_INSTANCE_OUTPUT_SWITCH[] = 
{
  NODE_TYPE_CONFIG_LEAF_OUTPUT_SWITCH_FRIENDLY_NAME,
  NODE_TYPE_CONFIG_LEAF_OUTPUT_SWITCH_PIN
};
PROGMEM static const uint8_t children_of_NODE_TYPE_CONFIG_DEVICE_INSTANCE_PWM_OUTPUT[] = 
{
  NODE_TYPE_CONFIG_LEAF_PWM_OUTPUT_FRIENDLY_NAME,
  NODE_TYPE_CONFIG_LEAF_PWM_OUTPUT_PIN,
  NODE_TYPE_CONFIG_LEAF_PWM_OUTPUT_USE_SOFT_PWM
};
PROGMEM static const uint8_t children_of_NODE_TYPE_CONFIG_DEVICE_INSTANCE_BUZZER[] = 
{
  NODE_TYPE_CONFIG_LEAF_BUZZER_FRIENDLY_NAME,
  NODE_TYPE_CONFIG_LEAF_BUZZER_PIN,
  NODE_TYPE_CONFIG_LEAF_BUZZER_USE_SOFT_PWM
};
PROGMEM static const uint8_t children_of_NODE_TYPE_CONFIG_DEVICE_INSTANCE_TEMP_SENSOR[] = 
{
  NODE_TYPE_CONFIG_LEAF_TEMP_SENSOR_FRIENDLY_NAME,
  NODE_TYPE_CONFIG_LEAF_TEMP_SENSOR_PIN,
  NODE_TYPE_CONFIG_LEAF_TEMP_SENSOR_TYPE
};
PROGMEM static const uint8_t children_of_NODE_TYPE_CONFIG_DEVICE_INSTANCE_HEATER[] = 
{
  NODE_TYPE_CONFIG_LEAF_HEATER_FRIENDLY_NAME,
  NODE_TYPE_CONFIG_LEAF_HEATER_PIN,
  NODE_TYPE_CONFIG_LEAF_HEATER_USE_SOFT_PWM
};
//
// Configuration Tree Definition
//
PROGMEM static const ConfigNodeInfo node_info_array[] = 
{
  //
  // Config Related Nodes
  //

  GROUP_NODE(NODE_TYPE_CONFIG_ROOT),

  // First level groups
  GROUP_NODE(NODE_TYPE_GROUP_SYSTEM),
  GROUP_NODE(NODE_TYPE_GROUP_DEVICES),
  GROUP_NODE(NODE_TYPE_GROUP_STATISTICS),
  GROUP_NODE(NODE_TYPE_GROUP_DEBUG),
    
  // Device Type Nodes
  INSTANCE_CHILDREN_NODE(NODE_TYPE_CONFIG_DEVICE_INPUT_SWITCHES, 
      NODE_TYPE_CONFIG_DEVICE_INSTANCE_INPUT_SWITCH, 
      Device_InputSwitch::GetNumDevices, Device_InputSwitch::IsInUse), 
  INSTANCE_CHILDREN_NODE(NODE_TYPE_CONFIG_DEVICE_OUTPUT_SWITCHES,
      NODE_TYPE_CONFIG_DEVICE_INSTANCE_OUTPUT_SWITCH, 
      Device_OutputSwitch::GetNumDevices, Device_OutputSwitch::IsInUse), 
  INSTANCE_CHILDREN_NODE(NODE_TYPE_CONFIG_DEVICE_PWM_OUTPUTS,
      NODE_TYPE_CONFIG_DEVICE_INSTANCE_PWM_OUTPUT, 
      Device_PwmOutput::GetNumDevices, Device_PwmOutput::IsInUse), 
  INSTANCE_CHILDREN_NODE(NODE_TYPE_CONFIG_DEVICE_BUZZERS,
      NODE_TYPE_CONFIG_DEVICE_INSTANCE_BUZZER, 
      Device_Buzzer::GetNumDevices, Device_Buzzer::IsInUse), 
  INSTANCE_CHILDREN_NODE(NODE_TYPE_CONFIG_DEVICE_TEMP_SENSORS,
      NODE_TYPE_CONFIG_DEVICE_INSTANCE_TEMP_SENSOR, 
      Device_TemperatureSensor::GetNumDevices, Device_TemperatureSensor::IsInUse), 
  INSTANCE_CHILDREN_NODE(NODE_TYPE_CONFIG_DEVICE_HEATERS,
      NODE_TYPE_CONFIG_DEVICE_INSTANCE_HEATER, 
      Device_Heater::GetNumDevices, Device_Heater::IsInUse), 
    
  // Device Type Instance Nodes
  UNNAMED_GROUP_NODE(NODE_TYPE_CONFIG_DEVICE_INSTANCE_INPUT_SWITCH),
  UNNAMED_GROUP_NODE(NODE_TYPE_CONFIG_DEVICE_INSTANCE_OUTPUT_SWITCH),
  UNNAMED_GROUP_NODE(NODE_TYPE_CONFIG_DEVICE_INSTANCE_PWM_OUTPUT),
  UNNAMED_GROUP_NODE(NODE_TYPE_CONFIG_DEVICE_INSTANCE_BUZZER),
  UNNAMED_GROUP_NODE(NODE_TYPE_CONFIG_DEVICE_INSTANCE_TEMP_SENSOR),
  UNNAMED_GROUP_NODE(NODE_TYPE_CONFIG_DEVICE_INSTANCE_HEATER),
    
  // Device related leaf nodes
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_INPUT_SWITCH_FRIENDLY_NAME,
      LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_STRING),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_INPUT_SWITCH_PIN,
      LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_UINT8),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_OUTPUT_SWITCH_FRIENDLY_NAME,
      LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_STRING),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_OUTPUT_SWITCH_PIN,
      LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_UINT8),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_PWM_OUTPUT_FRIENDLY_NAME,
      LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_STRING),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_PWM_OUTPUT_PIN,
      LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_UINT8),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_PWM_OUTPUT_USE_SOFT_PWM,
      LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_BOOL),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_BUZZER_FRIENDLY_NAME,
      LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_STRING),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_BUZZER_PIN,
      LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_UINT8),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_BUZZER_USE_SOFT_PWM,
      LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_BOOL),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_TEMP_SENSOR_FRIENDLY_NAME,
      LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_STRING),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_TEMP_SENSOR_PIN,
      LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_UINT8),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_TEMP_SENSOR_TYPE,
      LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_UINT8),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_HEATER_FRIENDLY_NAME,
      LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_STRING),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_HEATER_PIN,
      LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_UINT8),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_HEATER_USE_SOFT_PWM,
      LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_BOOL),
      
  // System config related leaf nodes
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_SYSTEM_HARDWARE_NAME,
    LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_STRING),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_SYSTEM_HARDWARE_TYPE,
    LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_UINT8),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_SYSTEM_HARDWARE_REV,
    LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_UINT8),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_SYSTEM_BOARD_IDENTITY,
    LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_STRING),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_SYSTEM_BOARD_SERIAL_NUM,
    LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_STRING),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_INPUT_SWITCHES,
    LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_UINT8),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_OUTPUT_SWITCHES,
    LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_UINT8),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_PWM_OUTPUTS,
    LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_UINT8),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_BUZZERS,
    LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_UINT8),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_HEATERS,
    LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_UINT8),
  LEAF_NODE(NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_TEMP_SENSORS,
    LEAF_CLASS_CONFIG, LEAF_OPERATIONS_READWRITABLE, LEAF_SET_DATATYPE_UINT8),

    // Statistics related leaf nodes
  LEAF_NODE(NODE_TYPE_STATS_LEAF_RX_PACKET_COUNT,
    LEAF_CLASS_STATISTICS, LEAF_OPERATIONS_READABLE, LEAF_SET_DATATYPE_INVALID),
  LEAF_NODE(NODE_TYPE_STATS_LEAF_RX_ERROR_COUNT,
    LEAF_CLASS_STATISTICS, LEAF_OPERATIONS_READABLE, LEAF_SET_DATATYPE_INVALID),
  LEAF_NODE(NODE_TYPE_STATS_LEAF_QUEUE_MEMORY,
    LEAF_CLASS_STATISTICS, LEAF_OPERATIONS_READABLE, LEAF_SET_DATATYPE_INVALID),
    
  // Diagnostics related leaf nodes
  LEAF_NODE(NODE_TYPE_DEBUG_LEAF_STACK_MEMORY,
    LEAF_CLASS_STATISTICS, LEAF_OPERATIONS_READABLE, LEAF_SET_DATATYPE_INVALID),
  LEAF_NODE(NODE_TYPE_DEBUG_LEAF_STACK_LOW_WATER_MARK,
    LEAF_CLASS_STATISTICS, LEAF_OPERATIONS_READABLE, LEAF_SET_DATATYPE_INVALID),
    
};

//
// Public Methods
//

uint8_t ConfigurationTreeNode::GetName(char *buffer, uint8_t buffer_length) const
{
  const char *name_string;
  uint8_t cnt = 0;
  
  if (!IsInstanceNode())
  {
    if (node_info_index == INVALID_NODE_INFO_INDEX)
      return 0;
    if (sizeof(node_info_array[node_info_index].name) == 2)
      name_string = (const char *)pgm_read_word(&node_info_array[node_info_index].name);
    else
      name_string = (const char *)pgm_read_dword(&node_info_array[node_info_index].name);
    if (name_string == 0)
      return 0;
    while (cnt < buffer_length)
    {
      char ch = pgm_read_byte(name_string);
      *buffer = ch;
      if (ch == '\0')
        return cnt;
      cnt += 1;
      name_string += 1;
      buffer += 1;
    }
  }
  else
  {
    char byte_string[4];
    utoa(instance_id, byte_string, 10);
    name_string = &byte_string[0];
    while (cnt < buffer_length)
    {
      char ch = *name_string;
      *buffer = ch;
      if (ch == '\0')
        return cnt;
      cnt += 1;
      name_string += 1;
      buffer += 1;
    }
  }
  return buffer_length;
}

int8_t ConfigurationTreeNode::CompareName(const char *str) const
{
  const char *name_string;
  char ch;
  int8_t cnt = 0;
  
  if (!IsInstanceNode())
  {
    if (node_info_index == INVALID_NODE_INFO_INDEX)
      return -1;
    if (sizeof(node_info_array[node_info_index].name) == 2)
      name_string = (const char *)pgm_read_word(&node_info_array[node_info_index].name);
    else
      name_string = (const char *)pgm_read_dword(&node_info_array[node_info_index].name);
    if (name_string == 0)
      return -1;
    while ((ch = pgm_read_byte(name_string)) == *str)
    {
      if (ch == '\0')
        return cnt;
      name_string += 1;
      str += 1;
      cnt += 1;
    }
    if (ch == '\0' && *str == '.')
      return cnt;
  }
  else
  {
    char byte_string[4];
    utoa(instance_id, byte_string, 10);
    name_string = &byte_string[0];
    while ((ch = *name_string) == *str)
    {
      if (ch == '\0')
        return cnt;
      name_string += 1;
      str += 1;
      cnt += 1;
    }
    if (ch == '\0' && *str == '.')
      return cnt;
  }
  return -1;
}

bool 
ConfigurationTreeNode::IsInUse() const
{
  if (node_info_index == INVALID_NODE_INFO_INDEX)
    return false;
  in_use_functor_type my_in_use_functor; 
  if (sizeof(node_info_array[node_info_index].child_in_use_functor) == 2)
    my_in_use_functor = (in_use_functor_type)pgm_read_word(&node_info_array[node_info_index].child_in_use_functor);
  else
    my_in_use_functor = (in_use_functor_type)pgm_read_dword(&node_info_array[node_info_index].child_in_use_functor);
  if (my_in_use_functor == 0)
    return true;
  return my_in_use_functor(instance_id);
}

uint8_t 
ConfigurationTreeNode::GetLeafClass() const
{
  if (node_info_index == INVALID_NODE_INFO_INDEX)
    return false;
  return pgm_read_byte(&node_info_array[node_info_index].leaf_node_class);
}

uint8_t 
ConfigurationTreeNode::GetLeafOperations() const
{
  if (node_info_index == INVALID_NODE_INFO_INDEX)
    return false;
  return pgm_read_byte(&node_info_array[node_info_index].leaf_operations);
}

uint8_t 
ConfigurationTreeNode::GetLeafSetDataType() const
{
  if (node_info_index == INVALID_NODE_INFO_INDEX)
    return false;
  return pgm_read_byte(&node_info_array[node_info_index].leaf_datatype);
}
  

///////////////////////////////////////////////////////////////////////////////
//
// Private Functions 
//

void 
ConfigurationTreeNode::Clear()
{
  node_type = NODE_TYPE_INVALID;
  node_info_index = INVALID_NODE_INFO_INDEX;
  instance_id = INVALID_INSTANCE_ID;
}

void 
ConfigurationTreeNode::SetAsRootNode() 
{ 
  Clear();
  node_type = NODE_TYPE_CONFIG_ROOT; 
  node_info_index = FindNodeInfoIndex(NODE_TYPE_CONFIG_ROOT);
}

uint8_t 
ConfigurationTreeNode::FindNodeInfoIndex(uint8_t type) const
{
  for (uint8_t i=0; i<sizeof(node_info_array)/sizeof(node_info_array[0]); i++)
    if (pgm_read_byte(&node_info_array[i].node_type) == type)
      return i;
  return INVALID_NODE_INFO_INDEX;
}


bool 
ConfigurationTreeNode::InitializeNextChild(ConfigurationTreeNode &child) const
{
  if (node_info_index == INVALID_NODE_INFO_INDEX)
    return false;
  const ConfigNodeInfo *node_info = &node_info_array[node_info_index];
  const uint8_t instance_child_type = pgm_read_byte(&node_info->instance_child_type);
  const num_children_functor_type num_children_functor = (sizeof(num_children_functor_type) == 2) ?
            (num_children_functor_type)pgm_read_word(&node_info->num_children) :
            (num_children_functor_type)pgm_read_dword(&node_info->num_children);
    
  if (instance_child_type != NODE_TYPE_INVALID)
  {
    if (child.node_type != instance_child_type)
    {
      child.node_type = instance_child_type;
      child.node_info_index = FindNodeInfoIndex(instance_child_type);
      child.instance_id = 0;
      return true;
    }
    if (child.instance_id < num_children_functor() - 1)
    {
      child.instance_id += 1;
      return true;
    }
  }
  else
  {
    if (child.node_type == NODE_TYPE_INVALID)
    {
      child.node_type = (sizeof(node_info->named_child_types) == 2) ?
            pgm_read_byte(&((const uint8_t*)pgm_read_word(&node_info->named_child_types))[0]) :
            pgm_read_byte(&((const uint8_t*)pgm_read_dword(&node_info->named_child_types))[0]);
      child.node_info_index = FindNodeInfoIndex(child.node_type);
      child.instance_id = INVALID_INSTANCE_ID;
      return true;
    }
    // the functor pointer is acually storing a raw value in this case
    const uint8_t num_children = (uint8_t)(uint32_t)num_children_functor; 
    for (uint8_t i=0; i<num_children-1; i++)
    {
      const uint8_t named_child_type = (sizeof(node_info->named_child_types) == 2) ?
            pgm_read_byte(&((const uint8_t*)pgm_read_word(&node_info->named_child_types))[i]) :
            pgm_read_byte(&((const uint8_t*)pgm_read_dword(&node_info->named_child_types))[i]);
      if (child.node_type == named_child_type)
      {
        child.node_type = (sizeof(node_info->named_child_types) == 2) ?
              pgm_read_byte(&((const uint8_t*)pgm_read_word(&node_info->named_child_types))[i+1]) :
              pgm_read_byte(&((const uint8_t*)pgm_read_dword(&node_info->named_child_types))[i+1]);
        child.node_info_index = FindNodeInfoIndex(child.node_type);
        child.instance_id = INVALID_INSTANCE_ID;
        return true;
      }
    }
  }
  child.Clear();
  return false;
}