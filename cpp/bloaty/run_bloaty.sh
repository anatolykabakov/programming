#!/bin/bash

# build project
mkdir cpp/bloaty/example/build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build cpp/bloaty/example/build --target all -- -j8
cmake --build cpp/bloaty/example/build --target install -j8 -- DESTDIR=install-dir/
cmake --build cpp/bloaty/example/build --target install/strip -j8 -- DESTDIR=install-dir-strip/

# build bloaty
git clone --recursive https://github.com/google/bloaty.git
cmake -B bloaty/build -G Ninja -S . && cmake --build bloaty/build && cmake --build bloaty/build --target install

# make file size profiler report
./bloaty/build/bloaty cpp/bloaty/example/build/install-dir-strip/main \
    -d bloaty_package,compileunits \
    -c cpp/bloaty/example/pattters.bloaty \
    -s file \
    -n 0 \
    --debug-file=cpp/bloaty/example/build/install-dir/main | tee file_size_report.txt

# make file size profiler diff report
./bloaty/build/bloaty cpp/bloaty/example/build/install-dir-strip/main -- cpp/bloaty/example/build/old/install-dir-strip/old_main \
    -d compileunits \
    -c cpp/bloaty/example/pattters.bloaty \
    -s file \
    -n 0 \
    --debug-file=cpp/bloaty/example/build/install-dir/main \
    --debug-file=cpp/bloaty/example/build/install-dir/old_main | tee file_size_diff_report.txt
