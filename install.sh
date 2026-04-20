#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_SRC="$SCRIPT_DIR/build/it100mqtt"
BIN_DST="/usr/local/bin/it100mqtt"
CFG_SRC="$SCRIPT_DIR/settings-example.conf"
CFG_DST="/usr/local/etc/it100mqtt.conf"
SERVICE_SRC="$SCRIPT_DIR/dist/systemd/it100mqtt.service"
SERVICE_DST="/etc/systemd/system/it100mqtt.service"

if [[ "${EUID}" -eq 0 ]]; then
	SUDO=()
elif command -v sudo >/dev/null 2>&1; then
	SUDO=(sudo)
else
	echo "Error: this script needs root privileges and 'sudo' is not available." >&2
	exit 1
fi

echo "Building it100mqtt..."
"$SCRIPT_DIR/build.sh"

if [[ ! -f "$BIN_SRC" ]]; then
	echo "Error: build output not found at $BIN_SRC" >&2
	exit 1
fi

if [[ ! -f "$CFG_SRC" ]]; then
	echo "Error: config template not found at $CFG_SRC" >&2
	exit 1
fi

if [[ ! -f "$SERVICE_SRC" ]]; then
	echo "Error: systemd service file not found at $SERVICE_SRC" >&2
	exit 1
fi

echo "Installing binary to $BIN_DST..."
"${SUDO[@]}" install -D -m 0755 "$BIN_SRC" "$BIN_DST"

if [[ ! -f "$CFG_DST" ]]; then
	echo "Installing default config to $CFG_DST..."
	"${SUDO[@]}" install -D -m 0660 "$CFG_SRC" "$CFG_DST"
fi

echo "Ensuring config owner/group and permissions..."
"${SUDO[@]}" chown root:root "$CFG_DST"
"${SUDO[@]}" chmod 0660 "$CFG_DST"

echo "Installing systemd service to $SERVICE_DST..."
"${SUDO[@]}" install -D -m 0644 "$SERVICE_SRC" "$SERVICE_DST"

echo "Reloading systemd and enabling/starting it100mqtt..."
"${SUDO[@]}" systemctl daemon-reload
"${SUDO[@]}" systemctl enable it100mqtt.service
"${SUDO[@]}" systemctl start it100mqtt.service

echo "Install complete."
