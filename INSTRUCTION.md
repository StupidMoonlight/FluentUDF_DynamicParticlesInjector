# INSTRUCTION.md – UDF: Introducing_particles

## 中文说明

### 功能概述
该UDF用于在每个固定时间步间隔（由 `SPAN` 控制），从满足一定物理条件的网格单元中**随机抽取**指定数量的位置，并将DPM粒子注入到这些位置。  
适用场景：动态成核（如空化气泡、晶体析出），粒子的生成位置随流动实时变化。  
核心逻辑：所有节点收集满足条件的单元坐标 → 主节点随机采样 → 广播 → 重定位粒子。

### 使用前提
1. **注射源设置**：在Fluent中建立一个 **group** 或 **cone** 注射源，并将其 **Number of particles** 设为至少 `P_NUM`（建议设大一些，如1000），确保每个时间步有足够粒子可用。
2. **网格条件**：UDF中硬编码了条件 `center[1] > 0` (y>0)。请根据实际物理条件修改此判断。
3. **线程ID**：代码中 `Lookup_Thread(domain, 2)` 的 **2** 是网格中默认的计算域的ID，如果是多计算域的算例请根据实际情况修改。必须与你的网格对应。建议在Fluent中通过 `Define → Cell Zones` 查看正确ID，或者改用 zone name 动态查找（但本版本为简洁保留硬编码）。

### 参数配置
- `INIT_DATA_SIZE = 128`：数组初始大小，会自动扩展，一般无需修改。
- `SPAN = 1`：每隔多少时间步注入一次。若设为5，则每5个时间步注入一次。
- `P_NUM = 10`：每次注入的粒子数量。可根据物理需求修改。
- **采样数量自适应**：若全局有效单元数少于 `P_NUM`，则采样数自动降为有效单元数（不会出错）。

### 并行机制说明（重要）
- **所有节点都会调用该宏**，每个节点处理自己拥有的注射源粒子列表。因此代码中使用了跨进程通信：
  - 主节点(0)收集所有节点的候选单元坐标。这个算法在作者开源的另一个库中(p2p method)。
  - 主节点随机选取后，通过 `PRF_GRSUM` 将结果广播至所有节点。
- **为什么不用更直接的广播函数？** 因为Fluent UDF未暴露标准MPI广播，而 `PRF_GRSUM` 是一种稳定且版本兼容的技巧：只有主节点有非零数据，其余节点为零，求和后所有节点得到相同结果。此方法开销微小，保证适用性。
- **重复采样概率**：当总候选单元数 >> 采样数时，重复概率极低（例如1e6个单元采样10个，重复概率约1e-5），工程上可忽略。若应用严格要求唯一性，需自行添加去重逻辑。

### 常见问题
**Q：为什么在Fluent中一定要设置group或cone注射源？**  
A：Fluent的DPM注射源默认被分配到某个节点，但宏本身在所有节点上被调用。只有这两个注射源满足所有的粒子都在一个节点被注入，所以可以对所有粒子进行操作。由于算例的不同，**group** 或 **cone** 注射源所在的节点也不同，所以本UDF通过通信确保所有节点获得相同的采样位置，从而每个节点都能正确处理其拥有的粒子（虽然可能某些节点粒子数为0）。

**Q：条件判断能否改为更复杂的表达式？**  
A：可以。直接修改 `if (center[1] > 0)` 内的条件，甚至可将当前单元的物理量（如压力、温度）作为条件，参考 `C_P(c,t)`、`C_T(c,t)` 等宏。

**Q：动态网格下自动扩容是否足够？**  
A：是的，`realloc` 每次翻倍，可应对任意网格规模。但若满足条件的网格量巨大（>1e7），建议预先估算容量以避免多次重分配。

**Q：如何验证粒子是否被正确注入？**  
A：在Fluent后处理中绘制DPM粒子位置，每个注入时间步后观察粒子是否出现在满足条件的区域内。

---

## English Version

### Overview
This UDF injects a fixed number of particles (controlled by `P_NUM`) at a prescribed time interval (`SPAN`).  
Particles are placed at **cell centroids** that satisfy a user-defined condition (example: `y > 0`).  
It is designed for dynamic nucleation phenomena (e.g., cavitation, crystal precipitation) where nucleation sites evolve transiently.  

**Core workflow:**  
1. Collect cell centroids meeting the condition on every parallel process.  
2. Gather all centroids to the master process (node 0). This algorithm is found in another open-source code that was also made available by the author(p2p method). 
3. Master randomly selects `P_NUM` positions (with replacement).  
4. Broadcast the selected positions to all processes via a global sum trick.  
5. Reposition the first `P_NUM` particles in the injection source; remove extra ones.

### Prerequisites
1. **Injection source setup**: Create a **group** or **cone** injection in Fluent and set its **Number of particles** to a value **≥ P_NUM** (e.g., 1000). This ensures enough particles exist each time step.
2. **Condition**: The hardcoded `center[1] > 0` (y>0) must be replaced with your actual physical criterion.
3. **Thread ID**: The `Lookup_Thread(domain, 2)` line refers to the ID of the default calculation domain in the grid. If the calculation example involves multiple domains, please modify it according to the actual situation. Verify the correct ID in Fluent: `Define → Cell Zones`. For robustness, consider using zone name identification (not provided here for simplicity).

### Parameters (all in the code)
- `INIT_DATA_SIZE = 128` – initial array size; auto-expands if needed.
- `SPAN = 1` – injection interval (number of time steps between injections).
- `P_NUM = 10` – number of particle to inject per injection step.
- **Safety**: If candidates are fewer than `P_NUM`, the injection number automatically drops (no error).

### Parallelism – Why This Communication?
- `DEFINE_DPM_INJECTION_INIT` runs on **every** compute node, each handling its own local particle list (from the injection source).  
- To achieve consistent random positions across all nodes, we:
  - Gather all candidate coordinates to node 0.
  - Node 0 samples randomly.
  - Use `PRF_GRSUM` to broadcast the sampled positions to all nodes (since only node 0 has non-zero values, summing replicates the data).
- This method is stable across Fluent versions and avoids relying on undocumented MPI functions.  
- **Duplication probability**: When the candidate pool is large (e.g., 1e6 cells) and sampling count small (e.g., 10), the chance of picking the same cell twice is negligible (~1e-5). If uniqueness is critical, add a reject‑duplicate loop.

### Frequently Asked Questions
**Q: Why is it necessary to set group or cone injection sources in Fluent?**  
A: The Fluent DPM injection source is by default assigned to a specific node, but the macro is called on all nodes. Only these two injection sources ensure that all particles are injected at the same node, allowing operations on all particles. Since the case differs, the node where **group** or **cone** injection sources are located also varies. Therefore, this UDF ensures all nodes obtain the same sample location through communication, so that each node can properly process its own particles (even though some nodes may have zero particles).

**Q: Can I use more complex conditions (e.g., pressure > threshold)?**  
A: Yes. Inside the `begin_c_loop_int` loop, access cell variables like `C_P(c,t)` for pressure, `C_T(c,t)` for temperature, etc.

**Q: Is the dynamic array resize adequate for moving/deforming meshes?**  
A: Yes. `realloc` doubles the capacity each time, handling any cell count. For extremely large meshes meeting physical conditions (>10 million cells), preset `INIT_DATA_SIZE` to a high value to avoid frequent reallocation.

**Q: How to verify injection is working?**  
A: In Fluent post-processing, plot DPM particle positions. After each injection time step, particles should appear inside the user‑defined condition region.
