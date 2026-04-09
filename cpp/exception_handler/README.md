https://chromium.googlesource.com/breakpad/breakpad/+/HEAD/docs/linux_starter_guide.md
https://github.com/google/breakpad/blob/main/docs/linux_starter_guide.md
https://docs.sentry.io/platforms/native/guides/breakpad/
https://github.com/google/breakpad/tree/main

git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=/workspace/cpp/exception_handler/depot_tools:$PATH
mkdir breakpad && cd breakpad
fetch breakpad
cd src
./configure && make
make install

cmake -S . -B build  && cmake --build build
./build/exception_handler
ls /tmp/minidump.tmp

breakpad/src/src/tools/linux/dump_syms/dump_syms ./build/exception_handler > build/exception_handler.sym
head -n1 build/exception_handler.sym
mkdir -p ./build/symbols/exception_handler/09764CC86003A7131BEF44274F4B391E0
mv build/exception_handler.sym ./build/symbols/exception_handler/09764CC86003A7131BEF44274F4B391E0/

breakpad/src/src/processor/minidump_stackwalk /tmp/minidump.dmp ./build/symbols | tee build/minidump.log
