#!/bin/bash

# build project
mkdir cpp/static_analysis/example/build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build cpp/static_analysis/example/build --target all -- -j8

python3 cpp/static_analysis/run_clang_tidy.py -p build -checks="*,llvm-header-guard" -export-fixes build/fixes.yaml
