# 项目面试问题整理

适用项目：

- `D:\workspace\test\ActionRoguelike`
- `D:\workspace\test\tinyrenderer-master`

本文档目标不是背诵标准答案，而是帮助面试时把项目讲清楚：做了什么、为什么这么做、遇到什么问题、如何验证、还有哪些不足。

---

## 1. 项目整体叙事

### ActionRoguelike Survivor Progression

一句话介绍：

> 这是一个基于 Tom Looman ActionRoguelike 示例工程的 UE5 C++ 玩法扩展项目。我在已有第三人称动作框架上，加入了类 Vampire Survivors 的经验、拾取、升级选择、机制型强化、持续刷怪和稳定性调试流程。

适合强调：

- UE Gameplay Framework：`GameMode`、`GameState`、`PlayerState`、`Subsystem`、`HUD/UserWidget`。
- C++ 与 Blueprint/资产协作：避免盲改 `.uasset`，核心逻辑放 C++，展示和配置留给资产。
- 玩法系统工程化：升级池、机制型强化、拾取系统、刷怪难度、测试命令。
- 稳定性和调试：日志扫描、崩溃定位、RHI/Niagara/AttributeSet/Actor 生命周期问题。

边界要说清：

- 不是从零写完整商业游戏。
- 原始战斗框架、部分 AI、基础动作系统来自示例项目。
- 自己主要扩展的是 survivor progression、升级机制、拾取/奖励、测试与稳定性。

### tinyrenderer

一句话介绍：

> 这是一个无图形 API 的 C++ 软件渲染器练习项目，通过手写矩阵变换、光栅化、重心坐标、z-buffer、纹理采样、法线贴图和 Phong 光照，理解 GPU 渲染管线的核心原理。

适合强调：

- 图形学基础：模型变换、视图变换、透视投影、viewport 映射。
- 光栅化：bounding box 扫描、重心坐标、三角形覆盖测试。
- 深度测试：z-buffer。
- 插值：透视校正插值。
- 着色：Phong、diffuse/specular/normal map。
- C++ 基础：模板向量/矩阵、OBJ/TGA 解析、OpenMP 并行扫描。

边界要说清：

- 这是学习/训练性质的软件渲染器，不是实时 GPU 渲染器。
- 目标是理解渲染管线，不是替代 OpenGL/DirectX/Vulkan。

---

## 2. ActionRoguelike 面试问题

### 2.1 项目背景与职责

**Q1：这个 UE 项目你主要做了什么？**

回答要点：

- 基于原 ActionRoguelike 示例工程做 survivor-like progression 扩展。
- 核心循环从“动作战斗 demo”扩展为：
  1. 击杀敌人。
  2. 掉落 XP 球和金币。
  3. 拾取 XP 升级。
  4. 升级时三选一强化。
  5. 敌人随时间持续生成并增强压力。
- 主要 C++ 系统：
  - `ARoguePlayerState`：等级、经验、升级选项、强化堆叠。
  - `URoguePickupSubsystem`：金币/XP 拾取、吸附、ISM 渲染。
  - `ARogueGameModeBase`：刷怪、难度、击杀触发机制。
  - `URogueMainHUDWidget` / `URogueUpgradeChoiceWidget`：XP HUD、升级选择 UI。
  - `URogueCheatManager`：功能测试命令。

**Q2：为什么选择基于示例项目扩展，而不是从零开发？**

回答要点：

- 目标是展示玩法系统、UE C++ 工程能力和问题排查能力，而不是重复造基础移动/相机/动作框架。
- 示例项目已有动作、AI、交互、属性和存档基础，可以更快验证 survivor loop。
- 真实工程也经常是在已有系统上迭代，重点是理解边界、避免破坏原框架。

**Q3：这个项目最能体现你能力的部分是什么？**

回答要点：

- 不只是加数值，而是做机制型强化：
  - `KillExplosion`：击杀延迟爆炸、范围指示、范围伤害、击退。
  - `LastStandShield`：濒死保护、冷却、堆叠。
  - `ChainLightning`：直接攻击命中触发范围弹射、内置冷却、防递归。
- 做了可测试性：
  - `GrantUpgrade ChainLightning`
  - `SpawnMonster Monster_MinionRanged 8`
  - `AddXP`
  - `ForceLevelUp`
  - `GodMode`
- 做了稳定性修复：
  - 防止无效 Actor/AttributeSet 崩溃。
  - 处理 Niagara 示例资源 ensure。
  - 处理刷怪失败、碰撞失败、日志刷屏。

---

### 2.2 UE Gameplay Framework

**Q4：为什么经验和升级放在 `PlayerState`，而不是 Character？**

回答要点：

- `Character/Pawn` 可能死亡、重生、被重新 possess；`PlayerState` 更适合保存玩家身份相关状态。
- 经验、等级、强化堆叠属于玩家进度，不属于某一具身体。
- `PlayerState` 天然支持网络复制，适合多人扩展。
- UI 可以监听 `PlayerState` 的 delegate，如 `OnExperienceChanged`、`OnLevelChanged`。

可追问：

- 多人共享经验应该放哪里？

回答：

- 当前是 per-player `PlayerState`。
- 如果做多人共享进度，应迁移到 `GameState` 或专门的 team progression component/subsystem。
- `PlayerState` 保留个人选择、个人属性应用；`GameState` 管共享 XP、等级和升级阶段。

**Q5：为什么刷怪逻辑放在 `GameMode`？**

回答要点：

- `GameMode` 只存在于服务器，适合权威控制规则：刷怪、难度、击杀奖励、生成成本。
- 刷怪不能由客户端决定，否则会出现作弊和状态不同步。
- 怪物生成依赖 DataTable、EQS/NavMesh、spawn credit，都属于服务端规则。

**Q6：`GameState` 在项目里承担什么？**

回答要点：

- 适合保存所有客户端都需要看到的 match 状态。
- 项目中用于复制 pickup cosmetic locations 等共享状态。
- 未来多人共享 XP、升级投票阶段也更适合放到 `GameState`。

---

### 2.3 经验、升级和强化系统

**Q7：经验升级系统怎么设计的？**

回答要点：

- `ARoguePlayerState` 保存：
  - `Level`
  - `Experience`
  - `BaseExperienceToNextLevel`
  - `ExperienceGrowthRate`
- 每级需求：

```text
BaseExperienceToNextLevel * ExperienceGrowthRate^(Level - 1)
```

- `AddExperience()` 支持一次获得大量经验并连续升级。
- 升级后生成 pending upgrade choices，并通过 UI 让玩家选择。
- 经验变化、等级变化通过 delegate 通知 HUD。

**Q8：升级选项为什么用 `FRogueUpgradeChoice` + stack？**

回答要点：

- `FRogueUpgradeChoice` 描述候选项：ID、名称、描述、稀有度、效果类型、属性 tag、数值、最大层数。
- `FRogueUpgradeStack` 存玩家已获得层数。
- 好处：
  - UI、选择逻辑、效果应用解耦。
  - 支持数值升级和机制升级。
  - 后续可从 `URogueUpgradeDataAsset` 读取，减少硬编码。

可追问：

- 为什么不用 `TMap<FName, int32>` 复制？

回答：

- UE 反射/复制对 map 的支持有版本和配置限制，项目里选择 `TArray<FRogueUpgradeStack>` 更稳。
- 数量不大，线性查找成本可以接受。

**Q9：机制型强化和纯数值强化有什么区别？**

回答要点：

- 纯数值强化：攻击力 +5、最大生命 +20、拾取半径 +200。
- 机制型强化：改变战斗行为或决策空间。
  - 击杀爆炸会改变站位和清怪节奏。
  - 闪电链让攻击有范围收益。
  - 濒死盾改变容错和风险收益。
- Roguelike 面试里要强调“改变玩法策略”，不是简单堆数值。

**Q10：`ChainLightning` 之前有什么 bug？怎么修？**

回答要点：

- 之前把电刀触发挂在通用 `ApplyDamage()`，只用“context tags 为空”判断。
- Blueprint 的流血/燃烧周期伤害如果没传 context tag，也会被当成直接攻击命中，从而误触发电刀。
- 修复：
  - 从 `ApplyDamage()` 移除电刀触发。
  - 只在 `ApplyDirectionalDamage()` 成功后尝试触发。
  - 电刀自身、爆炸、反伤都带 context tag，防止递归。
- 这是一个典型的“通用入口放了具体机制”的设计问题。

**Q11：`ChainLightning` 为什么需要 cooldown 和 source lock？**

回答要点：

- cooldown 控制触发频率，避免每帧/每个多段命中刷爆伤害。
- source lock 防止同一个主目标在很短时间内反复触发，尤其是持续碰撞、多段投射物或周期性效果。
- 只有实际命中至少一个 secondary target 时才消耗 cooldown，避免空触发影响手感。

**Q12：`KillExplosion` 为什么做 1 秒延迟和范围指示器？**

回答要点：

- 直接爆炸反馈不清楚，也难以规避。
- 延迟 + 地面/球形范围指示能让玩家理解将发生什么。
- 设计上更像技能预警，让机制可读。
- 技术上用 timer 延迟，爆炸时 overlap 查找敌人，应用带 context 的伤害和击退。

---

### 2.4 拾取系统和性能

**Q13：XP 球和金币拾取怎么做的？**

回答要点：

- 使用 `URoguePickupSubsystem` 管理 pickup 数据。
- 金币和 XP 分开存储位置、数量、吸附目标、ISM instance id。
- 一定范围内自动吸附到玩家。
- 到达 collect radius 后给玩家加 XP 或 credits。
- 使用 ISM 而不是每个掉落物一个 Actor，减少大量掉落物时的 Actor 开销。

**Q14：为什么用 `UTickableWorldSubsystem` 管拾取？**

回答要点：

- pickup 是世界级系统，和具体 Actor 生命周期解耦。
- 可以统一 tick 吸附逻辑、统一管理 ISM 和音效组件。
- 适合 survivor 类大量拾取物，避免每个 pickup 自己 tick。

可追问：

- tick 会不会有性能问题？

回答：

- 会，所以当前用集中式 tick + ISM 降低 overhead。
- 后续可优化：
  - 空数组时 `IsTickable=false`。
  - 空间分区，只查玩家附近 pickup。
  - 分帧处理。
  - 使用 Niagara/GPU 表现，服务端只保留逻辑点。

**Q15：为什么旧金币会“靠近假消失”？**

回答要点：

- 旧金币走 `ARoguePickupActor_Credits` 交互路径，不是新 ISM pickup subsystem。
- 交互高亮 overlay/material 会让透明/发光材质看起来消失。
- 修复方向：
  - 手动拾取金币只按 F 交互。
  - 自动拾取 actor overlap 只在服务端权威执行。
  - 对 auto pickup 跳过 interaction overlay。

---

### 2.5 刷怪、难度和资源驱动

**Q16：刷怪逻辑是什么？**

回答要点：

- `GameMode` 定时累积 spawn credit。
- 从 `MonsterTable` 读取怪物候选，按 weight 随机。
- 检查 `SpawnCost <= AvailableSpawnCredit`。
- 通过 EQS 找生成点。
- 生成成功后才扣 spawn credit。
- 如果 EQS 失败，fallback 到玩家周围 NavMesh 采样点。
- Survivor 地图会启用 continuous spawning defaults，提高初始压力和持续刷怪强度。

**Q17：为什么 spawn credit 成功生成后才扣？**

回答要点：

- 之前如果 EQS 点位有效但 SpawnActor 因碰撞失败，仍然消耗 credit，会导致刷怪节奏异常。
- 现在用 `AdjustIfPossibleButAlwaysSpawn`，且只在 `NewBot` 成功后扣费。
- 这是稳定性和玩法节奏一致性的修复。

**Q18：为什么需要 DataTable / PrimaryDataAsset？**

回答要点：

- 怪物 class、权重、生成成本、actions 可以数据驱动。
- 设计和调参不需要频繁改 C++。
- `URogueMonsterData` 可以作为怪物资产入口，异步加载并应用 action 列表。
- 后续升级池也可用 `URogueUpgradeDataAsset` 从硬编码迁移到资产。

---

### 2.6 UI 和输入

**Q19：升级 UI 怎么做的？**

回答要点：

- `URogueMainHUDWidget` 监听 PlayerState delegate。
- 升级时生成/显示 `URogueUpgradeChoiceWidget`。
- 选择后调用 `SelectUpgradeChoice(Index)`。
- 卡片显示名称、描述、稀有度、层数、下一层具体数值。
- 当前为 C++ 生成 UI，后续更适合做正式 UMG 资产。

**Q20：为什么暂停一开始会卡死？怎么处理？**

回答要点：

- UE 暂停会影响世界 tick、输入模式、UI focus 等，尤其如果 widget 不是 focusable 会报 `InputMode:UIOnly` 相关问题。
- 修复方向：
  - 保证选择 UI 可 focus/click。
  - 弹窗打开时设置正确 input mode。
  - 单机可硬暂停，但多人不能用硬暂停。
- 多人应使用限时选择阶段，比如 20 秒投票/选择，而不是 `SetGamePaused`。

**Q21：Shift 同时做疾跑和冲刺怎么设计？**

回答要点：

- 使用同一个输入动作区分短按和长按。
- 短按低于阈值触发 `Action.Dash`。
- 长按超过阈值触发 `Action.Sprint`，释放停止 sprint。
- 这样保留原有长按疾跑，同时扩展短按冲刺。

---

### 2.7 稳定性、日志和调试

**Q22：你遇到过哪些崩溃或稳定性问题？**

回答要点：

- `ApplyDamage` 对无效 Actor / 无 AttributeSet actor 过于乐观，导致 assert 或 pure virtual 风险。
- Niagara 示例资源 `NE_PostProcess / CameraShakeSourceComponent` 在 CDO/启动阶段触发 handled ensure。
- SpawnActor collision failure 导致刷怪失败和扣费错误。
- `Effect_Burning` 周期伤害 instigator 失效。
- `BP_ApplyEffectZone` 缺 AttributeSet。

**Q23：你怎么定位这些问题？**

回答要点：

- 看 `Saved/Logs` 和 `Saved/Crashes`。
- 写了/使用 `Tools/scan_ue_logs.py --logs 3` 汇总 error/warning/ensure/crash。
- 先按日志定位 callstack 和错误签名，再看是 C++ 生命周期、资产配置还是渲染资源问题。
- 对高频 warning 做降级或过滤，对真实问题加防御检查。

**Q24：为什么不直接忽略 warning？**

回答要点：

- UE 项目里 warning 很多，但需要分类：
  - 可恢复噪音：降级到 Verbose 或节流。
  - 配置错误：修资产或补 tag。
  - 生命周期风险：加 `IsValid`、weak pointer、清 timer。
  - RHI/Niagara：避免危险资产默认硬引用。
- 日志干净后，新的 crash 信号更容易被发现。

**Q25：如何验证 C++ 修改？**

回答要点：

- 使用完整 UBT 命令，关闭 Live Coding：

```powershell
& 'D:\EpicGame\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe' ActionRoguelikeEditor Win64 Development -NoLiveCoding -Project='D:\workspace\test\ActionRoguelike\ActionRoguelike.uproject' -WaitMutex -FromMsBuild
```

- PIE 里用 cheat command 做功能验证。
- 长时间测试后扫日志。

---

### 2.8 UE C++ 追问

**Q26：`UPROPERTY` 在这个项目里为什么重要？**

回答要点：

- 让 UObject 指针被 GC 跟踪。
- 支持编辑器暴露、复制、保存、蓝图访问。
- UE5 推荐 `TObjectPtr<>`。
- 对 transient runtime state 和 asset reference 要区分。

**Q27：`TWeakObjectPtr` 用在哪里？为什么？**

回答要点：

- 用在延迟爆炸、周期效果、吸附目标等可能跨帧/跨 timer 使用的 Actor 引用。
- Actor 可能在 timer 触发前被销毁，weak pointer 可以安全判断是否还有效。

**Q28：为什么不要盲改 `.uasset`？**

回答要点：

- 二进制资产不利于代码 review。
- 容易引入不可见配置变化。
- 更适合通过编辑器明确修改，或把核心逻辑放 C++，资产只做配置和展示。

**Q29：如果要把这个项目做成多人，最大改动是什么？**

回答要点：

- 经验从 per-player 移到 shared match/team state。
- 升级选择变成多人限时阶段。
- pickup 由服务端权威结算，共享 XP。
- 升级应用规则需要设计：个人各选、投票选一个、房主决定、默认超时选择。
- UI 不能硬暂停整个 game。

**Q30：项目还有哪些不足？**

回答要点：

- UI 仍是 C++ 生成原型，应该做正式 UMG 资产。
- XP 球、爆炸、电刀 VFX 还需要专用美术资源。
- 数值未完整调优。
- 升级池 DataAsset 还应在编辑器内正式配置。
- 多人共享进度未完成。

---

## 3. tinyrenderer 面试问题

### 3.1 项目背景

**Q1：tinyrenderer 是什么？你从中学到了什么？**

回答要点：

- 一个 CPU 软件渲染器，不使用 OpenGL/DirectX/Vulkan。
- 从 OBJ 模型和 TGA 贴图输入，输出 `framebuffer.tga`。
- 手写近似 GPU 管线：
  - vertex shader
  - clipping 后的 NDC 转换
  - viewport
  - rasterization
  - fragment shader
  - z-buffer
  - texture sampling
  - normal mapping
  - Phong shading
- 理解图形 API 背后的实际工作。

**Q2：这个项目和真实 GPU 管线有什么对应关系？**

回答要点：

- `PhongShader::vertex()` 类似 vertex shader。
- `rasterize()` 类似固定功能光栅化阶段。
- `PhongShader::fragment()` 类似 fragment shader。
- `zbuffer` 类似 depth buffer。
- `TGAImage framebuffer` 类似 color render target。
- `ModelView/Perspective/Viewport` 对应常见 MVP/viewport 变换。

---

### 3.2 渲染管线

**Q3：从模型到图像的流程是什么？**

回答要点：

1. `Model` 读取 OBJ 顶点、法线、UV、三角面索引。
2. 加载 diffuse、normal、specular TGA 贴图。
3. `lookat()` 构建 view matrix。
4. `init_perspective()` 构建透视矩阵。
5. `init_viewport()` 映射 NDC 到屏幕坐标。
6. 每个三角形调用 vertex shader 得到 clip coordinates。
7. `rasterize()` 做透视除法，得到 NDC。
8. 计算 screen space bounding box。
9. 对 box 内每个 pixel 算重心坐标。
10. 做三角形覆盖测试、深度测试。
11. fragment shader 采样贴图并计算光照。
12. 写入 framebuffer。

**Q4：`lookat()` 做了什么？**

回答要点：

- 构建相机坐标系。
- `n = normalized(eye - center)` 是相机 backward/view z 方向。
- `l = normalized(cross(up, n))` 是相机 right。
- `m = normalized(cross(n, l))` 是修正后的 up。
- 组合旋转和平移，把世界坐标变换到 eye/view space。

可追问：

- 为什么不用原始 up？

回答：

- 原始 up 可能不与视线方向正交。
- 通过 cross 重新构造正交基，避免相机坐标系歪斜。

**Q5：透视投影矩阵在这里怎么实现？**

回答要点：

- `Perspective` 中设置 `Perspective[3][2] = -1/f`。
- 顶点乘透视矩阵后，后续做 `clip / clip.w`。
- 这模拟了透视除法，远处物体视觉上变小。
- `f` 使用相机到中心点距离，属于简化实现。

**Q6：Viewport 变换是什么？**

回答要点：

- 把 NDC 坐标映射到屏幕像素坐标。
- NDC 大致在 [-1, 1] 范围。
- viewport matrix 将其缩放到 `[x, x+w]`、`[y, y+h]`。
- 这一步不改变模型形状，只改变输出图像区域。

---

### 3.3 光栅化与重心坐标

**Q7：三角形光栅化怎么做？**

回答要点：

- 先把三个顶点转换到屏幕空间。
- 计算三角形 bounding box。
- 遍历 bounding box 内每个像素。
- 用重心坐标判断像素是否在三角形内。
- 在三角形内则插值属性、做深度测试和着色。

**Q8：为什么不用遍历全屏？**

回答要点：

- 只遍历 bounding box 可以减少计算。
- 对每个三角形，只有包围盒内像素可能被覆盖。
- 这是 CPU 软件渲染里很直接的优化。

**Q9：重心坐标有什么用？**

回答要点：

- 判断点是否在三角形内：三个权重都 >= 0。
- 插值顶点属性：
  - UV
  - normal
  - depth
  - view-space position
- 对任意三角形内部点，三个权重和为 1。

**Q10：代码里怎么计算重心坐标？**

回答要点：

- 构造屏幕三角形矩阵 `ABC`：

```cpp
mat<3,3> ABC = {{
  {screen[0].x, screen[0].y, 1.},
  {screen[1].x, screen[1].y, 1.},
  {screen[2].x, screen[2].y, 1.}
}};
```

- 对像素 `(x,y,1)` 乘 `ABC.invert_transpose()` 得到 barycentric。
- 本质是在解线性方程：点是三个顶点的线性组合。

**Q11：为什么要做透视校正插值？**

回答要点：

- 屏幕空间线性插值 UV 会产生透视形变错误。
- 正确做法是先对属性除以 `w`，插值后再归一化。
- 代码里：

```cpp
vec3 bc_clip = {
  bc_screen.x / clip[0].w,
  bc_screen.y / clip[1].w,
  bc_screen.z / clip[2].w
};
bc_clip = bc_clip / (bc_clip.x + bc_clip.y + bc_clip.z);
```

- fragment shader 使用 `bc_clip` 插值 UV/normal。

**Q12：为什么深度 `z` 用 `bc_screen` 插值？**

回答要点：

- 当前实现中 `z` 来自 NDC 后的 z，用屏幕空间重心做插值。
- 这是教学实现，足以配合 z-buffer 做遮挡。
- 更完整管线会更严格处理 clip space、depth range 和 perspective-correct depth。

---

### 3.4 z-buffer 与剔除

**Q13：z-buffer 是什么？**

回答要点：

- 每个像素保存当前最靠近相机的深度值。
- 新 fragment 的深度如果不比已有值更近，就丢弃。
- 解决隐藏面问题。

代码对应：

```cpp
if (z <= zbuffer[x + y * framebuffer.width()]) continue;
zbuffer[x + y * framebuffer.width()] = z;
framebuffer.set(x, y, color);
```

**Q14：初始 z-buffer 为什么是 `-1000`？**

回答要点：

- 当前实现中更大的 z 被认为更靠前。
- 初始化为很小的值，保证第一个有效 fragment 可以写入。
- 真实引擎中 depth convention 可能相反，比如越小越近。

**Q15：背面剔除怎么做？**

回答要点：

- 代码用 `ABC.det() < 1` return。
- determinant 的符号和面积可以判断三角形朝向。
- 同时小于 1 的三角形也被当作小于一个像素的微小三角形丢弃。
- 这是简单教学实现，不是完整鲁棒的 clip/cull 流程。

---

### 3.5 着色、法线贴图和切线空间

**Q16：Phong shading 怎么算？**

回答要点：

- ambient：固定环境光。
- diffuse：`max(0, n dot l)`。
- specular：反射向量和视线方向夹角的幂。
- 最终颜色：

```text
diffuse_texture * (ambient + diffuse + specular)
```

**Q17：为什么要用 normal map？**

回答要点：

- 低多边形模型可以通过贴图提供高频表面细节。
- 不增加几何顶点数量，也能改变光照效果。
- fragment shader 从 normal map 采样法线，再参与光照。

**Q18：切线空间 normal map 怎么转到当前空间？**

回答要点：

- 根据三角形边 `E` 和 UV 变化 `U` 求 tangent、bitangent。
- 构建 Darboux frame：
  - tangent
  - bitangent
  - interpolated normal
- 从 normal map 采样的切线空间法线乘这个 frame，变换到 view space。

代码对应：

```cpp
mat<2,4> E = { tri[1]-tri[0], tri[2]-tri[0] };
mat<2,2> U = { varying_uv[1]-varying_uv[0], varying_uv[2]-varying_uv[0] };
mat<2,4> T = U.invert() * E;
```

**Q19：为什么法线要用 inverse transpose 变换？**

回答要点：

- 法线不是普通位置向量。
- 如果模型有非均匀缩放，直接乘 model matrix 会破坏法线与表面的垂直关系。
- 正确方式是乘 inverse transpose matrix。
- 项目里：

```cpp
varying_nrm[vert] = ModelView.invert_transpose() * model.normal(face, vert);
```

---

### 3.6 C++ 和性能

**Q20：`geometry.h` 的模板矩阵有什么特点？**

回答要点：

- 使用 `template<int n>` / `template<int nrows, int ncols>` 在编译期确定维度。
- 支持点乘、矩阵乘法、转置、逆、行列式。
- `dt<n>` 递归模板计算 determinant。
- 优点是代码短、教学清楚；缺点是性能和数值鲁棒性有限。

**Q21：这个 renderer 的性能瓶颈在哪里？**

回答要点：

- CPU 上逐像素遍历三角形 bounding box。
- 每个 pixel 都要算重心坐标、深度、fragment shader。
- fragment shader 里还有矩阵、normalize、pow、texture sample。
- 大模型/高分辨率时成本明显。

**Q22：OpenMP 在这里怎么用？有什么风险？**

回答要点：

- 在 `rasterize()` 的 x loop 上 `#pragma omp parallel for`。
- 风险：
  - 多线程同时写 framebuffer/zbuffer 可能有数据竞争。
  - 如果同一个三角形内部不同线程写不同像素通常问题小。
  - 但不同三角形之间仍是外层顺序执行；如果并行到三角形级别就必须处理 depth race。
- 更严谨做法：
  - tile-based rendering。
  - 每 tile 独立 zbuffer。
  - 或加锁/原子，但会影响性能。

**Q23：OBJ loader 有什么限制？**

回答要点：

- 只支持 triangulated mesh。
- face 格式假设为 `f v/t/n`。
- 不处理四边形、多材质、复杂 OBJ 语法。
- 贴图按文件名后缀加载：
  - `_diffuse.tga`
  - `_nm_tangent.tga`
  - `_spec.tga`

**Q24：这个 renderer 和现代引擎还有哪些差距？**

回答要点：

- 没有硬件加速。
- 没有完整 clipping。
- 没有 mipmap、各向异性过滤、gamma correction。
- 没有 PBR、IBL、shadow map、deferred rendering。
- 没有资源管理、实时窗口、交互相机。
- 但它足以解释 GPU 管线核心概念。

---

## 4. 两个项目可以组合出的面试亮点

### 亮点 1：既懂游戏玩法工程，也懂底层图形管线

可讲：

- UE 项目体现 gameplay architecture、网络复制意识、资产协作和调试能力。
- tinyrenderer 体现对渲染管线、数学、光栅化和着色原理的理解。
- 两者结合说明不是只会调蓝图或套引擎，也理解引擎底层一部分原理。

### 亮点 2：问题定位能力

可讲：

- UE 中定位过：
  - AttributeSet assert。
  - periodic damage instigator 失效。
  - `ChainLightning` 被流血误触发。
  - Niagara Examples startup ensure。
  - SpawnActor collision failure。
- 处理方法不是只看现象，而是从日志、调用路径、生命周期、context tag 设计上修。

### 亮点 3：知道工程边界

可讲：

- ActionRoguelike 没有声称从零写全部战斗系统，而是在已有框架上扩展 survivor 玩法。
- tinyrenderer 没有声称是实时渲染引擎，而是用于理解 GPU pipeline。
- 这种表述比夸大项目更可信。

---

## 5. 高频综合问题

**Q1：你最有技术含量的项目是哪一个？**

回答方向：

- 如果岗位偏 UE/gameplay：讲 ActionRoguelike。
- 如果岗位偏图形/引擎：讲 tinyrenderer，然后补充 UE 项目证明工程实践。

示例回答：

> 如果从工程完整度看，我会讲 UE 的 ActionRoguelike 扩展，因为它包含玩法、UI、拾取、刷怪、数据驱动、调试和稳定性。如果从基础原理看，tinyrenderer 更能体现我对渲染管线的理解。两个项目刚好覆盖了“用引擎做系统”和“理解引擎底层图形基础”。

**Q2：项目里最难的 bug 是什么？**

推荐讲 `ChainLightning` 误触发：

- 现象：攻击后持续流血/燃烧也像攻击一样触发电刀。
- 根因：机制挂在了通用 `ApplyDamage()`。
- 修复：移动到 `ApplyDirectionalDamage()`，机制伤害加 context tag。
- 收获：通用底层 API 不应该承担具体玩法语义。

也可以讲 Niagara ensure：

- 现象：启动/加载时出现 handled ensure。
- 根因：Niagara Examples 爆炸系统引用 `NE_PostProcess / CameraShakeSourceComponent`。
- 修复：移除 C++ 默认硬引用，后续用项目内安全 VFX。
- 收获：第三方/示例资产不能直接作为运行时默认资源。

**Q3：你怎么保证项目质量？**

回答要点：

- 小步改 C++。
- 每个功能用 cheat command 快速验证。
- UBT full build。
- 长时间 PIE 后扫日志。
- 避免盲改二进制资产。
- 对 crash/warning 做分类处理。

**Q4：如果继续做，你下一步会做什么？**

ActionRoguelike：

- 正式 UMG 升级界面。
- 专用 XP 球、爆炸、电刀 VFX。
- 升级池 DataAsset 化。
- 更多机制型强化。
- 多人共享 XP 和限时选择。
- 局外金币成长树。

tinyrenderer：

- 补完整 clipping。
- mipmap/双线性过滤/gamma correction。
- shadow mapping。
- tile-based rasterization。
- 更严格的线程安全和性能分析。

---

## 6. 面试时不要这样说

不要说：

- “我从零实现了完整 UE Roguelike 游戏。”
- “我写了完整商业级 survivor 游戏。”
- “tinyrenderer 是我自研的高性能实时渲染引擎。”
- “这个项目已经完整支持多人共享升级。”
- “所有特效和美术资源都是我做的。”

推荐说：

- “我基于已有 UE action framework 做了 survivor progression 扩展。”
- “我负责/实现了经验、升级、拾取、机制强化、刷怪调优和稳定性修复。”
- “tinyrenderer 是我用于理解软件光栅化和 GPU 管线的练习项目。”
- “目前 UI/VFX/多人仍是后续计划，我能清楚说明下一步怎么做。”

---

## 7. 快速复习清单

面试前重点看：

- ActionRoguelike:
  - `Source/ActionRoguelike/Player/RoguePlayerState.h/.cpp`
  - `Source/ActionRoguelike/Core/RogueGameModeBase.h/.cpp`
  - `Source/ActionRoguelike/Pickups/RoguePickupSubsystem.h/.cpp`
  - `Source/ActionRoguelike/Core/RogueGameplayFunctionLibrary.cpp`
  - `Source/ActionRoguelike/UI/RogueUpgradeChoiceWidget.cpp`
  - `.agents/ue-project-context.md`

- tinyrenderer:
  - `main.cpp`
  - `our_gl.cpp`
  - `our_gl.h`
  - `geometry.h`
  - `model.cpp`

必须能讲清：

- 为什么经验放 `PlayerState`。
- 为什么刷怪放 `GameMode`。
- 为什么 pickup 用 subsystem + ISM。
- 为什么电刀不能挂 `ApplyDamage()`。
- 重心坐标怎么判断三角形覆盖。
- z-buffer 怎么解决遮挡。
- normal map 为什么需要 tangent space。
- inverse transpose 为什么用于法线。

