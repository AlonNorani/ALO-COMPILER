#!/bin/bash

# Path to your compiler binary (relative to this script)
COMPILER="./build/ALO-COMPILER"

if [ ! -f "$COMPILER" ]; then
    echo "Error: Compiler binary not found at $COMPILER"
    exit 1
fi

echo "Running passing tests:"
for f in tests/should_pass/*.alo; do
    echo -n "$f: "
    if "$COMPILER" "$f" >/dev/null 2>&1; then
        echo -e "\e[32mPASS\e[0m"
    else
        echo -e "\e[31mFAIL (should pass!)\e[0m"
    fi
done

echo
echo "Running failing tests:"
for f in tests/should_fail/*.alo; do
    echo -n "$f: "
    if "$COMPILER" "$f" >/dev/null 2>&1; then
        echo -e "\e[31mPASS (should fail!)\e[0m"
    else
        echo -e "\e[32mFAIL (correct)\e[0m"
    fi
done