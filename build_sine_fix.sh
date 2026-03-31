#!/bin/bash
set -e

REPO="/Users/holmeshobbot/.openclaw/workspace-dev/AM32-220"
GCC_PREFIX="${REPO}/tools/macos/xpack-arm-none-eabi-gcc-10.3.1-2.3/bin/arm-none-eabi"
OBJDIR="${REPO}/obj_clang_sine_fix"
TARGET_DEFINE="CM_MINI_DUAL_E230"
OUTPUT_HEX="${REPO}/AM32_CM_MINI_DUAL_E230_3.0_SINE_FIX.hex"

cd "${REPO}"
rm -rf "${OBJDIR}"
mkdir -p "${OBJDIR}"

SRCS=(
    Src/dshot.c Src/firmwareversion.c Src/functions.c Src/kiss_telemetry.c
    Src/main.c Src/signal.c Src/sounds.c
    Mcu/e230/Src/ADC.c Mcu/e230/Src/comparator.c Mcu/e230/Src/eeprom.c
    Mcu/e230/Src/gd32e23x_it.c Mcu/e230/Src/IO.c Mcu/e230/Src/peripherals.c
    Mcu/e230/Src/phaseouts.c Mcu/e230/Src/serial_telemetry.c
    Mcu/e230/Src/system_gd32e23x.c Mcu/e230/Src/systick.c
)

for f in Mcu/e230/Drivers/GD32E23x_standard_peripheral/Source/*.c; do
    SRCS+=("$f")
done

OBJS=()
for src in "${SRCS[@]}"; do
    obj="${OBJDIR}/$(basename "${src}" .c).o"
    OBJS+=("$obj")
    clang \
        --target=thumbv8m.base-none-eabi \
        -mcpu=cortex-m23 -mthumb -mfloat-abi=soft \
        -DGD32E230 -DSTM32G031xx -DUSE_STDPERIPH_DRIVER \
        -D${TARGET_DEFINE} -DAM32_MCU=\"E230\" \
        -I${REPO}/Mcu/e230/Inc \
        -I${REPO}/Mcu/e230/Drivers/CMSIS/Include \
        -I${REPO}/Mcu/e230/Drivers/CMSIS/Core/Include \
        -I${REPO}/Mcu/e230/Drivers/GD32E23x_standard_peripheral/Include \
        -I${REPO}/Inc \
        -isystem ${REPO}/tools/macos/xpack-arm-none-eabi-gcc-10.3.1-2.3/arm-none-eabi/include \
        -O3 -ffunction-sections -fdata-sections -fshort-enums -fshort-wchar \
        -Wall -Wno-unused-parameter \
        -c "${src}" -o "${obj}"
done

# Startup (Keil-match)
${GCC_PREFIX}-gcc -mcpu=cortex-m23 -mthumb -mfloat-abi=soft \
    -c "${REPO}/Mcu/e230/Startup/startup_gd32e23x_keil_match.s" \
    -o "${OBJDIR}/startup.o"
OBJS+=("${OBJDIR}/startup.o")

# Link with Keil-match linker script
${GCC_PREFIX}-gcc -mcpu=cortex-m23 -mthumb -mfloat-abi=soft \
    --specs=nosys.specs --specs=nano.specs -lnosys \
    -Wl,--gc-sections -Wl,--print-memory-usage \
    -T"${REPO}/Mcu/e230/GD32E230K8_FLASH_KEIL_MATCH.ld" \
    "${OBJS[@]}" -o "${OBJDIR}/firmware.elf"

${GCC_PREFIX}-objcopy -O ihex "${OBJDIR}/firmware.elf" "${OUTPUT_HEX}"
${GCC_PREFIX}-size "${OBJDIR}/firmware.elf"
echo "=== BUILD SUCCESS: ${OUTPUT_HEX} ==="
