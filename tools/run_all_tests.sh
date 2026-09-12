#!/usr/bin/env bash
set -euo pipefail

echo "==> Running standard tests..."
plz test //...

echo "==> Running optimized tests (-c opt)..."
plz test -c opt //...

echo "==> Running AddressSanitizer tests (--profile asan)..."
plz test --profile asan //...

echo "==> All test suites passed successfully!"
