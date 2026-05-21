#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(dirname "$(realpath "$0")")
APP_DIR=$(realpath "$SCRIPT_DIR/..")
EXECUTABLE="$APP_DIR/build/bin/collector_demo"
DEBUG=false
CONFIG_PATH="$APP_DIR/config.json"
APP_ARGS=()

print_help() {
    cat <<EOF
Usage: ./apps/examples/collector_demo/scripts/run.sh [options]

Run the collector_demo app.

Options:
  --config PATH     Path to config.json (default: apps/examples/collector_demo/config.json)
  --mode MODE       Run mode override: summary, compact, full
  --debug           Run under gdb
  -h, --help        Show this help message

Build first with:
  ./apps/examples/collector_demo/scripts/build.sh
EOF
}

while [[ "$#" -gt 0 ]]; do
    case $1 in
        --config) CONFIG_PATH="$2"; shift 2 ;;
        --mode) APP_ARGS+=("--mode" "$2"); shift 2 ;;
        --debug) DEBUG=true; shift ;;
        -h|--help) print_help; exit 0 ;;
        *) APP_ARGS+=("$1"); shift ;;
    esac
done

if [ ! -f "$EXECUTABLE" ]; then
    echo "Executable not found: $EXECUTABLE" >&2
    echo "Build it with ./apps/examples/collector_demo/scripts/build.sh." >&2
    exit 1
fi

if [ ! -f "$CONFIG_PATH" ]; then
    echo "Config file not found: $CONFIG_PATH" >&2
    exit 1
fi

if [ "$DEBUG" = true ]; then
    exec gdb --args "$EXECUTABLE" --config "$CONFIG_PATH" "${APP_ARGS[@]}"
else
    exec "$EXECUTABLE" --config "$CONFIG_PATH" "${APP_ARGS[@]}"
fi
