#!/bin/bash

echo "Running passing tests:"
for f in should_pass/*.alo; do
    echo -n "$f: "
    if ./alo "$f" >/dev/null 2>&1; then
        echo "PASS"
    else
        echo "FAIL (should pass!)"
    fi
done

echo
echo "Running failing tests:"
for f in should_fail/*.alo; do
    echo -n "$f: "
    if ./alo "$f" >/dev/null 2>&1; then
        echo "PASS (should fail!)"
    else
        echo "FAIL (correct)"
    fi
done
