#!/bin/bash

# === Configuration ===
SKETCH="$1"
BOARD="esp32:esp32:esp32"  # Replace with your specific ESP32 board if needed
BUILD_DIR="./build"
CPP_STANDARD="c++17"

# === Tools Check ===
command -v arduino-cli >/dev/null 2>&1 || { echo >&2 "arduino-cli is not installed. Aborting."; exit 1; }
command -v cppcheck >/dev/null 2>&1 || { echo >&2 "cppcheck is not installed. Aborting."; exit 1; }

# === Step 1: Compile with arduino-cli ===
echo "==> [1/3] Compiling with arduino-cli..."
arduino-cli compile --fqbn $BOARD "$SKETCH" --output-dir "$BUILD_DIR"
if [ $? -ne 0 ]; then
    echo "❌ Compilation failed. Please fix the above syntax/type errors."
    exit 1
else
    echo "✅ Compilation succeeded. No syntax/type errors detected."
fi

# === Step 2: Static analysis with cppcheck ===
echo "==> [2/3] Running cppcheck for static analysis..."
cppcheck --enable=all --inconclusive --std=$CPP_STANDARD --quiet "$SKETCH" 2> cppcheck_report.txt
if [ -s cppcheck_report.txt ]; then
    echo "🔍 cppcheck found potential issues:"
    cat cppcheck_report.txt
else
    echo "✅ No major issues found by cppcheck."
fi

# === Step 3: Optional - clang-tidy ===
if command -v clang-tidy >/dev/null 2>&1; then
    echo "==> [3/3] Running clang-tidy..."
    find "$SKETCH" -name '*.ino' -o -name '*.cpp' | while read file; do
        clang-tidy "$file" -- -std=$CPP_STANDARD
    done
else
    echo "ℹ️ Skipping clang-tidy (not installed)."
fi

# === Step 4: Code Cleanliness Suggestions ===
echo "==> [Bonus] Recommendations:"
echo "🧹 Code cleanup tips:"
grep -r "delay(" "$SKETCH" && echo "⚠️ Consider using non-blocking delays (millis) instead of delay()."
grep -r "String " "$SKETCH" && echo "⚠️ Avoid dynamic String objects on ESP32 (heap fragmentation risk)."
grep -r "Serial.print" "$SKETCH" | wc -l | awk '{if($1>10) print "⚠️ Too much Serial.print usage. Consider conditional debugging macros."}'

echo "✅ Analysis complete."
