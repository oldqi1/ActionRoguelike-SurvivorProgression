// Fill out your copyright notice in the Description page of Project Settings.


#include "RoguePickupActor_Credits.h"
#include "Player/RoguePlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RoguePickupActor_Credits)


#define LOCTEXT_NAMESPACE "InteractableActors"


ARoguePickupActor_Credits::ARoguePickupActor_Credits()
{
	CreditsAmount = 80;
}


void ARoguePickupActor_Credits::Interact_Implementation(AController* InstigatorController)
{
	if (ARoguePlayerState* PS = InstigatorController->GetPlayerState<ARoguePlayerState>())
	{
		PS->AddCredits(CreditsAmount);
		HideAndCooldown();
	}
}


FText ARoguePickupActor_Credits::GetInteractText_Implementation(AController* InstigatorController)
{
	return FText::Format(LOCTEXT("Credits_InteractMessage", "Pick up {0} credits."), FText::AsNumber(CreditsAmount));
}


#undef LOCTEXT_NAMESPACE
