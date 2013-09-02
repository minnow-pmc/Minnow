#ifndef LANGUAGE_H
#define LANGUAGE_H

#include "config.h"

// NOTE: IF YOU CHANGE THIS FILE / MERGE THIS FILE WITH CHANGES
//

// Languages
// 1  English
// 2  German - Deutsch

#ifndef LANGUAGE_CHOICE
  #error Please set LANGUAGE_CHOICE in config.h
#endif

// The language used for firmware configuration strings can be set independently
// of the main language choice (for instance if the use of standard English 
// firmware configurations is desired)
#ifndef CONFIG_STRINGS_LANGUAGE_CHOICE
  #define CONFIG_STRINGS_LANGUAGE_CHOICE LANGUAGE_CHOICE
#endif

#define STRINGIFY_(n) #n
#define STRINGIFY(n) STRINGIFY_(n)


#if LANGUAGE_CHOICE == 1 // English

#define MSG_ERR_HARDWARE_CANNOT_SOFT_RESET "Hardware does not support software reset. System stopped"
#define ERR_MSG_INSUFFICENT_BYTES "Insufficient parameter bytes received. Rcvd: "
#define ERR_MSG_GENERIC_APP_AT_OFFSET "Parameter data invalid at index: "
#define ERR_MSG_CONFIG_NODE_NOT_FOUND "Config element invalid: "
#define ERR_MSG_CONFIG_NODE_NOT_COMPLETE "Config element not complete: "
#define MSG_ERR_EEPROM_NOT_ENABLED "Non-volatile EEPROM storage not enabled"
#define MSG_ERR_CANNOT_HANDLE_FIRMWARE_CONFIG_REQUEST "Don't know how to handle firmware configuration request. Line "
#define MSG_ERR_NO_RESPONSE_GENERATED "No response generated"
#define MSG_ERR_NO_RESPONSE_TO_SEND "No response to send"
#define ERR_MSG_DEVICE_NOT_IN_USE "Device not in use"
#define ERR_MSG_INVALID_PIN_NUMBER "Invalid pin number"
#define ERR_MSG_EXPECTED_UINT8_VALUE "Expected a uint8 value (0-255)."
#define ERR_MSG_EXPECTED_INT16_VALUE "Expected a int16 value (-32,768-32,767)."
#define ERR_MSG_EXPECTED_BOOL_VALUE "Expected a boolean value (0,1,true,false)."
#define ERR_MSG_CONFIG_NODE_NOT_WRITABLE "Firmware configuration value is not a writable element."
#define ERR_MSG_CONFIG_NODE_NOT_READABLE "Firmware configuration value is not a readable element."
#define ERR_MSG_QUEUE_ORDER_NOT_PERMITTED "Order not permitted for queuing: "

#define MSG_EXPECTING "Expecting: "

#endif // ENGLISH

#if LANGUAGE_CHOICE == 2 // German - Deutsch

#define MSG_ERR_HARDWARE_CANNOT_SOFT_RESET "Hardware unterstützt keinen Software Reset. System angehalten"
#define ERR_MSG_INSUFFICENT_BYTES "Zu weniger Parameter Bytes empfangen. Empfangen: "
#define ERR_MSG_GENERIC_APP_AT_OFFSET "Parameter Daten ungültig am Index: "
#define ERR_MSG_CONFIG_NODE_NOT_FOUND "Konfigurations Element ungültig: "
#define ERR_MSG_CONFIG_NODE_NOT_COMPLETE "Konfigurations Element nicht vollständig: "
#define MSG_ERR_EEPROM_NOT_ENABLED "Nicht-flüchtiger EEPROM Speicher ist nicht aktiviert"
#define MSG_ERR_CANNOT_HANDLE_FIRMWARE_CONFIG_REQUEST "Unbekannte Firmware Konfigurations Anfordering. Zeile "
#define MSG_ERR_NO_RESPONSE_GENERATED "Keien Antwort erzeugt"
#define MSG_ERR_NO_RESPONSE_TO_SEND "Keine Antwort verfügbar"
#define ERR_MSG_DEVICE_NOT_IN_USE "Gerät wird nicht verwendet"
#define ERR_MSG_INVALID_PIN_NUMBER "Ungültige Pin Nummer"
#define ERR_MSG_EXPECTED_UINT8_VALUE "Erwartet war eine uint8 Zahl (0-255)."
#define ERR_MSG_EXPECTED_INT16_VALUE "Erwartet war eine int16 Zahl (-32,768-32,767)."
#define ERR_MSG_EXPECTED_BOOL_VALUE "Erwartet war ein boolescher Wert (0,1,true,false)."
#define ERR_MSG_CONFIG_NODE_NOT_WRITABLE "Firmware Konfigurations Wert ist nicht schreibbar."
#define ERR_MSG_CONFIG_NODE_NOT_READABLE "Firmware Konfigurations Wert kann nicht gelesen werden."
#define ERR_MSG_QUEUE_ORDER_NOT_PERMITTED "Befehl kann nicht in die Warteschlange aufgenommen werden: "

#define MSG_EXPECTING "Erwartung: "

#endif // German - Deutsch


#if CONFIG_STRINGS_LANGUAGE_CHOICE == 1 // English

// Generic attribute strings
#define CONFIG_STR_PIN                    "pin"
#define CONFIG_STR_NAME                   "name"

// Groups
#define CONFIG_STR_SYSTEM                 "system"
#define CONFIG_STR_DEVICES                "devices"
#define CONFIG_STR_STATS                  "stats"
#define CONFIG_STR_DEBUG                  "debug"

// Device Types
#define CONFIG_STR_INPUT_DIG              "input_dig"
#define CONFIG_STR_OUTPUT_DIG             "output_dig"
#define CONFIG_STR_OUTPUT_PWM             "output_pwm"
#define CONFIG_STR_BUZZER                 "buzzer"
#define CONFIG_STR_HEATER                 "heater"

// Specific attribute names
#define CONFIG_STR_HARDWARE_NAME          "hardware_name"
#define CONFIG_STR_HARDWARE_TYPE          "hardware_type"
#define CONFIG_STR_HARDWARE_REV           "hardware_rev"
#define CONFIG_STR_BOARD_IDENTITY         "board_identity"
#define CONFIG_STR_BOARD_SERIAL_NUM       "board_serialnum"

#define CONFIG_STR_HEATER_HEATER_PIN       "heater_pin"

#define CONFIG_STR_RX_COUNT               "rx_count"
#define CONFIG_STR_RX_ERROR               "rx_error"
#define CONFIG_STR_QUEUE_MEMORY           "queue_memory"

#define CONFIG_STR_STACK_MEMORY           "stack_memory"
#define CONFIG_STR_STACK_LOW_WATER_MARK   "stack_low_water_mark"

#endif // ENGLISH

#if CONFIG_STRINGS_LANGUAGE_CHOICE == 2 // German - Deutsch

// Generic attribute strings
#define CONFIG_STR_PIN                    "pin"
#define CONFIG_STR_NAME                   "name"

// Groups
#define CONFIG_STR_SYSTEM                 "system"
#define CONFIG_STR_DEVICES                "geräte"
#define CONFIG_STR_STATS                  "status"
#define CONFIG_STR_DEBUG                  "debug"

// Device Types
#define CONFIG_STR_INPUT_DIG              "eingang_digital"
#define CONFIG_STR_OUTPUT_DIG             "ausgang_digital"
#define CONFIG_STR_OUTPUT_PWM             "ausgang_pwm"
#define CONFIG_STR_BUZZER                 "buzzer"
#define CONFIG_STR_HEATER                 "heizelement"

// Specific attribute names
#define CONFIG_STR_HARDWARE_NAME          "hardware_name"
#define CONFIG_STR_HARDWARE_TYPE          "hardware_typ"
#define CONFIG_STR_HARDWARE_REV           "hardware_rev"
#define CONFIG_STR_BOARD_IDENTITY         "board_identität"
#define CONFIG_STR_BOARD_SERIAL_NUM       "board_seriennummer"

#define CONFIG_STR_HEATER_HEATER_PIN       "heitzelement_pin"

#define CONFIG_STR_RX_COUNT               "empfangene_bytes"
#define CONFIG_STR_RX_ERROR               "empfangs_fehler"
#define CONFIG_STR_QUEUE_MEMORY           "queue_speicher"

#define CONFIG_STR_STACK_MEMORY           "stack_speicher"
#define CONFIG_STR_STACK_LOW_WATER_MARK   "stack_unbenutzt_markierung"

#endif // German - Deutsch

#endif // ifndef LANGUAGE_H
