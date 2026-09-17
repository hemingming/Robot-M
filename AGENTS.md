
# Robot-M Agent Guide

## Project Shape
Project build complete
- This is a PlatformIO Arduino firmware project for an ESP32-S3 DevKit.
- The active environment is `esp32-s3-devkitc-1`; the target uses 32 MB flash and PSRAM.
- Hardware inventory and intended peripherals are documented in [README.md](README.md).
- Keep hardware-specific constants in [src/robot_config.h](src/robot_config.h). Do not scatter pin numbers, I2C addresses, or timing values through the runtime.

## Source Boundaries

- [src/main.cpp](src/main.cpp) owns Arduino lifecycle, peripheral initialization, the loop cadence, and serial diagnostics.
- [src/robot_runtime.h](src/robot_runtime.h) and [src/robot_runtime.cpp](src/robot_runtime.cpp) own sensor snapshots and robot mode transitions. Keep this layer independent of display and sensor-driver calls.
- Platform and display/library configuration belongs in [platformio.ini](platformio.ini). Preserve the ESP32-S3 board, flash, PSRAM, TFT, and dependency settings unless the hardware target is intentionally changing.
- Treat `RobotRuntime` as a safety boundary: low battery forces `EmergencyStop`, and an unhealthy or tilted walking state enters `Recovery`.

## Development Loop

- Build with `pio run` from the repository root.
- Upload only when hardware is connected: `pio run --target upload`.
- Use `pio device monitor` at 115200 baud for boot, mode, battery, and distance diagnostics.
- There is currently no test directory or host-test setup. For runtime-state changes, add or run focused tests if a test harness is introduced; otherwise validate with a build and explain hardware-only checks that were not possible.
- If `pio` is unavailable, report that prerequisite instead of substituting an unrelated build command.

## Change Rules

- Prefer small, local changes and preserve the existing Arduino/C++ style.
- Update configuration and documentation when a pin, bus, address, dependency, board setting, or safety threshold changes.
- Keep sensor acquisition in the hardware-facing layer and pass plain data through `SensorSnapshot`; do not make the state machine reach into peripheral drivers.
- Do not commit machine-specific serial monitor paths or unrelated generated PlatformIO output.
- Before claiming success, run the narrowest relevant PlatformIO build or test command available and report any missing hardware or toolchain limitation.

