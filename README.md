# ActionRoguelike Survivor Progression

UE5 C++ gameplay extension based on Tom Looman's ActionRoguelike sample project.

This repository extends the original third-person action roguelike sample with a Vampire Survivors-style progression loop. The current work focuses on kill-based experience gain, player leveling, and event hooks for future upgrade choices.

## Current Features

- Ported the project to Unreal Engine 5.7 for local development.
- Added a player progression foundation in `ARoguePlayerState`.
- Added replicated `Level` and `Experience` state.
- Added experience growth calculation with configurable base XP and growth rate.
- Added robust `AddExperience()` and level-up handling.
- Awarded experience from `ARogueGameModeBase::OnActorKilled()` when a player kills an enemy.
- Added level-up and experience-change delegates for future UI integration.

## Planned Features

- Experience orb pickup dropped by defeated enemies.
- Level-up choice screen with three random upgrade options.
- Upgrade effects for attack damage, movement speed, and max health.
- HUD display for level and XP progress.

## Key Files

```text
Source/ActionRoguelike/Player/RoguePlayerState.h
Source/ActionRoguelike/Player/RoguePlayerState.cpp
Source/ActionRoguelike/Core/RogueGameModeBase.h
Source/ActionRoguelike/Core/RogueGameModeBase.cpp
```

## Build Environment

- Unreal Engine 5.7
- Visual Studio 2022
- Windows SDK 10.0.26100.0

## Notes

The original project targets Unreal Engine 5.6. This fork has small UE 5.7 compatibility adjustments, including explicit plugin dependencies and a disabled experimental Niagara Data Channel path that is not required for the progression work.

Original project:

<https://github.com/tomlooman/ActionRoguelike>
