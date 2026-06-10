# UE Project Context

Last updated: 2026-06-08

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
- `Source/ActionRoguelike/World/RogueRadiusIndicatorActor.h`
- `Source/ActionRoguelike/World/RogueRadiusIndicatorActor.cpp`

Implemented behavior:

- `ARoguePlayerState` tracks `Level`, `Experience`, `BaseExperienceToNextLevel`, and `ExperienceGrowthRate`.
- Kill rewards call `AddExperience(ExperiencePerKill)` in `ARogueGameModeBase::OnActorKilled`.
- Level thresholds scale with `BaseExperienceToNextLevel * ExperienceGrowthRate^(Level - 1)`.
- Experience can carry across multiple level-ups.
- `Level` and `Experience` replicate.
- `OnExperienceChanged` and `OnLevelChanged` delegates are available for HUD updates.
- Level-up now generates a small three-choice upgrade candidate list in `ARoguePlayerState`.
- Level-up now shows a first-pass C++-generated three-choice upgrade panel in `URogueMainHUDWidget`.
- `bAutoSelectUpgradeChoices` defaults to false so players choose one of the generated upgrade options.
- Upgrade choices can now be loaded from `URogueUpgradeDataAsset` via the `UpgradeData` field on `ARoguePlayerState`. If no asset is assigned, or the asset has no entries, the existing `BuildDefaultUpgradePool` hard-coded pool remains the fallback.
- First upgrade effects implemented:
  - `AttackDamage`: applies `Attribute.AttackDamage` `AddBase +5`.
  - `HealthMax`: applies `Attribute.HealthMax` `AddBase +20` and also heals current `Attribute.Health` by the same amount. `HealthMax` now replicates and rebroadcasts `Attribute.Health` on clients so existing health widgets refresh after the max value changes.
  - `PickupMagnet`: increases coin and XP pickup attraction radius by `+200`, up to 3 stacks.
  - `KillExplosion`: 5-stack mechanism upgrade; killed enemies spawn a ground-projected range indicator actor with an expanding ground ring and optional 3D sphere volume, growing from a small warning size to the final explosion radius over the 1-second delay. It then plays an explosion sound, damages nearby living AI enemies, and applies knockback/upward launch. Higher stacks increase explosion radius and damage.
  - `LastStandShield`: 3-stack mechanism upgrade; when lethal damage would kill the player and the cooldown is ready, the player restores health instead of entering the death flow. Higher stacks restore more health.
  - `ChainLightning`: 4-stack mechanism upgrade; direct player attack hits arc damage from the hit enemy to nearby living AI enemies. It is tuned as splash damage, not full primary-hit damage: default damage coefficient is 35% of attack damage, +10% per stack, with 2 initial targets and +1 target per stack. It has a 0.65s internal cooldown and only consumes that cooldown when at least one secondary target is damaged. The trigger is intentionally attached to the direct-hit damage path (`ApplyDirectionalDamage`) rather than the generic `ApplyDamage` helper, so periodic effects such as burning/bleed, kill explosion, reflected damage, and chain-lightning damage do not trigger more chain lightning. Prototype visibility uses a short-lived Niagara impact effect on the original hit target and each secondary target.
- Upgrade choice cards now show a second detail line with the next selected stack's concrete numbers, such as attack damage gain, max health gain, pickup radius, kill-explosion damage/radius/delay, last-stand heal/cooldown, and chain-lightning damage/target count/radius.
- Upgrade state replicates through `PendingUpgradeChoices` and replicated `FRogueUpgradeStack` entries. Do not use replicated `TMap` here; UHT does not support replicated maps in this project configuration.
- For quick PIE testing, `URogueCheatManager::GrantUpgrade` can directly grant an upgrade by id:
  - `GrantUpgrade AttackDamage`
  - `GrantUpgrade HealthMax`
  - `GrantUpgrade PickupMagnet`
  - `GrantUpgrade KillExplosion`
  - `GrantUpgrade LastStandShield`
  - `GrantUpgrade ChainLightning`
- Additional PIE test commands now exist on `URogueCheatManager`:
  - `AddXP 100`: grant XP directly.
  - `AddCredits 100`: grant credits directly for testing the saved out-of-run currency path.
  - `ForceLevelUp`: grant enough XP for the next level.
  - `ShowUpgradeChoices`: generate and show the three-choice upgrade panel without grinding kills.
  - `GodMode`: toggle player damage immunity for manual testing.
  - `ClearMonsters`: destroy current AI monsters.
  - `SpawnMonster Monster_MinionRanged 3`: spawn three ranged minions in front of the player.
  - `SpawnMonster Monster_MinionRanged_Elite 1`: spawn one elite ranged minion in front of the player.

Input behavior:

- `Shift` is split by hold duration through the existing sprint input action:
  - Short press under `ARoguePlayerCharacter::SprintHoldThreshold` starts `Action.Dash`.
  - Hold at or above the threshold starts `Action.Sprint`; releasing Shift stops sprint.

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
- Original AI death logic spawned 100 coins as a pickup-system stress test. This was changed to configurable reward values and later tuned toward out-of-run meta-progression currency:
  - `CoinDropCount = 1`
  - `CreditsPerCoin = 5`
  - `CoinDropRadius = 512.0f`
- Added defensive checks for pickup subsystem and navigation system before spawning coin pickups.

Design note:

- Experience is the core level-up resource.
- Coins are kept as a saved out-of-run progression currency for future meta upgrades. They are not currently consumed by default in-run upgrade choices.
- Do not claim a meta-upgrade shop/tree is implemented yet. Only coin reward pacing, pickup, and SaveGame persistence are implemented.

Legacy interactable coin note:

- Placed `ARoguePickupActor_Credits` actors are part of the original interactable pickup actor path, not the new ISM pickup subsystem.
- They are now manual interact pickups: focus them and press Interact/F to collect credits. They should not auto-hide just because the player entered their overlap sphere.
- They still respawn through `HideAndCooldown()` after collection.
- If a future pickup subclass enables `bCanAutoPickup`, auto-pickup overlap must remain server-authoritative only. Do not execute pickup interaction locally on clients, or the pickup can appear to vanish and then reappear when replicated state corrects it.
- The interaction component skips highlight overlay materials for auto-pickup `ARoguePickupActor` instances. The overlay material can make transparent/emissive pickup meshes look like they disappear when focused.

### 4. Experience Orb Pickup Foundation

Files:

- `Source/ActionRoguelike/Pickups/RoguePickupSubsystem.h`
- `Source/ActionRoguelike/Pickups/RoguePickupSubsystem.cpp`
- `Source/ActionRoguelike/Pickups/RoguePickupItemReplication.h`
- `Source/ActionRoguelike/Pickups/RoguePickupItemReplication.cpp`
- `Source/ActionRoguelike/Pickups/RoguePickupPreviewActor.h`
- `Source/ActionRoguelike/Pickups/RoguePickupPreviewActor.cpp`
- `Source/ActionRoguelike/Core/RogueGameState.h`
- `Source/ActionRoguelike/Core/RogueGameState.cpp`
- `Source/ActionRoguelike/Core/RogueDeveloperSettings.h`
- `Config/DefaultGame.ini`
- `Source/ActionRoguelike/Core/RogueGameModeBase.cpp`
- `Source/ActionRoguelike/AI/RogueAICharacter.h`
- `Source/ActionRoguelike/AI/RogueAICharacter.cpp`

Implemented behavior:

- Monster deaths now create an experience pickup through `URoguePickupSubsystem`.
- Player pickup proximity awards experience by calling `ARoguePlayerState::AddExperience`.
- `ARogueGameModeBase::OnActorKilled` no longer grants kill experience directly.
- `ExperiencePerKill` is passed to spawned AI as its `ExperienceDropAmount`.
- `ARogueGameState` replicates experience pickup cosmetic locations through a second `FPickupLocationsArray`.
- Experience pickups use a separate ISM and audio component from coin pickups.
- Coin and experience pickups both auto-attract to nearby player characters.
- Default pickup attraction radius is `600.0f`, default final collect radius is `80.0f`, and default attraction speed is `1600.0f`.
- `URoguePickupSubsystem` now explicitly stops and destroys its transient world-registered audio components and ISM components in `Deinitialize()`. This addresses a PIE/map-cleanup Renderer ensure involving `MSS_Interactables_CurrencyPickup` remaining registered during world cleanup.
- `ARoguePickupPreviewActor` is an editor-placeable helper for previewing the configured coin or XP pickup mesh without starting PIE.
- Default XP pickup mesh is `/Game/ExampleContent/Meshes/SM_Pickup_Health.SM_Pickup_Health`.
- Default XP pickup material override is `/Game/ExampleContent/Materials/M_Pickup_Coin.M_Pickup_Coin`, because the health pickup mesh's default `M_Pickup_Health` material can warn about missing usage flags when used through the pickup ISM.
- Default XP pickup sound is `/Game/SanderAudio/Sources/Interactables/MSS_Interactables_AbilityOrb.MSS_Interactables_AbilityOrb`.
- Default XP drop radius is `512.0f`, matching the coin drop radius so XP does not appear directly under the player and get picked up immediately.
- Avoid using `/Game/tharlevfx_tutorials/CharacterFX/ParagonSourceAssets/Meshes/SM_Buff_Sphere.SM_Buff_Sphere` directly in the pickup ISM. Its default material `M_Buff_Sphere_Heart` triggered an instanced static mesh usage warning followed by a `GPUSkinVertexFactory` assertion during playtesting.

### 5. HUD Progression Display

Files:

- `Source/ActionRoguelike/UI/RogueMainHUDWidget.h`
- `Source/ActionRoguelike/UI/RogueMainHUDWidget.cpp`

Implemented behavior:

- `URogueMainHUDWidget` creates a compact runtime XP HUD panel and now first tries to attach it to the same parent container as the existing `PlayerHealth_Widget` / `PlayerRage_Widget`. If that asset structure is not available, it falls back to `MainCanvasPanel` positioning near the player status widgets.
- The panel displays current level, current XP, XP required for the next level, and an XP progress bar.
- It binds to `ARoguePlayerState::OnExperienceChanged` and `ARoguePlayerState::OnLevelChanged`, then refreshes when XP or level changes.
- It also binds to `ARoguePlayerState::OnUpgradeChoicesGenerated`, creates a temporary three-choice upgrade widget at high viewport z-order, and calls `SelectUpgradeChoice(Index)` when the player clicks an option.
- In standalone mode, the upgrade choice widget pauses gameplay while open, then restores game input and the previous pause state when closed. Do not use this hard-pause path as the final multiplayer flow.
- This was implemented in C++ to avoid modifying `.uasset` widget assets blindly.
- The XP panel is styled as a compact dark-backed status bar so it can sit with the existing lower-left health/rage widgets instead of reading as a separate floating progression card.
- The three-choice upgrade widget now has clearer card hierarchy: rarity bar, short text badge per upgrade type, rarity label, left-aligned description, stack status, and a footer select affordance.
- Upgrade choices play lightweight 2D UI feedback sounds on hover and select. These now use imported Kenney Interface Sounds CC0 assets:
  - `/Game/ActionRoguelike/Audio/UI/UI_Upgrade_Hover_Kenney_Select_001`
  - `/Game/ActionRoguelike/Audio/UI/UI_Upgrade_Select_Kenney_Confirmation_001`
  - Source `.wav` files and license notes live under `Content/ActionRoguelike/Audio/UI/Source/`.

Todo:

- Verify the XP panel placement in PIE. It is now programmatically attached near the existing combat HUD status widgets, but the best final version should still be rebuilt as a real UMG asset or copied from the `PlayerHealth_Widget` / `PlayerRage_Widget` style instead of remaining fully code-generated.
- Verify the upgrade choice panel visually in PIE at the target viewport resolution. The current version is still C++ generated and functional, not final art.

### 6. Survivor Test Map

Files:

- `Content/ActionRoguelike/Maps/SurvivorTestLevel.umap`
- `Source/ActionRoguelike/Core/RogueGameModeBase.h`
- `Source/ActionRoguelike/Core/RogueGameModeBase.cpp`

Implemented behavior:

- `SurvivorTestLevel.umap` is a copy of `TestLevel.umap` for continuous-spawn playtesting.
- `ARogueGameModeBase` enables continuous spawning defaults when the map name contains `Survivor` or the URL contains `?ContinuousSpawning`.
- Continuous spawning mode ignores the local development CVar `game.DisableBotSpawning`, so the original `TestLevel` can keep single-enemy/debug behavior while the survivor map keeps spawning.
- Continuous spawning mode raises initial spawn credit, fallback spawn credit, max bot count, difficulty scaling, and reduces spawn failure cooldown.

Known issue:

- `SurvivorTestLevel.umap` was copied on disk from `TestLevel.umap`. If it does not appear in UE's Content Browser immediately, refresh/restart the editor or resave the package so the Asset Registry discovers it.

## Key Existing Systems

Game rules and spawning:

- `Source/ActionRoguelike/Core/RogueGameModeBase.h`
- `Source/ActionRoguelike/Core/RogueGameModeBase.cpp`

Player progression state:

- `Source/ActionRoguelike/Player/RoguePlayerState.h`
- `Source/ActionRoguelike/Player/RoguePlayerState.cpp`
- `Source/ActionRoguelike/Player/RogueUpgradeDataAsset.h`
- `Source/ActionRoguelike/Player/RogueUpgradeDataAsset.cpp`

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
- `Content/ActionRoguelike/Maps/SurvivorTestLevel.umap`
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

Canonical checklist:

- `.agents/survivor-todo.md` tracks completed, polish, high-priority, medium-term, and low-priority work. Update that checklist when gameplay tasks are completed or reprioritized.

Priority order:

1. Three-choice upgrade UI:
   - First-pass C++ runtime panel is implemented.
   - Polish the visual style and placement to match the existing health/rage UI.
   - Standalone slows time while the panel is open. Multiplayer behavior still needs a shared voting/selection flow.

2. Multiplayer shared progression:
   - Move experience/level-up progression from per-player `ARoguePlayerState` toward shared match state, likely `ARogueGameState`.
   - Experience pickups should contribute to shared team experience.
   - XP required per level should scale by connected player count, roughly multiplying by player count so four players need about four times the XP.
   - On shared level-up, all players should receive the same upgrade choice phase and have about 20 seconds to choose.
   - Decide aggregation rule: each player gets their own selected upgrade, or team applies one option based on majority/host/default selection.
   - Avoid `SetGamePaused` for upgrade selection because it can stall UI/game flow in this prototype; use a timed phase and possibly time dilation or input gating if needed.

3. Upgrade data extraction:
   - C++ support exists through `URogueUpgradeDataAsset`; `ARoguePlayerState::BuildUpgradePool` prefers the assigned asset and falls back to `BuildDefaultUpgradePool`.
   - Next editor step: create a real upgrade-pool DataAsset under Content and assign it to the player state defaults / relevant Blueprint defaults.
   - Preserve support for effect types such as attribute add, pickup-radius modification, and mechanism grants.

4. More mechanism upgrades:
   - First extra mechanisms implemented: `LastStandShield` and `ChainLightning`.
   - Add further candidates such as dash-empowered next attack, chain lightning, low-health shield variants, or coin-charged attacks.
   - If a real `URogueUpgradeDataAsset` is assigned, remember that the default hard-coded pool is bypassed; new upgrades must be added to the asset to appear randomly.

5. Level-up feedback:
   - Replace the temporary on-screen debug level-up message with polished HUD text / sound.
   - Keep this lightweight before building the upgrade-selection UI.

6. XP pickup presentation polish:
   - Tune pickup radius, drop offset, scale, and material after playtesting.
   - Create and save a dedicated XP material asset with instanced static mesh / Nanite usage flags before assigning it to the pickup ISM.

7. Gameplay tuning:
   - Tune numeric values after playtesting instead of treating current defaults as final.
   - Current tuning targets include XP/coin drop radius, pickup attraction speed/radius, upgrade magnitudes, kill explosion radius/delay/damage, knockback/upward force, explosion sound volume, and ground indicator visibility/height.
- Kill explosion indicator now uses existing mesh/material assets for function testing; create a dedicated translucent sphere/ring material later if the temporary `M_Radius_Glow` look is too noisy or opaque.
- `ARogueRadiusIndicatorActor` now forces its ring and sphere mesh components to use non-Nanite fallback rendering and disables distance field lighting / ray tracing visibility. This avoids a D3D12RHI render-thread crash seen when additive `M_Radius_Glow` was applied to the Nanite `Sphere` mesh during repeated kill-explosion indicators.

### 10. Damage Safety Fixes

Files:

- `Source/ActionRoguelike/Core/RogueGameplayFunctionLibrary.cpp`
- `Source/ActionRoguelike/ActionSystem/RogueActionEffect.h`
- `Source/ActionRoguelike/ActionSystem/RogueActionEffect.cpp`

Implemented behavior:

- `URogueGameplayFunctionLibrary::CanApplyDamage` now rejects invalid damage causers and invalid targets instead of relying on `check`.
- `URogueGameplayFunctionLibrary::GetActionComponentFromActor` now rejects invalid actors and first looks for `URogueActionComponent` directly before falling back to the C++ interface path. This avoids pure virtual interface calls when damage is attempted from an actor that is already being destroyed.
- `URogueActionEffect` periodic timers now keep a weak instigator reference and skip/clear periodic execution when the instigator is no longer valid.
- `URogueGameplayFunctionLibrary::IsAlive` now treats invalid actors, actors without action components, and actors without health attributes as simply not alive instead of logging warnings or asserting.
- `URogueGameplayFunctionLibrary::CanApplyDamage` now rejects actors without `URogueActionComponent` or `Attribute.Health` at `VeryVerbose` instead of letting `ApplyDamage` warn. This prevents kill explosions or melee/radius overlaps from logging warnings when they touch treasure chests or other non-health interactables.
- `URogueActionComponent::GetAttribute` and `GetAttributeValue` now return null/0 for actors without an AttributeSet instead of asserting. This prevents utility/effect actors from crashing the editor when queried defensively.
- `URogueGameplayFunctionLibrary::ApplyDamage` now uses `DamageCoefficient` as flat damage when the damage causer has no `Attribute.AttackDamage`. Pawn/monster/projectile damage still scales by `AttackDamage`; utility actors such as effect zones can deal periodic damage without needing their own AttributeSet.
- `URogueActionComponent::StartActionByName` no longer prints on-screen failure messages for AI-controlled pawns. AI attack failure is often expected when the target is dead, missing, blocked, or out of valid attack state.
- `URogueActionComponent::AddAction` now accepts action effects without `ActivationTag` without triggering an ensure or logging `Ignoring ActionEffect...` repeatedly. Untagged effects simply skip tag-based stack lookup and are created normally.
- Bot spawn failure logs are throttled through `ARogueGameModeBase::LogSpawnFailureThrottled`, and common configuration/runtime failures such as missing `SpawnBotQuery`, failed EQS, empty EQS results, invalid monster data, or missing action components no longer spam every spawn tick or crash with `check`.
- Bot spawning now uses `AdjustIfPossibleButAlwaysSpawn` and only subtracts spawn credit after the monster actor is created, so temporary spawn-point collision does not waste spawn credit.
- Bot spawning now has a fallback path when `SpawnBotQuery` fails or returns no locations. It samples a random point around a living player, projects it to NavMesh, and then uses the normal async monster spawn path. Tunables are `FallbackSpawnMinDistance`, `FallbackSpawnMaxDistance`, and `FallbackSpawnAttempts`.
- Spawn fallback failures caused by temporary missing NavMesh sample locations are logged at `Verbose` instead of `Warning`; configuration problems remain warnings.
- Explosive barrel damage logging is now `Verbose`, reports the actual instigator actor, and no longer emits misleading `OtherActor: PlayerController...` warnings.
- Actor pool priming now exits early when `game.ActorPooling` is disabled, avoiding the corpse prewarm `World has no context` warning during editor startup.
- `Directors.Events.KillQuestA` is registered in `DefaultGameplayTags.ini` for the existing `ST_CombatDirector` asset.
- `URogueActionComponent` has `bRequireAttributeSet` for utility actors that need actions but not attributes. Keep it enabled for pawns/destructibles; disable it in editor for `BP_ApplyEffectZone` if that actor is only an effect trigger.
- Local log triage helper: run `python Tools/scan_ue_logs.py --logs 3` after a PIE session to summarize recent fatal/error/warning/ensure patterns and show recent crash folders.

Bug fixed:

- Selecting/using the kill explosion upgrade could trigger delayed or periodic damage while another actor/effect reference was no longer valid, causing UE to crash with `Pure virtual function being called` from `URogueGameplayFunctionLibrary::ApplyDamage`.
- After the player dies or AI loses target, behavior tree attack services could spam `Checking IsAlive on invalid or nullptr Actor: None` and `Failed to run: Action.PrimaryAttack`. These are now treated as normal failed conditions rather than user-facing errors.
- PIE startup and survivor playtests could repeatedly print `Ignoring ActionEffect, no ActivationTag...` and emit handled ensures from `URogueActionComponent::AddAction`. This was noise from stack-lookup logic, not a useful crash signal, and is now removed.
- Monster spawns could fail from collision after EQS returned a valid point, while still consuming spawn credit. Spawn collision handling and charge timing now avoid that.
- `RogueMonsterCorpse` pool prewarming created/destroyed actors while actor pooling was disabled, producing editor startup `World has no context` warnings. Pool priming now respects the pooling cvar.
- `ST_CombatDirector` referenced `Directors.Events.KillQuestA` without the tag being registered. The tag is now present in config.
- `Effect_Burning` periodic damage could crash with `Assertion failed: AttributeSet` when the damage causer was a utility actor without attributes. Attribute lookup and damage calculation are now defensive for that path.
- `ChainLightning` was previously attempted from the generic `ApplyDamage` path when context tags were empty. This allowed untagged periodic Blueprint damage such as burning/bleed to look like attack-hit damage. The trigger now lives in `ApplyDirectionalDamage` only.
- The default C++ hard reference to `/Game/NiagaraExamples/FX_Explosions/NS_Explosion_Medium` was removed from `KillExplosionVFX`. Niagara Examples explosion systems reference `NE_PostProcess` / `CameraShakeSourceComponent` and can create startup handled ensures before the typed-element registry is ready.

Remaining editor asset cleanup:

- `Effect_Burning` sound attach warning did not reappear in the latest logs. If `SpawnSoundAttached: NULL AttachComponent` returns, check the Blueprint and prefer attaching to the owning actor's mesh/root, or falling back to `SpawnSoundAtLocation`.
- `BP_ApplyEffectZone` `AttributeSetReceived` / missing AttributeSet warnings did not reappear in the latest logs. If they return, reopen the Blueprint and remove/replace the deleted interface event and confirm `bRequireAttributeSet` is disabled where the zone does not need attributes.
- Kill explosion currently keeps its warning indicator, damage, knockback, and sound, but has no default explosion Niagara VFX assigned in C++. Create/copy a project-local non-post-process explosion system before assigning this field again.

### P0 Stability And Tuning Notes

Current debugging workflow:

- After every crash or long PIE test, run `python Tools/scan_ue_logs.py --logs 3`.
- Watch for new `Fatal error`, `Unhandled Exception`, `Ensure condition failed`, `Pure virtual`, `Access violation`, `D3D12`, and `RHI` entries.
- Current latest logs did not show a fresh D3D12/RHI fatal after the radius-indicator Nanite mitigation, but there are recent historical crash folders under `Saved/Crashes`.

Primary tuning fields:

- Spawn count/frequency:
  - `ARogueGameModeBase::SpawnTimerInterval`
  - `ARogueGameModeBase::FallbackSpawnCreditsPerTick`
  - `ARogueGameModeBase::InitialSpawnCredit`
  - `ARogueGameModeBase::BaseMaxBotCount`
  - `ARogueGameModeBase::MaxBotCountPerDifficulty`
  - `ARogueGameModeBase::SpawnCreditDifficultyScale`
- XP upgrade speed:
  - `ARogueGameModeBase::ExperiencePerKill`
  - `ARoguePlayerState::BaseExperienceToNextLevel`
  - `ARoguePlayerState::ExperienceGrowthRate`
- Coin drop pacing:
  - `ARogueAICharacter::CoinDropCount`
  - `ARogueAICharacter::CreditsPerCoin`
  - `ARogueAICharacter::CoinDropRadius`
- Pickup attraction:
  - `URoguePickupSubsystem::PickupAttractRadius`
  - `URoguePickupSubsystem::PickupCollectRadius`
  - `URoguePickupSubsystem::PickupAttractSpeed`
- Kill explosion:
  - `ARoguePlayerState::KillExplosionRadius`
  - `ARoguePlayerState::KillExplosionRadiusPerStack`
  - `ARoguePlayerState::KillExplosionDamageCoefficient`
  - `ARoguePlayerState::KillExplosionDamageCoefficientPerStack`
  - `ARogueGameModeBase::KillExplosionDelay`
  - `ARogueGameModeBase::KillExplosionKnockbackStrength`
  - `ARogueGameModeBase::KillExplosionUpwardStrength`
  - `ARogueGameModeBase::KillExplosionCorpseImpulseStrength`

Todo:

- Move these prototype tuning values into a single DataAsset or developer settings object once the playtest target values are less volatile.
- Continue monitoring `Saved/Logs` for `RHI`, `D3D12`, corpse destruction, projectile, and kill-explosion signatures.

8. Out-of-run coin progression:
   - Use saved credits for a meta-upgrade tree or shop outside survivor runs.
   - Candidate meta upgrades: permanent max health, attack damage, movement speed, pickup magnet radius, starting upgrade choice, coin drop multiplier, or starting weapon unlocks.
   - Do not make coins duplicate the purpose of experience; XP stays in-run level-up progression.

9. Minimap:
   - Useful later, but lower priority than the kill-XP-level-up-upgrade loop.

## Resume Positioning

Recommended project name:

- `UE5 第三人称动作 Roguelike Demo 开发`

Recommended role:

- `独立开发者 / C++ 玩法系统开发`

Accurate summary:

- Built on top of an existing UE5 action framework.
- Implemented experience/leveling foundation, first-pass upgrade candidates, XP pickup foundation, dynamic spawn difficulty, and reward pacing fixes.
- Current implementation has not yet completed a player-facing upgrade selection UI, shop, reroll, or minimap.

Avoid saying:

- "Implemented full Vampire Survivors gameplay."
- "Implemented shop/reroll/player-facing three-choice upgrade UI."
- "Built all original combat systems from scratch."

## Last Known Good Commits

- `4f3b334 fix: stabilize spawn and coin reward pacing`
- `6be735a feat: add dynamic spawn difficulty`
- `08ccf0b Initial survivor progression fork`
