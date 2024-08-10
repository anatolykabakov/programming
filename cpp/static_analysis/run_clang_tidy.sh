#!/bin/bash

# build the example project
mkdir cpp/static_analysis/example/build
cmake -S cpp/static_analysis/example -B cpp/static_analysis/example/build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build cpp/static_analysis/example/build --target all -- -j8

# run clang-tidy
python3 cpp/static_analysis/run-clang-tidy.py -p cpp/static_analysis/example/build -config-file=cpp/static_analysis/.clang-tidy
