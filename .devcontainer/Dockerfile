FROM ubuntu:22.04

ARG USERNAME=${USERNAME:-user}
ARG USER_UID=1000
ARG USER_GID=$USER_UID

ENV DEBIAN_FRONTEND=noninteractive
# Install system packages
RUN apt update && apt install -y \
    # Basic utilities
    curl git git-lfs wget zip unzip tar software-properties-common sudo \
    # Build tools
    cmake g++ gcc make build-essential ninja-build automake libtool autoconf-archive pkg-config \
    # Cross-compilation toolchain (ARM64)
    gcc-aarch64-linux-gnu g++-aarch64-linux-gnu binutils-aarch64-linux-gnu \
    # Python ecosystem
    python3 python3-pip python3-tk python3-matplotlib \
    # Text editors
    nano vim gedit \
    && apt clean && rm -rf /var/lib/apt/lists/*
# Create non-root user and configure environment
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && usermod -aG sudo $USERNAME \
    && echo "$USERNAME ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME
# Install Python packages and setup symlinks
RUN pip3 install jinja2 pre-commit cmakelang yapf pylint pybind11-stubgen conan
# Install LLVM
RUN wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh \
    && ./llvm.sh 20 \
    && DEBIAN_FRONTEND=noninteractive apt update \
    && apt install -y clang-tidy-20 clang-format-20 \
    && ln -s /usr/bin/clang-tidy-20 /usr/bin/clang-tidy \
    && ln -s /usr/bin/clang-format-20 /usr/bin/clang-format \
    && clang-tidy --version && clang-format --version \
    && apt clean && rm -rf /var/lib/apt/lists/* llvm.sh
# Setup workspace and install zsh
RUN sh -c "$(wget -O- https://github.com/deluan/zsh-in-docker/releases/download/v1.2.1/zsh-in-docker.sh)" -- \
    -p git -p ssh-agent -p 'history-substring-search' \
    -a 'bindkey "\$terminfo[kcuu1]" history-substring-search-up' \
    -a 'bindkey "\$terminfo[kcud1]" history-substring-search-down'
USER $USERNAME
WORKDIR /workspace
