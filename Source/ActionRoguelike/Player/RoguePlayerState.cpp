// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/RoguePlayerState.h"

#include "ActionSystem/RogueActionComponent.h"
#include "ActionRoguelike.h"
#include "Pickups/RoguePickupSubsystem.h"
#include "Player/RogueUpgradeDataAsset.h"
#include "SaveSystem/RogueSaveGame.h"
#include "SharedGameplayTags.h"
#include "Internationalization/Text.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RoguePlayerState)


void ARoguePlayerState::AddCredits(int32 Delta)
{
	// Avoid user-error of adding a negative amount
	if (!ensure(Delta >= 0.0f))
	{
		return;
	}

	Credits += Delta;

	OnCreditsChanged.Broadcast(this, Credits, Delta);
}


bool ARoguePlayerState::TryRemoveCredits(int32 Delta)
{
	// Avoid user-error of adding a subtracting negative amount
	if (!ensure(Delta >= 0.0f))
	{
		return false;
	}

	if (Credits < Delta)
	{
		// Not enough credits available
		return false;
	}

	Credits -= Delta;

	OnCreditsChanged.Broadcast(this, Credits, -Delta);

	return true;
}


void ARoguePlayerState::AddExperience(int32 Delta)
{
	if (!ensure(Delta >= 0))
	{
		return;
	}

	if (Delta == 0)
	{
		return;
	}

	Experience += Delta;
	OnExperienceChanged.Broadcast(this, Experience, GetExperienceToNextLevel(), Delta);

	TryLevelUp();
}


int32 ARoguePlayerState::CalculateExperienceToNextLevel(int32 InLevel) const
{
	const int32 SafeLevel = FMath::Max(1, InLevel);
	const int32 SafeBaseXP = FMath::Max(1, BaseExperienceToNextLevel);
	const float SafeGrowthRate = FMath::Max(1.0f, ExperienceGrowthRate);

	return FMath::Max(1, FMath::RoundToInt(SafeBaseXP * FMath::Pow(SafeGrowthRate, SafeLevel - 1)));
}


void ARoguePlayerState::TryLevelUp()
{
	int32 ExperienceToNextLevel = GetExperienceToNextLevel();
	bool bLeveledUp = false;

	while (Experience >= ExperienceToNextLevel)
	{
		Experience -= ExperienceToNextLevel;

		const int32 OldLevel = Level;
		Level++;
		bLeveledUp = true;

		GenerateUpgradeChoices(Level);

		OnLevelChanged.Broadcast(this, Level, OldLevel);

		if (GEngine)
		{
			const FString LevelUpMessage = FString::Printf(TEXT("Level Up! Level %d"), Level);
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, LevelUpMessage);
		}

		UE_LOGFMT(LogGame, Log, "Player leveled up: {OldLevel} -> {NewLevel}", OldLevel, Level);

		ExperienceToNextLevel = GetExperienceToNextLevel();
	}

	if (bLeveledUp)
	{
		OnExperienceChanged.Broadcast(this, Experience, GetExperienceToNextLevel(), 0);
	}
}


void ARoguePlayerState::GenerateUpgradeChoices(int32 NewLevel)
{
	if (!HasAuthority())
	{
		return;
	}

	TArray<FRogueUpgradeChoice> UpgradePool;
	BuildUpgradePool(UpgradePool);

	TArray<FRogueUpgradeChoice> OfferableChoices;
	OfferableChoices.Reserve(UpgradePool.Num());
	for (const FRogueUpgradeChoice& Choice : UpgradePool)
	{
		if (CanOfferUpgrade(Choice))
		{
			OfferableChoices.Add(Choice);
		}
	}

	PendingUpgradeChoices.Reset();
	const int32 ChoiceCount = FMath::Min(3, OfferableChoices.Num());
	for (int32 ChoiceIndex = 0; ChoiceIndex < ChoiceCount; ++ChoiceIndex)
	{
		const int32 RandomIndex = FMath::RandRange(0, OfferableChoices.Num() - 1);
		PendingUpgradeChoices.Add(OfferableChoices[RandomIndex]);
		OfferableChoices.RemoveAtSwap(RandomIndex);
	}

	UE_LOGFMT(LogGame, Log, "Generated {ChoiceCount} upgrade choices for level {Level}.", PendingUpgradeChoices.Num(), NewLevel);
	for (int32 ChoiceIndex = 0; ChoiceIndex < PendingUpgradeChoices.Num(); ++ChoiceIndex)
	{
		UE_LOGFMT(LogGame, Log, "Upgrade choice {Index}: {Name} ({Id})",
			ChoiceIndex,
			PendingUpgradeChoices[ChoiceIndex].DisplayName.ToString(),
			PendingUpgradeChoices[ChoiceIndex].UpgradeId);
	}

	OnUpgradeChoicesGenerated.Broadcast(this, NewLevel);

	if (bAutoSelectUpgradeChoices && PendingUpgradeChoices.Num() > 0)
	{
		SelectUpgradeChoice(0);
	}
}


void ARoguePlayerState::BuildUpgradePool(TArray<FRogueUpgradeChoice>& OutPool) const
{
	if (UpgradeData && UpgradeData->Upgrades.Num() > 0)
	{
		OutPool = UpgradeData->Upgrades;
		return;
	}

	BuildDefaultUpgradePool(OutPool);
}


void ARoguePlayerState::BuildDefaultUpgradePool(TArray<FRogueUpgradeChoice>& OutPool) const
{
	OutPool.Reset();

	FRogueUpgradeChoice AttackDamageChoice;
	AttackDamageChoice.UpgradeId = TEXT("AttackDamage");
	AttackDamageChoice.DisplayName = FText::FromString(TEXT("Attack Training"));
	AttackDamageChoice.Description = FText::FromString(TEXT("Attack damage +5."));
	AttackDamageChoice.Rarity = ERogueUpgradeRarity::Common;
	AttackDamageChoice.EffectType = ERogueUpgradeEffectType::AddAttribute;
	AttackDamageChoice.AttributeTag = SharedGameplayTags::Attribute_AttackDamage;
	AttackDamageChoice.Magnitude = AttackDamagePerLevel;
	AttackDamageChoice.MaxStacks = 0;
	OutPool.Add(AttackDamageChoice);

	FRogueUpgradeChoice HealthMaxChoice;
	HealthMaxChoice.UpgradeId = TEXT("HealthMax");
	HealthMaxChoice.DisplayName = FText::FromString(TEXT("Toughness"));
	HealthMaxChoice.Description = FText::FromString(TEXT("Maximum health +20."));
	HealthMaxChoice.Rarity = ERogueUpgradeRarity::Common;
	HealthMaxChoice.EffectType = ERogueUpgradeEffectType::AddAttribute;
	HealthMaxChoice.AttributeTag = SharedGameplayTags::Attribute_HealthMax;
	HealthMaxChoice.Magnitude = 20.0f;
	HealthMaxChoice.MaxStacks = 0;
	OutPool.Add(HealthMaxChoice);

	FRogueUpgradeChoice PickupRadiusChoice;
	PickupRadiusChoice.UpgradeId = TEXT("PickupMagnet");
	PickupRadiusChoice.DisplayName = FText::FromString(TEXT("Magnet Field"));
	PickupRadiusChoice.Description = FText::FromString(TEXT("Coin and XP attraction radius +200."));
	PickupRadiusChoice.Rarity = ERogueUpgradeRarity::Rare;
	PickupRadiusChoice.EffectType = ERogueUpgradeEffectType::ModifyPickupRadius;
	PickupRadiusChoice.Magnitude = 200.0f;
	PickupRadiusChoice.MaxStacks = 3;
	OutPool.Add(PickupRadiusChoice);

	FRogueUpgradeChoice KillExplosionChoice;
	KillExplosionChoice.UpgradeId = TEXT("KillExplosion");
	KillExplosionChoice.DisplayName = FText::FromString(TEXT("Kill Explosion"));
	KillExplosionChoice.Description = FText::FromString(TEXT("Killed enemies explode after a short delay."));
	KillExplosionChoice.Rarity = ERogueUpgradeRarity::Prismatic;
	KillExplosionChoice.EffectType = ERogueUpgradeEffectType::GrantKillExplosion;
	KillExplosionChoice.Magnitude = KillExplosionDamageCoefficient;
	KillExplosionChoice.MaxStacks = 5;
	KillExplosionChoice.bUnique = false;
	OutPool.Add(KillExplosionChoice);

	FRogueUpgradeChoice LastStandShieldChoice;
	LastStandShieldChoice.UpgradeId = TEXT("LastStandShield");
	LastStandShieldChoice.DisplayName = FText::FromString(TEXT("Last Stand"));
	LastStandShieldChoice.Description = FText::FromString(TEXT("Once on cooldown, lethal damage restores health instead. Higher levels restore more."));
	LastStandShieldChoice.Rarity = ERogueUpgradeRarity::Rare;
	LastStandShieldChoice.EffectType = ERogueUpgradeEffectType::GrantLastStandShield;
	LastStandShieldChoice.Magnitude = LastStandShieldHealAmount;
	LastStandShieldChoice.MaxStacks = 3;
	OutPool.Add(LastStandShieldChoice);

	FRogueUpgradeChoice ChainLightningChoice;
	ChainLightningChoice.UpgradeId = TEXT("ChainLightning");
	ChainLightningChoice.DisplayName = FText::FromString(TEXT("Chain Lightning"));
	ChainLightningChoice.Description = FText::FromString(TEXT("Attack hits arc bonus damage to nearby enemies."));
	ChainLightningChoice.Rarity = ERogueUpgradeRarity::Prismatic;
	ChainLightningChoice.EffectType = ERogueUpgradeEffectType::GrantChainLightning;
	ChainLightningChoice.Magnitude = ChainLightningDamageCoefficient;
	ChainLightningChoice.MaxStacks = 4;
	OutPool.Add(ChainLightningChoice);
}


bool ARoguePlayerState::CanOfferUpgrade(const FRogueUpgradeChoice& Choice) const
{
	if (Choice.UpgradeId.IsNone())
	{
		return false;
	}

	const int32 CurrentStacks = GetUpgradeStackCount(Choice.UpgradeId);
	if (Choice.bUnique && CurrentStacks > 0)
	{
		return false;
	}

	if (Choice.MaxStacks > 0 && CurrentStacks >= Choice.MaxStacks)
	{
		return false;
	}

	return true;
}


bool ARoguePlayerState::ApplyUpgradeChoice(const FRogueUpgradeChoice& Choice)
{
	if (!HasAuthority() || !CanOfferUpgrade(Choice))
	{
		return false;
	}

	APawn* MyPawn = GetPawn();
	if (MyPawn == nullptr)
	{
		UE_LOGFMT(LogGame, Warning, "Could not apply upgrade {UpgradeId}: player pawn is missing.", Choice.UpgradeId);
		return false;
	}

	bool bApplied = false;
	switch (Choice.EffectType)
	{
		case ERogueUpgradeEffectType::AddAttribute:
		{
			URogueActionComponent* ActionComp = URogueActionComponent::GetActionComponent(MyPawn);
			if (ActionComp == nullptr)
			{
				UE_LOGFMT(LogGame, Warning, "Could not apply upgrade {UpgradeId}: action component is missing on {Pawn}.", Choice.UpgradeId, GetNameSafe(MyPawn));
				return false;
			}

			const float OldValue = ActionComp->GetAttributeValue(Choice.AttributeTag);
			bApplied = ActionComp->ApplyAttributeChange(
				Choice.AttributeTag,
				Choice.Magnitude,
				MyPawn,
				EAttributeModifyType::AddBase);
			const float NewValue = ActionComp->GetAttributeValue(Choice.AttributeTag);
			UE_LOGFMT(LogGame, Log, "Upgrade applied: {UpgradeId}, {Attribute} {OldValue} -> {NewValue}.",
				Choice.UpgradeId,
				Choice.AttributeTag.ToString(),
				OldValue,
				NewValue);

			if (bApplied && Choice.AttributeTag == SharedGameplayTags::Attribute_HealthMax)
			{
				ActionComp->ApplyAttributeChange(
					SharedGameplayTags::Attribute_Health,
					Choice.Magnitude,
					MyPawn,
					EAttributeModifyType::AddBase);
			}
			break;
		}
		case ERogueUpgradeEffectType::ModifyPickupRadius:
		{
			if (URoguePickupSubsystem* PickupSubsystem = GetWorld()->GetSubsystem<URoguePickupSubsystem>())
			{
				PickupSubsystem->AddPickupAttractRadiusBonus(Choice.Magnitude);
				bApplied = true;
			}
			break;
		}
		case ERogueUpgradeEffectType::GrantKillExplosion:
		{
			bApplied = true;
			break;
		}
		case ERogueUpgradeEffectType::GrantLastStandShield:
		{
			bApplied = true;
			break;
		}
		case ERogueUpgradeEffectType::GrantChainLightning:
		{
			bApplied = true;
			break;
		}
		default:
			break;
	}

	if (bApplied)
	{
		int32& StackCount = FindOrAddUpgradeStack(Choice.UpgradeId);
		StackCount++;
		UE_LOGFMT(LogGame, Log, "Upgrade selected: {Name} ({Id}), stack {Stack}.",
			Choice.DisplayName.ToString(),
			Choice.UpgradeId,
			StackCount);
	}
	else
	{
		UE_LOGFMT(LogGame, Warning, "Upgrade did not apply: {Name} ({Id}).",
			Choice.DisplayName.ToString(),
			Choice.UpgradeId);
	}

	return bApplied;
}


bool ARoguePlayerState::UpdatePersonalRecord(float NewTime)
{
	// Higher time is better
	if (NewTime > PersonalRecordTime)
	{
		float OldRecord = PersonalRecordTime;

		PersonalRecordTime = NewTime;

		OnRecordTimeChanged.Broadcast(this, PersonalRecordTime, OldRecord);

		return true;
	}

	return false;
}


void ARoguePlayerState::SavePlayerState_Implementation(URogueSaveGame* SaveObject)
{
	if (SaveObject)
	{
		// Gather all relevant data for player
		FPlayerSaveData SaveData;
		SaveData.Credits = Credits;
		SaveData.PersonalRecordTime = PersonalRecordTime;
		// Stored as FString for simplicity (original Steam ID is uint64)
		SaveData.PlayerID = GetUniqueId().ToString();

		// May not be alive while we save
		if (APawn* MyPawn = GetPawn())
		{
			SaveData.Location = MyPawn->GetActorLocation();
			SaveData.Rotation = MyPawn->GetActorRotation();
			SaveData.bResumeAtTransform = true;
		}
		
		SaveObject->SavedPlayers.Add(SaveData);
	}
}


void ARoguePlayerState::LoadPlayerState_Implementation(URogueSaveGame* SaveObject)
{
	if (SaveObject)
	{
		FPlayerSaveData* FoundData = SaveObject->GetPlayerData(this);
		if (FoundData)
		{
			// Makes sure we trigger credits changed event
			AddCredits(FoundData->Credits);

			PersonalRecordTime = FoundData->PersonalRecordTime;
		}
		else
		{
			UE_LOGFMT(LogGame, Warning, "Could not find SaveGame data for player id: {playerid}.", GetPlayerId());
		}
	}
}


void ARoguePlayerState::OnRep_Credits(int32 OldCredits)
{
	OnCreditsChanged.Broadcast(this, Credits, Credits - OldCredits);
}


void ARoguePlayerState::OnRep_Level(int32 OldLevel)
{
	OnLevelChanged.Broadcast(this, Level, OldLevel);
}


void ARoguePlayerState::OnRep_Experience(int32 OldExperience)
{
	OnExperienceChanged.Broadcast(this, Experience, GetExperienceToNextLevel(), Experience - OldExperience);
}


void ARoguePlayerState::OnRep_PendingUpgradeChoices()
{
	OnUpgradeChoicesGenerated.Broadcast(this, Level);
}


int32 ARoguePlayerState::GetCredits() const
{
	return Credits;
}


int32 ARoguePlayerState::GetPlayerLevel() const
{
	return Level;
}


int32 ARoguePlayerState::GetExperience() const
{
	return Experience;
}


int32 ARoguePlayerState::GetExperienceToNextLevel() const
{
	return CalculateExperienceToNextLevel(Level);
}


bool ARoguePlayerState::SelectUpgradeChoice(int32 ChoiceIndex)
{
	if (!HasAuthority())
	{
		ServerSelectUpgradeChoice(ChoiceIndex);
		return true;
	}

	if (!PendingUpgradeChoices.IsValidIndex(ChoiceIndex))
	{
		UE_LOGFMT(LogGame, Warning, "Invalid upgrade choice index {ChoiceIndex}.", ChoiceIndex);
		return false;
	}

	const FRogueUpgradeChoice SelectedChoice = PendingUpgradeChoices[ChoiceIndex];
	const bool bApplied = ApplyUpgradeChoice(SelectedChoice);
	if (bApplied)
	{
		PendingUpgradeChoices.Reset();
	}

	return bApplied;
}


void ARoguePlayerState::ServerSelectUpgradeChoice_Implementation(int32 ChoiceIndex)
{
	SelectUpgradeChoice(ChoiceIndex);
}


bool ARoguePlayerState::GrantUpgradeById(FName UpgradeId)
{
	if (!HasAuthority())
	{
		return false;
	}

	TArray<FRogueUpgradeChoice> UpgradePool;
	BuildUpgradePool(UpgradePool);

	for (const FRogueUpgradeChoice& Choice : UpgradePool)
	{
		if (Choice.UpgradeId == UpgradeId)
		{
			return ApplyUpgradeChoice(Choice);
		}
	}

	UE_LOGFMT(LogGame, Warning, "Could not grant upgrade: unknown id {UpgradeId}.", UpgradeId);
	return false;
}


void ARoguePlayerState::DebugGenerateUpgradeChoices()
{
	if (!HasAuthority())
	{
		return;
	}

	GenerateUpgradeChoices(Level);
}


TArray<FRogueUpgradeChoice> ARoguePlayerState::GetPendingUpgradeChoices() const
{
	return PendingUpgradeChoices;
}


FText ARoguePlayerState::GetUpgradePreviewText(const FRogueUpgradeChoice& Choice) const
{
	const int32 CurrentStacks = GetUpgradeStackCount(Choice.UpgradeId);
	const int32 PreviewStack = CurrentStacks + 1;

	if (Choice.UpgradeId == TEXT("AttackDamage"))
	{
		return FText::Format(NSLOCTEXT("RogueUpgrades", "AttackDamagePreview", "Damage: +{0} attack damage"),
			FText::AsNumber(FMath::RoundToInt(Choice.Magnitude)));
	}

	if (Choice.UpgradeId == TEXT("HealthMax"))
	{
		return FText::Format(NSLOCTEXT("RogueUpgrades", "HealthMaxPreview", "Max health: +{0}, heal +{0}"),
			FText::AsNumber(FMath::RoundToInt(Choice.Magnitude)));
	}

	if (Choice.UpgradeId == TEXT("PickupMagnet"))
	{
		const float NewBonus = Choice.Magnitude * PreviewStack;
		return FText::Format(NSLOCTEXT("RogueUpgrades", "PickupMagnetPreview", "Pickup attraction: +{0} radius total"),
			FText::AsNumber(FMath::RoundToInt(NewBonus)));
	}

	if (Choice.UpgradeId == TEXT("KillExplosion"))
	{
		const float PreviewRadius = KillExplosionRadius + KillExplosionRadiusPerStack * FMath::Max(0, PreviewStack - 1);
		const float PreviewDamage = KillExplosionDamageCoefficient + KillExplosionDamageCoefficientPerStack * FMath::Max(0, PreviewStack - 1);
		return FText::Format(
			NSLOCTEXT("RogueUpgrades", "KillExplosionPreview", "Explosion: {0}% damage, {1} radius, {2}s delay"),
			FText::AsNumber(FMath::RoundToInt(PreviewDamage)),
			FText::AsNumber(FMath::RoundToInt(PreviewRadius)),
			FText::AsNumber(1));
	}

	if (Choice.UpgradeId == TEXT("LastStandShield"))
	{
		const float PreviewHeal = LastStandShieldHealAmount + LastStandShieldHealPerStack * FMath::Max(0, PreviewStack - 1);
		return FText::Format(
			NSLOCTEXT("RogueUpgrades", "LastStandShieldPreview", "Lethal save: heal {0}, {1}s cooldown"),
			FText::AsNumber(FMath::RoundToInt(PreviewHeal)),
			FText::AsNumber(FMath::RoundToInt(LastStandShieldCooldown)));
	}

	if (Choice.UpgradeId == TEXT("ChainLightning"))
	{
		const float PreviewDamage = ChainLightningDamageCoefficient + ChainLightningDamageCoefficientPerStack * FMath::Max(0, PreviewStack - 1);
		const int32 PreviewTargets = FMath::Max(0, ChainLightningTargetCount + ChainLightningTargetCountPerStack * FMath::Max(0, PreviewStack - 1));
		return FText::Format(
			NSLOCTEXT("RogueUpgrades", "ChainLightningPreview", "Arc: {0}% damage, {1} target(s), {2} radius, {3}s cooldown"),
			FText::AsNumber(FMath::RoundToInt(PreviewDamage)),
			FText::AsNumber(PreviewTargets),
			FText::AsNumber(FMath::RoundToInt(ChainLightningRadius)),
			FText::AsNumber(ChainLightningCooldown));
	}

	return Choice.Description;
}


bool ARoguePlayerState::HasUpgrade(FName UpgradeId) const
{
	return GetUpgradeStackCount(UpgradeId) > 0;
}


int32 ARoguePlayerState::GetUpgradeStackCount(FName UpgradeId) const
{
	for (const FRogueUpgradeStack& UpgradeStack : UpgradeStacks)
	{
		if (UpgradeStack.UpgradeId == UpgradeId)
		{
			return UpgradeStack.StackCount;
		}
	}

	return 0;
}


int32& ARoguePlayerState::FindOrAddUpgradeStack(FName UpgradeId)
{
	for (FRogueUpgradeStack& UpgradeStack : UpgradeStacks)
	{
		if (UpgradeStack.UpgradeId == UpgradeId)
		{
			return UpgradeStack.StackCount;
		}
	}

	FRogueUpgradeStack& NewStack = UpgradeStacks.AddDefaulted_GetRef();
	NewStack.UpgradeId = UpgradeId;
	return NewStack.StackCount;
}


bool ARoguePlayerState::HasKillExplosionUpgrade() const
{
	return HasUpgrade(TEXT("KillExplosion"));
}


float ARoguePlayerState::GetKillExplosionRadius() const
{
	const int32 StackCount = GetUpgradeStackCount(TEXT("KillExplosion"));
	if (StackCount <= 0)
	{
		return 0.0f;
	}

	return KillExplosionRadius + KillExplosionRadiusPerStack * FMath::Max(0, StackCount - 1);
}


float ARoguePlayerState::GetKillExplosionDamageCoefficient() const
{
	const int32 StackCount = GetUpgradeStackCount(TEXT("KillExplosion"));
	if (StackCount <= 0)
	{
		return 0.0f;
	}

	return KillExplosionDamageCoefficient + KillExplosionDamageCoefficientPerStack * FMath::Max(0, StackCount - 1);
}


bool ARoguePlayerState::HasLastStandShieldUpgrade() const
{
	return HasUpgrade(TEXT("LastStandShield"));
}


bool ARoguePlayerState::TryActivateLastStandShield(URogueActionComponent* TargetActionComp, AActor* HealInstigator)
{
	if (!HasAuthority() || !TargetActionComp || !HasLastStandShieldUpgrade())
	{
		return false;
	}

	UWorld* World = GetWorld();
	const float CurrentTime = World ? World->TimeSeconds : 0.0f;
	if (CurrentTime < LastStandShieldNextReadyTime)
	{
		return false;
	}

	const int32 StackCount = GetUpgradeStackCount(TEXT("LastStandShield"));
	if (StackCount <= 0)
	{
		return false;
	}

	const float HealAmount = LastStandShieldHealAmount + LastStandShieldHealPerStack * FMath::Max(0, StackCount - 1);
	if (HealAmount <= 0.0f)
	{
		return false;
	}

	const bool bHealed = TargetActionComp->ApplyAttributeChange(
		SharedGameplayTags::Attribute_Health,
		HealAmount,
		HealInstigator ? HealInstigator : GetPawn(),
		EAttributeModifyType::AddBase);
	if (bHealed)
	{
		LastStandShieldNextReadyTime = CurrentTime + LastStandShieldCooldown;
		UE_LOGFMT(LogGame, Log, "Last Stand restored {HealAmount} health for {Player}.", HealAmount, GetNameSafe(this));
	}

	return bHealed;
}


bool ARoguePlayerState::HasChainLightningUpgrade() const
{
	return HasUpgrade(TEXT("ChainLightning"));
}


float ARoguePlayerState::GetChainLightningRadius() const
{
	const int32 StackCount = GetUpgradeStackCount(TEXT("ChainLightning"));
	if (StackCount <= 0)
	{
		return 0.0f;
	}

	return ChainLightningRadius;
}


float ARoguePlayerState::GetChainLightningDamageCoefficient() const
{
	const int32 StackCount = GetUpgradeStackCount(TEXT("ChainLightning"));
	if (StackCount <= 0)
	{
		return 0.0f;
	}

	return ChainLightningDamageCoefficient + ChainLightningDamageCoefficientPerStack * FMath::Max(0, StackCount - 1);
}


int32 ARoguePlayerState::GetChainLightningTargetCount() const
{
	const int32 StackCount = GetUpgradeStackCount(TEXT("ChainLightning"));
	if (StackCount <= 0)
	{
		return 0;
	}

	return FMath::Max(0, ChainLightningTargetCount + ChainLightningTargetCountPerStack * FMath::Max(0, StackCount - 1));
}


float ARoguePlayerState::GetChainLightningCooldown() const
{
	return ChainLightningCooldown;
}


bool ARoguePlayerState::IsChainLightningReady() const
{
	const UWorld* World = GetWorld();
	const float CurrentTime = World ? World->TimeSeconds : 0.0f;
	return CurrentTime >= ChainLightningNextReadyTime;
}


bool ARoguePlayerState::CanTriggerChainLightningFrom(AActor* SourceActor) const
{
	if (!IsChainLightningReady())
	{
		return false;
	}

	if (!SourceActor)
	{
		return true;
	}

	const UWorld* World = GetWorld();
	const float CurrentTime = World ? World->TimeSeconds : 0.0f;
	return SourceActor != ChainLightningLockedSource.Get() || CurrentTime >= ChainLightningSourceLockUntil;
}


void ARoguePlayerState::CommitChainLightningCooldown(AActor* SourceActor)
{
	const UWorld* World = GetWorld();
	const float CurrentTime = World ? World->TimeSeconds : 0.0f;
	ChainLightningNextReadyTime = CurrentTime + ChainLightningCooldown;

	ChainLightningLockedSource = SourceActor;
	ChainLightningSourceLockUntil = CurrentTime + FMath::Max(ChainLightningCooldown, ChainLightningSourceLockDuration);
}


void ARoguePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARoguePlayerState, Credits);
	DOREPLIFETIME(ARoguePlayerState, Level);
	DOREPLIFETIME(ARoguePlayerState, Experience);
	DOREPLIFETIME(ARoguePlayerState, PendingUpgradeChoices);
	DOREPLIFETIME(ARoguePlayerState, UpgradeStacks);
}
