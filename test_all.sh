#!/bin/bash
set -euo pipefail

echo ""
echo "========================================================================"
echo " TEST REPORT "
echo "========================================================================"

# Dev Note: Some executable below are fixed to kicad only and thus does not have zero style mode
# ./test_standard_single.sh <EXECUTABLE>            <SKIP ZERO STYLE>
./test_standard_single.sh ./sexp_prettify_kicad_cli true
./test_standard_single.sh ./sexp_prettify_cpp_cli   false
./test_standard_single.sh ./sexp_prettify_cli.py    true
./test_standard_single.sh ./sexp_prettify           false

echo "All tests passed in all executables!"
exit 0