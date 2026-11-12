# Debug 记录

> 项目要求：从第一天开始记录真实 Bug 与解决过程。
> 用途：简历/面试时能讲出"遇到过什么、怎么定位、为什么这样解决"。

---

## 记录 01：VS Code 编译通过，但烧录报 error 138（ST-Link 连不上）

**日期**：2026-10-13
**环境**：Windows / NUCLEO-F446RE / VS Code + STM32Cube VS Code 扩展 / CMake + Ninja + GNU Tools for STM32 14.3.1
**状态**：✅ **已彻底解决**（调试接口驱动缺失 + ST-Link 固件过旧，两处修复后 F5 调试完全正常）

### 现象

1. VS Code 编译完全正常：
   ```
   [23/23] Linking C executable F446RE_test_1.elf
   RAM:   1592 B / 128 KB (1.21%)
   FLASH: 7340 B / 512 KB (1.40%)
   生成已完成，退出代码为 0
   ```
2. 按 `F5` 调试 → 报错：
   ```
   Failed to decode cstring 'could not connect (error 138)': ...
   ```
3. 乱码部分是 GBK 被错读的 Windows 消息：
   `ÏµÍ³ÊÔÍ¼½«Çý¶¯Æ÷ºÏ²¢µ½ºÏ²¢Çý¶¯Æ÷ÉÏµÄÄ¿Â¼`
   → **"系统试图将驱动器合并到合并驱动器上的目录。"**

### 排查过程

| # | 做法 | 结果 |
|---|---|---|
| 1 | 查注册表 `HKLM:\SYSTEM\CurrentControlSet\Enum\USB` 有无 ST 厂商设备 | 有 `VID_0483&PID_374B`（ST-Link/V2-1），VCP 占 COM9 |
| 2 | 逐个查三个 USB 接口的驱动绑定 | **MI_00（ST-Link Debug）的 Service 是空的 → 没有任何驱动** |
| 3 | 用扩展命令「安装 ST-Link USB 驱动程序」 | 失败：`Not able to get STLink installer` |
| 4 | 改用 **Keil 自带驱动**：管理员运行 `C:\Keil_v5\ARM\STLink\USBDriver\stlink_winusb_install.bat` | ✅ 成功，MI_00 绑定 `WinUSB`，设备名 `STMicroelectronics STLink dongle` |
| 5 | 重新 `F5` | ❌ 仍报 error 138 |
| 6 | 跑 `STLinkUpgrade.jar` 查固件 | 初始读不到版本；**完整断电（拔 USB 等 30 秒）后**读到当前 `V2J40M35`，可升 `V2J46M35` |
| 7 | 排除探针被占用（关掉 VS Code / 无 stlinkserver 进程） | ❌ 仍失败，JNI 命令错误 `0x00079` |
| 8 | **改用 ST-Link 虚拟磁盘拖拽烧录** | ✅ **成功，LED 按代码闪烁** |

### 根因分析

- **主因（已修复）**：ST-Link 调试接口（MI_00）的 USB 驱动**从未安装**。
  因为本机装的是**独立版 STM32CubeMX + VS Code 扩展**，没有装会附带安装驱动的 STM32CubeIDE / CubeProgrammer 完整版。
  > 注意：验收时"设备管理器能看到 STLink Virtual COM Port"是**串口接口**（MI_02），
  > 而**调试接口（MI_00）是另一个独立接口，驱动也是独立的**——串口能用不代表调试能用。

- **次因（已修复）**：ST-Link 固件过旧（`V2J40M35`），主机侧调试握手仍失败（JNI 命令错误 `0x00079`）。
  升级到 `V2J48M35` 后一切正常。

> **结论**：error 138 是**两个问题叠加**——调试接口**驱动缺失** **+** **固件过旧**。
> 只修驱动不够（第 5 步仍失败），两处都修好后才通。**排查这类问题不要过早下单一结论。**

### 解决方案（替代路径）

用 ST-Link 的**虚拟磁盘（MSD）拖拽烧录**，完全绕开调试接口：

```powershell
# 1. elf 转 bin
& "...\gnu-tools-for-stm32\14.3.1+st.2\bin\arm-none-eabi-objcopy.exe" `
    -O binary "build\Debug\F446RE_test_1.elf" "build\Debug\F446RE_test_1.bin"

# 2. 把 .bin 拖到 ST-Link 的虚拟磁盘（此机为 E:），磁盘自动重挂载 = 写 flash + 复位目标
```

### 最终解决（正式方案）

两处修复叠加，**缺一不可**：

1. **补装调试接口驱动**（用 Keil 自带的）→ MI_00 绑定 `WinUSB`
2. **升级 ST-Link 固件** `V2J40M35 → V2J48M35`

   - **关键**：升级前必须**完整断电**（拔 USB 等 30 秒再插），否则升级工具读不到固件版本、`Upgrade` 按钮是灰的
   - 升级时**不要勾 `Change Type`**，保持 `Debug+Mass storage+VCP`——这样虚拟磁盘拖拽烧录的退路还在

**升级后 F5 调试输出（全部正常）：**

```
ST-LINK SN  : 066EFF494855807167153215
ST-LINK FW  : V2J48M35
Voltage     : 3.23V
Connect mode: Under Reset
Reset mode  : Hardware reset
Device ID   : 0x421
Device name : STM32F446xx
Device CPU  : Cortex-M4
NVM size    : 512 KBytes
Erasing internal memory sector 0
File download complete  (7340 bytes @ 0x08000000)
Temporary breakpoint 1, main () at main.c:75
```

### 遗留（不影响使用）

- 虚拟磁盘卷标仍是 `UNDEFINED`，GDB server 里 `Board: --`：ST-Link 未从目标板读到板名，
  但**烧录和调试都正常**，可忽略。
- [x] 验证 COM9 串口 printf 调试可用（Phase 1 主要靠它）→ ✅ **已通过**（见记录 02）

### 经验教训

1. **ST-Link 的三个 USB 接口（调试 / 虚拟磁盘 / 串口）驱动是相互独立的**，
   串口能枚举不代表调试能用。
2. 排查这类问题，**查注册表里每个接口的 `Service` 绑定**比翻设备管理器更准、更快。
3. **NUCLEO 板载 ST-Link 的"拖拽烧录"是极好的兜底方案**，不依赖调试接口，出问题时能先保住开发进度。
4. 报错里的**乱码别跳过**——解码 GBK 乱码后拿到的原始 Windows 消息，往往是关键线索。
5. Windows 上 `npx` 受 PowerShell 执行策略限制时，改用 `npx.cmd`。
6. **ST-Link 类工具"读不到设备"时，先做一次完整断电**（拔 USB 等 30 秒）——
   本次升级工具从"读不到版本"变成"能读到版本"，靠的就是这一步。
7. 复杂故障往往是**多因叠加**。本次修好驱动后仍失败，不能就此认定"驱动不是根因"，
   而要沿现象继续往下查（固件版本）。

---

## 记录 02：串口 printf 完全没有输出（但编译、烧录、点灯都正常）

**日期**：2026-10-13
**环境**：NUCLEO-F446RE / USART2 (PA2/PA3) → ST-Link 虚拟串口 COM9 / 115200 8N1
**状态**：✅ 已解决

### 现象

加入 printf 重定向后：编译 0 error、烧录成功、LED 正常闪烁，
但**串口监视器一个字都没有**，且没有任何报错。

### 根因（两个独立问题叠加）

**根因 1：重定向函数名拼错 → 静默失效**

```c
int __io_pitchar(int ch)   // ❌ 拼错了
int __io_putchar(int ch)   // ✅ 正确
```

CubeMX 生成的 `syscalls.c` 中：

```c
extern int __io_putchar(int ch) __attribute__((weak));
__attribute__((weak)) int _write(int file, char *ptr, int len)
{
  for (DataIdx = 0; DataIdx < len; DataIdx++) __io_putchar(*ptr++);
}
```

函数名拼错 → **没有覆盖那个 weak 符号** → 链接到空实现 → printf 无输出，
而且**编译、链接全程不报错**。

同处还有一个错误：`HAL_UART_Transmit(&huart2, (uint8_t)&ch, ...)`
应为 `(uint8_t *)&ch`（把指针转成了整数，触发 `-Wint-conversion`）。

**根因 2：调试会话把程序停住了**

- `launch.json` 里 `"runEntry": "main"` 使程序**每次 `F5` 启动都停在 main 第一行**，
  必须**再按一次 `F5`（继续）**才真正开始运行
- 另外在 `__io_putchar` 那行误设了**断点** → 每次 printf 都停在断点上，一个字符都发不出去

### 关键排查手段：验证代码是否真的编进去了

```powershell
# 在编译产物里搜字符串，区分"代码没编进去"和"程序没跑起来"
$bytes = [System.IO.File]::ReadAllBytes("build\Debug\F446RE_test_1.bin")
$text = -join ($bytes | ForEach-Object { if($_ -ge 32 -and $_ -lt 127){[char]$_}else{'.'} })
# → 找到 "Hello from NUCLLEO-F446RE!  tick=%lu"  ✅ 证明代码确实编译进去了
```

（顺带发现：bin 从 7340 字节涨到 14000 字节，多出来的是 printf 库代码，正常。）

### 解决

1. 函数名改为 `__io_putchar`
2. 指针转换改为 `(uint8_t *)&ch`
3. **删除 `__io_putchar` 上的断点**
4. `F5` 启动后**再按一次 `F5` 继续运行**

### 经验教训

1. **`__attribute__((weak))` 的钩子函数必须逐字母对上**——拼错会静默失效，
   编译链接都不报错，是最难查的一类 bug。
2. **调试会话中程序被 halt 时，所有外设都不工作**（LED 不闪、串口不输出）。
   排查"外设没反应"，第一步永远是确认**程序是否真的在运行**
   （看调试工具栏是 ⏸ 还是 ▶）。
3. 用"**在编译产物里搜字符串**"验证代码是否被编进去，比反复改代码高效得多。
4. 排查顺序建议：**配置 → 代码 → 编译产物 → 运行状态 → 物理链路**，
   一层层缩小范围，不要跳着猜。

[11月11日] 继电器与光敏ADC测试通过。发现5V继电器与3.3V MCU电平兼容问题，通过改用H触发、解决共地问题及更换PA10接线，目前可稳定控制并正常采集光敏数据。LDR光敏ADC测试通过。不遮光值约1067，遮光值约2148，变化明显。继电器H触发稳定。PIR人体感应测试通过。发现接错引脚，纠正后能准确输出状态变化。为了解决 HC-SR501 延时过长和串口刷屏问题，调整了 Tx 旋钮，并在代码中增加了状态跳变检测（边沿检测），现在只在有人/无人切换时打印。