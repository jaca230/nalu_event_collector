#!/bin/bash

# Parse arguments
BACKGROUND=false
DEBUG=false

# Function to print usage/help
print_help() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --debug          Run the executable with debugger (gdb)"
    echo "  --background     Run the executable in background mode"
    echo "  --help           Display this help message"
    echo ""
    echo "If --background is specified, the collector will start in background mode."
    echo "Otherwise, you can manually call the 'collect' method within the executable."
}

# Check if help is requested
if [[ "$#" -eq 1 && "$1" == "--help" ]]; then
    print_help
    exit 0
fi

while [[ "$#" -gt 0 ]]; do
    case $1 in
        --debug) DEBUG=true; shift ;;
        --background) BACKGROUND=true; shift ;;
        --help) print_help; exit 0 ;;
        *) echo "Unknown option: $1"; print_help; exit 1 ;;
    esac
done

# Get the absolute path of the script directory
SCRIPT_DIR=$(dirname "$(realpath "$0")")

# Path to the executable
EXECUTABLE="$SCRIPT_DIR/../build/bin/main"

# Check if the executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo "Executable not found! Please run ./scripts/build.sh first."
    exit 1
fi

# Prepare the arguments to pass to the executable
EXEC_ARGS=()
# Run with or without debugger based on the --debug flag
if [ "$DEBUG" == "true" ]; then
    echo "Running with debugger (gdb)..."
    gdb --args "$EXECUTABLE" "${EXEC_ARGS[@]}"
else
    if [ "$BACKGROUND" == "true" ]; then
        echo "Running the executable in background mode..."
        "$EXECUTABLE" "${EXEC_ARGS[@]}" &
    else
        echo "Running the executable..."
        "$EXECUTABLE" "${EXEC_ARGS[@]}"
    fi
fi

