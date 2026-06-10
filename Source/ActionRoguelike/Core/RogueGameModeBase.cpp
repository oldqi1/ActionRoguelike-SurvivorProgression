// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueGameModeBase.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "AI/RogueAICharacter.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Player/RoguePlayerCharacter.h"
#include "Player/RoguePlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "ActionSystem/RogueAction.h"
#include "RogueMonsterData.h"
#include "ActionRoguelike.h"
#include "RogueDeferredTaskSystem.h"
#include "RogueGameplayFunctionLibrary.h"
#include "RogueGameState.h"
#include "SharedGameplayTags.h"
#include "ActionSystem/RogueActionComponent.h"
#include "SaveSystem/RogueSaveGameSubsystem.h"
#include "Development/RogueDeveloperLocalSettings.h"
#include "Engine/AssetManager.h"
#include "Performance/RogueActorPoolingSubsystem.h"
#include "UI/RogueHUD.h"
#include "Windows/WindowsPlatformPerfCounters.h"
#include "Engine/OverlapResult.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "World/RogueRadiusIndicatorActor.h"
#include "Core/RogueGameplayInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueGameModeBase)



ARogueGameModeBase::ARogueGameModeBase()
{
	PlayerStateClass = ARoguePlayerState::StaticClass();
	HUDClass = ARogueHUD::StaticClass();
	GameStateClass = ARogueGameState::StaticClass();

	static ConstructorHelpers::FObjectFinder<USoundBase> KillExplosionSoundFinder(TEXT("/Game/SanderAudio/Sources/Environment/MSS_Environmental_BarrelExplode.MSS_Environmental_BarrelExplode"));
	if (KillExplosionSoundFinder.Succeeded())
	{
		KillExplosionSound = KillExplosionSoundFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ChainLightningImpactFinder(TEXT("/Game/NiagaraExamples/FX_Weapons/Impacts/NS_Impact_Metal.NS_Impact_Metal"));
	if (ChainLightningImpactFinder.Succeeded())
	{
		ChainLightningImpactVFX = ChainLightningImpactFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> ChainLightningSoundFinder(TEXT("/Game/SanderAudio/Sources/DefaultEnemy/Combat/S_Combat_Enemy_Proj_Sizzle-001.S_Combat_Enemy_Proj_Sizzle-001"));
	if (ChainLightningSoundFinder.Succeeded())
	{
		ChainLightningSound = ChainLightningSoundFinder.Object;
	}

	// Leave KillExplosionVFX unassigned by default. The Niagara Examples explosion systems
	// include a post-process/camera-shake emitter that can trigger editor typed-element ensures.
}


void ARogueGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	bForceContinuousBotSpawning = MapName.Contains(TEXT("Survivor"), ESearchCase::IgnoreCase)
		|| UGameplayStatics::HasOption(Options, TEXT("ContinuousSpawning"));
	if (bForceContinuousBotSpawning)
	{
		ApplyContinuousSpawningDefaults();
	}

	// (Save/Load logic moved into new SaveGameSubsystem)
	URogueSaveGameSubsystem* SG = GetGameInstance()->GetSubsystem<URogueSaveGameSubsystem>();

	// Optional slot name (Falls back to slot specified in SaveGameSettings class/INI otherwise)
	FString SelectedSaveSlot = UGameplayStatics::ParseOption(Options, "SaveGame");
	SG->LoadSaveGame(SelectedSaveSlot);
}


void ARogueGameModeBase::StartPlay()
{
	Super::StartPlay();

	AvailableSpawnCredit = InitialSpawnCredit;

	StartSpawningBots();
	
	// Make sure we have assigned at least one power-up class
	if (ensure(PowerupClasses.Num() > 0))
	{
		// Skip the Blueprint wrapper and use the direct C++ option which the Wrapper uses as well
		FEnvQueryRequest Request(PowerupSpawnQuery, this);
		Request.Execute(EEnvQueryRunMode::AllMatching, this, &ARogueGameModeBase::OnPowerupSpawnQueryCompleted);
	}
	
	// We run the prime logic after the BeginPlay call to avoid accidentally running that on stored/primed actors
	RequestPrimedActors();
}


void ARogueGameModeBase::RequestPrimedActors()
{
	URogueActorPoolingSubsystem* PoolingSystem = GetWorld()->GetSubsystem<URogueActorPoolingSubsystem>();
	if (PoolingSystem->IsPoolingEnabled(this))
	{
		for (auto& Entry : ActorPoolClasses)
		{
			PoolingSystem->PrimeActorPool(Entry.Key, Entry.Value);
		}
	}
}


void ARogueGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// Calling Before Super:: so we set variables before 'beginplayingstate' is called in PlayerController (which is where we instantiate UI)
	URogueSaveGameSubsystem* SG = GetGameInstance()->GetSubsystem<URogueSaveGameSubsystem>();
	SG->HandleStartingNewPlayer(NewPlayer);

	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	// Now we're ready to override spawn location
	// Alternatively we could override core spawn location to use store locations immediately (skipping the whole 'find player start' logic)
	SG->OverrideSpawnTransform(NewPlayer);
}


void ARogueGameModeBase::StartSpawningBots()
{
	// Continuous timer to spawn in more bots.
	// Actual amount of bots and whether it's allowed to spawn determined by spawn logic later in the chain...
	GetWorldTimerManager().SetTimer(TimerHandle_SpawnBots, this, &ARogueGameModeBase::SpawnBotTimerElapsed, SpawnTimerInterval, true);
}


void ARogueGameModeBase::SpawnBotTimerElapsed()
{
#if !UE_BUILD_SHIPPING
    // disabled as we now use big button in level for debugging, but in normal gameplay something like this is useful
    // does require some code update on how it handles this as 'override' currently not properly set up.
	if (!bForceContinuousBotSpawning && GetDefault<URogueDeveloperLocalSettings>()->bDisableSpawnBotsOverride)
	{
		return;
	}
#endif

	// Give points to spend
	float SpawnCreditsToAdd = FallbackSpawnCreditsPerTick;
	if (SpawnCreditCurve)
	{
		SpawnCreditsToAdd = FMath::Max(SpawnCreditsToAdd, SpawnCreditCurve->GetFloatValue(GetWorld()->TimeSeconds));
	}
	AvailableSpawnCredit += SpawnCreditsToAdd * GetSpawnCreditMultiplier();

	if (CooldownBotSpawnUntil > GetWorld()->TimeSeconds)
	{
		// Still cooling down
		return;
	}
	
	// Count alive bots before spawning
	int32 NrOfAliveBots = 0;
	for (ARogueAICharacter* Bot : TActorRange<ARogueAICharacter>(GetWorld()))
	{
		if (URogueGameplayFunctionLibrary::IsAlive(Bot))
		{
			NrOfAliveBots++;
		}
	}

	UE_LOGFMT(LogGame, Verbose, "Found {number} alive bots.", NrOfAliveBots);

	const int32 MaxBotCount = GetCurrentMaxBotCount();
	if (NrOfAliveBots >= MaxBotCount)
	{
		UE_LOGFMT(LogGame, Verbose, "At maximum bot capacity {AliveBots}/{MaxBots}. Skipping bot spawn.", NrOfAliveBots, MaxBotCount);
		return;
	}

	// Row to pass along with EQS delegate
	FMonsterInfoRow* SelectedRow = nullptr;

	if (!MonsterTable)
	{
		LogSpawnFailureThrottled(TEXT("MonsterTable is not configured."));
		return;
	}
	
	TArray<FMonsterInfoRow*> Rows;
	MonsterTable->GetAllRows("", Rows);
	if (Rows.Num() == 0)
	{
		LogSpawnFailureThrottled(TEXT("MonsterTable has no rows."));
		return;
	}

	// Get total weight
	float TotalWeight = 0;
	for (FMonsterInfoRow* Entry : Rows)
	{
		TotalWeight += Entry->Weight;
	}
	if (TotalWeight <= 0.0f)
	{
		LogSpawnFailureThrottled(TEXT("MonsterTable has no positive spawn weights."));
		return;
	}

	// Random number within total random
	const float RandomWeight = FMath::FRandRange(0.0f, TotalWeight);

	//Reset
	TotalWeight = 0;

	// Get monster based on random weight
	for (FMonsterInfoRow* Entry : Rows)
	{
		TotalWeight += Entry->Weight;

		if (RandomWeight <= TotalWeight)
		{
			SelectedRow = Entry;
			break;
		}
	}

	if (!SelectedRow)
	{
		LogSpawnFailureThrottled(TEXT("Failed to select a monster row."));
		return;
	}

	if (SelectedRow->SpawnCost > AvailableSpawnCredit)
	{
		// Too expensive to spawn, try again soon
		CooldownBotSpawnUntil = GetWorld()->TimeSeconds + CooldownTimeBetweenFailures;
		return;
	}

	// Skip the Blueprint wrapper and use the direct C++ option which the Wrapper uses as well
	if (!SpawnBotQuery)
	{
		LogSpawnFailureThrottled(TEXT("SpawnBotQuery is not configured."));
		return;
	}

	FEnvQueryRequest Request(SpawnBotQuery, this);

	FQueryFinishedSignature FinishedDelegate = FQueryFinishedSignature::CreateUObject(this, &ARogueGameModeBase::OnBotSpawnQueryCompleted, SelectedRow);
	
	Request.Execute(EEnvQueryRunMode::RandomBest5Pct, FinishedDelegate);
}


void ARogueGameModeBase::ApplyContinuousSpawningDefaults()
{
	InitialSpawnCredit = FMath::Max(InitialSpawnCredit, 100);
	FallbackSpawnCreditsPerTick = FMath::Max(FallbackSpawnCreditsPerTick, 10.0f);
	SpawnTimerInterval = FMath::Min(SpawnTimerInterval, 1.5f);
	CooldownTimeBetweenFailures = FMath::Min(CooldownTimeBetweenFailures, 2.0f);
	BaseMaxBotCount = FMath::Max(BaseMaxBotCount, 16);
	MaxBotCountPerDifficulty = FMath::Max(MaxBotCountPerDifficulty, 4);
	SpawnCreditDifficultyScale = FMath::Max(SpawnCreditDifficultyScale, 0.35f);
}


void ARogueGameModeBase::LogSpawnFailureThrottled(const TCHAR* Reason, bool bWarning)
{
	const float CurrentTime = GetWorld() ? GetWorld()->TimeSeconds : 0.0f;
	if (CurrentTime < NextSpawnFailureLogTime)
	{
		return;
	}

	NextSpawnFailureLogTime = CurrentTime + SpawnFailureLogInterval;
	if (bWarning)
	{
		UE_LOGFMT(LogGame, Warning, "Skipping bot spawn: {Reason}", Reason);
	}
	else
	{
		UE_LOGFMT(LogGame, Verbose, "Skipping bot spawn: {Reason}", Reason);
	}
}


void ARogueGameModeBase::OnBotSpawnQueryCompleted(TSharedPtr<FEnvQueryResult> Result, FMonsterInfoRow* SelectedRow)
{
	FEnvQueryResult* QueryResult = Result.Get();
	if (!QueryResult || !QueryResult->IsSuccessful())
	{
		FVector FallbackLocation;
		if (TryFindFallbackSpawnLocation(FallbackLocation))
		{
			RequestMonsterSpawnAtLocation(SelectedRow, FallbackLocation);
			return;
		}

		LogSpawnFailureThrottled(TEXT("Spawn bot EQS query failed and fallback spawn could not find a nav location."), false);
		return;
	}

	// Retrieve all possible locations that passed the query
	TArray<FVector> Locations;
	QueryResult->GetAllAsLocations(Locations);

	if (Locations.IsValidIndex(0) && MonsterTable && SelectedRow)
	{
		RequestMonsterSpawnAtLocation(SelectedRow, Locations[0]);
	}
	else
	{
		FVector FallbackLocation;
		if (TryFindFallbackSpawnLocation(FallbackLocation))
		{
			RequestMonsterSpawnAtLocation(SelectedRow, FallbackLocation);
			return;
		}

		LogSpawnFailureThrottled(TEXT("Spawn bot EQS query returned no valid locations and fallback spawn could not find a nav location."), false);
	}
}


bool ARogueGameModeBase::TryFindFallbackSpawnLocation(FVector& OutLocation) const
{
	UWorld* World = GetWorld();
	UNavigationSystemV1* NavigationSystem = World ? UNavigationSystemV1::GetCurrent(World) : nullptr;
	if (!World || !NavigationSystem)
	{
		return false;
	}

	TArray<ARoguePlayerCharacter*> PlayerPawns;
	for (ARoguePlayerCharacter* PlayerPawn : TActorRange<ARoguePlayerCharacter>(World))
	{
		if (URogueGameplayFunctionLibrary::IsAlive(PlayerPawn))
		{
			PlayerPawns.Add(PlayerPawn);
		}
	}

	if (PlayerPawns.Num() == 0)
	{
		return false;
	}

	const float MinDistance = FMath::Max(0.0f, FallbackSpawnMinDistance);
	const float MaxDistance = FMath::Max(MinDistance, FallbackSpawnMaxDistance);
	const int32 Attempts = FMath::Max(1, FallbackSpawnAttempts);

	for (int32 AttemptIndex = 0; AttemptIndex < Attempts; ++AttemptIndex)
	{
		const ARoguePlayerCharacter* AnchorPlayer = PlayerPawns[FMath::RandRange(0, PlayerPawns.Num() - 1)];
		const float AngleRadians = FMath::FRandRange(0.0f, UE_TWO_PI);
		const float Distance = FMath::FRandRange(MinDistance, MaxDistance);
		const FVector Offset(FMath::Cos(AngleRadians) * Distance, FMath::Sin(AngleRadians) * Distance, 0.0f);
		const FVector CandidateLocation = AnchorPlayer->GetActorLocation() + Offset;

		FNavLocation NavLocation;
		if (NavigationSystem->ProjectPointToNavigation(CandidateLocation, NavLocation, FVector(500.0f, 500.0f, 1200.0f)))
		{
			OutLocation = NavLocation.Location;
			return true;
		}
	}

	return false;
}


void ARogueGameModeBase::RequestMonsterSpawnAtLocation(FMonsterInfoRow* SelectedRow, const FVector& SpawnLocation)
{
	if (!SelectedRow)
	{
		LogSpawnFailureThrottled(TEXT("Cannot request monster spawn without a selected monster row."));
		return;
	}

	UAssetManager& Manager = UAssetManager::Get();
	const FPrimaryAssetId MonsterId = SelectedRow->MonsterId;

	TArray<FName> Bundles;
	FStreamableDelegate Delegate = FStreamableDelegate::CreateUObject(this, &ARogueGameModeBase::OnMonsterLoaded, MonsterId, SpawnLocation, SelectedRow->SpawnCost);
	Manager.LoadPrimaryAsset(MonsterId, Bundles, Delegate);
}


void ARogueGameModeBase::OnMonsterLoaded(FPrimaryAssetId LoadedId, FVector SpawnLocation, float SpawnCost)
{
	UAssetManager& Manager = UAssetManager::Get();

	URogueMonsterData* MonsterData = Cast<URogueMonsterData>(Manager.GetPrimaryAssetObject(LoadedId));
	if (!MonsterData || !MonsterData->MonsterClass)
	{
		UE_LOGFMT(LogGame, Warning, "Could not spawn monster: invalid monster data {MonsterId}.", LoadedId.ToString());
		return;
	}
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* NewBot = GetWorld()->SpawnActor<AActor>(MonsterData->MonsterClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (NewBot)
	{
		AvailableSpawnCredit -= SpawnCost;

		if (ARogueAICharacter* NewAI = Cast<ARogueAICharacter>(NewBot))
		{
			NewAI->SetExperienceDropAmount(ExperiencePerKill);
		}

		// Grant special actions, buffs etc.
		URogueActionComponent* ActionComp = URogueGameplayFunctionLibrary::GetActionComponentFromActor(NewBot);
		if (!ActionComp)
		{
			UE_LOGFMT(LogGame, Warning, "Could not grant monster actions: {Actor} has no ActionComponent.", GetNameSafe(NewBot));
			return;
		}
		
		for (TSubclassOf<URogueAction> ActionClass : MonsterData->Actions)
		{
			if (ActionClass)
			{
				ActionComp->AddAction(NewBot, ActionClass);
			}
		}
	}
	else
	{
		LogSpawnFailureThrottled(TEXT("SpawnActor failed even after collision adjustment."));
	}
}


void ARogueGameModeBase::OnPowerupSpawnQueryCompleted(TSharedPtr<FEnvQueryResult> Result)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(OnPowerupSpawnQueryCompleted);
	
	FEnvQueryResult* QueryResult = Result.Get();
	if (!QueryResult->IsSuccessful())
	{
		UE_LOGFMT(LogGame, Warning, "Spawn bot EQS Query Failed!");
		return;
	}
	

	uint64 CyclesStart = FPlatformTime::Cycles64();

	// Retrieve all possible locations that passed the query
	TArray<FVector> Locations;
	QueryResult->GetAllAsLocations(Locations);

	// Keep used locations to easily check distance between points
	TArray<FVector> UsedLocations;

	int32 SpawnCounter = 0;
	// Break out if we reached the desired count or if we have no more potential positions remaining
	while (SpawnCounter < DesiredPowerupCount && Locations.Num() > 0)
	{
		// Pick a random location from remaining points.
		int32 RandomLocationIndex = FMath::RandRange(0, Locations.Num() - 1);

		FVector PickedLocation = Locations[RandomLocationIndex];
		// Remove to avoid picking again
		Locations.RemoveAtSwap(RandomLocationIndex);

		// Check minimum distance requirement
		bool bValidLocation = true;
		for (FVector OtherLocation : UsedLocations)
		{
			float DistanceTo = (PickedLocation - OtherLocation).Size();

			if (DistanceTo < RequiredPowerupDistance)
			{
				// Show skipped locations due to distance
				//DrawDebugSphere(GetWorld(), PickedLocation, 50.0f, 20, FColor::Red, false, 10.0f);

				// too close, skip to next attempt
				bValidLocation = false;
				break;
			}
		}

		// Failed the distance test
		if (!bValidLocation)
		{
			continue;
		}

		// Pick a random powerup-class
		int32 RandomClassIndex = FMath::RandRange(0, PowerupClasses.Num() - 1);
		TSubclassOf<AActor> RandomPowerupClass = PowerupClasses[RandomClassIndex];

#if USE_DEFERRED_TASKS
		UWorld* World = GetWorld();

		// Defer the spawning across multiple frames (depending on available budget)
		URogueDeferredTaskSystem::AddLambda(this,[World,RandomPowerupClass,PickedLocation]()
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				
				World->SpawnActor<AActor>(RandomPowerupClass, PickedLocation, FRotator::ZeroRotator, SpawnParams);
			});
#else
		GetWorld()->SpawnActor<AActor>(RandomPowerupClass, PickedLocation, FRotator::ZeroRotator);
#endif
		

		// Keep for distance checks
		UsedLocations.Add(PickedLocation);
		SpawnCounter++;
	}

	uint64 CyclesEnd = FPlatformTime::Cycles64();

	UE_LOG(LogGame, Log, TEXT("OnPowerupSpawnQueryCompleted: %llu Cycles"), (CyclesEnd - CyclesStart));
}


void ARogueGameModeBase::RespawnPlayerElapsed(AController* Controller)
{
	Controller->UnPossess();
	RestartPlayer(Controller);
}


int32 ARogueGameModeBase::GetDifficultyLevel() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	const float SafeInterval = FMath::Max(1.0f, DifficultyInterval);
	return FMath::Max(0, FMath::FloorToInt(World->TimeSeconds / SafeInterval));
}


int32 ARogueGameModeBase::GetCurrentMaxBotCount() const
{
	const int32 DifficultyLevel = GetDifficultyLevel();
	return FMath::Max(0, BaseMaxBotCount + DifficultyLevel * MaxBotCountPerDifficulty);
}


float ARogueGameModeBase::GetSpawnCreditMultiplier() const
{
	const int32 DifficultyLevel = GetDifficultyLevel();
	return FMath::Max(0.0f, 1.0f + DifficultyLevel * SpawnCreditDifficultyScale);
}


void ARogueGameModeBase::OnActorKilled(AActor* VictimActor, AActor* Killer)
{
	UE_LOGFMT(LogGame, Log, "OnActorKilled: Victim: {victim}, Killer: {killer}", GetNameSafe(VictimActor), GetNameSafe(Killer));

	// Handle Player death
	ARoguePlayerCharacter* Player = Cast<ARoguePlayerCharacter>(VictimActor);
	if (Player)
	{
		// Auto-respawn
		if (bAutoRespawnPlayer)
		{
			FTimerHandle TimerHandle_RespawnDelay;
			FTimerDelegate Delegate;
			Delegate.BindUObject(this, &ThisClass::RespawnPlayerElapsed, Player->GetController());
 
			const float RespawnDelay = 2.0f;
			GetWorldTimerManager().SetTimer(TimerHandle_RespawnDelay, Delegate, RespawnDelay, false);
		}

		// Store time if it was better than previous record
		ARoguePlayerState* PS = Player->GetPlayerState<ARoguePlayerState>();
		if (PS)
		{
			PS->UpdatePersonalRecord(GetWorld()->TimeSeconds);
		}

		URogueSaveGameSubsystem* SG = GetGameInstance()->GetSubsystem<URogueSaveGameSubsystem>();
		// Immediately auto save on death
		SG->WriteSaveGame();
	}

	// Apply player kill-triggered effects. Coin rewards are spawned as pickups by the AI death flow.
	APawn* KillerPawn = Cast<APawn>(Killer);
	// Don't credit kills of self
	if (KillerPawn && KillerPawn != VictimActor)
	{
		// Only Players will have a 'PlayerState' instance, bots have nullptr
		ARoguePlayerState* PS = KillerPawn->GetPlayerState<ARoguePlayerState>();
		if (PS) 
		{
			ApplyKillExplosionUpgrade(VictimActor, KillerPawn, PS);
		}
	}
}


void ARogueGameModeBase::ApplyKillExplosionUpgrade(AActor* VictimActor, APawn* KillerPawn, ARoguePlayerState* KillerPlayerState)
{
	if (!VictimActor || !KillerPawn || !KillerPlayerState || !KillerPlayerState->HasKillExplosionUpgrade())
	{
		return;
	}

	const float ExplosionRadius = KillerPlayerState->GetKillExplosionRadius();
	const float DamageCoefficient = KillerPlayerState->GetKillExplosionDamageCoefficient();
	if (ExplosionRadius <= 0.0f || DamageCoefficient <= 0.0f)
	{
		return;
	}

	const FVector ExplosionOrigin = VictimActor->GetActorLocation();
	TSubclassOf<ARogueRadiusIndicatorActor> IndicatorClass = KillExplosionIndicatorClass;
	if (!IndicatorClass)
	{
		IndicatorClass = ARogueRadiusIndicatorActor::StaticClass();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FTransform IndicatorTransform = MakeGroundProjectedTransform(ExplosionOrigin, VictimActor);
	if (ARogueRadiusIndicatorActor* Indicator = GetWorld()->SpawnActor<ARogueRadiusIndicatorActor>(IndicatorClass, IndicatorTransform, SpawnParams))
	{
		Indicator->InitializeIndicator(ExplosionRadius, KillExplosionDelay);
	}

	FTimerHandle TimerHandle_KillExplosion;
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(
		this,
		&ThisClass::DetonateKillExplosion,
		ExplosionOrigin,
		TWeakObjectPtr<APawn>(KillerPawn),
		TWeakObjectPtr<ARoguePlayerState>(KillerPlayerState));

	GetWorldTimerManager().SetTimer(TimerHandle_KillExplosion, TimerDelegate, KillExplosionDelay, false);
}


void ARogueGameModeBase::DetonateKillExplosion(FVector Origin, TWeakObjectPtr<APawn> KillerPawn, TWeakObjectPtr<ARoguePlayerState> KillerPlayerState)
{
	APawn* KillerPawnPtr = KillerPawn.Get();
	ARoguePlayerState* KillerPlayerStatePtr = KillerPlayerState.Get();
	if (!KillerPawnPtr || !KillerPlayerStatePtr)
	{
		return;
	}

	const float ExplosionRadius = KillerPlayerStatePtr->GetKillExplosionRadius();
	const float DamageCoefficient = KillerPlayerStatePtr->GetKillExplosionDamageCoefficient();
	if (ExplosionRadius <= 0.0f || DamageCoefficient <= 0.0f)
	{
		return;
	}

	if (KillExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, KillExplosionSound, Origin);
	}

	if (KillExplosionVFX)
	{
		const float VisualScale = FMath::Clamp(ExplosionRadius / 450.0f, 0.75f, 2.25f);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), KillExplosionVFX, Origin, FRotator::ZeroRotator, FVector(VisualScale), true, true, ENCPoolMethod::AutoRelease);
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KillExplosion), false, KillerPawnPtr);

	const bool bHasOverlaps = GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		Origin,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(ExplosionRadius),
		QueryParams);

	if (!bHasOverlaps)
	{
		return;
	}

	int32 DamagedTargets = 0;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!TargetActor || TargetActor == KillerPawnPtr)
		{
			continue;
		}

		if (!TargetActor->IsA<ARogueAICharacter>() || !URogueGameplayFunctionLibrary::IsAlive(TargetActor))
		{
			continue;
		}

		FGameplayTagContainer ExplosionContext;
		ExplosionContext.AddTag(SharedGameplayTags::Context_KillExplosion);
		if (URogueGameplayFunctionLibrary::ApplyDamage(KillerPawnPtr, TargetActor, DamageCoefficient, ExplosionContext))
		{
			DamagedTargets++;

			const FVector TargetLocation = TargetActor->GetActorLocation();
			FVector KnockbackDirection = TargetLocation - Origin;
			KnockbackDirection.Z = 0.0f;
			if (!KnockbackDirection.Normalize())
			{
				KnockbackDirection = KillerPawnPtr->GetActorForwardVector();
				KnockbackDirection.Z = 0.0f;
				KnockbackDirection.Normalize();
			}

			if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
			{
				if (UCharacterMovementComponent* MoveComp = TargetCharacter->GetCharacterMovement())
				{
					MoveComp->SetMovementMode(MOVE_Falling);
				}

				const FVector LaunchVelocity = KnockbackDirection * KillExplosionKnockbackStrength + FVector::UpVector * KillExplosionUpwardStrength;
				TargetCharacter->LaunchCharacter(LaunchVelocity, true, true);
			}

			if (IRogueGameplayInterface* GameplayInterface = Cast<IRogueGameplayInterface>(TargetActor))
			{
				const FVector Impulse = KnockbackDirection * KillExplosionCorpseImpulseStrength + FVector::UpVector * (KillExplosionCorpseImpulseStrength * 0.25f);
				GameplayInterface->AddImpulseAtLocationCustom(Impulse, TargetLocation);
			}
		}
	}

	if (DamagedTargets > 0)
	{
		UE_LOGFMT(LogGame, Log, "Kill explosion damaged {Count} target(s).", DamagedTargets);
	}
}


void ARogueGameModeBase::ApplyChainLightningUpgrade(AActor* HitActor, APawn* DamageCauserPawn, ARoguePlayerState* DamageCauserPlayerState)
{
	if (!HitActor || !DamageCauserPawn || !DamageCauserPlayerState || !DamageCauserPlayerState->HasChainLightningUpgrade() || !DamageCauserPlayerState->CanTriggerChainLightningFrom(HitActor))
	{
		return;
	}

	const float ArcRadius = DamageCauserPlayerState->GetChainLightningRadius();
	const float DamageCoefficient = DamageCauserPlayerState->GetChainLightningDamageCoefficient();
	const int32 MaxTargets = DamageCauserPlayerState->GetChainLightningTargetCount();
	if (ArcRadius <= 0.0f || DamageCoefficient <= 0.0f || MaxTargets <= 0)
	{
		return;
	}

	const FVector ArcOrigin = HitActor->GetActorLocation();
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ChainLightning), false, DamageCauserPawn);

	const bool bHasOverlaps = GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		ArcOrigin,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(ArcRadius),
		QueryParams);

	if (!bHasOverlaps)
	{
		return;
	}

	TArray<AActor*> ValidTargets;
	TSet<AActor*> SeenTargets;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!TargetActor || TargetActor == DamageCauserPawn || TargetActor == HitActor)
		{
			continue;
		}

		if (!TargetActor->IsA<ARogueAICharacter>() || !URogueGameplayFunctionLibrary::IsAlive(TargetActor))
		{
			continue;
		}

		if (SeenTargets.Contains(TargetActor))
		{
			continue;
		}

		SeenTargets.Add(TargetActor);
		ValidTargets.Add(TargetActor);
	}

	ValidTargets.Sort([ArcOrigin](const AActor& Left, const AActor& Right)
	{
		return FVector::DistSquared(Left.GetActorLocation(), ArcOrigin) < FVector::DistSquared(Right.GetActorLocation(), ArcOrigin);
	});

	if (ValidTargets.Num() == 0)
	{
		return;
	}

	int32 DamagedTargets = 0;
	TArray<FVector> DamagedTargetLocations;
	DamagedTargetLocations.Reserve(MaxTargets);
	for (AActor* TargetActor : ValidTargets)
	{
		if (DamagedTargets >= MaxTargets)
		{
			break;
		}

		FGameplayTagContainer ChainContext;
		ChainContext.AddTag(SharedGameplayTags::Context_ChainLightning);
		if (URogueGameplayFunctionLibrary::ApplyDamage(DamageCauserPawn, TargetActor, DamageCoefficient, ChainContext))
		{
			DamagedTargets++;

			const FVector TargetLocation = TargetActor->GetActorLocation();
			DamagedTargetLocations.Add(TargetLocation);

			FVector KnockbackDirection = TargetLocation - ArcOrigin;
			KnockbackDirection.Z = 0.0f;
			if (!KnockbackDirection.Normalize())
			{
				KnockbackDirection = DamageCauserPawn->GetActorForwardVector();
				KnockbackDirection.Z = 0.0f;
				KnockbackDirection.Normalize();
			}

			if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
			{
				if (UCharacterMovementComponent* MoveComp = TargetCharacter->GetCharacterMovement())
				{
					MoveComp->SetMovementMode(MOVE_Falling);
				}

				const FVector LaunchVelocity = KnockbackDirection * (KillExplosionKnockbackStrength * 0.55f) + FVector::UpVector * (KillExplosionUpwardStrength * 0.55f);
				TargetCharacter->LaunchCharacter(LaunchVelocity, true, true);
			}
		}
	}

	if (DamagedTargets > 0)
	{
		DamageCauserPlayerState->CommitChainLightningCooldown(HitActor);
		for (const FVector& TargetLocation : DamagedTargetLocations)
		{
			PlayChainLightningVFX(ArcOrigin, TargetLocation);
		}
		if (ChainLightningSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, ChainLightningSound, ArcOrigin, ChainLightningSoundVolume);
		}
		if (ChainLightningImpactVFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ChainLightningImpactVFX, ArcOrigin + FVector(0.0f, 0.0f, 45.0f), FRotator::ZeroRotator, FVector(0.45f), true, true, ENCPoolMethod::AutoRelease);
		}
		UE_LOGFMT(LogGame, Log, "Chain Lightning damaged {Count} target(s).", DamagedTargets);
	}
}


void ARogueGameModeBase::PlayChainLightningVFX(const FVector& StartLocation, const FVector& EndLocation) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Start = StartLocation + FVector(0.0f, 0.0f, 60.0f);
	const FVector End = EndLocation + FVector(0.0f, 0.0f, 60.0f);

	constexpr float ArcDuration = 0.18f;
	const FColor MainColor(20, 180, 255);
	const FColor BranchColor(120, 235, 255);
	DrawDebugLine(World, Start, End, MainColor, false, ArcDuration, 0, 8.0f);

	const FVector ArcVector = End - Start;
	const FVector ArcDirection = ArcVector.GetSafeNormal();
	FVector SideVector = FVector::CrossProduct(ArcDirection, FVector::UpVector).GetSafeNormal();
	if (SideVector.IsNearlyZero())
	{
		SideVector = FVector::RightVector;
	}

	for (int32 BranchIndex = 0; BranchIndex < 2; ++BranchIndex)
	{
		const float Sign = BranchIndex == 0 ? 1.0f : -1.0f;
		const FVector StartOffset = SideVector * (18.0f * Sign) + FVector::UpVector * 8.0f;
		const FVector EndOffset = SideVector * (-22.0f * Sign) + FVector::UpVector * -6.0f;
		DrawDebugLine(World, Start + StartOffset, End + EndOffset, BranchColor, false, ArcDuration, 0, 3.0f);
	}

	DrawDebugPoint(World, Start, 14.0f, BranchColor, false, ArcDuration, 0);
	DrawDebugPoint(World, End, 18.0f, BranchColor, false, ArcDuration, 0);

	if (ChainLightningImpactVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, ChainLightningImpactVFX, End, FRotator::ZeroRotator, FVector(0.45f), true, true, ENCPoolMethod::AutoRelease);
	}
}


FTransform ARogueGameModeBase::MakeGroundProjectedTransform(const FVector& Origin, const AActor* IgnoredActor) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return FTransform(FRotator::ZeroRotator, Origin);
	}

	const FVector TraceStart = Origin + FVector(0.0f, 0.0f, 250.0f);
	const FVector TraceEnd = Origin - FVector(0.0f, 0.0f, 1000.0f);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KillExplosionGroundTrace), false);
	if (IgnoredActor)
	{
		QueryParams.AddIgnoredActor(IgnoredActor);
	}

	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams) && Hit.bBlockingHit)
	{
		const FVector UpVector = Hit.ImpactNormal.GetSafeNormal();
		const FQuat Rotation = FRotationMatrix::MakeFromZX(UpVector, FVector::ForwardVector).ToQuat();
		return FTransform(Rotation, Hit.ImpactPoint + UpVector * 3.0f);
	}

	return FTransform(FRotator::ZeroRotator, Origin + FVector(0.0f, 0.0f, 6.0f));
}
