// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueAttributeSet.h"

#include "RogueActionComponent.h"
#include "SharedGameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"


URogueMonsterAttributeSet::URogueMonsterAttributeSet()
{
	// Override with lower default attack damage than players
	AttackDamage = FRogueAttribute(10);
		
	MoveSpeed = FRogueAttribute(450);
}

// -- Health Attribute Set -- //

void URogueHealthAttributeSet::OnRep_Health(FRogueAttribute OldValue)
{
	float NewValue = Health.GetValue();

	FAttributeModification Modification;
	Modification.AttributeTag = SharedGameplayTags::Attribute_Health;
	Modification.Magnitude = Health.GetValue() - OldValue.GetValue();
	Modification.TargetComp = OwningComp;

	OwningComp->BroadcastAttributeChanged(SharedGameplayTags::Attribute_Health, NewValue, Modification);
}


void URogueHealthAttributeSet::OnRep_HealthMax(FRogueAttribute OldValue)
{
	const float NewValue = HealthMax.GetValue();

	FAttributeModification Modification;
	Modification.AttributeTag = SharedGameplayTags::Attribute_HealthMax;
	Modification.Magnitude = HealthMax.GetValue() - OldValue.GetValue();
	Modification.TargetComp = OwningComp;

	OwningComp->BroadcastAttributeChanged(SharedGameplayTags::Attribute_HealthMax, NewValue, Modification);

	// Existing health widgets often only subscribe to Attribute.Health and then query HealthMax.
	// Broadcast Health again so those widgets refresh after the replicated max value changes.
	FAttributeModification HealthRefreshModification;
	HealthRefreshModification.AttributeTag = SharedGameplayTags::Attribute_Health;
	HealthRefreshModification.Magnitude = 0.0f;
	HealthRefreshModification.TargetComp = OwningComp;
	OwningComp->BroadcastAttributeChanged(SharedGameplayTags::Attribute_Health, Health.GetValue(), HealthRefreshModification);
}


void URogueHealthAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URogueHealthAttributeSet, Health);
	DOREPLIFETIME(URogueHealthAttributeSet, HealthMax);
}

// -- Pawn Attribute Set -- //

void URoguePawnAttributeSet::ApplyMovementSpeed()
{
	// Assume all Pawns are Characters with CMC (may change with Mover 2.0)
	ACharacter* OwningCharacter = CastChecked<ACharacter>(OwningComp->GetOwner());
	OwningCharacter->GetCharacterMovement()->MaxWalkSpeed = MoveSpeed.GetValue();
}
