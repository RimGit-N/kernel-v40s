#!/usr/bin/env bash
# Kernel Build Script for Unisoc T618 / UMS512 (ZTE Blade V40s)
# Target: 5.4.161-NevaCPS-12-9-gb17cbc51204f-tempest

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KERNEL_DIR="${ROOT_DIR}/bsp/kernel/kernel5.4"
OUT_DIR="${ROOT_DIR}/out"
DEFCONFIG="${DEFCONFIG:-sprd_sharkl5Pro_defconfig}"
BUILD_TARGET="${BUILD_TARGET:-Image Image.gz}"

export ARCH=arm64
export KBUILD_BUILD_USER="NevaCPS"
export KBUILD_BUILD_HOST="LocalBuilder"

echo "=== Android Kernel Build ==="
echo "Kernel Root : ${KERNEL_DIR}"
echo "Output Dir  : ${OUT_DIR}"
echo "Architecture: ${ARCH}"
echo "Defconfig   : ${DEFCONFIG}"
echo "Target      : ${BUILD_TARGET}"

mkdir -p "${OUT_DIR}"

echo "--> Generating .config..."
make -C "${KERNEL_DIR}" O="${OUT_DIR}" \
  ARCH=arm64 \
  LLVM=1 \
  LLVM_IAS=1 \
  CROSS_COMPILE=aarch64-linux-gnu- \
  CROSS_COMPILE_COMPAT=arm-linux-gnueabi- \
  "${DEFCONFIG}"

echo "--> Compiling kernel..."
make -C "${KERNEL_DIR}" O="${OUT_DIR}" \
  -j"$(nproc)" \
  ARCH=arm64 \
  LLVM=1 \
  LLVM_IAS=1 \
  CROSS_COMPILE=aarch64-linux-gnu- \
  CROSS_COMPILE_COMPAT=arm-linux-gnueabi- \
  ${BUILD_TARGET} 2>&1 | tee "${ROOT_DIR}/build.log"

RELEASE="$(cat "${OUT_DIR}/include/config/kernel.release" 2>/dev/null || echo 'Unknown')"
echo "=== Build Complete ==="
echo "Kernel Release: ${RELEASE}"
echo "Image location: ${OUT_DIR}/arch/arm64/boot/"
