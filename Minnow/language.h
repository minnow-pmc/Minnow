#ifndef LANGUAGE_H
#define LANGUAGE_H

#include "config.h"

// NOTE: IF YOU CHANGE THIS FILE / MERGE THIS FILE WITH CHANGES
//

#ifndef LANGUAGE_CHOICE
  #error Please set LANGUAGE_CHOICE in config.h
#endif

// The language used for firmware configuration strings can be set independently
// of the main language choice (for instance if the use of standard English 
// firmware configurations is desired)
#ifndef CONFIG_STRING_LANGUAGE_CHOICE
  #define CONFIG_STRING_LANGUAGE_CHOICE LANGUAGE_CHOICE
#endif

#define MSG(str) MSG_INTERNAL(LANGUAGE_CHOICE,str)
#define PMSG(str) PSTR(MSG_INTERNAL(LANGUAGE_CHOICE,str)) 
#define CONFIG_STR(str) CONFIG_STR_INTERNAL(CONFIG_STRING_LANGUAGE_CHOICE,str)

// Two levels of indirection are required to get this to expand properly.
#define MSG_INTERNAL(lang,str) MSG_INTERNAL2(lang,str)
#define MSG_INTERNAL2(lang,str) str##_##lang
#define CONFIG_STR_INTERNAL(lang,str) CONFIG_STR_INTERNAL2(lang,str)
#define CONFIG_STR_INTERNAL2(lang,str) CONFIG_STR_##str##_##lang
//
// If you are adding new strings to this file, please define all other languages
// to be equal to the english language define initially (until someone translates
// it). This way the other languages will still compile.
//

/////////////////////////////////////////////////////
// Normal String Library
/////////////////////////////////////////////////////

//
// Error Strings
//


#define ERR_MSG_INSUFFICENT_BYTES_ENGLISH "Insufficient parameter bytes received. Rcvd: "
#define ERR_MSG_INSUFFICENT_BYTES_DEUTSCH "Zu weniger Parameter Bytes empfangen. Empfangen: "

#define ERR_MSG_GENERIC_APP_AT_OFFSET_ENGLISH "Parameter data invalid at index: "
#define ERR_MSG_GENERIC_APP_AT_OFFSET_DEUTSCH "Parameter Daten ungültig am Index: "

#define ERR_MSG_CONFIG_NODE_NOT_FOUND_ENGLISH "Config element invalid: "
#define ERR_MSG_CONFIG_NODE_NOT_FOUND_DEUTSCH "Konfigurations Element ungültig: "

#define ERR_MSG_CONFIG_NODE_NOT_COMPLETE_ENGLISH "Config element not complete: "
#define ERR_MSG_CONFIG_NODE_NOT_COMPLETE_DEUTSCH "Konfigurations Element nicht vollständig: "

#define MSG_ERR_EEPROM_NOT_ENABLED_ENGLISH "Non-volatile EEPROM storage not enabled"
#define MSG_ERR_EEPROM_NOT_ENABLED_DEUTSCH "Nicht-flüchtiger EEPROM Speicher ist nicht aktiviert"

#define MSG_ERR_CANNOT_HANDLE_FIRMWARE_CONFIG_REQUEST_ENGLISH "Don't know how to handle firmware configuration request. Line "
#define MSG_ERR_CANNOT_HANDLE_FIRMWARE_CONFIG_REQUEST_DEUTSCH "Unbekannte Firmware Konfigurations Anfordering. Zeile "

#define MSG_ERR_NO_RESPONSE_GENERATED_ENGLISH "No response generated"
#define MSG_ERR_NO_RESPONSE_GENERATED_DEUTSCH "Keien Antwort erzeugt"

#define MSG_ERR_NO_RESPONSE_TO_SEND_ENGLISH "No response to send"
#define MSG_ERR_NO_RESPONSE_TO_SEND_DEUTSCH "Keine Antwort verfügbar"

#define ERR_MSG_DEVICE_NOT_IN_USE_ENGLISH "Device not in use"
#define ERR_MSG_DEVICE_NOT_IN_USE_DEUTSCH "Gerät wird nicht verwendet"

#define ERR_MSG_INVALID_PIN_NUMBER_ENGLISH "Invalid pin number"
#define ERR_MSG_INVALID_PIN_NUMBER_DEUTSCH "Ungültige Pin Nummer"

#define ERR_MSG_EXPECTED_UINT8_VALUE_ENGLISH "Expected a uint8 value (0-255)."
#define ERR_MSG_EXPECTED_UINT8_VALUE_DEUTSCH "Erwartet war eine uint8 Zahl (0-255)."

#define ERR_MSG_EXPECTED_INT16_VALUE_ENGLISH "Expected a int16 value (-32,768-32,767)."
#define ERR_MSG_EXPECTED_INT16_VALUE_DEUTSCH "Erwartet war eine int16 Zahl (-32,768-32,767)."

#define ERR_MSG_EXPECTED_BOOL_VALUE_ENGLISH "Expected a boolean value (0,1,true,false)."
#define ERR_MSG_EXPECTED_BOOL_VALUE_DEUTSCH "Erwartet war ein boolescher Wert (0,1,true,false)."

#define ERR_MSG_CONFIG_NODE_NOT_WRITABLE_ENGLISH "Firmware configuration value is not a writable element"
#define ERR_MSG_CONFIG_NODE_NOT_WRITABLE_DEUTSCH "Firmware Konfigurations Wert ist nicht schreibbar."

#define ERR_MSG_CONFIG_NODE_NOT_READABLE_ENGLISH "Firmware configuration value is not a readable element"
#define ERR_MSG_CONFIG_NODE_NOT_READABLE_DEUTSCH "Firmware Konfigurations Wert kann nicht gelesen werden."

#define ERR_MSG_QUEUE_ORDER_NOT_PERMITTED_ENGLISH "Order not permitted for queuing: "
#define ERR_MSG_QUEUE_ORDER_NOT_PERMITTED_DEUTSCH "Befehl kann nicht in die Warteschlange aufgenommen werden: "

#define MSG_ERR_UNKNOWN_VALUE_ENGLISH "Unknown value specified"
#define MSG_ERR_UNKNOWN_VALUE_DEUTSCH MSG_ERR_UNKNOWN_VALUE_ENGLISH

#define MSG_ERR_ALREADY_INITIALIZED_ENGLISH "Already initialized"
#define MSG_ERR_ALREADY_INITIALIZED_DEUTSCH MSG_ERR_ALREADY_INITIALIZED_ENGLISH

#define MSG_ERR_INSUFFICIENT_MEMORY_ENGLISH "Insufficient memory"
#define MSG_ERR_INSUFFICIENT_MEMORY_DEUTSCH MSG_ERR_INSUFFICIENT_MEMORY_ENGLISH

#define ERR_MSG_INSUFFICIENT_QUEUE_MEMORY_ENGLISH "Insufficient memory to allocate minimum queue size: "
#define ERR_MSG_INSUFFICIENT_QUEUE_MEMORY_DEUTSCH ERR_MSG_INSUFFICIENT_QUEUE_MEMORY_ENGLISH

//
// Other strings
//

#define MSG_EXPECTING_ENGLISH "Expecting: "
#define MSG_EXPECTING_DEUTSCH "Erwartung: "

/////////////////////////////////////////////////////
// Config String Library
/////////////////////////////////////////////////////


// Generic attribute strings
#define CONFIG_STR_PIN_ENGLISH                    "pin"
#define CONFIG_STR_PIN_DEUTSCH                    "pin"

#define CONFIG_STR_NAME_ENGLISH                   "name"
#define CONFIG_STR_NAME_DEUTSCH                   "name"

#define CONFIG_STR_TYPE_ENGLISH                   "type"
#define CONFIG_STR_TYPE_DEUTSCH                   CONFIG_STR_TYPE_ENGLISH

// Groups
#define CONFIG_STR_SYSTEM_ENGLISH                 "system"
#define CONFIG_STR_SYSTEM_DEUTSCH                 "system"

#define CONFIG_STR_DEVICES_ENGLISH                "devices"
#define CONFIG_STR_DEVICES_DEUTSCH                "geräte"

#define CONFIG_STR_STATS_ENGLISH                  "stats"
#define CONFIG_STR_STATS_DEUTSCH                  "status"

#define CONFIG_STR_DEBUG_ENGLISH                  "debug"
#define CONFIG_STR_DEBUG_DEUTSCH                  "debug"

// Device Types
#define CONFIG_STR_DIGITAL_INPUT_ENGLISH          "digital_input"
#define CONFIG_STR_INPUT_DIG_DEUTSCH              "eingang_digital"

#define CONFIG_STR_DIGITAL_OUTPUT_ENGLISH         "digital_output"
#define CONFIG_STR_OUTPUT_DIG_DEUTSCH             "ausgang_digital"

#define CONFIG_STR_PWM_OUTPUT_ENGLISH             "pwm_output"
#define CONFIG_STR_OUTPUT_PWM_DEUTSCH             "ausgang_pwm"

#define CONFIG_STR_BUZZER_ENGLISH                 "buzzer"
#define CONFIG_STR_BUZZER_DEUTSCH                 "buzzer"

#define CONFIG_STR_HEATER_ENGLISH                 "heater"
#define CONFIG_STR_HEATER_DEUTSCH                 "heizelement"

#define CONFIG_STR_TEMP_SENSOR_ENGLISH            "temp_sensor"
#define CONFIG_STR_TEMP_SENSOR_DEUTSCH            CONFIG_STR_TEMP_SENSOR_ENGLISH

// Device Type counts
#define CONFIG_STR_NUM_DIGITAL_INPUTS_ENGLISH     "num_digital_inputs"
#define CONFIG_STR_NUM_DIGITAL_INPUTS_DEUTSCH     CONFIG_STR_NUM_DIGITAL_INPUTS_ENGLISH

#define CONFIG_STR_NUM_DIGITAL_OUTPUTS_ENGLISH    "num_digital_outputs"
#define CONFIG_STR_NUM_DIGITAL_OUTPUTS_DEUTSCH     CONFIG_STR_NUM_DIGITAL_OUTPUTS_ENGLISH

#define CONFIG_STR_NUM_PWM_OUTPUTS_ENGLISH        "num_pwm_outputs"
#define CONFIG_STR_NUM_PWM_OUTPUTS_DEUTSCH        CONFIG_STR_NUM_PWM_OUTPUTS_ENGLISH

#define CONFIG_STR_NUM_BUZZERS_ENGLISH            "num_buzzers"
#define CONFIG_STR_NUM_BUZZERS_DEUTSCH            CONFIG_STR_NUM_BUZZERS_ENGLISH

#define CONFIG_STR_NUM_HEATERS_ENGLISH            "num_heaters"
#define CONFIG_STR_NUM_HEATERS_DEUTSCH            CONFIG_STR_NUM_HEATERS_ENGLISH

#define CONFIG_STR_NUM_TEMP_SENSORS_ENGLISH       "num_temp_sensors"
#define CONFIG_STR_NUM_TEMP_SENSORS_DEUTSCH       CONFIG_STR_NUM_HEATERS_ENGLISH

// Specific attribute names
#define CONFIG_STR_HARDWARE_NAME_ENGLISH          "hardware_name"
#define CONFIG_STR_HARDWARE_NAME_DEUTSCH          "hardware_name"

#define CONFIG_STR_HARDWARE_TYPE_ENGLISH          "hardware_type"
#define CONFIG_STR_HARDWARE_TYPE_DEUTSCH          "hardware_typ"

#define CONFIG_STR_HARDWARE_REV_ENGLISH           "hardware_rev"
#define CONFIG_STR_HARDWARE_REV_DEUTSCH           "hardware_rev"

#define CONFIG_STR_BOARD_IDENTITY_ENGLISH         "board_identity"
#define CONFIG_STR_BOARD_IDENTITY_DEUTSCH         "board_identität"

#define CONFIG_STR_BOARD_SERIAL_NUM_ENGLISH       "board_serialnum"
#define CONFIG_STR_BOARD_SERIAL_NUM_DEUTSCH       "board_seriennummer"

#define CONFIG_STR_RX_COUNT_ENGLISH               "rx_count"
#define CONFIG_STR_RX_COUNT_DEUTSCH               "empfangene_bytes"

#define CONFIG_STR_RX_ERROR_ENGLISH               "rx_error"
#define CONFIG_STR_RX_ERROR_DEUTSCH               "empfangs_fehler"

#define CONFIG_STR_QUEUE_MEMORY_ENGLISH           "queue_memory"
#define CONFIG_STR_QUEUE_MEMORY_DEUTSCH           "queue_speicher"

#define CONFIG_STR_STACK_MEMORY_ENGLISH           "stack_memory"
#define CONFIG_STR_STACK_MEMORY_DEUTSCH           "stack_speicher"

#define CONFIG_STR_STACK_LOW_WATER_MARK_ENGLISH   "stack_low_water_mark"
#define CONFIG_STR_STACK_LOW_WATER_MARK_DEUTSCH   "stack_unbenutzt_markierung"

#define CONFIG_STR_USE_SOFT_PWM_ENGLISH           "use_soft_pwm"
#define CONFIG_STR_USE_SOFT_PWM_DEUTSCH           CONFIG_STR_USE_SOFT_PWM_ENGLISH



#endif // ifndef LANGUAGE_H
