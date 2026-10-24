#!/bin/sh

echo "I"
sudo apt install --no-install-recommends -y git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget python3-dev python3-venv python3-tk \
  xz-utils file make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1

python3 -m venv ./.venv
source .venv/bin/activate

pip install west
west init -m https://github.com/zephyrproject-rtos/zephyr .
west update
west packages pip --install
west zephyr-export