<div align="center">
  <h1>UWB Project</h1>
  <p>UWB ranging, trilateration, ESP32-S3 firmware, and visualization for a mother-ship UAV docking system.</p>

  <p>
    <a href="README.zh-CN.md">Chinese</a>
    &middot;
    <a href="https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System">Main Project</a>
    &middot;
    <a href="https://github.com/Ha22yX/OpenMV-AprilTag">Vision Module</a>
    &middot;
    <a href="https://isef.rosebeg.com">Project Website</a>
    &middot;
    <a href="#quickstart">Quickstart</a>
    &middot;
    <a href="#features">Features</a>
    &middot;
    <a href="#tech-stack">Tech Stack</a>
  </p>

  <p>
    <img alt="Arduino: ESP32-S3" src="https://img.shields.io/badge/Arduino-ESP32--S3-00878F?style=for-the-badge&logo=arduino&logoColor=white" />
    <img alt="UWB: trilateration" src="https://img.shields.io/badge/UWB-trilateration-287866?style=for-the-badge" />
    <img alt="Pixhawk: MAVLink" src="https://img.shields.io/badge/Pixhawk-MAVLink-2f6f67?style=for-the-badge" />
    <img alt="Status: bench tests" src="https://img.shields.io/badge/Status-bench%20tests-6b7f73?style=for-the-badge" />
  </p>
</div>

<p align="center">
  <img src=".github/assets/readme-hero.svg" alt="UWB Project overview image" width="100%" />
</p>

## Overview

This repository owns the mid-range relative-position layer of the Mother-Ship Docking Drone System. GPS/RTK can bring the child UAV near the mother platform, but docking needs a local coordinate estimate relative to the mother frame.

The experiments here focus on turning UWB anchor-to-tag ranges into an inspectable `x, y, z` estimate. I keep the UWB module here and the code that connects it to the flight controller in the main project.

## Drone firmware moved to the main project

Some of my mother/child control code ended up in this repository while I was developing the UWB module. I have moved those files to [Mother-Ship-Docking-Drone-System](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System), so the whole-drone experiments are easier to find together.

| Previous location here | New location in the main project |
| --- | --- |
| `firmware/docking/` | [Mother/child ESP-NOW firmware](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System/tree/main/firmware/docking) |
| `firmware/pixhawk/` | [Pixhawk communication and control experiments](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System/tree/main/firmware/pixhawk) |
| `firmware/openmv/` | [ESP32-side OpenMV integration](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System/tree/main/firmware/openmv) |
| `firmware/uwb/uwb_follow_pixhawk/` | [UWB-to-Pixhawk follow experiment](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System/tree/main/firmware/integration/uwb_follow_pixhawk) |
| `archive/old-main/` | [Earlier whole-drone prototypes](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System/tree/main/archive/old-main) |
| `tools/debug/sik_debug.py` | [SiK radio debugging](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System/blob/main/tools/debug/sik_debug.py) |
| `tools/visualization/world_camera.py` | [AprilTag pose viewer](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System/blob/main/tools/visualization/world_camera.py) |
| `docs/reference/px4_mavlink_docs.md` | [PX4 / MAVLink reference](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System/blob/main/docs/reference/px4_mavlink_docs.md) |

The [migration notes](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System/blob/main/docs/repository-layout.md) record the original revision and the 55 moved files. UWB ranging, anchor/tag firmware, standalone position solvers, viewers, wiring notes, and module reference material remain here. The code contents were preserved during the move.

## Features

- ESP32-S3 Arduino sketches for UWB anchor/tag communication and ranging.
- Four-anchor docking-frame geometry for relative localization experiments.
- Trilateration and least-squares solving path for converting ranges into local position.
- Python serial viewers for inspecting UWB distance and position streams.
- Companion role to the main docking project, alongside the OpenMV AprilTag vision module.

## How It Works

1. Mount UWB anchors on the mother docking frame and a UWB tag on the child UAV.
2. Read per-anchor distance packets from the ESP32-S3/UWB hardware path.
3. Filter distance spikes and map ranges to the configured anchor geometry.
4. Solve the relative position estimate and visualize it on the PC tools.
5. Use the result as the mid-range localization signal before terminal vision alignment.

## Quickstart

Clone the repo, install the PC visualization dependencies, and choose a sketch from `firmware/uwb/` for Arduino IDE or your ESP32 toolchain.

```bash
git clone https://github.com/Ha22yX/UWB-Project.git
cd UWB-Project
pip install -r tools/requirements.txt
# Open a matching .ino from firmware/uwb/ in Arduino IDE
python tools/visualization/uwb_viewer.py
```

Update serial ports, UWB IDs, anchor coordinates, and baud rates for your actual hardware before running the viewers. `uwb_viewer.py` also needs a Python installation with Tkinter support. Flight-controller integration sketches are now in the [main project's firmware guide](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System/blob/main/firmware/README.md).

## Configuration

| Item | What to adjust |
| --- | --- |
| Anchor geometry | Measure anchor positions on the mother docking frame and keep units consistent. |
| Serial ports | Set the ESP32-S3 / UWB serial port and baud rate used by the viewer scripts. |
| UWB IDs | Match firmware anchor/tag IDs to the physical modules. |

## Tech Stack

| Layer | Technology | Role |
| --- | --- | --- |
| Firmware | Arduino, ESP32-S3 | UWB anchor/tag and ranging experiments. |
| Localization | UWB trilateration | Convert ranges into mother-frame relative position. |
| Visualization | pyserial, matplotlib, Tkinter | Inspect UWB serial data, distances, and positions. |
| Integration in the main project | Pixhawk, MAVLink | Connect the position estimate to flight-control experiments. |

## Project Layout

```text
firmware/uwb/             UWB ranging and solver sketches
tools/visualization/      UWB distance and position viewers
tools/requirements.txt    Dependencies for the UWB viewers
docs/                    UWB wiring notes and module reference material
archive/prototypes/      Earlier UWB prototypes kept for context
```

## Status

Hardware research workspace. It is useful for bench testing and documentation, but it is not a ready-to-fly autopilot or safety-certified docking controller.

## Related Projects

- [Mother-Ship-Docking-Drone-System](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System) - main autonomous docking project.
- [OpenMV-AprilTag](https://github.com/Ha22yX/OpenMV-AprilTag) - terminal visual localization module.

## License

[MIT](LICENSE).
