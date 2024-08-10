#!/bin/bash

# install clang-tidy
wget https://apt.llvm.org/llvm.sh
chmod u+x llvm.sh
sudo ./llvm.sh 20
sudo apt install clang-tidy-20
# sudo ln -s /usr/bin/clang-tidy-20 /usr/bin/clang-tidy
clang-tidy --version
