FROM ubuntu:22.04

USER root

# 安装构建所需依赖
RUN apt update && apt install -y \
    bsdmainutils \
    git \
    build-essential \
    ninja-build \
    pkg-config \
    libglib2.0-dev \
    libpixman-1-dev \
    meson \
    python3-venv \
    python3-pip \
    ca-certificates \
    binutils-riscv64-linux-gnu \
    gcc-riscv64-linux-gnu \
    gdb-multiarch \
    && apt clean && rm -rf /var/lib/apt/lists/*

RUN wget https://download.qemu.org/qemu-8.2.10.tar.xz -O /tmp/qemu.tar.xz
# 解压并构建 QEMU
RUN mkdir -p /tmp/qemu && \
    tar -xf /tmp/qemu.tar.xz -C /tmp/qemu --strip-components=1 && \
    mkdir -p /tmp/qemu/build && \
    cd /tmp/qemu/build && \
    ../configure --target-list=riscv64-softmmu && \
    make -j$(nproc) && \
    make install && \
    cd / && rm -rf /tmp/qemu /tmp/qemu.tar.xz

# 切换为清华源
RUN sed -i 's|http://archive.ubuntu.com/ubuntu/|http://mirrors.tuna.tsinghua.edu.cn/ubuntu/|g' /etc/apt/sources.list && \
    sed -i 's|http://security.ubuntu.com/ubuntu/|http://mirrors.tuna.tsinghua.edu.cn/ubuntu/|g' /etc/apt/sources.list


RUN pip install pyelftools && \
    pip config set global.index-url https://mirrors.aliyun.com/pypi/simple && \
    pip config set install.trusted-host mirrors.aliyun.com



# 设置默认工作目录
WORKDIR /root

CMD ["/bin/bash"]
