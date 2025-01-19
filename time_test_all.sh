#!/bin/bash
set -euo pipefail

# List of formatter executables
executables=(
    "./sexp_prettify_cli.py"
    "./sexp_prettify_kicad_original_cli"
    "./sexp_prettify_kicad_cli"
    "./sexp_prettify_cpp_cli"
    "./sexp_prettify"
)


echo ""
echo "========================================================================"
echo " Timing Test Report "
echo "========================================================================"

# Run each formatter executable on the unformatted file
for executable in "${executables[@]}"; do
    # Make sure the executable exists and is executable
    if [[ ! -x "$executable" ]]; then
        echo "ERROR: Executable '$executable' not found or not executable!"
        exit 1
    fi
    # Run timing test
    ./time_test.sh "$executable"
done
