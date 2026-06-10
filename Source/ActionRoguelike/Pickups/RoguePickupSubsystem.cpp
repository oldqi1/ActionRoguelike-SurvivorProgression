// Fill out your copyright notice in the Description page of Project Settings.


#include "RoguePickupSubsystem.h"

#include "EngineUtils.h"
#include "SharedGameplayTags.h"
#include "ActionSystem/RogueActionComponent.h"
#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Core/RogueDeveloperSettings.h"
#include "Core/RogueGameState.h"
#include "Player/RoguePlayerCharacter.h"
#include "Player/RoguePlayerState.h"
#include "ActionRoguelike.h"



void URoguePickupSubsystem::AddCoinsPickup(TArray<FVector> Locations, TArray<int32> CoinAmount)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(RoguePickupSubsystem::AddCoinsPickup)
	
	ENetMode NetMode = GetWorld()->GetNetMode();
	// Clients only react to data received from host
	check(GetWorld()->GetNetMode() != NM_Client);

	CoinPickupLocations.Append(Locations);
	CoinPickupAmount.Append(CoinAmount);
	CoinPickupAttractTargets.AddDefaulted(Locations.Num());

	// Convert to transforms for ISM
	TArray<FTransform> Transforms;
	Transforms.Reserve(Locations.Num());
	for (int i = 0; i < Locations.Num(); ++i)
	{
		Transforms.Add(FTransform(Locations[i]));
	}
	
	TArray<FPrimitiveInstanceId> NewMeshIDs = AddCoinMeshInstances(Transforms);
	MeshIDs.Append(NewMeshIDs);
	
	// Are we playing a networked game
	if (NetMode > NM_Standalone)
	{
		ARogueGameState* GS = GetWorld()->GetGameState<ARogueGameState>();

		// Grab Locations & Mesh IDs for replication
		// Note: Unclear if we can Append() and mark the items dirty, instead we just add one by one
		for (int i = 0; i < NewMeshIDs.Num(); ++i)
		{
			FPickupLocationItem& NewItem = GS->CoinPickupData.Items.Add_GetRef(FPickupLocationItem(Locations[i], NewMeshIDs[i]));
			GS->CoinPickupData.MarkItemDirty(NewItem);
		}
	}
}

void URoguePickupSubsystem::AddExperiencePickup(TArray<FVector> Locations, TArray<int32> ExperienceAmount)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(RoguePickupSubsystem::AddExperiencePickup)

	ENetMode NetMode = GetWorld()->GetNetMode();
	check(GetWorld()->GetNetMode() != NM_Client);

	ExperiencePickupLocations.Append(Locations);
	ExperiencePickupAmount.Append(ExperienceAmount);
	ExperiencePickupAttractTargets.AddDefaulted(Locations.Num());

	TArray<FTransform> Transforms;
	Transforms.Reserve(Locations.Num());
	for (int i = 0; i < Locations.Num(); ++i)
	{
		Transforms.Add(FTransform(FRotator::ZeroRotator, Locations[i], FVector(0.35f)));
	}

	TArray<FPrimitiveInstanceId> NewMeshIDs = AddExperienceMeshInstances(Transforms);
	ExperienceMeshIDs.Append(NewMeshIDs);

	if (NetMode > NM_Standalone)
	{
		ARogueGameState* GS = GetWorld()->GetGameState<ARogueGameState>();

		for (int i = 0; i < NewMeshIDs.Num(); ++i)
		{
			FPickupLocationItem& NewItem = GS->ExperiencePickupData.Items.Add_GetRef(FPickupLocationItem(Locations[i], NewMeshIDs[i]));
			GS->ExperiencePickupData.MarkItemDirty(NewItem);
		}
	}
}


void URoguePickupSubsystem::AddPickupAttractRadiusBonus(float RadiusBonus)
{
	if (RadiusBonus <= 0.0f)
	{
		return;
	}

	PickupAttractRadius += RadiusBonus;
	UE_LOGFMT(LogGame, Log, "Pickup attract radius increased to {Radius}.", PickupAttractRadius);
}


float URoguePickupSubsystem::GetPickupAttractRadius() const
{
	return PickupAttractRadius;
}


void URoguePickupSubsystem::RemoveCoinsPickup(int32 InIndex)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(RoguePickupSubsystem::RemoveCoinsPickup)
	
	ENetMode NetMode = GetWorld()->GetNetMode();
	check(NetMode != NM_Client);
	
	CoinPickupLocations.RemoveAt(InIndex);
	CoinPickupAmount.RemoveAt(InIndex);
	CoinPickupAttractTargets.RemoveAt(InIndex);

	// Playing any networked game, clients should not reach here in the first place
	if (NetMode > NM_Standalone)
	{
		ARogueGameState* GS = GetWorld()->GetGameState<ARogueGameState>();

		// Find match based on local ID again
		FPrimitiveInstanceId IdToFind = MeshIDs[InIndex];
		GS->CoinPickupData.Items.Remove(FPickupLocationItem(FVector::ZeroVector, IdToFind));
		GS->CoinPickupData.MarkArrayDirty();
	}

	CoinWorldISM->RemoveInstanceById(MeshIDs[InIndex]);
	MeshIDs.RemoveAt(InIndex);
}

void URoguePickupSubsystem::RemoveExperiencePickup(int32 InIndex)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(RoguePickupSubsystem::RemoveExperiencePickup)

	ENetMode NetMode = GetWorld()->GetNetMode();
	check(NetMode != NM_Client);

	ExperiencePickupLocations.RemoveAt(InIndex);
	ExperiencePickupAmount.RemoveAt(InIndex);
	ExperiencePickupAttractTargets.RemoveAt(InIndex);

	if (NetMode > NM_Standalone)
	{
		ARogueGameState* GS = GetWorld()->GetGameState<ARogueGameState>();

		FPrimitiveInstanceId IdToFind = ExperienceMeshIDs[InIndex];
		GS->ExperiencePickupData.Items.Remove(FPickupLocationItem(FVector::ZeroVector, IdToFind));
		GS->ExperiencePickupData.MarkArrayDirty();
	}

	ExperienceWorldISM->RemoveInstanceById(ExperienceMeshIDs[InIndex]);
	ExperienceMeshIDs.RemoveAt(InIndex);
}

FPrimitiveInstanceId URoguePickupSubsystem::AddCoinMeshInstance(FVector InLocation)
{
	// Lazy init
	if (!IsValid(CoinWorldISM))
	{
		CreateCoinWorldISM();
	}
	
	return CoinWorldISM->AddInstanceById(FTransform(InLocation), true);
}

TArray<FPrimitiveInstanceId> URoguePickupSubsystem::AddCoinMeshInstances(const TArray<FTransform>& InAdded)
{
	// Lazy init
	if (!IsValid(CoinWorldISM))
	{
		CreateCoinWorldISM();
	}

	// Batch-add
	return CoinWorldISM->AddInstancesById(InAdded, true, false);
}

TArray<FPrimitiveInstanceId> URoguePickupSubsystem::AddExperienceMeshInstances(const TArray<FTransform>& InAdded)
{
	if (!IsValid(ExperienceWorldISM))
	{
		CreateExperienceWorldISM();
	}

	return ExperienceWorldISM->AddInstancesById(InAdded, true, false);
}

void URoguePickupSubsystem::RemoveCoinMeshInstances(const TArray<FPrimitiveInstanceId>& IdsToRemove)
{
	check(CoinWorldISM);
	CoinWorldISM->RemoveInstancesById(IdsToRemove, false);
}

void URoguePickupSubsystem::RemoveExperienceMeshInstances(const TArray<FPrimitiveInstanceId>& IdsToRemove)
{
	check(ExperienceWorldISM);
	ExperienceWorldISM->RemoveInstancesById(IdsToRemove, false);
}


void URoguePickupSubsystem::CreateCoinWorldISM()
{
	UWorld* World = GetWorld();

	// Temp sync loading of the mesh, can hitch
	UStaticMesh* Mesh = GetDefault<URogueDeveloperSettings>()->PickupCoinMesh.LoadSynchronous();
		
	CoinWorldISM = NewObject<UInstancedStaticMeshComponent>(World, NAME_None, RF_Transient);
	CoinWorldISM->SetStaticMesh(Mesh);
	CoinWorldISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CoinWorldISM->RegisterComponentWithWorld(World);
}


void URoguePickupSubsystem::CreateExperienceWorldISM()
{
	UWorld* World = GetWorld();

	UStaticMesh* Mesh = GetDefault<URogueDeveloperSettings>()->PickupExperienceMesh.LoadSynchronous();
	UMaterialInterface* Material = GetDefault<URogueDeveloperSettings>()->PickupExperienceMaterial.LoadSynchronous();

	ExperienceWorldISM = NewObject<UInstancedStaticMeshComponent>(World, NAME_None, RF_Transient);
	ExperienceWorldISM->SetStaticMesh(Mesh);
	if (Material)
	{
		ExperienceWorldISM->SetMaterial(0, Material);
	}
	ExperienceWorldISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ExperienceWorldISM->RegisterComponentWithWorld(World);
}


void URoguePickupSubsystem::PlayCoinPickupSound()
{
	if (!IsValid(CoinPickupAudioComp))
	{
		return;
	}

	if (!CoinPickupAudioComp->IsPlaying())
	{
		CoinPickupAudioComp->Play();
	}

	// by repeatedly triggering this event we play a sequence of higher pitched pickups
	// The metasound handles "resetting" the pitch of the pickup sequence automatically
	CoinPickupAudioComp->SetTriggerParameter("CoinPickedUp");
}

void URoguePickupSubsystem::PlayExperiencePickupSound()
{
	if (!IsValid(ExperiencePickupAudioComp))
	{
		return;
	}

	if (!ExperiencePickupAudioComp->IsPlaying())
	{
		ExperiencePickupAudioComp->Play();
	}
}


void URoguePickupSubsystem::UpdatePickupAttraction(
	TArray<FVector>& PickupLocations,
	TArray<TWeakObjectPtr<ARoguePlayerCharacter>>& AttractTargets,
	const TArray<ARoguePlayerCharacter*>& PlayerPawns,
	UInstancedStaticMeshComponent* MeshComponent,
	const TArray<FPrimitiveInstanceId>& InstanceIDs,
	float DeltaTime,
	const FVector& MeshScale) const
{
	if (!MeshComponent)
	{
		return;
	}

	const float AttractRadiusSqrd = PickupAttractRadius * PickupAttractRadius;

	for (int32 PickupIndex = 0; PickupIndex < PickupLocations.Num(); ++PickupIndex)
	{
		ARoguePlayerCharacter* TargetPawn = AttractTargets[PickupIndex].Get();
		if (!TargetPawn)
		{
			float BestDistanceSqrd = AttractRadiusSqrd;
			for (ARoguePlayerCharacter* PlayerPawn : PlayerPawns)
			{
				if (!PlayerPawn)
				{
					continue;
				}

				const float DistanceSqrd = FVector::DistSquared(PickupLocations[PickupIndex], PlayerPawn->GetActorLocation());
				if (DistanceSqrd <= BestDistanceSqrd)
				{
					BestDistanceSqrd = DistanceSqrd;
					TargetPawn = PlayerPawn;
				}
			}

			AttractTargets[PickupIndex] = TargetPawn;
		}

		if (!TargetPawn)
		{
			continue;
		}

		const FVector TargetLocation = TargetPawn->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);
		PickupLocations[PickupIndex] = FMath::VInterpConstantTo(
			PickupLocations[PickupIndex],
			TargetLocation,
			DeltaTime,
			PickupAttractSpeed);

		if (InstanceIDs.IsValidIndex(PickupIndex))
		{
			MeshComponent->UpdateInstanceTransformById(
				InstanceIDs[PickupIndex],
				FTransform(FRotator::ZeroRotator, PickupLocations[PickupIndex], MeshScale),
				true,
				false);
		}
	}
}

void URoguePickupSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* World = GetWorld();
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(RoguePickupSubsystem::Tick)

		// Performance Note: This processing is laid out to move around and touch as few things per iteration
		// Therefor we first process all the possible coin pickups and count the total Coins before we
		// award any of the players which may end up triggering a bunch of other delegates and pulling classes/data into memory

		TArray<FVector> Players;
		TArray<ARoguePlayerCharacter*> PlayerPawns;
		TArray<int32> TotalCoinsPerPlayer;
		TArray<int32> TotalExperiencePerPlayer;
		
		for (ARoguePlayerCharacter* PlayerPawn : TActorRange<ARoguePlayerCharacter>(World))
		{
			Players.Add(PlayerPawn->GetActorLocation());
			PlayerPawns.Add(PlayerPawn);
		}

		UpdatePickupAttraction(CoinPickupLocations, CoinPickupAttractTargets, PlayerPawns, CoinWorldISM, MeshIDs, DeltaTime, FVector::OneVector);
		UpdatePickupAttraction(ExperiencePickupLocations, ExperiencePickupAttractTargets, PlayerPawns, ExperienceWorldISM, ExperienceMeshIDs, DeltaTime, FVector(0.35f));

		const float CollectRadiusSqrd = PickupCollectRadius * PickupCollectRadius;

		// Find pickups and track Coins to grant
		for (FVector& PlayerLocation : Players)
		{
			// Track all pickups that need to be picked up.
			TArray<int32> ProcessList;

			for (int Index = 0; Index < CoinPickupLocations.Num(); ++Index)
			{
				float DistSqrd = FVector::DistSquared(CoinPickupLocations[Index], PlayerLocation);
				if (DistSqrd < CollectRadiusSqrd)
				{
					// Bookkeep all pickups that need processing for later
					ProcessList.Add(Index);
				}
			}
			
			int32 TotalCoins = 0;
			for (int i = ProcessList.Num() - 1; i >= 0; --i)
			{
				TotalCoins += CoinPickupAmount[ProcessList[i]];
				
				RemoveCoinsPickup(ProcessList[i]);
			}
			
			TotalCoinsPerPlayer.Add(TotalCoins);

			TArray<int32> ExperienceProcessList;
			for (int Index = 0; Index < ExperiencePickupLocations.Num(); ++Index)
			{
				float DistSqrd = FVector::DistSquared(ExperiencePickupLocations[Index], PlayerLocation);
				if (DistSqrd < CollectRadiusSqrd)
				{
					ExperienceProcessList.Add(Index);
				}
			}

			int32 TotalExperience = 0;
			for (int i = ExperienceProcessList.Num() - 1; i >= 0; --i)
			{
				TotalExperience += ExperiencePickupAmount[ExperienceProcessList[i]];

				RemoveExperiencePickup(ExperienceProcessList[i]);
			}

			TotalExperiencePerPlayer.Add(TotalExperience);
		}

		// Award each player
		for (int i = 0; i < PlayerPawns.Num(); ++i)
		{
			int32 AwardAmount = TotalCoinsPerPlayer[i];
			if (AwardAmount == 0)
			{
				continue;
			}
			
			FAttributeModification Mod = FAttributeModification(SharedGameplayTags::Attribute_Credits, AwardAmount);

			PlayerPawns[i]->GetActionComponent()->ApplyAttributeChange(Mod);

			// @todo: play sound properly for networked players...eg. they receive these Coins w/ a pickup contextTag
			PlayCoinPickupSound();
		}

		for (int i = 0; i < PlayerPawns.Num(); ++i)
		{
			int32 AwardAmount = TotalExperiencePerPlayer[i];
			if (AwardAmount == 0)
			{
				continue;
			}

			if (ARoguePlayerState* PS = PlayerPawns[i]->GetPlayerState<ARoguePlayerState>())
			{
				PS->AddExperience(AwardAmount);
				PlayExperiencePickupSound();
			}
		}
	}

	// Debug Rendering
	//for (int Index = 0; Index < CoinPickupLocations.Num(); ++Index)
	{
		//DrawDebugBox(World, CoinPickupLocations[Index], FVector(5.0f), FColor::Blue);
	}
}

bool URoguePickupSubsystem::IsTickable() const
{
	// Run everywhere except clients. Only standalone/host will check for "overlaps" during tick
	return GetWorld()->GetNetMode() < NM_Client;
}

void URoguePickupSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UWorld* World = GetWorld();

	CoinPickupAudioComp = NewObject<UAudioComponent>(World, NAME_None, RF_Transient);
	CoinPickupAudioComp->bAutoActivate = false;
	CoinPickupAudioComp->RegisterComponentWithWorld(World);

	FLoadSoftObjectPathAsyncDelegate Delegate;
	Delegate.BindUObject(this, &ThisClass::OnCoinSoundAssetLoadComplete);
	GetDefault<URogueDeveloperSettings>()->PickupCoinSound.LoadAsync(Delegate);

	ExperiencePickupAudioComp = NewObject<UAudioComponent>(World, NAME_None, RF_Transient);
	ExperiencePickupAudioComp->bAutoActivate = false;
	ExperiencePickupAudioComp->RegisterComponentWithWorld(World);

	FLoadSoftObjectPathAsyncDelegate ExperienceDelegate;
	ExperienceDelegate.BindUObject(this, &ThisClass::OnExperienceSoundAssetLoadComplete);
	GetDefault<URogueDeveloperSettings>()->PickupExperienceSound.LoadAsync(ExperienceDelegate);
}

void URoguePickupSubsystem::Deinitialize()
{
	if (IsValid(CoinPickupAudioComp))
	{
		CoinPickupAudioComp->Stop();
		CoinPickupAudioComp->DestroyComponent();
		CoinPickupAudioComp = nullptr;
	}

	if (IsValid(ExperiencePickupAudioComp))
	{
		ExperiencePickupAudioComp->Stop();
		ExperiencePickupAudioComp->DestroyComponent();
		ExperiencePickupAudioComp = nullptr;
	}

	if (IsValid(CoinWorldISM))
	{
		CoinWorldISM->DestroyComponent();
		CoinWorldISM = nullptr;
	}

	if (IsValid(ExperienceWorldISM))
	{
		ExperienceWorldISM->DestroyComponent();
		ExperienceWorldISM = nullptr;
	}

	CoinPickupLocations.Reset();
	CoinPickupAmount.Reset();
	CoinPickupAttractTargets.Reset();
	MeshIDs.Reset();
	ExperiencePickupLocations.Reset();
	ExperiencePickupAmount.Reset();
	ExperiencePickupAttractTargets.Reset();
	ExperienceMeshIDs.Reset();

	Super::Deinitialize();
}

void URoguePickupSubsystem::OnCoinSoundAssetLoadComplete(const FSoftObjectPath& SoftObjectPath, UObject* LoadedObject)
{
	if (!IsValid(CoinPickupAudioComp))
	{
		return;
	}

	CoinPickupAudioComp->SetSound(Cast<USoundBase>(LoadedObject));
}

void URoguePickupSubsystem::OnExperienceSoundAssetLoadComplete(const FSoftObjectPath& SoftObjectPath, UObject* LoadedObject)
{
	if (!IsValid(ExperiencePickupAudioComp))
	{
		return;
	}

	ExperiencePickupAudioComp->SetSound(Cast<USoundBase>(LoadedObject));
}
