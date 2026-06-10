// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "RogueCheatManager.generated.h"

/**
 * 
 */
UCLASS()
class ACTIONROGUELIKE_API URogueCheatManager : public UCheatManager
{
	GENERATED_BODY()

	
	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void HealSelf(float Amount = 100);
	
	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void KillAll();
	
	UFUNCTION(Exec)
	void DeleteSaveGame();

	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void GrantUpgrade(FName UpgradeId);

	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void AddXP(int32 Amount = 100);

	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void AddCredits(int32 Amount = 100);

	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void ForceLevelUp();

	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void ShowUpgradeChoices();

	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void GodMode();

	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void ClearMonsters();

	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void SpawnMonster(FName MonsterAssetName = TEXT("Monster_MinionRanged"), int32 Count = 1, float Distance = 800.0f);

};
