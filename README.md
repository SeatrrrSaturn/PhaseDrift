# Phase Drift

**实验性相位延迟 / 回声处理器 · Experimental phase + echo processor — VST3**

44 个参数：4 段独立相位旋转 + **4 段各自不同时间的回声**，**左右声道完全独立**，
可同步/并联两种拓扑，带反馈、波折叠与立体声 M/S 旋转。
所有旋钮和界面都用矢量图形实时绘制（无位图资源），扁平化暗色界面。

---

## 声音设计 / What it does

信号被拆成 4 个频段（LR2 分频，分频点由相邻频段中心频率的几何平均值自动决定），
每个频段各有一条**自己的延迟线**：

```
x ──► LP/HP ──► band0 ──► 4×allpass(a0) ──► delay(t0·scale0) ─┐
        │  └──► band1 ──► 4×allpass(a1) ──► delay(t1·scale1) ─┤
        │       └──► band2 ──► 4×allpass(a2) ──► delay(…) ─────┼──► Σ (parallel)
        │            └──► band3 ──► 4×allpass(a3) ──► delay(…) ─┘
        └──────────────────────────────────────► 16×allpass (series)
```

* 每个频段是一条 **4 级一阶全通链**，系数 `a = tan/(tan+1)` 由该段中心频率决定，
  再乘以 PHASE 旋钮（−1 … +1，负值 = 反相旋转），并叠加该段自己的 LFO。
* **BAND ECHO（4 个）** 给每个频段一个延迟倍率（0.25× … 3.0×）：
  不同频段回声时间不同 → 低频拖得长、高频跑得早，产生会"散架"的梳状回声。
* **ECHO / LEFT 与 ECHO / RIGHT 完全独立**：两条声道各有自己的 TIME（2 ms … 1 s）、
  FEEDBK、DAMP（阻尼低通）与 LEVEL，可以做出左右不对称的回声与相位差。
* **TOPOLOGY** 在串联（整条信号依次穿过 4 条相位链，相位随频率累积）与
  并联（各频段旋转后重新相加，产生随频率移动的相消凹槽）之间连续变形。
* **FEEDBACK + CHAOS + FB TONE**：带阻尼的反馈环，CHAOS 每块随机抖动各段速率，
  让系统缓慢漂移，不会稳稳停在同一状态。
* **DRIVE** 是正弦波折叠（wavefolder），把相位相消产生的尖峰折成新的谐波。
* **RING** 与 band-1 的 LFO 做双极相乘，RING=100% 即纯环形调制。
* **M / S ROTATE** 把立体声在 Mid/Side 平面上旋转，破坏原有的左右相位关系。
* **FREEZE** 切断输入，把回声/反馈留在环里自激，相位链继续在内部旋转。

## 参数 / Parameters — 44

| 组 | 参数 |
| --- | --- |
| SPECTRAL BANDS | 每段 FREQ / PHASE / DEPTH / RATE （16 个） |
| BAND ECHO | 每段 ECHO / E 延迟倍率（4 个） |
| CORE | IN, DRIVE, TILT, MIX, OUT, FEEDBK, RING, CHAOS （8 个） |
| MOTION | SPEED, SHAPE, LFO PH, SPREAD, M/S ROT, TOPOLOGY, FB TONE （7 个） |
| ECHO L / R | TIME, FEEDBK, DAMP, LEVEL ×2 声道 （8 个） |
| EXTRA | FREEZE （1 个开关） |

出厂预设：`INIT` / `GLASS ROTOR` / `GHOST COMB` / `DOPPLER WELL` / `TAPE MELT`。

* 双击旋钮恢复默认值。
* 旋钮采用纵向拖动，按住 Alt 拖动可获得更细的分辨率（JUCE 默认行为）。

## 安装 / Installing

### 方式一：安装包（推荐）

下载 `PhaseDrift-Setup.exe`，双击 → 管理员权限运行 → 点 `INSTALL`。
它会写入 `C:\Program Files\Common Files\VST3\Phase Drift.vst3`。
卸载用同一个 exe 里的 `UNINSTALL`（或 `PhaseDrift-Setup.exe /uninstall`）。
静默安装：`PhaseDrift-Setup.exe /install`（退出码 0 = 成功）。

### 方式二：手动

把 `Phase Drift.vst3` 拖进 `C:\Program Files\Common Files\VST3`
（或用户目录 `%LOCALAPPDATA%\Programs\Common\VST3`）。

装好后在 DAW 里重新扫描插件即可，名称显示为 `Seater Saturn / Phase Drift`。

## 构建 / Building

需要 CMake ≥ 3.22 和 C++17 编译器。JUCE 8 会在需要时通过 `FetchContent` 自动下载，
也可以把 JUCE 检出放到 `JUCE/` 或 `../_deps/juce/`。

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

产物：`build/PhaseDrift_artefacts/Release/VST3/Phase Drift.vst3`

Windows + MSVC（推荐）：

```bash
# 在 "x64 Native Tools Command Prompt for VS" 里执行
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

注意：JUCE 8 **不支持 MinGW**（`juce_TargetPlatform.h` 会直接 `#error`），
Windows 上请使用 MSVC 工具链。

### 生成安装包

```bash
# Windows，需要 .NET Framework 的 csc.exe
PhaseDrift\installer\build_setup.cmd
```

它把编译好的 `Phase Drift.vst3` 作为资源嵌进 `dist/PhaseDrift-Setup.exe`
（单文件、约 3.5 MB、manifest 已声明 requireAdministrator）。

## 已知特性 / Notes

* 相位效果在单声道素材上也能工作，但 SPREAD / M/S ROT / 左右独立 ECHO 只在立体声下有意义。
* 反馈 > 70% 且 CHAOS 高时是故意不稳定的 —— 这是设计的一部分。
* 最大回声时间 1 s（每段再乘 0.25×…3.0×，最长约 3 s），尾部长度按 12 s 申报。
* 内部有 DC blocker 与 ±16 的硬限幅，防止反馈环跑飞。

## 变更 / Changelog

* **0.9.0** (2026-11-21)
  * 新：**真实延迟/回声引擎** —— 4 频段 × 2 声道共 8 条环形延迟线，分数延迟插值，
    每频段可设不同延迟倍率（`BAND ECHO`）。
  * 新：**左右声道独立** 的 TIME / FEEDBK / DAMP / LEVEL（`ECHO / LEFT`、`ECHO / RIGHT`）。
  * 参数从 32 个扩到 44 个；尾部时间 0.5 s → 12 s。
  * 界面：**去掉 logo**，旋钮改为**扁平化**（纯色盘体 + 细描边 + 双色圆弧 + 指针线），
    配色降饱和、字号整体加大，布局重排为 1360×860。
  * 修复：编辑器一打开就崩溃（宿主报 `0xC0000005` 指向本插件 DLL）。
    原因是源码里的 `"\u00b0"` 在中文（CP936）Windows 上被 MSVC 编成 GBK 字节
    `A1 E3`，交给 `String::fromUTF8` 后解出坏码点，JUCE 在
    `Font::getStringWidthFloat` 里按 UTF-8 逐字节推进时冲出缓冲区末尾 →
    读到未映射页。改为 `String::charToString (0x00b0)`，不再依赖源码编码。
    具体触发点是 LFO PH 旋钮下方的 `0°` 数值文本。
  * 公司 / VST3 vendor 名改为 **Seater Saturn**（manufacturer code 与 plugin code
    未变，插件 CID 不变，老工程仍然识别同一个插件）。
  * 新增 `installer/`（C# 单文件 `setup.exe` 生成脚本）。

## 许可 / License

AGPL-3.0（因为使用 JUCE 8 的 AGPL 授权）。见 `LICENSE`。
