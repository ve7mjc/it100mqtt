#ifndef IT100COMMANDS_H
#define IT100COMMANDS_H

#include <QByteArray>

namespace it100 {

/**
  * Application Originated Commands
  */
inline const QByteArray CMD_POLL = "000";
inline const QByteArray CMD_STATUS_REQUEST = "001";
inline const QByteArray CMD_LABELS_REQUEST = "002";
inline const QByteArray CMD_SET_TIME_AND_DATE = "010";
inline const QByteArray CMD_COMMAND_OUTPUT_CONTROL = "020";
inline const QByteArray CMD_PARTITION_ARM_CONTROL_AWAY = "030";
inline const QByteArray CMD_PARTITION_ARM_CONTROL_STAY = "031";
inline const QByteArray CMD_PARTITION_ARM_CONTROL_ARMED_NO_ENTRY_DELAY = "032";
inline const QByteArray CMD_PARTITION_ARM_CONTROL_WITH_CODE = "033";
inline const QByteArray CMD_PARTITION_DISARM_CONTROL_WITH_CODE = "040";
inline const QByteArray CMD_TIME_STAMP_CONTROL = "055";
inline const QByteArray CMD_TIME_DATE_BROADCAST_CONTROL = "056";
inline const QByteArray CMD_TEMPERATURE_BROADCAST_CONTROL = "057";
inline const QByteArray CMD_VIRTUAL_KEYPAD_CONTROL = "058";
inline const QByteArray CMD_TRIGGER_PANIC_ALARM = "060";
inline const QByteArray CMD_KEY_PRESSED_VIRT = "070";
inline const QByteArray CMD_BAUD_RATE_CHANGE = "080";
inline const QByteArray CMD_GET_TEMPERATURE_SET_POINT = "095";
inline const QByteArray CMD_TEMPERATURE_CHANGE = "096";
inline const QByteArray CMD_SAVE_TEMPERATURE_SETTING = "097";
inline const QByteArray CMD_CODE_SEND = "200";

/**
  * IT-100 Originated Commands
  */
inline const QByteArray CMD_COMMAND_ACKNOWLEDGE = "500";
inline const QByteArray CMD_COMMAND_ERROR = "501";
inline const QByteArray CMD_SYSTEM_ERROR = "502";
inline const QByteArray CMD_TIME_DATE_BROADCAST = "550";
inline const QByteArray CMD_RING_DETECETD = "560";
inline const QByteArray CMD_INDOOR_TEMPERATURE_BROADCAST = "561";
inline const QByteArray CMD_OUTDOOR_TEMPERATURE_BROADCAST = "562";
inline const QByteArray CMD_THERMOSTAT_SET_POINTS = "563";
inline const QByteArray CMD_BROADCAST_LABELS = "570";
inline const QByteArray CMD_BAUD_RATE_SET = "580";
inline const QByteArray CMD_ZONE_ALARM = "601";
inline const QByteArray CMD_ZONE_ALARM_RESTORE = "602";
inline const QByteArray CMD_ZONE_TAMPER = "603";
inline const QByteArray CMD_ZONE_TAMPER_RESTORE = "604";
inline const QByteArray CMD_ZONE_FAULT = "605";
inline const QByteArray CMD_ZONE_FAULT_RESTORE = "606";
inline const QByteArray CMD_ZONE_OPEN = "609";
inline const QByteArray CMD_ZONE_RESTORED = "610";
inline const QByteArray CMD_DURESS_ALARM = "620";
inline const QByteArray CMD_F_KEY_ALARM = "621";
inline const QByteArray CMD_F_KEY_RESTORAL = "622";
inline const QByteArray CMD_A_KEY_ALARM = "623";
inline const QByteArray CMD_A_KEY_RESTORAL = "624";
inline const QByteArray CMD_P_KEY_ALARM = "625";
inline const QByteArray CMD_P_KEY_RESTORAL = "626";
inline const QByteArray CMD_AUXILIARY_INPUT_ALARM = "631";
inline const QByteArray CMD_AUXILIARY_INPUT_ALARM_RESTORED = "632";
inline const QByteArray CMD_PARTITION_READY = "626";
inline const QByteArray CMD_PARTITION_NOT_READY = "625";
inline const QByteArray CMD_PARTITION_ARMED_DESCRIPTIVE_MODE = "652";
inline const QByteArray CMD_PARTITION_IN_READY_TO_FORCE_ALARM = "653";
inline const QByteArray CMD_PARTITION_IN_ALARM = "654";
inline const QByteArray CMD_PARTITION_DISARMED = "655";
inline const QByteArray CMD_EXIT_DELAY_IN_PROGRESS = "656";
inline const QByteArray CMD_ENTRY_DELAY_IN_PROGRESS = "657";
inline const QByteArray CMD_KEYPAD_LOCKOUT = "658";
inline const QByteArray CMD_KEYPAD_BLANKING = "659";
inline const QByteArray CMD_COMMAND_OUTPUT_IN_PROGRESS = "660";
inline const QByteArray CMD_INVALID_ACCESS_CODE = "670";
inline const QByteArray CMD_FUNCTION_NOT_AVAILABLE = "671";
inline const QByteArray CMD_FAIL_TO_ARM = "672";
inline const QByteArray CMD_PARTITION_BUSY = "673";
inline const QByteArray CMD_USER_CLOSING = "700";
inline const QByteArray CMD_SPECIAL_CLOSING = "701";
inline const QByteArray CMD_PARTIAL_CLOSING = "702";
inline const QByteArray CMD_USER_OPENING = "750";
inline const QByteArray CMD_SPECIAL_OPENING = "751";
inline const QByteArray CMD_PANEL_BATTERY_TROUBLE = "800";
inline const QByteArray CMD_PANEL_BATTERY_TROUBLE_RESTORE ="801";
inline const QByteArray CMD_PANEL_AC_TROUBLE = "802";
inline const QByteArray CMD_PANEL_AC_RESTORE = "803";
inline const QByteArray CMD_SYSTEM_BELL_TROUBLE ="806";
inline const QByteArray CMD_SYSTEM_BELL_TROUBLE_RESTORAL = "807";
inline const QByteArray CMD_TLM_LINE_1_TROUBLE = "810";
inline const QByteArray CMD_TLM_LINE_1_TROUBLE_RESTORAL = "811";
inline const QByteArray CMD_TLM_LINE_2_TROUBLE = "812";
inline const QByteArray CMD_TLM_LINE_2_TROUBLE_RESTORAL = "813";
inline const QByteArray CMD_FTC_TROUBLE = "814";
inline const QByteArray CMD_BUFFER_NEAR_FULL = "816";
inline const QByteArray CMD_GENERAL_DEVICE_LOW_BATTERY = "821";
inline const QByteArray CMD_GENERAL_DEVICE_LOW_BATTERY_RESTORE = "822";
inline const QByteArray CMD_WIRELESS_KEY_LOW_BATTERY_TROUBLE = "825";
inline const QByteArray CMD_WIRELESS_KEY_LOW_BATTERY_TROUBLE_RESTORE = "826";
inline const QByteArray CMD_HANDHELD_KEYPAD_LOW_BATTERY_TROUBLE = "827";
inline const QByteArray CMD_HANDHELD_KEYPAD_LOW_BATTERY_TROUBLE_RESTORE = "828";
inline const QByteArray CMD_GENERAL_SYSTEM_TAMPER = "829";
inline const QByteArray CMD_GENERAL_SYSTEM_TAMPER_RESTORE = "830";
inline const QByteArray CMD_HOME_AUTOMATION_TROUBLE = "831";
inline const QByteArray CMD_HOME_AUTOMATION_TROUBLE_RESTORE = "832";
inline const QByteArray CMD_TROUBLE_STATUS_LED_ON = "840";
inline const QByteArray CMD_TROUBLE_STATUS_LED_OFF = "841";
inline const QByteArray CMD_FIRE_TROUBLE_ALARM = "842";
inline const QByteArray CMD_FIRE_TROUBLE_ALARM_RESTORE = "843";
inline const QByteArray CMD_CODE_REQUIRED = "900";
inline const QByteArray CMD_LCD_UPDATE = "901";
inline const QByteArray CMD_LCD_CURSOR = "902";
inline const QByteArray CMD_LED_STATUS = "903";
inline const QByteArray CMD_BEEP_STATUS = "904";
inline const QByteArray CMD_TONE_STATUS = "905";
inline const QByteArray CMD_BUZZER_STATUS = "906";
inline const QByteArray CMD_DOOR_CHIME_STATUS = "907";
inline const QByteArray CMD_SOFTWARE_VERSION = "908";

} // namespace it100

#endif // IT100COMMANDS_H
