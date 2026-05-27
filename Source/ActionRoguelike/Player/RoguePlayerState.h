// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "RoguePlayerState.generated.h"

class ARoguePlayerState; // Forward declared to satisfy the delegate macros below
class URogueSaveGame;

// Event Handler for Credits
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCreditsChanged, ARoguePlayerState*, PlayerState, int32, NewCredits, int32, Delta);
// Event Handler for Personal Record Time
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnRecordTimeChanged, ARoguePlayerState*, PlayerState, float, NewTime, float, OldRecord);
// Event Handler for Experience
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnExperienceChanged, ARoguePlayerState*, PlayerState, int32, NewExperience, int32, ExperienceToNextLevel, int32, Delta);
// Event Handler for Level Ups
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLevelChanged, ARoguePlayerState*, PlayerState, int32, NewLevel, int32, OldLevel);

/**
 * 
 */
UCLASS()
class ACTIONROGUELIKE_API ARoguePlayerState : public APlayerState
{
	GENERATED_BODY()
	
protected:

	UPROPERTY(Transient, EditDefaultsOnly, ReplicatedUsing="OnRep_Credits", Category = "Credits")
	int32 Credits;

	UPROPERTY(Transient, BlueprintReadOnly)
	float PersonalRecordTime;

	UPROPERTY(Transient, EditDefaultsOnly, ReplicatedUsing="OnRep_Level", Category = "Progression")
	int32 Level = 1;

	UPROPERTY(Transient, EditDefaultsOnly, ReplicatedUsing="OnRep_Experience", Category = "Progression")
	int32 Experience = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Progression")
	int32 BaseExperienceToNextLevel = 100;

	UPROPERTY(EditDefaultsOnly, Category = "Progression")
	float ExperienceGrowthRate = 1.25f;

	// OnRep_ can use a parameter containing the 'old value' of the variable it is bound to. Very useful in this case to figure out the 'delta'.
	UFUNCTION()
	void OnRep_Credits(int32 OldCredits);

	UFUNCTION()
	void OnRep_Level(int32 OldLevel);

	UFUNCTION()
	void OnRep_Experience(int32 OldExperience);

	int32 CalculateExperienceToNextLevel(int32 InLevel) const;

	void TryLevelUp();

public:

	/* Checks current record and only sets if better time was passed in. */
	UFUNCTION(BlueprintCallable)
	bool UpdatePersonalRecord(float NewTime);

	UFUNCTION(BlueprintCallable, Category = "Credits")
	int32 GetCredits() const;

	UFUNCTION(BlueprintCallable, Category = "Credits")
	void AddCredits(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "Credits")
	bool TryRemoveCredits(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "Progression")
	void AddExperience(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "Progression")
	int32 GetPlayerLevel() const;

	UFUNCTION(BlueprintCallable, Category = "Progression")
	int32 GetExperience() const;

	UFUNCTION(BlueprintCallable, Category = "Progression")
	int32 GetExperienceToNextLevel() const;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCreditsChanged OnCreditsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnRecordTimeChanged OnRecordTimeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnExperienceChanged OnExperienceChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnLevelChanged OnLevelChanged;

	UFUNCTION(BlueprintNativeEvent)
	void SavePlayerState(URogueSaveGame* SaveObject);

	UFUNCTION(BlueprintNativeEvent)
	void LoadPlayerState(URogueSaveGame* SaveObject);

};
