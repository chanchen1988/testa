# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this directory is

Board support package (BSP) for the 安富莱 armfly STM32-V7 board (STM32H743XIH6, Cortex-M7) on
RT-Thread 5.3.0. Only the board layer lives here — the kernel (`src/`), components, libcpu and the
shared STM32 HAL drivers sit three levels up at `../../..` (this tree is **not** a git repository).

Repo-wide rules in the root `../../../AGENTS.md` apply on top of this file. The ones that matter
most in practice: keep diffs minimal and task-scoped, never hand-edit `rtconfig.h`, don't add new
subdirectories under major directories, don't touch `packages/` third-party code unless the task
requires it, and language rules live in `../../../.github/copilot-instructions.md`.

## Build

`scons`, `pkgs` and `menuconfig` are **not** on PATH in a plain shell. They ship with the RT-Thread
env tool: run `I:\env-windows\env.bat` first (its bundled Python 3.11 has scons). Keil uVision is on
PATH (`G:\soft\Keil_v5\UV4`).

```bash
pkgs --update                 # REQUIRED before the first build; pulls at_device, mymqtt,
                              # CMSIS-Core, stm32h7_cmsis_driver, stm32h7_hal_driver into packages/
```

`SConstruct` registers a pre-build check that aborts with this instruction if those package
directories are missing — if `pkgs --update` fetches nothing, try `pkgs --upgrade` then again.

**MDK5 (the path this tree is actually built with — `keil.log`, `rtthread.bin`, `build/keil/`):**

```bash
scons --target=mdk5           # regenerates project.uvprojx from .config + the SConscript tree
# then build project.uvprojx in uVision (AC6/armclang); output build/keil/Obj/rt-thread.axf → rtthread.bin
```

Re-run `scons --target=mdk5` after **any** menuconfig change, otherwise the MDK project drifts from
`.config` (it already has: the checked-in `project.uvprojx` defines `RT_USING_LIBC`/`RT_USING_ARMLIBC`,
which the current `rtconfig.h` does not).

**GCC:**

```bash
export RTT_CC=gcc RTT_EXEC_PATH=<dir containing arm-none-eabi-*>
scons -j8                     # scons -c to clean
```

`rtconfig.py` ships placeholder `EXEC_PATH` values (`C:\Users\XXYYZZ`) — always override via the
`RTT_EXEC_PATH` env var rather than editing them. GCC output lands in the BSP root (`rt-thread.elf`,
`rtthread.bin`, `rtthread.map`).

There is **no test suite**: `RT_USING_UTEST` is off and nothing runs host-side. Verification means
flash the board and exercise the `msh` console on `uart1` (115200 8N1, PA9/PA10).

## Configuration system

`board/Kconfig` + `../../../Kconfig` + package Kconfigs → `.config` (source of truth) → `rtconfig.h`.
Change config only through the toolchain:

```bash
scons --menuconfig            # interactive TUI
scons --pyconfig-silent       # non-interactive, from the existing .config
```

`.config` and the checked-in `rtconfig.h` currently disagree (e.g. `rtconfig.h` enables the at_device
NL668 and ESP32C61 classes that `.config` does not), so regenerate rather than trusting either one,
and don't patch `rtconfig.h` by hand.

## Build architecture

`SConstruct` sets `RTT_ROOT` to `../../..`, runs `PrepareBuilding`, then **explicitly adds**
`../libraries/HAL_Drivers/SConscript`. The root `SConscript` then walks every subdirectory containing
a `SConscript` (`applications/`, `board/`, `packages/`) and force-defines `STM32H743xx`.

The important consequence: `../../libraries/HAL_Drivers/` (i.e. `bsp/stm32/libraries/HAL_Drivers`)
is **shared by every STM32 BSP**. Editing it to fix this board affects all of them — prefer
board-local fixes where possible.

Driver selection is `GetDepend()`-driven in `libraries/HAL_Drivers/drivers/SConscript`, keyed off
`rtconfig.h` macros — e.g. `RT_USING_SERIAL` + `!RT_USING_SERIAL_V2` pulls in `drv_usart.c`;
`BSP_USING_ETH` + `RT_USING_LWIP` pulls in `drv_eth.c`. Registering a new peripheral type usually
means adding a Kconfig option in `board/Kconfig` and a matching `GetDepend` branch there.

Per-instance hardware description lives per STM32 series, not per BSP:

- `libraries/HAL_Drivers/drivers/config/h7/uart_config.h` — `UART1_CONFIG` (`name = "uart1"`,
  `Instance = USART1`, `USART1_IRQn`). UART1–UART5 descriptors already exist; only UART1 is enabled.
- `board/CubeMX_Config/Src/stm32h7xx_hal_msp.c` — CubeMX-generated pin muxing (`HAL_UART_MspInit`
  assigns PA9/PA10 to AF7 USART1). Regenerate from `board/CubeMX_Config/CubeMX_Config.ioc`, don't
  hand-edit unless you accept losing it on the next CubeMX export.
- `board/board.h` — flash/SRAM bounds plus `HEAP_BEGIN`/`HEAP_END` (toolchain-specific, mirrors what
  the linker script puts where). Pin naming is `GET_PIN(GPIOx, n)` from `drivers/drv_gpio.h`.

Adding your own code: drop a `.c` file under `applications/` (`Glob('*.c')` picks it up
automatically) and hook it with `INIT_APP_EXPORT`/`INIT_BOARD_EXPORT` — `RT_USING_COMPONENTS_INIT` is
on, so the auto-init table is the idiomatic entry point. `board/ports/` does not exist in this BSP.

## Board hardware facts

- Clock (`board/board.c:SystemClock_Config`): HSE 25 MHz (matches `HSE_VALUE` in
  `board/CubeMX_Config/Inc/stm32h7xx_hal_conf.h`), PLL1 M=5 N=160 P=2 → **400 MHz** SYSCLK, AHB /2 →
  200 MHz HCLK. The BSP `README.md` claims 480 MHz — the code is authoritative.
- Memory (`board/linker_scripts/link.lds`, `link.sct` agree): ROM 2048K @ `0x08000000`,
  RAM 512K @ `0x24000000` (AXI SRAM). `board.h` puts the RT-Thread heap between `__bss_end` and
  `0x24080000`. `project.uvprojx` also advertises DTCM `0x20000000` (128K) and XRAM regions, but the
  linker scripts only claim AXI SRAM — nothing is placed in DTCM today.
- Console: `RT_CONSOLE_DEVICE_NAME "uart1"`, `RT_TICK_PER_SECOND 1000`.

## Known gaps worth knowing before you dig

- `board/Kconfig` exposes **only UART1**. The H7 `uart_config.h` already describes UART2–UART5, so
  adding one is: a Kconfig block in `board/Kconfig` + pin init in the CubeMX MSP file +
  `scons --target=mdk5`.
- This matters immediately because every enabled at_device sample (ESP8266, ESP32, L610, NL668,
  ESP32C61) defaults `*_SAMPLE_CLIENT_NAME` to `"uart2"` and registers its device from
  `INIT_APP_EXPORT`. As configured, they bind to a device that does not exist — AT/network work
  needs UART2 enabled or the client name changed.
- `README.md`'s peripheral-support table (GPIO + UART only) is far behind what `.config` enables:
  SAL/lwIP 2.0.3, AT client + at_device, mymqtt, DFS/devfs and POSIX layer are all on. Treat
  `.config` as the accurate inventory.
- Network is a mix of two stacks: on-chip Ethernet is **not** enabled (`BSP_USING_ETH` absent), so
  connectivity currently flows through the AT devices over UART; SAL is built with both
  `SAL_USING_LWIP` and `SAL_USING_AT`.
- `keil.log`, `rtthread.bin`, `rt-thread.elf`, `rtthread.map`, `build/` and `packages/` are build
  products/local state (`.gitignore`d) — don't edit or commit them.
