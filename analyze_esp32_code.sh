#!/bin/bash

# Usage: ./analyze_esp32_code.sh /path/to/your/esp32/arduino/code

CODE_DIR="$1"

if [[ -z "$CODE_DIR" || ! -d "$CODE_DIR" ]]; then
  echo "Usage: $0 /path/to/esp32/arduino/code"
  exit 1
fi

echo "Analyzing ESP32 Arduino code in: $CODE_DIR"
echo "-------------------------------------------"

# Step 1: Static analysis and maintainability
echo -e "\n🧠 Running cppcheck (static analysis & complexity)..."
cppcheck --enable=all --inconclusive --std=c++17 --language=c++ --force "$CODE_DIR" 2> cppcheck_report.txt
echo "Static analysis complete. See cppcheck_report.txt for details."

# Step 2: Cyclomatic Complexity and Maintainability Index
echo -e "\n📊 Running lizard (complexity and maintainability)..."
lizard "$CODE_DIR" > lizard_report.txt
cat lizard_report.txt | head -n 20  # preview top 20 lines
echo "Full complexity report saved to lizard_report.txt"

# Step 3: Codebase Structure and Size
echo -e "\n📦 Running cloc (code size and structure)..."
cloc "$CODE_DIR"

echo -e "\n✅ Analysis complete!"
