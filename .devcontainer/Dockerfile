FROM debian:stable

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install basic dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    make \
    git \
    wget \
    curl \
    unzip \
    pkg-config \
    python3 \
    python3-pip \
    gdb \
    valgrind \
    && rm -rf /var/lib/apt/lists/*

# Install C++23 compatible compilers
RUN apt-get update && apt-get install -y \
    gcc-11 \
    g++-11 \
    clang-14 \
    clang++-14 \
    && rm -rf /var/lib/apt/lists/*

# Set default compiler to GCC 11
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 100 \
    && update-alternatives --install /usr/bin/clang clang /usr/bin/clang-14 100 \
    && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-14 100

# Install development tools
RUN pip3 install --no-cache-dir gcovr

# Create non-root user for development
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && apt-get update \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /workspace

# Set the default user
USER $USERNAME

# Verify C++23 support
RUN echo "Testing C++23 support..." && \
    echo '#include <iostream>\nint main() { std::cout << "C++23 test successful\\n"; return 0; }' > /tmp/test.cpp && \
    g++ -std=c++23 -o /tmp/test /tmp/test.cpp && /tmp/test && rm /tmp/test /tmp/test.cpp
