# UWB-Project

[中文说明](README.zh-CN.md)

UWB-Project is an experimental UWB positioning and flight-integration workspace for a parent-child UAV aerial docking system. The core goal is to compute reliable relative position from UWB modules, then use that position signal together with Pixhawk/MAVLink, ESP-NOW, and optional OpenMV AprilTag vision to support close-range docking experiments.

The repository contains firmware sketches, Python visualization/debugging tools, wiring notes, module datasheets, and older prototype implementations. It is organized as a research and integration project rather than a single packaged library.

## Project Goals

- Read UWB ranging output from ESP32-S3-connected modules.
- Estimate tag position from a fixed anchor layout for relative localization.
- Visualize UWB positions and serial output during bench tests.
- Connect localization experiments with Pixhawk telemetry/control over MAVLink.
- Support parent-child UAV communication through ESP-NOW firmware.
- Keep older experiments available while separating them from the active code path.

## Repository Structure

```text
firmware/
  docking/      Active parent/child UAV docking firmware using ESP-NOW and MAVLink.
  uwb/          UWB module tests, anchor/tag sketches, solvers, and follow experiments.
  pixhawk/      Pixhawk TELEM1 sniffing, parsing, and control test sketches.
  openmv/       OpenMV UART and AprilTag web display experiments.

tools/
  visualization/  Python viewers for UWB and camera pose serial output.
  debug/          Serial/MAVLink/SiK radio debugging scripts.

docs/
  hardware/    UWB wiring and ESP32-S3 pin-fix notes.
  reference/   PX4/MAVLink notes.

vendor/
  datasheets/  UWB module PDFs and AT command notes.

archive/
  old-main/    Earlier parent/child docking implementations and support scripts.
  prototypes/  One-off exploratory sketches kept for reference.
```

## Main Firmware

### Parent-Child UAV Docking

- `firmware/docking/UAVDocking_Mother_ESPNOW`
- `firmware/docking/UAVDocking_Child_ESPNOW`

These are the current main sketches for the aerial docking system. They use ESP-NOW for node-to-node communication and MAVLink over Pixhawk telemetry ports. The mother firmware also includes a web server interface.

### UWB Positioning

Key UWB sketches live under `firmware/uwb/`:

- `uwb_anchors_esp32_1`: ESP32-S3 anchor-side multi-UWB setup.
- `uwb_tag_esp32_2`: tag-side solver with anchor geometry and filtering.
- `uwb_tag_solver`: tag-side position solver experiment.
- `uwb_tag_module_pos`: reads the module's built-in position output.
- `uwb_follow_pixhawk`: combines UWB tag position with Pixhawk MAVLink control experiments.
- `uwb_single_test`, `uwb3_test`, `uwb_two_nodes`, `uwb_esp32s3_uart_test`: serial, wiring, and role-switching tests.

The current anchor geometry used by several sketches is a square layout with a 35.5 inch side length, using anchors at the midpoints of the four edges. Adjust those constants in the relevant sketch before using a different physical layout.

## Tools

Install Python dependencies:

```bash
pip install -r tools/requirements.txt
```

Useful scripts:

- `tools/visualization/uwb_viewer.py`: Tkinter/Matplotlib 3D viewer for solved UWB positions.
- `tools/visualization/uwb_tag_viewer.py`: Matplotlib viewer for anchor/tag ranges and tag position.
- `tools/visualization/world_camera.py`: pyqtgraph 3D camera pose viewer for OpenMV AprilTag output.
- `tools/debug/sik_debug.py`: serial debugging helper for SiK telemetry radios and MAVLink heartbeat detection.

Most scripts have COM port constants near the top of the file. Update `PORT` and `BAUD` for your local device before running.

## Hardware Notes

See:

- `docs/hardware/uwb_wiring.md`
- `docs/hardware/UWB_PIN_FIX.md`
- `vendor/datasheets/m5_uwb.txt`
- `vendor/datasheets/bu01db_at.txt`

Common ESP32-S3/UWB wiring used in the sketches:

```text
UWB1: ESP32-S3 GPIO4  -> UWB RX, GPIO5  <- UWB TX
UWB2: ESP32-S3 GPIO15 -> UWB RX, GPIO16 <- UWB TX
UWB3: ESP32-S3 GPIO21 -> UWB RX, GPIO47 <- UWB TX
UWB4: ESP32-S3 GPIO48 -> UWB RX, GPIO40 <- UWB TX
```

Pixhawk TELEM wiring used in the tests:

```text
Pixhawk TELEM TX -> ESP32-S3 RX GPIO10
Pixhawk TELEM RX -> ESP32-S3 TX GPIO11
GND              -> GND
```

## Development Setup

Arduino sketches are intended for ESP32-S3 boards. Depending on the sketch, install:

- Arduino ESP32 board support.
- MAVLink Arduino headers or library for Pixhawk-related sketches.
- EspSoftwareSerial for sketches that enable a fourth UWB serial port.

Open each sketch from its containing directory, for example:

```text
firmware/uwb/uwb_tag_solver/uwb_tag_solver.ino
firmware/docking/UAVDocking_Mother_ESPNOW/UAVDocking_Mother_ESPNOW.ino
```

Arduino IDE expects the `.ino` file name to match its folder name; this structure preserves that convention.

## Archive

`archive/old-main/` contains earlier Wi-Fi/HTTP based parent-child docking implementations and support scripts. `archive/prototypes/` contains exploratory sketches that are still useful as references but are not part of the current main path.

## License

No license file is currently included.
