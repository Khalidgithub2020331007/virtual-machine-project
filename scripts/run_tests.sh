#!/usr/bin/env bash
# Automated test runner for the VM.
# Usage: bash scripts/run_tests.sh  (from repo root)
# Exit code: 0 if all tests pass, 1 if any fail.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VM="$REPO_ROOT/vm"
PROGRAMS="$REPO_ROOT/programs"
MAKE_PROGRAMS="$PROGRAMS/make_programs"

PASS=0
FAIL=0

# ── helpers ────────────────────────────────────────────────────────────────────

red()   { printf '\033[0;31m%s\033[0m\n' "$*"; }
green() { printf '\033[0;32m%s\033[0m\n' "$*"; }

check() {
    local name="$1" binary="$2" input="$3"
    shift 3
    local expected=("$@")

    printf '  %-20s' "$name"

    # Run the VM; feed stdin if needed
    if [[ -n "$input" ]]; then
        actual=$(printf '%s\n' "$input" | "$VM" "$binary" 2>/dev/null)
    else
        actual=$("$VM" "$binary" 2>/dev/null)
    fi
    exit_code=$?

    # Check exit code
    if [[ $exit_code -ne 0 ]]; then
        FAIL=$((FAIL + 1))
        red "FAIL (exit code $exit_code)"
        return
    fi

    # Extract only numeric output lines (strip VM diagnostic messages).
    # Also handle "Input: N" lines where the prompt and result share a line.
    mapfile -t got < <(sed 's/^Input: //' <<< "$actual" | grep -E '^-?[0-9]+$')

    local mismatch=0
    if [[ ${#got[@]} -ne ${#expected[@]} ]]; then
        mismatch=1
    else
        for i in "${!expected[@]}"; do
            if [[ "${got[$i]}" != "${expected[$i]}" ]]; then
                mismatch=1
                break
            fi
        done
    fi

    if [[ $mismatch -eq 0 ]]; then
        PASS=$((PASS + 1))
        green "PASS"
    else
        FAIL=$((FAIL + 1))
        red "FAIL"
        printf '      expected: %s\n' "${expected[*]}"
        printf '      got:      %s\n' "${got[*]}"
    fi
}

# ── build phase ────────────────────────────────────────────────────────────────

echo "=== Build ==="

# Build VM binary
if [[ ! -x "$VM" ]]; then
    printf '  Building vm ... '
    make -C "$REPO_ROOT" vm -s && green "ok" || { red "FAILED"; exit 1; }
fi

# Build program generator and regenerate .bin files
printf '  Generating .bin files ... '
make -C "$REPO_ROOT" programs -s && green "ok" || { red "FAILED"; exit 1; }

echo ""
echo "=== Tests ==="

# ── test cases ─────────────────────────────────────────────────────────────────

# Basic add: R0 = 10 + 20, expect 30
check "basic_add"  "$PROGRAMS/test.bin"      "" 30

# Countdown: print 10 down to 0, expect 11 numbers
check "countdown"  "$PROGRAMS/countdown.bin" "" 10 9 8 7 6 5 4 3 2 1 0

# Factorial: 5! = 120
check "factorial"  "$PROGRAMS/factorial.bin" "" 120

# Fibonacci: first 10 numbers
check "fibonacci"  "$PROGRAMS/fibonacci.bin" "" 0 1 1 2 3 5 8 13 21 34

# Input test: read N=7, print 7*2 = 14
check "input_test" "$PROGRAMS/input_test.bin" "7" 14

# ── summary ────────────────────────────────────────────────────────────────────

echo ""
echo "=== Results: $PASS passed, $FAIL failed ==="

[[ $FAIL -eq 0 ]]
