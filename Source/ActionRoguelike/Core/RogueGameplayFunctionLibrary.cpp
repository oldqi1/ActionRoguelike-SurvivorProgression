// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/RogueGameplayFunctionLibrary.h"

#include "ActionRoguelike.h"
#include "RogueGameplayInterface.h"
#include "RogueGameModeBase.h"
#include "ShaderPipelineCache.h"
#include "SharedGameplayTags.h"
#include "ActionSystem/RogueActionComponent.h"
#include "ActionSystem/RogueActionSystemInterface.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Player/RoguePlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueGameplayFunctionLibrary)


namespace
{
	void TryTriggerChainLightningFromDirectHit(AActor* DamageCauser, AActor* TargetActor, const FGameplayTagContainer& InContextTags)
	{
		if (!InContextTags.IsEmpty())
		{
			return;
		}

		APawn* DamageCauserPawn = Cast<APawn>(DamageCauser);
		ARoguePlayerState* DamageCauserPlayerState = DamageCauserPawn ? DamageCauserPawn->GetPlayerState<ARoguePlayerState>() : nullptr;
		UWorld* World = DamageCauser ? DamageCauser->GetWorld() : nullptr;
		ARogueGameModeBase* GameMode = World ? World->GetAuthGameMode<ARogueGameModeBase>() : nullptr;
		if (DamageCauserPlayerState && DamageCauserPlayerState->HasChainLightningUpgrade() && DamageCauserPlayerState->CanTriggerChainLightningFrom(TargetActor) && GameMode)
		{
			GameMode->ApplyChainLightningUpgrade(TargetActor, DamageCauserPawn, DamageCauserPlayerState);
		}
	}
}


URogueActionComponent* URogueGameplayFunctionLibrary::GetActionComponentFromActor(AActor* FromActor)
{
	if (!IsValid(FromActor))
	{
		// ...could easily pass in nullptr from Blueprint
		UE_LOG(LogGame, Warning, TEXT("Attempting to get Action Component from invalid or nullptr Actor: %s"), *GetNameSafe(FromActor));
		return nullptr;
	}

	if (URogueActionComponent* ActionComp = FromActor->FindComponentByClass<URogueActionComponent>())
	{
		return ActionComp;
	}
	
	// Note: Cast<T> on interface only works if the interface was implemented on the Actor in C++
	// For BP implemented we should change this code to call Execute_GetActionComponent instead...
	const IRogueActionSystemInterface* ASI = Cast<IRogueActionSystemInterface>(FromActor);
	if (ASI)
	{
		return ASI->GetActionComponent();
	}
	/*if (InActor && InActor->Implements<URogueGameplayInterface>()) // example reference for a BP interface
	{
		URogueActionComponent* ActionComp = nullptr;
		if (IRogueGameplayInterface::Execute_GetActionComponent(InActor, ActionComp))
		{
			return ActionComp;
		}
	}*/

	// Fallback when interface is missing
	return FromActor->FindComponentByClass<URogueActionComponent>();
}

bool URogueGameplayFunctionLibrary::IsAlive(AActor* InActor)
{
	// Allow nullptr as BP may pass in non exist
	if (!IsValid(InActor))
	{
		return false;
	}
	
	URogueActionComponent* ActionComp = GetActionComponentFromActor(InActor);
	if (!ActionComp)
	{
		UE_LOG(LogGame, Verbose, TEXT("Checking IsAlive on Actor without ActionComponent: %s"), *GetNameSafe(InActor));
		return false;
	}

	FRogueAttribute* FoundAttribute = ActionComp->GetAttribute(SharedGameplayTags::Attribute_Health);
	if (!FoundAttribute)
	{
		UE_LOG(LogGame, Verbose, TEXT("Checking IsAlive on Actor without Health attribute: %s"), *GetNameSafe(InActor));
		return false;
	}
		
	return FoundAttribute->GetValue() > 0.0f;
}


bool URogueGameplayFunctionLibrary::KillActor(AActor* InActor)
{
	URogueActionComponent* ActionComp = URogueActionComponent::GetActionComponent(InActor);
	const FRogueAttribute* HealthMaxAttribute = ActionComp->GetAttribute(SharedGameplayTags::Attribute_HealthMax);
	
	return ActionComp->ApplyAttributeChange(SharedGameplayTags::Attribute_Health, -HealthMaxAttribute->GetValue(),
		InActor, EAttributeModifyType::AddBase);
}


bool URogueGameplayFunctionLibrary::IsFullHealth(AActor* InActor)
{
	URogueActionComponent* ActionComp = URogueActionComponent::GetActionComponent(InActor);

	const FRogueAttribute* HealthAttribute = ActionComp->GetAttribute(SharedGameplayTags::Attribute_Health);
	const FRogueAttribute* HealthMaxAttribute = ActionComp->GetAttribute(SharedGameplayTags::Attribute_HealthMax);

	return HealthAttribute->GetValue() >= HealthMaxAttribute->GetValue();
}


bool URogueGameplayFunctionLibrary::ApplyDamage(AActor* DamageCauser, AActor* TargetActor, float DamageCoefficient, FGameplayTagContainer InContextTags)
{
	if (!CanApplyDamage(DamageCauser, TargetActor, InContextTags))
	{
		return false;
	}
	
	URogueActionComponent* InstigatorComp = GetActionComponentFromActor(DamageCauser);
	// Blueprint might be missing the component for now
	if (InstigatorComp == nullptr)
	{
		UE_LOG(LogGame, Warning, TEXT("Actor (%s) has no ActionComponent."), *DamageCauser->GetName());
		return false;
	}

	const FRogueAttribute* FoundAttribute = InstigatorComp->GetAttribute(SharedGameplayTags::Attribute_AttackDamage);
	float TotalDamage = DamageCoefficient;
	if (FoundAttribute)
	{
		// Coefficient is a %, to scale all out damage off the instigator's base attack damage.
		TotalDamage = FoundAttribute->GetValue() * (DamageCoefficient * 0.01f);
	}
	else
	{
		UE_LOG(LogGame, VeryVerbose, TEXT("Actor (%s) has no AttackDamage attribute. Using coefficient %.2f as flat damage."),
			*GetNameSafe(DamageCauser), DamageCoefficient);
	}

	URogueActionComponent* VictimComp = GetActionComponentFromActor(TargetActor);
	if (VictimComp == nullptr)
	{
		return false;
	}

	FAttributeModification AttriMod = FAttributeModification(
		SharedGameplayTags::Attribute_Health,
		-TotalDamage, // Make sure we apply a negative amount to the Health
		VictimComp,
		DamageCauser,
		EAttributeModifyType::AddBase,
		InContextTags);

	const bool bApplied = VictimComp->ApplyAttributeChange(AttriMod);
	if (!bApplied)
	{
		return false;
	}

	return true;
}


bool URogueGameplayFunctionLibrary::ApplyDirectionalDamage(AActor* DamageCauser, AActor* TargetActor, float DamageCoefficient, const FHitResult& HitResult, FGameplayTagContainer InContextTags)
{
	if (!CanApplyDamage(DamageCauser, TargetActor, InContextTags))
	{
		return false;
	}
	
	if (ApplyDamage(DamageCauser, TargetActor, DamageCoefficient, InContextTags))
	{
		TryTriggerChainLightningFromDirectHit(DamageCauser, TargetActor, InContextTags);

		UPrimitiveComponent* HitComp = HitResult.GetComponent();

		ensure(!HitResult.Normal.IsZero());
		// @todo: allow configuration for impulse strength
		const FVector Impulse = HitResult.Normal * 100000.f;

		bool bHandled = false;
		// Special case to allow Corpses as the hit result that do not belong to the original receiver of the damage...
		if (IRogueGameplayInterface* Interface = Cast<IRogueGameplayInterface>(TargetActor))
		{
			bHandled = Interface->AddImpulseAtLocationCustom(Impulse, HitResult.ImpactPoint, HitResult.BoneName);
		}
		
		if (!bHandled && HitComp && HitComp->bApplyImpulseOnDamage && HitComp->IsSimulatingPhysics(HitResult.BoneName))
		{
			HitComp->AddImpulseAtLocation(Impulse, HitResult.ImpactPoint, HitResult.BoneName);
			// Alternative for more consistent defaults as it ignores Mass
			//HitComp->AddVelocityChangeImpulseAtLocation(Direction * 3000000.f, HitResult.ImpactPoint, HitResult.BoneName);
		}
		return true;
	}

	return false;
}

bool URogueGameplayFunctionLibrary::CanApplyDamage(AActor* DamageCauser, AActor* TargetActor, FGameplayTagContainer InContextTags)
{
	// @todo: verify if damagecauser (aka instigator on projectiles) isnt sometimes nullptr on clients
	if (!IsValid(DamageCauser) || !IsValid(TargetActor))
	{
		UE_LOG(LogGame, Warning, TEXT("CanApplyDamage rejected invalid actor. DamageCauser: %s, TargetActor: %s"),
			*GetNameSafe(DamageCauser), *GetNameSafe(TargetActor));
		return false;
	}

	if (!TargetActor->CanBeDamaged())
	{
		return false;
	}

	URogueActionComponent* TargetActionComp = GetActionComponentFromActor(TargetActor);
	if (!TargetActionComp)
	{
		UE_LOG(LogGame, VeryVerbose, TEXT("CanApplyDamage rejected actor without ActionComponent: %s"), *GetNameSafe(TargetActor));
		return false;
	}

	if (!TargetActionComp->GetAttribute(SharedGameplayTags::Attribute_Health))
	{
		UE_LOG(LogGame, VeryVerbose, TEXT("CanApplyDamage rejected actor without Health attribute: %s"), *GetNameSafe(TargetActor));
		return false;
	}

	return true;
}

/*
bool URogueGameplayFunctionLibrary::ApplyRadialDamage(AActor* DamageCauser, FVector Origin, float DamageRadius, float DamageCoefficient)
{
	UWorld* World = DamageCauser->GetWorld();
	// do async overlap to find list of potential victims
	// only test for actors with action component / or gameobject interface
	// 2nd pass is another async trace for occlusion tests (optional)
	// GameObject interface: GetDamageTraceLocations(TArray<FVector>& OutLocations);
	// allow objects or pawns to specify which locations they want to use for occlusion tests, for example
	// head, spine, hands, legs on a character
	// for large and oddly shapes objects, it can also use "nearest collision point" from origin rather than the actor location

	FCollisionShape Shape;
	Shape.SetSphere(DamageRadius);

	FCollisionQueryParams Params;
	//Params.MobilityType = EQueryMobilityType::Dynamic;

	FCollisionResponseParams ResponseParams;
*/
	/*
	FOverlapDelegate* Delegate;
	Delegate->BindLambda([](const FTraceHandle& Handle, FOverlapDatum& Datum)
		{
			// ... called when ready
		});*/

	//FTraceHandle Handle; // @todo: can pass in additional params here if needed for multi-pass stuff
	// @todo:need to pass "this", which wont work in static function
/*
	// Fill any useful dmg info
	FDamageInfo Info;
	Info.DamageInstigator = DamageCauser;
	Info.AttackDamage = 0.0f; // InstigatorDmg * (DamageCoefficient*0.01f)
	
	FOverlapDelegate Delegate = FOverlapDelegate::CreateUObject(this, &URogueGameplayFunctionLibrary::OnDamageOverlapComplete, Info);

	World->AsyncOverlapByChannel(Origin, FQuat::Identity, COLLISION_PROJECTILE,
		Shape, Params, ResponseParams, &Delegate);



	return false;
}
*/

/*	
void URogueGameplayFunctionLibrary::OnDamageOverlapComplete(const FTraceHandle& TraceHandle, FOverlapDatum& OverlapDatum, FDamageInfo DamageInfo)
{
	// if second pass w/ line traces is async too, we are two frames 'behind' the initial request for damage.

	// @todo: iterate the victims to apply damage: OverlapDatum.OutOverlaps

	check(DamageInfo.DamageInstigator.Get());

	for (FOverlapResult& Overlap : OverlapDatum.OutOverlaps)
	{
		ApplyDamage(DamageInfo.DamageInstigator.Get(), Overlap.GetActor(), DamageInfo.AttackDamage);
	}
}
*/

int32 URogueGameplayFunctionLibrary::GetRemainingBundledPSOs()
{
	// Counts Bundled PSOs remaining, exposed for UI access
	return FShaderPipelineCache::NumPrecompilesRemaining();
}
