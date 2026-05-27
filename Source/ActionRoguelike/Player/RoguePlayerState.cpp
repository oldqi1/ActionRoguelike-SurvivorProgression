// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/RoguePlayerState.h"

#include "ActionRoguelike.h"
#include "SaveSystem/RogueSaveGame.h"
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


void ARoguePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARoguePlayerState, Credits);
	DOREPLIFETIME(ARoguePlayerState, Level);
	DOREPLIFETIME(ARoguePlayerState, Experience);
}
