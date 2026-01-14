# Use Ubuntu 24.04 for the most modern base packages
FROM mcr.microsoft.com/devcontainers/cpp

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install core build tools and the libraries found on your Mac environment
RUN apt-get update && apt-get -y install --no-install-recommends \
    lsb-release \
    wget \
    software-properties-common \
    gnupg \
    build-essential \
    ninja-build \
    pkg-config \
    libuv1-dev \
    libssl-dev \
    libsqlite3-dev \
    zlib1g-dev \
    libevent-dev \
    libpugixml-dev \
    libminiupnpc-dev \
    libsimdjson-dev \
    libzstd-dev \
    libicu-dev \
    libncurses-dev \
    libreadline-dev \
    libbz2-dev \
    liblz4-dev \
    valgrind \
    gcovr \
    python3-pip \
    htop \
    ripgrep \
    fd-find \
    && apt-get autoremove -y && apt-get clean -y && rm -rf /var/lib/apt/lists/*

# Install LLVM 19 (latest stable) to closely match your Homebrew Clang 21
RUN wget https://apt.llvm.org/llvm.sh \
    && chmod +x llvm.sh \
    && ./llvm.sh 19 all \
    && rm llvm.sh \
    && update-alternatives --install /usr/bin/clang clang /usr/bin/clang-19 100 \
    && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-19 100 \
    && update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-19 100 \
    && update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-19 100

# Set GCC 13 as default (Ubuntu 24.04 standard)
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100

# Set working directory
WORKDIR /workspace

# Verify environment
RUN echo "Testing C++23 support..." && \
    echo '#include <iostream>\n#include <version>\nint main() { std::cout << "C++23 test successful on Ubuntu " << __cplusplus << "\\n"; return 0; }' > /tmp/test.cpp && \
    g++ -std=c++23 -o /tmp/test /tmp/test.cpp && /tmp/test && \
    clang++ -std=c++23 -o /tmp/test-clang /tmp/test.cpp && /tmp/test-clang && \
    rm /tmp/test /tmp/test-clang /tmp/test.cpp
