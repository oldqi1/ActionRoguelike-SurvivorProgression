// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GameplayTagContainer.h"
#include "RoguePlayerState.generated.h"

class ARoguePlayerState; // Forward declared to satisfy the delegate macros below
class URogueActionComponent;
class URogueSaveGame;
class URogueUpgradeDataAsset;

// Event Handler for Credits
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCreditsChanged, ARoguePlayerState*, PlayerState, int32, NewCredits, int32, Delta);
// Event Handler for Personal Record Time
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnRecordTimeChanged, ARoguePlayerState*, PlayerState, float, NewTime, float, OldRecord);
// Event Handler for Experience
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnExperienceChanged, ARoguePlayerState*, PlayerState, int32, NewExperience, int32, ExperienceToNextLevel, int32, Delta);
// Event Handler for Level Ups
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLevelChanged, ARoguePlayerState*, PlayerState, int32, NewLevel, int32, OldLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUpgradeChoicesGenerated, ARoguePlayerState*, PlayerState, int32, NewLevel);

UENUM(BlueprintType)
enum class ERogueUpgradeRarity : uint8
{
	Common,
	Rare,
	Prismatic
};

UENUM(BlueprintType)
enum class ERogueUpgradeEffectType : uint8
{
	AddAttribute,
	ModifyPickupRadius,
	GrantKillExplosion,
	GrantLastStandShield,
	GrantChainLightning
};

USTRUCT(BlueprintType)
struct FRogueUpgradeChoice
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FName UpgradeId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	ERogueUpgradeRarity Rarity = ERogueUpgradeRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	ERogueUpgradeEffectType EffectType = ERogueUpgradeEffectType::AddAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FGameplayTag AttributeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	float Magnitude = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	int32 MaxStacks = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	bool bUnique = false;
};

USTRUCT(BlueprintType)
struct FRogueUpgradeStack
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName UpgradeId;

	UPROPERTY(BlueprintReadOnly)
	int32 StackCount = 0;
};

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

	UPROPERTY(EditDefaultsOnly, Category = "Progression|Rewards")
	float AttackDamagePerLevel = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Progression|Upgrades")
	bool bAutoSelectUpgradeChoices = false;

	UPROPERTY(EditDefaultsOnly, Category = "Progression|Upgrades")
	TObjectPtr<URogueUpgradeDataAsset> UpgradeData;

	UPROPERTY(EditDefaultsOnly, Category = "Progression|Upgrades", meta = (ClampMin = "0.0"))
	float KillExplosionRadius = 450.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Progression|Upgrades", meta = (ClampMin = "0.0"))
	float KillExplosionRadiusPerStack = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Progression|Upgrades", meta = (ClampMin = "0.0"))
	float KillExplosionDamageCoefficient = 75.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Progression|Upgrades", meta = (ClampMin = "0.0"))
	float KillExplosionDamageCoefficientPerStack = 25.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Progression|Upgrades", meta = (ClampMin = "0.0"))
	float LastStandShieldHealAmount = 35.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Progression|Upgrades", meta = (ClampMin = "0.0"))
	float LastStandShieldHealPerStack = 15.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Progression|Upgrades", meta = (ClampMin = "0.0"))
	float LastStandShieldCooldown = 45.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Progression|Upgrades", meta = (ClampMin = "0.0"))
	float ChainLightningRadius = 700.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Progression|Upgrades", meta = (ClampMin = "0.0"))
	float ChainLightningDamageCoefficient = 35.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Progression|Upgrades", meta = (ClampMin = "0.0"))
	float ChainLightningDamageCoefficientPerStack = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Progression|Upgrades", meta = (ClampMin = "1"))
	int32 ChainLightningTargetCount = 2;

	UPROPERTY(EditDefaultsOnly, Category = "Progression|Upgrades", meta = (ClampMin = "0"))
	int32 ChainLightningTargetCountPerStack = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Progression|Upgrades", meta = (ClampMin = "0.0"))
	float ChainLightningCooldown = 0.65f;

	UPROPERTY(EditDefaultsOnly, Category = "Progression|Upgrades", meta = (ClampMin = "0.0"))
	float ChainLightningSourceLockDuration = 1.25f;

	float LastStandShieldNextReadyTime = 0.0f;

	float ChainLightningNextReadyTime = 0.0f;

	float ChainLightningSourceLockUntil = 0.0f;

	TWeakObjectPtr<AActor> ChainLightningLockedSource;

	UPROPERTY(Transient, ReplicatedUsing = "OnRep_PendingUpgradeChoices")
	TArray<FRogueUpgradeChoice> PendingUpgradeChoices;

	UPROPERTY(Transient, Replicated)
	TArray<FRogueUpgradeStack> UpgradeStacks;

	// OnRep_ can use a parameter containing the 'old value' of the variable it is bound to. Very useful in this case to figure out the 'delta'.
	UFUNCTION()
	void OnRep_Credits(int32 OldCredits);

	UFUNCTION()
	void OnRep_Level(int32 OldLevel);

	UFUNCTION()
	void OnRep_Experience(int32 OldExperience);

	UFUNCTION()
	void OnRep_PendingUpgradeChoices();

	int32 CalculateExperienceToNextLevel(int32 InLevel) const;

	void TryLevelUp();

	void GenerateUpgradeChoices(int32 NewLevel);

	void BuildUpgradePool(TArray<FRogueUpgradeChoice>& OutPool) const;

	void BuildDefaultUpgradePool(TArray<FRogueUpgradeChoice>& OutPool) const;

	bool CanOfferUpgrade(const FRogueUpgradeChoice& Choice) const;

	bool ApplyUpgradeChoice(const FRogueUpgradeChoice& Choice);

	int32& FindOrAddUpgradeStack(FName UpgradeId);

	UFUNCTION(Server, Reliable)
	void ServerSelectUpgradeChoice(int32 ChoiceIndex);

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

	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrades")
	bool SelectUpgradeChoice(int32 ChoiceIndex);

	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrades")
	bool GrantUpgradeById(FName UpgradeId);

	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrades")
	void DebugGenerateUpgradeChoices();

	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrades")
	TArray<FRogueUpgradeChoice> GetPendingUpgradeChoices() const;

	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrades")
	FText GetUpgradePreviewText(const FRogueUpgradeChoice& Choice) const;

	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrades")
	bool HasUpgrade(FName UpgradeId) const;

	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrades")
	int32 GetUpgradeStackCount(FName UpgradeId) const;

	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrades")
	bool HasKillExplosionUpgrade() const;

	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrades")
	float GetKillExplosionRadius() const;

	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrades")
	float GetKillExplosionDamageCoefficient() const;

	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrades")
	bool HasLastStandShieldUpgrade() const;

	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrades")
	bool TryActivateLastStandShield(URogueActionComponent* TargetActionComp, AActor* HealInstigator);

	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrades")
	bool HasChainLightningUpgrade() const;

	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrades")
	float GetChainLightningRadius() const;

	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrades")
	float GetChainLightningDamageCoefficient() const;

	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrades")
	int32 GetChainLightningTargetCount() const;

	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrades")
	float GetChainLightningCooldown() const;

	bool IsChainLightningReady() const;

	bool CanTriggerChainLightningFrom(AActor* SourceActor) const;

	void CommitChainLightningCooldown(AActor* SourceActor);

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCreditsChanged OnCreditsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnRecordTimeChanged OnRecordTimeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnExperienceChanged OnExperienceChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnLevelChanged OnLevelChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnUpgradeChoicesGenerated OnUpgradeChoicesGenerated;

	UFUNCTION(BlueprintNativeEvent)
	void SavePlayerState(URogueSaveGame* SaveObject);

	UFUNCTION(BlueprintNativeEvent)
	void LoadPlayerState(URogueSaveGame* SaveObject);

};
