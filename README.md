# rockchip-3126-buildroot-bsp
0.Prepare your host computer build environment
sudo apt-get install repo git-core gitk git-gui gcc-arm-linux-gnueabihf u-boot-tools
device-tree-compiler gcc-aarch64-linux-gnu mtools parted libudev-dev libusb-1.0-0-dev
python-linaro-image-tools linaro-image-tools autoconf autotools-dev libsigsegv2 m4 intltool
libdrm-dev curl sed make binutils build-essential gcc g++ bash patch gzip bzip2 perl tar
cpio python unzip rsync file bc wget libncurses5 libqt4-dev libglib2.0-dev libgtk2.0-dev
libglade2-dev cvs git mercurial rsync openssh-client subversion asciidoc w3m dblatex
graphviz python-matplotlib libc6:i386 libssl-dev texinfo liblz4-tool genext2fs expect
patchelf xutils-dev

1. config buildroot platform
        ./build.sh ./device/rockchip/rk3126c/BoardConfig.mk

2. build image
        ./build.sh

3. generated image for upgrade firmware RKDevTool
        rockdev/update.img

4.Install driver and RKDevTool in your Windows
        Drivers: ./tools/windows/DriverAssitant_v4.91.zip
        Update Too: ./tools/windows/RKDevTool_Release_v2.74.zip
        Please unzip those files and install in your Windows.

5.Using RKDevTool, to write rockdev/update.img into your board.
