# CLAUDE.md

BSP for 安富莱 STM32-V7 (STM32H743XIH6, Cortex-M7) on RT-Thread 5.3.0.
仅 board 层；内核/HAL 在 `../../..`。根规则见 `../../../AGENTS.md`，
语言规则见 `../../../.github/copilot-instructions.md`。

## 核心规则
- 最小 diff，任务范围。
- 不手改 `rtconfig.h`；配置只通过 menuconfig/pyconfig，以 `.config` 为准。
- 不动 `packages/` 第三方代码，除非任务必须。
- `build/`、`packages/`、`*.bin/*.elf/*.map/keil.log` 不提交。

## 构建
- 先运行 RT-Thread env 脚本（提供 scons/pkgs/menuconfig）。
- 首次构建：`pkgs --update`；缺包时试 `pkgs --upgrade`。
- MDK5：`scons --target=mdk5` 生成 `project.uvprojx`，再用 Keil AC6 构建。
- GCC：`export RTT_CC=gcc RTT_EXEC_PATH=...`，然后 `scons -j8`。
- 无测试；烧录后通过 `uart1` 115200 8N1 的 `msh` 验证。

## 关键路径
- 时钟/内存：`board/board.c`、`board/board.h`、`board/linker_scripts/`
- UART：`board/Kconfig`、`libraries/HAL_Drivers/drivers/config/h7/uart_config.h`、`board/CubeMX_Config/Src/stm32h7xx_hal_msp.c`
- 应用入口：`applications/*.c` + `INIT_APP_EXPORT`
- 共享 HAL：`../../libraries/HAL_Drivers/` 被所有 STM32 BSP 共用，改前慎重。

## 常见任务
- 加 UART2：`board/Kconfig` 加选项 + CubeMX MSP 引脚 + `scons --target=mdk5`。
- at_device 样例默认 `uart2`；当前仅 UART1，需启用 UART2 或改 `*_SAMPLE_CLIENT_NAME`。
- 配置以 `.config` 为准，README 可能过时。