#!/usr/bin/env bash
# Build each test, run it, and report. A test is a PASS only if it compiles,
# links, runs, exits 0, and prints no line containing "FAIL".
set -u
cd "$(dirname "$0")/.."
# COMP may be set by the caller (e.g. ctest points it at the CMake build);
# otherwise default to bin/comp and build it with CMake if it's missing.
COMP="${COMP:-./bin/comp}"
if [ ! -x "$COMP" ]; then
    cmake -B build >/dev/null 2>&1 && cmake --build build >/dev/null 2>&1 \
        || { echo "could not build $COMP"; exit 1; }
fi

pass=0; fail=0
for src in $(find tests -name '*.mpl' | sort); do
    name=$(basename "$src")
    exe="${src%.mpl}.exe"
    if ! "$COMP" "$src" -o "$exe" >/tmp/mpl_build.log 2>&1; then
        echo "BUILD FAIL  $name"; sed 's/^/    /' /tmp/mpl_build.log; fail=$((fail+1)); continue
    fi
    out=$("$exe" 2>&1); code=$?
    echo "=== $name (exit $code) ==="
    echo "$out" | sed 's/^/    /'
    if [ $code -eq 0 ] && ! echo "$out" | grep -q FAIL; then pass=$((pass+1)); else fail=$((fail+1)); fi
done
echo "-----------------------------------"
echo "tests passed: $pass, failed: $fail"
exit $([ $fail -eq 0 ] && echo 0 || echo 1)
