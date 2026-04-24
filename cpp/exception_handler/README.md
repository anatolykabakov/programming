# exception_handler playground

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

## simple_backtrace example

`simple_backtrace` registers signal handlers and prints a backtrace on crash.

```bash
cmake --build build --target simple_backtrace -j
./build/simple_backtrace
```

Notes:
- target is built with `-g -fno-omit-frame-pointer -rdynamic` for readable symbols.
- process ends with `Segmentation fault` after printing stack trace (expected).

## coredump example

`coredump` enables core dumps via `setrlimit(RLIMIT_CORE, RLIM_INFINITY)` and crashes intentionally.

```bash
cmake --build build --target coredump -j
./build/coredump
```

If core files are not created in current directory:

```bash
ulimit -c unlimited
sudo sysctl -w kernel.core_pattern='core.%e.%p'
```

Read core with gdb:

```bash
gdb ./build/coredump core.<exe>.<pid>
# inside gdb:
bt
bt full
```

## Breakpad references

- https://chromium.googlesource.com/breakpad/breakpad/+/HEAD/docs/linux_starter_guide.md
- https://github.com/google/breakpad/blob/main/docs/linux_starter_guide.md
- https://docs.sentry.io/platforms/native/guides/breakpad/
- https://github.com/google/breakpad/tree/main

### Quick Breakpad flow

```bash
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=/workspace/cpp/exception_handler/depot_tools:$PATH
mkdir breakpad && cd breakpad
fetch breakpad
cd src
./configure && make
make install

cmake -S . -B build && cmake --build build
./build/exception_handler
ls /tmp/minidump.tmp

breakpad/src/src/tools/linux/dump_syms/dump_syms ./build/exception_handler > build/exception_handler.sym
head -n1 build/exception_handler.sym
mkdir -p ./build/symbols/exception_handler/09764CC86003A7131BEF44274F4B391E0
mv build/exception_handler.sym ./build/symbols/exception_handler/09764CC86003A7131BEF44274F4B391E0/

breakpad/src/src/processor/minidump_stackwalk /tmp/minidump.dmp ./build/symbols | tee build/minidump.log
```
