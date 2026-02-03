# UWB / Pixhawk / OpenMV Tests

## Pixhawk TELEM1 Control Test
Project: `pixhawk_telem1_control_test`

Wiring:
- Pixhawk TELEM1 Pin 2 (TX) -> ESP32-S3 IO10 (RX)
- Pixhawk TELEM1 Pin 3 (RX) -> ESP32-S3 IO11 (TX)
- GND -> GND

Baud:
- `115200` (adjust if your TELEM1 is set differently)

Serial commands:
- `arm`    -> send ARM (`MAV_CMD_COMPONENT_ARM_DISARM`)
- `disarm` -> send DISARM
- `hb on`  -> enable heartbeat
- `hb off` -> disable heartbeat
- `help`   -> show commands

Expected output:
- `[TX] HEARTBEAT` every 1s (when enabled)
- `[ACK] cmd=400 result=...` after ARM/DISARM

## Pixhawk TELEM1 MAVLink Parser
Project: `pixhawk_telem1_mavlink_parser`

Purpose:
- Receive and parse MAVLink messages
- Print message ID, SYS, COMP, LEN

## OpenMV UART Test
Project: `openmv_uart_test`

Wiring:
- OpenMV P4 (TXD) -> ESP32-S3 IO6 (RX)
- OpenMV P5 (RXD) -> ESP32-S3 IO7 (TX)
- GND -> GND

Baud:
- `19200` on both OpenMV and ESP32-S3

Behavior:
- Forwards ASCII text from OpenMV UART to USB Serial.

## PX4 / MAVLink Docs
Local file: `px4_mavlink_docs.md`

