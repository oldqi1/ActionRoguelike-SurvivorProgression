// Fill out your copyright notice in the Description page of Project Settings.


#include "RoguePickupPreviewActor.h"

#include "Components/StaticMeshComponent.h"
#include "Core/RogueDeveloperSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RoguePickupPreviewActor)


ARoguePickupPreviewActor::ARoguePickupPreviewActor()
{
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(MeshComp);
}

void ARoguePickupPreviewActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RefreshPreviewMesh();
}

#if WITH_EDITOR
void ARoguePickupPreviewActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RefreshPreviewMesh();
}
#endif

void ARoguePickupPreviewActor::RefreshPreviewMesh()
{
	const URogueDeveloperSettings* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings)
	{
		return;
	}

	MeshComp->SetMaterial(0, nullptr);

	if (PreviewType == ERoguePickupPreviewType::Experience)
	{
		MeshComp->SetStaticMesh(Settings->PickupExperienceMesh.LoadSynchronous());
		if (UMaterialInterface* Material = Settings->PickupExperienceMaterial.LoadSynchronous())
		{
			MeshComp->SetMaterial(0, Material);
		}
		MeshComp->SetRelativeScale3D(FVector(ExperiencePreviewScale));
		return;
	}

	MeshComp->SetStaticMesh(Settings->PickupCoinMesh.LoadSynchronous());
	MeshComp->SetRelativeScale3D(FVector::OneVector);
}
