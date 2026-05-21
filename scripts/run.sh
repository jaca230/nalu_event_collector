#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(dirname "$(realpath "$0")")
PROJECT_DIR=$(realpath "$SCRIPT_DIR/..")

print_help() {
    cat <<EOF
Usage: ./scripts/run.sh <app> [app args...]

Run one of the standalone example apps.

Available apps:
  collector_demo

Examples:
  ./scripts/run.sh collector_demo
  ./scripts/run.sh collector_demo --background
EOF
}

if [ "$#" -lt 1 ]; then
    print_help
    exit 1
fi

APP_NAME="$1"
shift

case "$APP_NAME" in
    collector_demo)
        exec "$PROJECT_DIR/apps/examples/collector_demo/scripts/run.sh" "$@"
        ;;
    -h|--help)
        print_help
        exit 0
        ;;
    *)
        echo "Unknown app: $APP_NAME" >&2
        echo >&2
        print_help
        exit 1
        ;;
esac
