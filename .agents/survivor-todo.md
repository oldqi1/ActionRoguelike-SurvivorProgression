# Survivor Roguelike Todo

Last updated: 2026-06-04

## 已完成

- [x] 阅读并维护 `.agents/ue-project-context.md` 项目上下文。
- [x] 基础经验/等级系统：击杀、经验、等级、升级阈值、复制。
- [x] 击杀奖励改为掉落 XP 球，不再直接给经验。
- [x] XP 球和金币都支持一定范围内自动吸附到玩家并拾取。
- [x] XP 拾取使用独立的 pickup visual/audio 数据，不再混用金币数据结构。
- [x] 添加 `ARoguePickupPreviewActor`，可在不开始游戏时预览金币/XP 拾取物模型。
- [x] 调整 XP 球默认模型/材质，避开曾触发 RHI 崩溃的 Buff Sphere 资源。
- [x] 添加玩家 XP/等级 HUD 面板，并挂到现有生命值/怒气状态区域附近。
- [x] 添加三选一升级选择 UI。
- [x] 升级选择 UI 改为单机暂停，不再使用子弹时间。
- [x] 升级选择 UI 可点击，焦点/输入模式问题已修。
- [x] 升级选择 UI 做了第一轮视觉优化：横向卡片、稀有度、徽章、层数、选择提示。
- [x] 升级选择 UI 音效改为 Kenney Interface Sounds CC0 专用 UI hover/select 音效。
- [x] 添加快速测试命令：`AddXP`、`ForceLevelUp`、`ShowUpgradeChoices`、`GrantUpgrade`、`GodMode`、`ClearMonsters`、`SpawnMonster`。
- [x] 初始升级：攻击力、最大生命、拾取磁铁、击杀爆炸。
- [x] `HealthMax` 升级后刷新生命值状态栏。
- [x] `KillExplosion` 支持多级叠加。
- [x] `KillExplosion` 增加 1 秒延迟爆炸、地面/球形范围指示器。
- [x] `KillExplosion` 增加爆炸音效、范围伤害、击退/上抛物理反馈。
- [x] `SurvivorTestLevel.umap` 从 `TestLevel` 复制出来用于持续刷怪测试。
- [x] Survivor/URL 连续刷怪模式：无视本地禁用刷怪 CVar，并提高刷怪压力。
- [x] 刷怪逻辑优化：EQS 失败后 fallback 到玩家周围 NavMesh 采样点。
- [x] 刷怪碰撞失败处理：改为 `AdjustIfPossibleButAlwaysSpawn`，成功创建后才扣 spawn credit。
- [x] Spawn 失败日志节流，避免刷屏。
- [x] 旧场景悬浮金币 `ARoguePickupActor_Credits` 改为按 `F` 手动拾取，不再靠近假消失。
- [x] 自动拾取 Actor overlap 改为服务端权威执行，避免客户端本地假隐藏。
- [x] 自动拾取 Actor 跳过交互 overlay，避免透明/发光材质被高亮材质影响。
- [x] `Effect_Burning` 周期伤害导致的 `AttributeSet` assert 崩溃已修。
- [x] `GetAttribute` / `GetAttributeValue` 对无 AttributeSet actor 改为安全返回。
- [x] `ApplyDamage` 对无 `AttackDamage` 的 utility/effect actor 使用 flat damage，避免崩溃。
- [x] 旧 `Ignoring ActionEffect, no ActivationTag...` 日志/ensure 已清理。
- [x] `Directors.Events.KillQuestA` GameplayTag 已注册。
- [x] Actor pool corpse prewarm 编辑器 warning 已缓解。
- [x] 刷怪 fallback 找不到 NavMesh 位置的可恢复失败已从 Warning 降为 Verbose，避免长时间 PIE 刷屏。
- [x] 新增 `URogueUpgradeDataAsset` 升级池数据资产支持，`ARoguePlayerState` 会优先读取资产，未配置时回退到当前默认升级池。
- [x] 添加机制型升级 `LastStandShield`：濒死时自动恢复生命并进入冷却，叠加提高恢复量。
- [x] 添加机制型升级 `ChainLightning`：玩家攻击命中时从被命中的敌人向附近敌人弹射伤害，叠加提高目标数/伤害。
- [x] `ChainLightning` 从击杀触发改为攻击命中触发，并用伤害 context tag 防止弹射/爆炸/反伤递归触发。
- [x] `ChainLightning` 伤害调低为溅射定位：默认 35% 攻击力，每层 +10%，避免高于本体攻击。
- [x] `ChainLightning` 增加 0.65 秒内置冷却；只有实际命中至少一个弹射目标时才消耗冷却。
- [x] `ChainLightning` 触发条件收紧为仅直接玩家伤害触发；带 context 的爆炸、反伤、闪电链等机制伤害不会触发。
- [x] `ChainLightning` trigger moved from generic `ApplyDamage` to direct-hit `ApplyDirectionalDamage`, so untagged periodic burning/bleed damage no longer triggers Statikk-style arcs.
- [x] `ChainLightning` 移除蓝色 debug line/sphere，可视化改为在原命中目标和弹射目标处播放 Niagara 命中特效原型。
- [x] 升级选择卡增加下一层具体数值展示：攻击、生命、拾取半径、爆炸范围/伤害、闪电链伤害/目标数/半径等。
- [x] `Shift` 输入支持短按冲刺、长按疾跑：短按触发 `Action.Dash`，长按超过阈值后启动 `Action.Sprint`，松开停止疾跑。
- [x] 金币定位改为局外成长货币，不再默认放进局内消耗型强化。
- [x] 金币掉落节奏调整为默认每只怪掉 1 枚、每枚 5 credits，作为局外成长货币的起点。
- [x] 添加快速测试命令 `AddCredits`，用于直接测试局外金币数值/存档路径。

## 已实现但还需要打磨

- [ ] XP/等级 HUD 最终风格：现在是 C++ 生成的近似样式，后续最好做成正式 UMG 资产并完全参考血条/怒气条。
- [ ] 升级选择 UI 最终美术：当前可用但仍是 C++ 生成卡片，后续可改成正式 UMG、加入动画和更强反馈。
- [ ] XP 球专用材质/模型：当前可用但仍借用 health mesh + coin material，需要专用 XP 球资源。
- [ ] 爆炸范围指示器最终材质：当前功能可用，后续需要专用半透明地面圈/球体材质。
- [ ] `ChainLightning` 最终特效：当前复用 Niagara 命中特效做原型反馈，后续可换成专用链状电弧、命中闪光和音效。
- [ ] Kill explosion final VFX: avoid hard-referencing Niagara Examples explosion systems that include `NE_PostProcess` / camera shake; create a project-local runtime-safe effect first.
- [ ] `Effect_Burning` 蓝图音效 attach：最新日志未再出现 `SpawnSoundAttached NULL AttachComponent`，保留观察；如复现再进蓝图修 AttachComponent/fallback 到 SpawnSoundAtLocation。
- [ ] `BP_ApplyEffectZone` 蓝图清理：最新日志未再出现 `AttributeSetReceived` / `No default AttributeSet`，保留观察；如复现再进蓝图确认旧 interface event 和 `bRequireAttributeSet`。
- [ ] 数值调优：刷怪频率、XP 速度、金币数量、吸附半径/速度、爆炸半径/伤害/击退、闪电链伤害/目标数/范围。
- [ ] 在编辑器中创建正式升级池 DataAsset，并赋值到玩家 `PlayerState`/相关蓝图默认值；当前 C++ 已支持，但还未创建 `.uasset`。
- [ ] 如果已启用升级池 DataAsset，把 `LastStandShield` 和 `ChainLightning` 加进资产；未配置 DataAsset 时默认硬编码池已经包含它们。
- [ ] 每次较长 PIE 后继续跑 `python Tools\scan_ue_logs.py --logs 3 --crashes 5`，确认没有新崩溃目录。

## 下一步高优先级

- [ ] 继续添加更多机制型强化，而不是纯数值强化。
- [ ] 添加更多怪物类型。
- [ ] 添加武器系统或可切换/可成长武器。
- [ ] 设计局外金币强化：永久生命/攻击/移速、开局金币磁铁、开局技能、掉落倍率等，避免金币和经验功能重复。

## 中期目标

- [ ] 多人共享经验/等级：经验进入队伍进度，而不是每个 `PlayerState` 独立升级。
- [ ] 多人升级选择流程：所有玩家同时看到同一组选项，约 20 秒选择。
- [ ] 多人升级投票/应用规则：决定多数投票、房主选择、每人各选一项，或默认选项。
- [ ] 多人升级流程不能用硬暂停，需要限时阶段、输入门控或轻量 time dilation。
- [ ] 关卡/Director 难度曲线进一步完善：随时间增加怪物压力和精英概率。

## 低优先级

- [ ] Minimap。
- [ ] 商店/重随升级选项。
- [ ] 更完整的 UI 动画、过场提示、升级特效。
- [ ] 将原型调参值集中到 DeveloperSettings/DataAsset，减少散落在 C++ 默认值里。
