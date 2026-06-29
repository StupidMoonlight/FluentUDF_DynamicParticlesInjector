# FluentUDF_DynamicParticlesInjector
Initial particles can be introduced within the satisfied mesh for transient calculations to simulate flow phenomena triggered by a small number of free nuclei, such as condensation of a small amount of water vapor, solute precipitation and aggregation in saturated solution transport, and cavitation with a small amount.
# DynamicParticlesInjector

## 中文简介
**DynamicParticlesInjector** 是一个 Ansys Fluent UDF 代码，用于在**任意满足瞬态物理条件**的网格单元质心处随机注入预设的 DPM 粒子。  
该算法在每一个指定时间步收集所有符合条件的单元坐标，从中随机采样，并将粒子注入到采样位置。  
适用于空化成核、晶体析出、气泡生成等动态成核场景。

**核心特点**：
- 基于网格质心随机采样，无需预定义注入区域。
- 自动跨进程并行通信，适用于大规模并行计算。
- 用户只需修改条件判断（如压力、温度、相分数）即可适配不同物理模型。

## 快速开始 (Quick Start)

1. **编译 UDF**：将 `DynamicParticlesInjector.c` 放入 Fluent 工作目录，使用 `Compile UDF` 加载。
2. **设置注射源**：创建 `group` 或 `cone` 注射源，**Number of particles** 设为 ≥ `P_NUM`。
3. **调节参数**：
   - `P_NUM`：每次注入粒子数
   - `SPAN`：注入间隔（时间步）
   - 条件判断：修改 `if (center[1] > 0)` 为你的物理条件。
4. **运行计算**：瞬态模拟开始后，粒子将自动按设定注入。
   
   **详细说明见 `INSTRUCTION.md`**。

## 环境要求
- Ansys Fluent **2021 R1**（作者测试的版本，理论上各个版本都可使用，但代码可能需要做细微修改）



## 许可

本项目采用 MIT 许可证。你可以自由使用、修改、商用，详情请见 LICENSE 文件。

## 反馈与引用

### 商业用途
如果你在商业项目中使用本代码，欢迎通过 Issues 或 Discussions 分享你的使用场景。  
这能帮助我们了解实际应用并优先改进。

### 学术用途
如果你在学术研究或论文中使用本项目，请引用它。  
引用格式将在 Zenodo 记录创建后补充。  
感谢你对开源研究的支持！

---

## English Description
**DynamicParticlesInjector** is an Ansys Fluent UDF code that injects pre‑initialized DPM particles at **any cell centroid satisfying transient physical conditions**.  
At each injection time step, the algorithm collects all eligible cell centroids across the entire domain, randomly samples a specified number of positions, and places particles at those locations.  
Ideal for modeling dynamic nucleation processes such as cavitation, crystal precipitation, or bubble generation.

**Key Features**:
- Random sampling from centroid pool – no predefined injection zones required.
- Fully parallelized with automatic inter‑process communication.
- Easily adapt to your physical criterion by modifying a single condition (pressure, temperature, phase fraction, etc.).

## Quick Start

1. **Compile UDF**: Place `DynamicParticlesInjector.c` into the Fluent working directory and load it using `Compile UDF`.
2. **Set Injection Source**: Create a `group` or `cone` injection source, set **Number of particles** to ≥ `P_NUM`.
3. **Adjust Parameters**:
   - `P_NUM`: Number of particles injected each time
   - `SPAN`: Injection interval (time step)
   - Condition judgment: Modify `if (center[1] > 0)` to your physical conditions.
4. **Run Simulation**: After starting the transient simulation, particles will be automatically injected according to the set parameters.
   
   **Details are in `INSTRUCTION.md`**.

## Environmental Requirements
- Ansys Fluent **2021 R1** (the version tested by the author, theoretically applicable to all versions, but code may require minor modifications)

## License
This project is licensed under the MIT License.  
You are free to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the software.  
See the `LICENSE` file for full details.

## Feedback & Citation
### Commercial Use
If you are using this project in a commercial product, we welcome you to share your story via Issues or Discussions.  
It helps us understand real-world usage and prioritize improvements.
### Academic Use
If you use this project in academic research or a paper, please cite it.  
A citation format will be added soon (once the Zenodo record is set up).  
Thank you for supporting open-source research!

---


*为 CFD 开发者而生，让离散相模拟更灵活。*  
*Built for CFD developers – making discrete phase modeling more flexible.*
