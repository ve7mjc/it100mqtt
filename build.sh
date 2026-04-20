#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

mkdir -p "$SCRIPT_DIR/build" && \
    qmake -o "$SCRIPT_DIR/build/Makefile" "CONFIG += warn_off debug" "$SCRIPT_DIR/src/it100mqtt.pro" && \
    make -C "$SCRIPT_DIR/build"
