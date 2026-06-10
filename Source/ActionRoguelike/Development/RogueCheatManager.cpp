// Fill out your copyright notice in the Description page of Project Settings.


#include "Development/RogueCheatManager.h"

#include "ActionRoguelike.h"
#include "EngineUtils.h"
#include "SharedGameplayTags.h"
#include "ActionSystem/RogueAction.h"
#include "ActionSystem/RogueActionComponent.h"
#include "SaveSystem/RogueSaveGameSettings.h"
#include "AI/RogueAICharacter.h"
#include "Core/RogueMonsterData.h"
#include "Core/RogueGameplayFunctionLibrary.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/RoguePlayerState.h"
#include "NavigationSystem.h"


void URogueCheatManager::HealSelf(float Amount /* = 100 */)
{
	APlayerController* MyPC = GetOuterAPlayerController();

	if (APawn* MyPawn = MyPC->GetPawn())
	{
		URogueActionComponent* ActionComp = URogueActionComponent::GetActionComponent(MyPawn);
		ActionComp->ApplyAttributeChange(SharedGameplayTags::Attribute_Health, Amount, MyPawn, EAttributeModifyType::AddBase);
	}
}


void URogueCheatManager::KillAll()
{
	for (ARogueAICharacter* Bot : TActorRange<ARogueAICharacter>(GetWorld()))
	{
		URogueGameplayFunctionLibrary::KillActor(Bot);
	}
}


void URogueCheatManager::DeleteSaveGame()
{
	const URogueSaveGameSettings* SGSettings = GetDefault<URogueSaveGameSettings>();
	UGameplayStatics::DeleteGameInSlot(SGSettings->SaveSlotName, 0);
}


void URogueCheatManager::GrantUpgrade(FName UpgradeId)
{
	APlayerController* MyPC = GetOuterAPlayerController();
	if (!MyPC)
	{
		return;
	}

	ARoguePlayerState* PlayerState = MyPC->GetPlayerState<ARoguePlayerState>();
	if (!PlayerState)
	{
		UE_LOGFMT(LogGame, Warning, "GrantUpgrade failed: player state is missing.");
		return;
	}

	if (PlayerState->GrantUpgradeById(UpgradeId))
	{
		UE_LOGFMT(LogGame, Log, "GrantUpgrade succeeded: {UpgradeId}.", UpgradeId);
	}
}


void URogueCheatManager::AddXP(int32 Amount)
{
	APlayerController* MyPC = GetOuterAPlayerController();
	if (!MyPC)
	{
		return;
	}

	ARoguePlayerState* PlayerState = MyPC->GetPlayerState<ARoguePlayerState>();
	if (!PlayerState)
	{
		UE_LOGFMT(LogGame, Warning, "AddXP failed: player state is missing.");
		return;
	}

	const int32 SafeAmount = FMath::Max(0, Amount);
	PlayerState->AddExperience(SafeAmount);
	UE_LOGFMT(LogGame, Log, "AddXP granted {Amount} XP.", SafeAmount);
}


void URogueCheatManager::AddCredits(int32 Amount)
{
	APlayerController* MyPC = GetOuterAPlayerController();
	if (!MyPC)
	{
		return;
	}

	ARoguePlayerState* PlayerState = MyPC->GetPlayerState<ARoguePlayerState>();
	if (!PlayerState)
	{
		UE_LOGFMT(LogGame, Warning, "AddCredits failed: player state is missing.");
		return;
	}

	const int32 SafeAmount = FMath::Max(0, Amount);
	PlayerState->AddCredits(SafeAmount);
	UE_LOGFMT(LogGame, Log, "AddCredits granted {Amount} credits.", SafeAmount);
}


void URogueCheatManager::ForceLevelUp()
{
	APlayerController* MyPC = GetOuterAPlayerController();
	if (!MyPC)
	{
		return;
	}

	ARoguePlayerState* PlayerState = MyPC->GetPlayerState<ARoguePlayerState>();
	if (!PlayerState)
	{
		UE_LOGFMT(LogGame, Warning, "ForceLevelUp failed: player state is missing.");
		return;
	}

	PlayerState->AddExperience(PlayerState->GetExperienceToNextLevel());
	UE_LOGFMT(LogGame, Log, "ForceLevelUp granted enough XP for the next level.");
}


void URogueCheatManager::ShowUpgradeChoices()
{
	APlayerController* MyPC = GetOuterAPlayerController();
	if (!MyPC)
	{
		return;
	}

	ARoguePlayerState* PlayerState = MyPC->GetPlayerState<ARoguePlayerState>();
	if (!PlayerState)
	{
		UE_LOGFMT(LogGame, Warning, "ShowUpgradeChoices failed: player state is missing.");
		return;
	}

	PlayerState->DebugGenerateUpgradeChoices();
	UE_LOGFMT(LogGame, Log, "ShowUpgradeChoices requested.");
}


void URogueCheatManager::GodMode()
{
	APlayerController* MyPC = GetOuterAPlayerController();
	if (!MyPC)
	{
		return;
	}

	APawn* MyPawn = MyPC->GetPawn();
	if (!MyPawn)
	{
		return;
	}

	const bool bNewCanBeDamaged = !MyPawn->CanBeDamaged();
	MyPawn->SetCanBeDamaged(bNewCanBeDamaged);
	UE_LOGFMT(LogGame, Log, "GodMode {State}.", bNewCanBeDamaged ? TEXT("disabled") : TEXT("enabled"));
}


void URogueCheatManager::ClearMonsters()
{
	int32 RemovedCount = 0;
	for (ARogueAICharacter* Bot : TActorRange<ARogueAICharacter>(GetWorld()))
	{
		if (IsValid(Bot))
		{
			Bot->Destroy();
			RemovedCount++;
		}
	}

	UE_LOGFMT(LogGame, Log, "ClearMonsters destroyed {Count} monster(s).", RemovedCount);
}


void URogueCheatManager::SpawnMonster(FName MonsterAssetName, int32 Count, float Distance)
{
	UWorld* World = GetWorld();
	APlayerController* MyPC = GetOuterAPlayerController();
	APawn* MyPawn = MyPC ? MyPC->GetPawn() : nullptr;
	if (!World || !MyPawn)
	{
		return;
	}

	const int32 SafeCount = FMath::Clamp(Count, 1, 50);
	const float SafeDistance = FMath::Max(100.0f, Distance);

	UAssetManager& AssetManager = UAssetManager::Get();
	const FPrimaryAssetId MonsterId(TEXT("Monsters"), MonsterAssetName);
	URogueMonsterData* MonsterData = Cast<URogueMonsterData>(AssetManager.GetPrimaryAssetObject(MonsterId));
	if (!MonsterData)
	{
		MonsterData = Cast<URogueMonsterData>(AssetManager.GetStreamableManager().LoadSynchronous(AssetManager.GetPrimaryAssetPath(MonsterId)));
	}

	if (!MonsterData || !MonsterData->MonsterClass)
	{
		UE_LOGFMT(LogGame, Warning, "SpawnMonster failed: could not load monster data {MonsterId}.", MonsterId.ToString());
		return;
	}

	const FVector Forward = MyPawn->GetActorForwardVector().GetSafeNormal();
	const FVector Right = MyPawn->GetActorRightVector().GetSafeNormal();
	const FVector BaseLocation = MyPawn->GetActorLocation() + Forward * SafeDistance;

	UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(World);
	int32 SpawnedCount = 0;
	for (int32 Index = 0; Index < SafeCount; ++Index)
	{
		const float SideOffset = (Index - (SafeCount - 1) * 0.5f) * 140.0f;
		FVector SpawnLocation = BaseLocation + Right * SideOffset;
		if (NavigationSystem)
		{
			FNavLocation NavLocation;
			if (NavigationSystem->ProjectPointToNavigation(SpawnLocation, NavLocation, FVector(250.0f, 250.0f, 500.0f)))
			{
				SpawnLocation = NavLocation.Location;
			}
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AActor* NewMonster = World->SpawnActor<AActor>(MonsterData->MonsterClass, SpawnLocation, MyPawn->GetActorRotation(), SpawnParams);
		if (!NewMonster)
		{
			continue;
		}

		if (ARogueAICharacter* NewAI = Cast<ARogueAICharacter>(NewMonster))
		{
			NewAI->SetExperienceDropAmount(25);
		}

		if (URogueActionComponent* ActionComp = URogueGameplayFunctionLibrary::GetActionComponentFromActor(NewMonster))
		{
			for (TSubclassOf<URogueAction> ActionClass : MonsterData->Actions)
			{
				if (ActionClass)
				{
					ActionComp->AddAction(NewMonster, ActionClass);
				}
			}
		}

		SpawnedCount++;
	}

	UE_LOGFMT(LogGame, Log, "SpawnMonster spawned {Count} instance(s) of {MonsterId}.", SpawnedCount, MonsterId.ToString());
}
