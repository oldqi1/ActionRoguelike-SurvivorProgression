// Fill out your copyright notice in the Description page of Project Settings.


#include "World/RogueChainLightningArcActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueChainLightningArcActor)


ARogueChainLightningArcActor::ARogueChainLightningArcActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Game/NiagaraExamples/StaticMesh/SM_Cylinder_1Unit.SM_Cylinder_1Unit"));
	if (CylinderMeshFinder.Succeeded())
	{
		BeamMesh = CylinderMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BeamMaterialFinder(TEXT("/Game/NiagaraExamples/Materials/MasterMaterials/M_Ribbon_Arc.M_Ribbon_Arc"));
	if (BeamMaterialFinder.Succeeded())
	{
		BeamMaterial = BeamMaterialFinder.Object;
	}

	SetLifeSpan(ArcLifetime);
}


void ARogueChainLightningArcActor::InitializeArc(const FVector& StartLocation, const FVector& EndLocation)
{
	if (!BeamMesh)
	{
		return;
	}

	SetActorLocation(StartLocation);
	SetLifeSpan(ArcLifetime);

	TArray<FVector> Points;
	const int32 SafeSegmentCount = FMath::Max(1, SegmentCount);
	Points.Reserve(SafeSegmentCount + 1);

	const FVector ArcVector = EndLocation - StartLocation;
	const FVector ArcDirection = ArcVector.GetSafeNormal();
	const FVector SideVector = FVector::CrossProduct(ArcDirection, FVector::UpVector).GetSafeNormal();
	const FVector UpJitter = FVector::UpVector * (ArcJitter * 0.35f);

	AddBeamSegment(StartLocation, EndLocation, MainBeamRadius);

	for (int32 PointIndex = 0; PointIndex <= SafeSegmentCount; ++PointIndex)
	{
		const float Alpha = static_cast<float>(PointIndex) / static_cast<float>(SafeSegmentCount);
		FVector Point = FMath::Lerp(StartLocation, EndLocation, Alpha);

		if (PointIndex > 0 && PointIndex < SafeSegmentCount)
		{
			const float SideOffset = FMath::FRandRange(-ArcJitter, ArcJitter);
			const float UpOffset = FMath::FRandRange(-ArcJitter * 0.25f, ArcJitter * 0.45f);
			Point += SideVector * SideOffset + UpJitter + FVector::UpVector * UpOffset;
		}

		Points.Add(Point);
	}

	for (int32 PointIndex = 0; PointIndex < Points.Num() - 1; ++PointIndex)
	{
		AddBeamSegment(Points[PointIndex], Points[PointIndex + 1], BranchBeamRadius);
	}
}


void ARogueChainLightningArcActor::AddBeamSegment(const FVector& SegmentStart, const FVector& SegmentEnd, float Radius)
{
	const FVector SegmentVector = SegmentEnd - SegmentStart;
	const float SegmentLength = SegmentVector.Size();
	if (SegmentLength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	UStaticMeshComponent* BeamComp = NewObject<UStaticMeshComponent>(this);
	if (!BeamComp)
	{
		return;
	}

	BeamComp->SetStaticMesh(BeamMesh);
	if (BeamMaterial)
	{
		UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BeamMaterial, BeamComp);
		if (DynamicMaterial)
		{
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), BeamColor);
			DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), BeamColor);
			DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), BeamColor);
			DynamicMaterial->SetVectorParameterValue(TEXT("Tint"), BeamColor);
			DynamicMaterial->SetVectorParameterValue(TEXT("ArcColor"), BeamColor);
			BeamComp->SetMaterial(0, DynamicMaterial);
		}
		else
		{
			BeamComp->SetMaterial(0, BeamMaterial);
		}
	}

	BeamComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BeamComp->SetCastShadow(false);
	BeamComp->SetGenerateOverlapEvents(false);
	BeamComp->SetupAttachment(SceneRoot);
	BeamComp->RegisterComponent();

	const FVector SegmentMidpoint = SegmentStart + SegmentVector * 0.5f;
	const FQuat SegmentRotation = FRotationMatrix::MakeFromZ(SegmentVector.GetSafeNormal()).ToQuat();
	BeamComp->SetWorldLocationAndRotation(SegmentMidpoint, SegmentRotation);

	const FBoxSphereBounds MeshBounds = BeamMesh->GetBounds();
	const FVector MeshExtent = MeshBounds.BoxExtent;
	const float MeshRadius = FMath::Max(1.0f, FMath::Max(MeshExtent.X, MeshExtent.Y));
	const float MeshLength = FMath::Max(1.0f, MeshExtent.Z * 2.0f);
	BeamComp->SetWorldScale3D(FVector(Radius / MeshRadius, Radius / MeshRadius, SegmentLength / MeshLength));

	BeamSegments.Add(BeamComp);
}
