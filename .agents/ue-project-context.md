# UE Project Context

Last updated: 2026-05-28

## Project Overview

This repository is a UE 5.7 fork of Tom Looman's ActionRoguelike project.

Current portfolio direction: turn the original third-person action combat demo into a small Action Roguelike / survivor-style gameplay demo. The target loop is:

1. Fight monsters.
2. Gain experience and a small amount of coin reward.
3. Level up and eventually choose upgrades.
4. Enemy pressure increases over time.

Important: this is not a complete commercial game. Treat it as a gameplay systems prototype built on top of an existing action framework.

## Local Environment

- Project path: `D:\workspace\test\ActionRoguelike`
- Engine: `D:\EpicGame\UE_5.7`
- Visual Studio: `D:\Visual Studio\Visual Studio2022`
- Target platform: Windows
- GitHub: `https://github.com/oldqi1/ActionRoguelike-SurvivorProgression`

Use Git LFS for Unreal binary assets. Existing tracking includes typical UE asset extensions such as `.uasset`, `.umap`, `.uexp`, `.ubulk`, `.upipelinecache`, and `.shk`.

## Build Command

Use this command for full C++ verification:

```powershell
& 'D:\EpicGame\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe' ActionRoguelikeEditor Win64 Development -NoLiveCoding -Project='D:\workspace\test\ActionRoguelike\ActionRoguelike.uproject' -WaitMutex -FromMsBuild
```

`-NoLiveCoding` matters on this machine. A background ASUS/ROG service can keep a Live Coding mutex alive even after Unreal Editor is closed, causing normal UBT builds to fail with "Unable to build while Live Coding is active."

## Modules And Plugins

Runtime module:

- `ActionRoguelike`

Editor module:

- `RogueEditor`

Important enabled plugins / systems:

- Enhanced Input
- UMG
- Niagara
- StateTree / GameplayStateTree
- SignificanceManager
- AnimationBudgetAllocator
- Iris
- PropertyBindingUtils
- OnlineSubsystemSteam

UE 5.7 compatibility work already done:

- `.uproject` now targets EngineAssociation `5.7`.
- `PropertyBindingUtils` plugin is enabled for StateTree property binding.
- `ActionRoguelike.Build.cs` adds `PropertyBindingUtils`.
- `ActionRoguelike.Build.cs` defines `USE_DEFERRED_TASKS=0`.
- `RogueGameState.cpp` includes `Pickups/RoguePickupSubsystem.h`.
- Incompatible Niagara DataChannel write path was disabled earlier in the projectile subsystem.

## Implemented Portfolio Changes

### 1. Kill Experience And Leveling Foundation

Files:

- `Source/ActionRoguelike/Player/RoguePlayerState.h`
- `Source/ActionRoguelike/Player/RoguePlayerState.cpp`
- `Source/ActionRoguelike/Core/RogueGameModeBase.h`
- `Source/ActionRoguelike/Core/RogueGameModeBase.cpp`

Implemented behavior:

- `ARoguePlayerState` tracks `Level`, `Experience`, `BaseExperienceToNextLevel`, and `ExperienceGrowthRate`.
- Kill rewards call `AddExperience(ExperiencePerKill)` in `ARogueGameModeBase::OnActorKilled`.
- Level thresholds scale with `BaseExperienceToNextLevel * ExperienceGrowthRate^(Level - 1)`.
- Experience can carry across multiple level-ups.
- `Level` and `Experience` replicate.
- `OnExperienceChanged` and `OnLevelChanged` delegates are available for future HUD / level-up UI.

Important naming note:

- Use `GetPlayerLevel()`, not `GetLevel()`, to avoid conflict with `AActor::GetLevel()`.

### 2. Dynamic Spawn Difficulty

Files:

- `Source/ActionRoguelike/Core/RogueGameModeBase.h`
- `Source/ActionRoguelike/Core/RogueGameModeBase.cpp`

Implemented behavior:

- Difficulty level is based on elapsed world time.
- `DifficultyInterval` defaults to `120.0f`.
- `BaseMaxBotCount` defaults to `10`.
- `MaxBotCountPerDifficulty` defaults to `2`.
- `SpawnCreditDifficultyScale` defaults to `0.2f`.
- Max alive bot count now uses `GetCurrentMaxBotCount()` instead of a hard-coded `10.0f`.
- Spawn credit gain is multiplied by `GetSpawnCreditMultiplier()`.

### 3. Spawn And Coin Reward Pacing Fixes

Files:

- `Source/ActionRoguelike/Core/RogueGameModeBase.h`
- `Source/ActionRoguelike/Core/RogueGameModeBase.cpp`
- `Source/ActionRoguelike/AI/RogueAICharacter.h`
- `Source/ActionRoguelike/AI/RogueAICharacter.cpp`

Implemented behavior:

- Added `FallbackSpawnCreditsPerTick` so spawn credit can recover even if the curve is missing or too low.
- Fixed spawn affordability check from `SpawnCost >= AvailableSpawnCredit` to `SpawnCost > AvailableSpawnCredit`.
- Added defensive checks for missing `MonsterTable`, empty rows, non-positive total weight, and failed monster selection.
- Original AI death logic spawned 100 coins as a pickup-system stress test. This was changed to configurable reward values:
  - `CoinDropCount = 5`
  - `CreditsPerCoin = 10`
  - `CoinDropRadius = 512.0f`
- Added defensive checks for pickup subsystem and navigation system before spawning coin pickups.

Design note:

- Experience is the core level-up resource.
- Coins are kept as a separate in-run economy resource, intended for future shop purchases, item costs, or upgrade-option refresh.
- Do not claim shop or refresh behavior is implemented yet. Only the coin reward pacing and configurability are implemented.

## Key Existing Systems

Game rules and spawning:

- `Source/ActionRoguelike/Core/RogueGameModeBase.h`
- `Source/ActionRoguelike/Core/RogueGameModeBase.cpp`

Player progression state:

- `Source/ActionRoguelike/Player/RoguePlayerState.h`
- `Source/ActionRoguelike/Player/RoguePlayerState.cpp`

AI character and death reward handling:

- `Source/ActionRoguelike/AI/RogueAICharacter.h`
- `Source/ActionRoguelike/AI/RogueAICharacter.cpp`

Pickup system:

- `Source/ActionRoguelike/Pickups/RoguePickupSubsystem.h`
- `Source/ActionRoguelike/Pickups/RoguePickupSubsystem.cpp`

Action and attribute system:

- `Source/ActionRoguelike/ActionSystem/RogueActionComponent.h`
- `Source/ActionRoguelike/ActionSystem/RogueAttributeSet.h`
- `Source/ActionRoguelike/ActionSystem/RogueAction.h`

UI:

- `Source/ActionRoguelike/UI/RogueHUD.h`
- `Source/ActionRoguelike/UI/RogueMainHUDWidget.h`
- `Content/ActionRoguelike/UI/`

Monster data:

- `Content/ActionRoguelike/Monsters/DT_Monsters.uasset`
- `Content/ActionRoguelike/Monsters/Monster_MinionRanged.uasset`
- `Content/ActionRoguelike/Monsters/Monster_MinionRanged_Elite.uasset`
- `Source/ActionRoguelike/Core/RogueMonsterData.h`

Maps:

- `Content/ActionRoguelike/Maps/TestLevel.umap`
- `Content/ActionRoguelike/Maps/MainMenu_Entry.umap`

## Coding Guidance For Future AI

- Prefer small verified increments. Compile after each meaningful C++ slice.
- Avoid large rewrites of the original ActionRoguelike architecture.
- Do not modify `.uasset` files blindly unless the user is explicitly working in the editor.
- Keep claims aligned with implementation. If a feature is only planned, label it as planned.
- For UE reflected fields, changing `UPROPERTY`, `UFUNCTION`, or class layout may require full editor restart / full build.
- Prefer event/delegate updates for UI over per-frame polling when practical.
- Use `TimerManager` or existing spawn timers instead of adding new Tick code unless needed.
- Maintain safety checks around data assets, tables, EQS results, and subsystem lookups.

## Recommended Next Gameplay Steps

Priority order:

1. Experience orb pickup:
   - Spawn visible XP pickups on monster death.
   - Picking them up calls `ARoguePlayerState::AddExperience`.
   - Keep coin and XP resource roles separate.

2. Level-up upgrade data model:
   - Create a simple C++ struct / data asset representation for upgrade options.
   - Start with safe upgrades such as max health, attack damage, movement speed, cooldown reduction.

3. Three-choice upgrade UI:
   - On level up, pause or slow input flow and show three options.
   - First implementation can be single-player only.

4. HUD progression display:
   - XP bar.
   - Level text.
   - Optional difficulty/time display.

5. Coin use case:
   - Use coins for upgrade reroll, shop purchase, or healing/item cost.
   - Do not make coins duplicate the purpose of experience.

6. Minimap:
   - Useful later, but lower priority than the kill-XP-level-up-upgrade loop.

## Resume Positioning

Recommended project name:

- `UE5 第三人称动作 Roguelike Demo 开发`

Recommended role:

- `独立开发者 / C++ 玩法系统开发`

Accurate summary:

- Built on top of an existing UE5 action framework.
- Implemented experience/leveling foundation, dynamic spawn difficulty, and reward pacing fixes.
- Current implementation has not yet completed XP orbs, upgrade selection UI, shop, reroll, or minimap.

Avoid saying:

- "Implemented full Vampire Survivors gameplay."
- "Implemented shop/reroll/three-choice upgrade UI."
- "Built all original combat systems from scratch."

## Last Known Good Commits

- `4f3b334 fix: stabilize spawn and coin reward pacing`
- `6be735a feat: add dynamic spawn difficulty`
- `08ccf0b Initial survivor progression fork`
