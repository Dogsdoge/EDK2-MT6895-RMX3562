#!/usr/bin/env bash
# build-pearl.sh — Build pearl (Xiaomi Redmi Note 12T Pro, MT6895) UEFI firmware + boot.img
# Usage: ./build-pearl.sh [RELEASE|DEBUG]
set -e

TARGET="${1:-RELEASE}"

# Check prerequisites
if ! command -v aarch64-linux-gnu-gcc &>/dev/null; then
    echo "Error: missing aarch64-linux-gnu-gcc (cross GCC)"
    exit 1
fi
if ! command -v clang &>/dev/null || ! command -v llvm-ar &>/dev/null; then
    echo "Error: missing clang/llvm-ar (LLVM 18+)"
    exit 1
fi

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR"

# Ensure submodules (at least brotli for BaseTools, openssl for crypto)
git submodule update --init --depth 1 Common/edk2 2>/dev/null || true
cd Common/edk2
git submodule update --init BaseTools/Source/C/BrotliCompress/brotli 2>/dev/null || true
git submodule update --init MdeModulePkg/Library/BrotliCustomDecompressLib/brotli 2>/dev/null || true
git submodule update --init CryptoPkg/Library/OpensslLib/openssl 2>/dev/null || true
cd "$ROOT_DIR"

# Build BaseTools
make -C Common/edk2/BaseTools -j$(nproc)

# Set up environment
export CLANG38_BIN=/usr/lib/llvm-18/bin/
export CLANG38_AARCH64_PREFIX=aarch64-linux-gnu-
export IASL_PREFIX=/usr/bin/
export PATH="$CLANG38_BIN:$ROOT_DIR/Common/edk2/BaseTools/BinWrappers/PosixLike:$ROOT_DIR/Build:$PATH"
export WORKSPACE="$ROOT_DIR"
export PACKAGES_PATH="$ROOT_DIR/Platform/Xiaomi:$ROOT_DIR/Common/edk2:$ROOT_DIR/Common/edk2-platforms:$ROOT_DIR/Silicon/MediaTek"

source Common/edk2/edksetup.sh >/dev/null 2>&1

# Build the firmware
build -p Platform/Xiaomi/pearlPkg/pearl.dsc -a AARCH64 -t CLANG38 -b "$TARGET" -j$(nproc)

# Build BootShim
make -C BootShim clean 2>/dev/null || true
make -C BootShim UEFI_BASE=0x50000000 UEFI_SIZE=0x700000

# Package shimmed + gzip + Android boot v4.
# pearl LK expects boot.img and vendor_boot.img as a pair. The boot image
# carries the UEFI payload as its kernel; the vendor_boot image supplies the
# stock DTB/vendor ramdisk and its non-zero ramdisk address.
FD="Build/pearl/${TARGET}_CLANG38/FV/PEARL_UEFI.fd"
BOOT_OUT="Build/boot-pearl.img"
VENDOR_BOOT_OUT="Build/vendor_boot-pearl.img"
# Stock pearl vendor_boot (slot A backup): supplies the stock DTB + vendor
# ramdisk + non-zero ramdisk address for MTK LK compatibility.
STOCK_VENDOR_BOOT="${STOCK_VENDOR_BOOT:-$ROOT_DIR/vendor_boot-pearl-stock.img}"
# Stock pearl boot os_version (Android 12.0.0, patch level 2023-11) so the LK
# anti-rollback check sees a matching image.
PEARL_OS_VERSION="0x18000199"

cat BootShim/BootShim.bin "$FD" > "${FD}.shimmed"
gzip -9 -c "${FD}.shimmed" > "${FD}.shimmed.gz"
python3 Tools/mkbootimg.py boot "${FD}.shimmed.gz" "$BOOT_OUT" --os-version "$PEARL_OS_VERSION"

if [ ! -f "$STOCK_VENDOR_BOOT" ]; then
    echo "Error: companion vendor_boot.img not found: $STOCK_VENDOR_BOOT"
    echo "Set STOCK_VENDOR_BOOT=/path/to/pearl/vendor_boot.img and retry."
    exit 1
fi
cp -f "$STOCK_VENDOR_BOOT" "$VENDOR_BOOT_OUT"
python3 Tools/mkbootimg.py inspect "$BOOT_OUT" "$VENDOR_BOOT_OUT"

# The vendor_boot image is copied, not rewritten: its stock ramdisk address,
# DTB and vendor ramdisk table remain intact for MTK LK compatibility.
echo ""
echo "=========== SUCCESS! ==========="
echo "Output boot.img:        $BOOT_OUT"
echo "Output vendor_boot.img: $VENDOR_BOOT_OUT"
echo "Firmware FD:            $FD"
echo "================================"
