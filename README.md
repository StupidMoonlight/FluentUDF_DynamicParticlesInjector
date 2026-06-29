# FluentUDF_DynamicParticlesInjector
Initial particles can be introduced within the satisfied mesh for transient calculations to simulate flow phenomena triggered by a small number of free nuclei, such as condensation of a small amount of water vapor, solute precipitation and aggregation in saturated solution transport, and cavitation with a small amount.
# DynamicSeedInjector

## 中文简介
**DynamicSeedInjector** 是一个 Ansys Fluent UDF 库，用于在**任意满足瞬态物理条件**的网格单元质心处随机注入预设的 DPM 粒子。  
该算法在每一个指定时间步收集所有符合条件的单元坐标，从中随机采样，并将粒子注入到采样位置。  
适用于空化成核、晶体析出、气泡生成等动态成核场景。

**核心特点**：
- 基于网格质心随机采样，无需预定义注入区域。
- 自动跨进程并行通信，适用于大规模并行计算。
- 用户只需修改条件判断（如压力、温度、相分数）即可适配不同物理模型。

**许可**：本代码开源且免费使用。若用于商业项目，请告知作者（联系方式见文末），无需付费。

## English Description
**DynamicSeedInjector** is an Ansys Fluent UDF library that injects pre‑initialized DPM particles at **any cell centroid satisfying transient physical conditions**.  
At each injection time step, the algorithm collects all eligible cell centroids across the entire domain, randomly samples a specified number of positions, and places particles at those locations.  
Ideal for modeling dynamic nucleation processes such as cavitation, crystal precipitation, or bubble generation.

**Key Features**:
- Random sampling from centroid pool – no predefined injection zones required.
- Fully parallelized with automatic inter‑process communication.
- Easily adapt to your physical criterion by modifying a single condition (pressure, temperature, phase fraction, etc.).

## License
This project is licensed under the MIT License.  
You are free to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the software.  
See the `LICENSE` file for full details.

## Feedback & Contributions
If you find this project helpful, or would like to share how you use it (including in commercial projects), feel free to open an Issue or start a Discussion. Your feedback helps me improve the project continuously. Thank you!

---

## 快速开始 (Quick Start)

1. **编译 UDF**：将 `DynamicParticlesInjector.c` 放入 Fluent 工作目录，使用 `Compile UDF` 加载。
2. **设置注射源**：创建 `group` 或 `cone` 注射源，**Number of particles** 设为 ≥ `P_NUM`。
3. **调节参数**：
   - `P_NUM`：每次注入粒子数
   - `SPAN`：注入间隔（时间步）
   - 条件判断：修改 `if (center[1] > 0)` 为你的物理条件。
4. **运行计算**：瞬态模拟开始后，粒子将自动按设定注入。

**详细说明见 `INSTRUCTION.md`**。

---

## License

本项目采用 MIT 许可证。你可以自由使用、修改、商用，详情请见 LICENSE 文件。

## 反馈与合作

如果你在项目中使用了本代码（无论个人还是商业用途），并愿意分享你的经验或想法，欢迎通过 [Issues](链接) 或 [Discussions](链接) 告诉我们。你的反馈对项目持续改进非常宝贵！

---

*为 CFD 开发者而生，让离散相模拟更灵活。*  
*Built for CFD developers – making discrete phase modeling more flexible.*
