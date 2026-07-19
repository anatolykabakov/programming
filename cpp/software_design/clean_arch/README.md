# Description
Блокнот хранит записи студентов (имя). Можно добавлять записи и вывести весь список.
Имя не может быть пустым


# Build & Run

```
conan install . --build=missing 2>&1

cmake -B build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake
cmake --build build

```
