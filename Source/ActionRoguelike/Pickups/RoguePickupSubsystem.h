// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RoguePickupSubsystem.generated.h"

class UInstancedStaticMeshComponent;
class UAudioComponent;
class ARoguePlayerCharacter;

/**
 * 
 */
UCLASS()
class ACTIONROGUELIKE_API URoguePickupSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	
	void AddCoinsPickup(TArray<FVector> Locations, TArray<int32> CoinAmount);

	void AddExperiencePickup(TArray<FVector> Locations, TArray<int32> ExperienceAmount);

	UFUNCTION(BlueprintCallable, Category = "PickupSubsystem")
	void AddPickupAttractRadiusBonus(float RadiusBonus);

	UFUNCTION(BlueprintCallable, Category = "PickupSubsystem")
	float GetPickupAttractRadius() const;

	TArray<FPrimitiveInstanceId> AddCoinMeshInstances(const TArray<FTransform>& InAdded);

	TArray<FPrimitiveInstanceId> AddExperienceMeshInstances(const TArray<FTransform>& InAdded);

	void RemoveCoinMeshInstances(const TArray<FPrimitiveInstanceId>& IdsToRemove);

	void RemoveExperienceMeshInstances(const TArray<FPrimitiveInstanceId>& IdsToRemove);

protected:
	
	void RemoveCoinsPickup(int32 InIndex);

	void RemoveExperiencePickup(int32 InIndex);

	// -- These arrays are in sync
	TArray<FVector> CoinPickupLocations;
	TArray<int32> CoinPickupAmount;
	TArray<TWeakObjectPtr<ARoguePlayerCharacter>> CoinPickupAttractTargets;
	TArray<FPrimitiveInstanceId> MeshIDs;
	// -- end

	// -- These arrays are in sync
	TArray<FVector> ExperiencePickupLocations;
	TArray<int32> ExperiencePickupAmount;
	TArray<TWeakObjectPtr<ARoguePlayerCharacter>> ExperiencePickupAttractTargets;
	TArray<FPrimitiveInstanceId> ExperienceMeshIDs;
	// -- end

	FPrimitiveInstanceId AddCoinMeshInstance(FVector InLocation);

	void UpdatePickupAttraction(
		TArray<FVector>& PickupLocations,
		TArray<TWeakObjectPtr<ARoguePlayerCharacter>>& AttractTargets,
		const TArray<ARoguePlayerCharacter*>& PlayerPawns,
		UInstancedStaticMeshComponent* MeshComponent,
		const TArray<FPrimitiveInstanceId>& InstanceIDs,
		float DeltaTime,
		const FVector& MeshScale) const;

	void CreateCoinWorldISM();

	void CreateExperienceWorldISM();

	void PlayCoinPickupSound();

	void PlayExperiencePickupSound();

	UPROPERTY(EditDefaultsOnly, Category = "PickupSubsystem", meta = (ClampMin = "0.0"))
	float PickupAttractRadius = 600.0f;

	UPROPERTY(EditDefaultsOnly, Category = "PickupSubsystem", meta = (ClampMin = "0.0"))
	float PickupCollectRadius = 80.0f;

	UPROPERTY(EditDefaultsOnly, Category = "PickupSubsystem", meta = (ClampMin = "0.0"))
	float PickupAttractSpeed = 1600.0f;

	/* Single ISM that holds all coins, registered directly with the world instead of Actor wrapper */
	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> CoinWorldISM;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> ExperienceWorldISM;
	
	UPROPERTY()
	TObjectPtr<UAudioComponent> CoinPickupAudioComp;

	UPROPERTY()
	TObjectPtr<UAudioComponent> ExperiencePickupAudioComp;

	virtual void Tick(float DeltaTime) override;

	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(URoguePickupSubsystem, STATGROUP_Tickables);
	}

	virtual bool IsTickable() const override;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	void OnCoinSoundAssetLoadComplete(const FSoftObjectPath& SoftObjectPath, UObject* LoadedObject);

	void OnExperienceSoundAssetLoadComplete(const FSoftObjectPath& SoftObjectPath, UObject* LoadedObject);
};
