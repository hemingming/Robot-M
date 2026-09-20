#!/bin/bash
echo '!!output-start-cell'
cd /Users/hemingming/worker/Robot-M
cd xiaozhi-esp32
source "$HOME/esp/esp-idf/export.sh"
idf.py --version
python3 -m serial.tools.list_ports -v#!/bin/bash
echo '!!output-start-cell'
cd /Users/hemingming/worker/Robot-M
export FLASH_PORT=/dev/cu.usbmodemXXXX
export LOG_PORT=/dev/cu.usbmodemYYYY#!/bin/bash
echo '!!output-start-cell'
cd /Users/hemingming/worker/Robot-M
python3 scripts/build.py robot-m --name robot-m#!/bin/bash
echo '!!output-start-cell'
cd /Users/hemingming/worker/Robot-M
idf.py -p "$FLASH_PORT" flash#!/bin/bash
echo '!!output-start-cell'
cd /Users/hemingming/worker/Robot-M
cd build
python3 -m esptool --chip esp32s3 \
  --port "$FLASH_PORT" --baud 460800 \
  --before no-reset --after no-reset \
  write-flash @flash_args
cd ..#!/bin/bash
echo '!!output-start-cell'
cd /Users/hemingming/worker/Robot-M
idf.py -p "$FLASH_PORT" app-flash#!/bin/bash
echo '!!output-start-cell'
cd /Users/hemingming/worker/Robot-M
idf.py -p "$LOG_PORT" monitor#!/bin/bash
echo '!!output-start-cell'
cd /Users/hemingming/worker/Robot-M
python3 -m serial.tools.miniterm "$LOG_PORT" 115200 \
  --raw --echo --dtr 0 --rts 0#!/bin/bash
echo '!!output-start-cell'
cd /Users/hemingming/worker/Robot-M
pio device list
pio run
pio device monitor --port "$LOG_PORT" --baud 115200#!/bin/bash
echo '!!output-start-cell'
cd /Users/hemingming/worker/Robot-M
pio run --target upload --upload-port "$FLASH_PORT"#!/bin/bash
echo '!!output-start-cell'
cd /Users/hemingming/worker/Robot-M
cp -n src/wifi_secrets.h.example src/wifi_secrets.h#!/bin/bash
echo '!!output-start-cell'
cd /Users/hemingming/worker/Robot-M
git status --short
git diff --stat
git diff --check#!/bin/bash
echo '!!output-start-cell'
cd /Users/hemingming/worker/Robot-M
python3 -m unittest discover -s scripts/tests -v#!/bin/bash
echo '!!output-start-cell'
cd /Users/hemingming/worker/Robot-M
烧录