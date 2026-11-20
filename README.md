# Phase Drift

**实验性相位延迟处理器 / Experimental phase-delay processor — VST3**

32 个参数，4 段独立相位旋转，可同步/并联两种拓扑，带反馈、波折叠与立体声 M/S 旋转。
所有旋钮和界面都用矢量图形实时绘制（无位图资源），暗色硬件风格。

---

## 声音设计 / What it does

信号被拆成 4 个频段（LR2 分频，分频点由相邻频段中心频率的几何平均值自动决定）：

```
x ──► LP/HP ──► band0 ──► 4×allpass(a0) ─┐
        │  └──► band1 ──► 4×allpass(a1) ─┤
        │       └──► band2 ──► 4×allpass(a2) ─┼──► Σ  (parallel)
        │            └──► band3 ──► 4×allpass(a3) ─┘
        └──────────────────────────────────────► 16×allpass (series)
```

* 每个频段是一条 **4 级一阶全通链**，系数 `a = tan/(tan+1)` 由该段中心频率决定，
  再乘以 PHASE 旋钮（−1 … +1，负值 = 反相旋转），并叠加该段自己的 LFO。
* **TOPOLOGY** 在串联（整条信号依次穿过 4 条相位链，相位随频率累积）与
  并联（各频段旋转后重新相加，产生随频率移动的相消凹槽）之间连续变形。
* **FEEDBACK + CHAOS + FB TONE**：带阻尼的反馈环，CHAOS 每块随机抖动各段速率，
  让系统缓慢漂移，不会稳稳停在同一状态。
* **DRIVE** 是正弦波折叠（wavefolder），把相位相消产生的尖峰折成新的谐波。
* **RING** 与 band-1 的 LFO 做双极相乘，RING=100% 即纯环形调制。
* **M / S ROTATE** 把立体声在 Mid/Side 平面上旋转，破坏原有的左右相位关系。
* **FREEZE** 切断输入并把反馈推到自激，相位链继续在内部旋转。

## 参数 / Parameters

| 组 | 参数 |
| --- | --- |
| SPECTRAL BANDS | 每段: FREQ / PHASE / DEPTH / RATE（共 16 个） |
| CORE | IN, DRIVE, TILT, MIX, OUT, FEEDBK, RING, CHAOS |
| MOTION | SPEED, SHAPE, LFO PH, SPREAD, M/S ROT, TOPOLOGY, FB TONE |
| EXTRA | FREEZE |

出厂预设：`INIT` / `GLASS ROTOR` / `GHOST COMB` / `DOPPLER WELL` / `TAPE MELT`。

* 双击旋钮恢复默认值。
* 旋钮采用纵向拖动，按 Alt 拖动可获得更细的分辨率（JUCE 默认行为）。

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

安装：把 `Phase Drift.vst3` 拖进 `C:\Program Files\Common Files\VST3`
（或用户目录 `%LOCALAPPDATA%\Programs\Common\VST3`）。

## 已知特性 / Notes

* 相位效果在单声道素材上也能工作，但 SPREAD / M/S ROT 只在立体声下有意义。
* 反馈 > 70% 且 CHAOS 高时是故意不稳定的 —— 这是设计的一部分。
* 内部有 DC blocker 与 ±16 的硬限幅，防止反馈环跑飞。

## 许可 / License

AGPL-3.0（因为使用 JUCE 8 的 AGPL 授权）。见 `LICENSE`。
