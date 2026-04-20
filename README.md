# it100-mqtt

Interact with DSC alarm system via MQTT topics.

* Send commands to panel
* Receive events and status updates
* Emulate LCD Keypad

## Prerequisites

* DSC PowerSeries Alarm Panel
* DSC RS232 *IT100 Module* on the DSC keybus
* Qt5 >=5.7.1
* Ser2net (or appropriate Serial-IP solution)
* MQTT Broker

## Install

- Builds and installs `it100mqtt` binary to `/usr/local/bin`
- Copies example config to `/usr/local/etc/it100mqtt.conf` if not already preset
- Sets `/usr/local/etc/it100mqtt.conf` -> `root:root` -> `ug+rw,o-rw`
- Installs systemd unit to `/etc/systemd/system/it100mqtt.service`
- Enable and starts `it100mqtt` service
    - it100mqtt is systemd notify-aware

```
./install.sh
```

# MQTT Topics

## Availability

Topic: TOPIC_PREFIX/availability

payload: online|offline QOS_1,retained

* online set when IT100 begins communicating
* offline set if it100 times out or via LWT if MQTT disconnects

## Partition

### Partition Armed

Topic: TOPIC_PREFIX/partition/PARTITION_NUMBER/armed

Payload: 0|1 (string)

### Partition State

Topic: TOPIC_PREFIX/partition/PARTITION_NUMBER/state

* exit_delay
* entry_delay
* armed_away
* armed_stay
* disarmed
* alarm
* busy (is this an error?)

### Partition Description Condition

Topic: TOPIC_PREFIX/partition/PARTITION_NUMBER/condition

* ready
* not_ready
* busy
* armed
* exit_delay
* entry_delay
* armed_away
* armed_stay
* alarm

### Partition Events

Topic: TOPIC_PREFIX/partition/PARTITION_NUMBER/event

Possible Values:

* alarm
* armed
* disarmed
* ready
* not_ready
* exit_delay
* entry_delay
* user_closing
* partial_closing
* special_closing
* invalid_access_code
* function_not_available

## Zone States

Need to prove -- can a zone be faulted and restored|violated?

Topic: TOPIC_PREFIX/zone/ZONE_NUMBER/state

* open
* closed

Topic: TOPIC_PREFIX/zone/ZONE_NUMBER/condition

* alarm
* violated
* secure
* tamper
* fault

## Zone Events

Topic: TOPIC_PREFIX/zone/ZONE_NUMBER/event

Possible Values:

* alarm
* alarm_restored
* open
* restored
* tamper
* tamper_restored
* fault
* fault_restored

### System Trouble Events

* trouble_panel_battery
* trouble_panel_battery_restore
* trouble_panel_ac
* trouble_panel_ac_restore
* trouble_panel_bell
* trouble_panel_bell_restore
* trouble_general_tamper
* trouble_general_tamper_restore
* trouble_general_device_low_battery
* trouble_general_device_low_battery_restore

## Commands

Topic: TOPIC_PREFIX/command

Possible Commands:

* arm
* disarm
* <char> (0-9,*,#,<,>,abcde,F,A,or P)

# Keypad Emulation

## LCD Display Contents

TOPIC_PREFIX/keypad_lcd_data

## Keypresses

*See Commands*

