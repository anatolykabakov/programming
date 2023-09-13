ARG VARIANT=ubuntu-20.04
FROM mcr.microsoft.com/vscode/devcontainers/base:0-${VARIANT}

# This Dockerfile's base image has a non-root user with sudo access. Use the "remoteUser"
# property in devcontainer.json to use it. On Linux, the container user's GID/UIDs
# will be updated to match your local UID/GID (when using the dockerFile property).
# See https://aka.ms/vscode-remote/containers/non-root-user for details.
ARG USERNAME=devel
ARG USER_UID=1000
ARG USER_GID=$USER_UID

WORKDIR /workspace/

ENV TERM xterm
ENV SHELL /bin/zsh

# Configure apt and install packages
RUN apt-get update \
    && export DEBIAN_FRONTEND=noninteractive \
    #
    # Install
    && apt-get -y install \
    git zip unzip python3-pip clang-format \
    build-essential gdb cmake cppcheck \
    valgrind kcachegrind \
    #
    # [Optional] Update UID/GID if needed
    && if [ "$USER_GID" != "1000" ] || [ "$USER_UID" != "1000" ]; then \
    groupmod --gid $USER_GID $USERNAME \
    && usermod --uid $USER_UID --gid $USER_GID $USERNAME \
    && chown -R $USER_UID:$USER_GID /home/$USERNAME; \
    fi \
    #
    # Clean up
    && apt-get autoremove -y \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

RUN sh -c "$(wget -O- https://github.com/deluan/zsh-in-docker/releases/download/v1.1.5/zsh-in-docker.sh)" -- \
    -p git \
    -p https://github.com/zsh-users/zsh-autosuggestions \
    -p https://github.com/zsh-users/zsh-completions

RUN echo "export DISABLE_AUTO_TITLE=true" >> /root/.zshrc
RUN echo 'LC_NUMERIC="en_US.UTF-8"' >> /root/.zshrc

RUN pip3 install conan pre-commit cmakelang
RUN git config --global --add safe.directory /workspace

CMD ["zsh"]

# RUN git submodule init
# RUN git submodule update --recursive --remote
