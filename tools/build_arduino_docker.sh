#!/bin/sh
# Optional build path for hosts unable to execute PlatformIO's AVR toolchain.
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
docker run --rm --platform linux/amd64 \
    --mount "type=bind,source=$project_dir,target=/project" \
    --mount type=volume,source=cdj800spi-platformio-linux,target=/root/.platformio \
    -w /project -e PLATFORMIO_BUILD_DIR=/project/.pio/build-linux \
    python:3.11-slim sh -c \
    'pip install --quiet --root-user-action=ignore platformio==6.2.0 && pio run -e uno -e mega2560 -e uno_midi'
