#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(dirname "$(realpath "$0")")
PROJECT_DIR=$(realpath "$SCRIPT_DIR/..")

echo "[cleanup.sh] Cleaning project build artifacts in: $PROJECT_DIR"

find "$PROJECT_DIR" -maxdepth 4 -type d -name build -prune -exec rm -rf {} +

echo "[cleanup.sh] Cleanup complete."
