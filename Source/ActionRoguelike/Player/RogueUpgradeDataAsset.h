// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Player/RoguePlayerState.h"
#include "RogueUpgradeDataAsset.generated.h"

/**
 * Data-driven pool of level-up upgrades.
 */
UCLASS()
class ACTIONROGUELIKE_API URogueUpgradeDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrades")
	TArray<FRogueUpgradeChoice> Upgrades;
};
