// Fill out your copyright notice in the Description page of Project Settings.


#include "World/RogueRadiusIndicatorActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueRadiusIndicatorActor)


ARogueRadiusIndicatorActor::ARogueRadiusIndicatorActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	RootSceneComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComp"));
	SetRootComponent(RootSceneComp);

	RingMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RingMeshComp"));
	RingMeshComp->SetupAttachment(RootSceneComp);
	RingMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RingMeshComp->SetGenerateOverlapEvents(false);
	RingMeshComp->SetCastShadow(false);
	RingMeshComp->SetAffectDistanceFieldLighting(false);
	RingMeshComp->SetVisibleInRayTracing(false);
	RingMeshComp->SetForceDisableNanite(true);
	RingMeshComp->bReceivesDecals = false;
	RingMeshComp->bDisallowNanite = true;

	SphereMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SphereMeshComp"));
	SphereMeshComp->SetupAttachment(RootSceneComp);
	SphereMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SphereMeshComp->SetGenerateOverlapEvents(false);
	SphereMeshComp->SetCastShadow(false);
	SphereMeshComp->SetAffectDistanceFieldLighting(false);
	SphereMeshComp->SetVisibleInRayTracing(false);
	SphereMeshComp->SetForceDisableNanite(true);
	SphereMeshComp->bReceivesDecals = false;
	SphereMeshComp->bDisallowNanite = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> RingMeshFinder(TEXT("/Game/ExampleContent/Meshes/SM_Radius_Ring.SM_Radius_Ring"));
	if (RingMeshFinder.Succeeded())
	{
		RingMesh = RingMeshFinder.Object;
		RingMeshComp->SetStaticMesh(RingMesh);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RingMaterialFinder(TEXT("/Game/ExampleContent/Materials/M_Radius_Glow.M_Radius_Glow"));
	if (RingMaterialFinder.Succeeded())
	{
		RingMaterial = RingMaterialFinder.Object;
		RingMeshComp->SetMaterial(0, RingMaterial);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Game/ExampleContent/Meshes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		SphereMesh = SphereMeshFinder.Object;
		SphereMeshComp->SetStaticMesh(SphereMesh);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SphereMaterialFinder(TEXT("/Game/ExampleContent/Materials/M_Radius_Glow.M_Radius_Glow"));
	if (SphereMaterialFinder.Succeeded())
	{
		SphereMaterial = SphereMaterialFinder.Object;
		SphereMeshComp->SetMaterial(0, SphereMaterial);
	}
}


void ARogueRadiusIndicatorActor::InitializeIndicator(float InRadius, float InLifetime)
{
	IndicatorRadius = FMath::Max(0.0f, InRadius);
	IndicatorLifetime = FMath::Max(0.05f, InLifetime);
	IndicatorElapsedTime = 0.0f;
	ApplyAnimatedScale();
	SetLifeSpan(IndicatorLifetime);
}


void ARogueRadiusIndicatorActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	IndicatorElapsedTime += DeltaSeconds;
	ApplyAnimatedScale();
}


void ARogueRadiusIndicatorActor::OnRep_IndicatorRadius()
{
	IndicatorElapsedTime = 0.0f;
	ApplyAnimatedScale();
}


void ARogueRadiusIndicatorActor::OnRep_IndicatorLifetime()
{
	IndicatorElapsedTime = 0.0f;
	SetLifeSpan(FMath::Max(0.05f, IndicatorLifetime));
	ApplyAnimatedScale();
}


void ARogueRadiusIndicatorActor::ApplyAnimatedScale()
{
	const float SafeInitialAlpha = FMath::Clamp(InitialRadiusAlpha, 0.0f, 1.0f);
	const float LifetimeAlpha = IndicatorLifetime > KINDA_SMALL_NUMBER
		? FMath::Clamp(IndicatorElapsedTime / IndicatorLifetime, 0.0f, 1.0f)
		: 1.0f;
	const float EasedAlpha = FMath::InterpEaseOut(SafeInitialAlpha, 1.0f, LifetimeAlpha, 2.0f);
	const float CurrentRadius = IndicatorRadius * EasedAlpha;

	const float RingMeshRadius = GetMeshRadius(RingMeshComp);
	const float RingScale = RingMeshRadius > KINDA_SMALL_NUMBER
		? CurrentRadius / RingMeshRadius
		: FMath::Max(0.1f, CurrentRadius / 100.0f);
	RingMeshComp->SetRelativeScale3D(FVector(RingScale, RingScale, 1.0f));

	SphereMeshComp->SetVisibility(bShowSphereVolume);
	if (bShowSphereVolume)
	{
		const float SphereMeshRadius = GetMeshRadius(SphereMeshComp);
		const float SphereScale = SphereMeshRadius > KINDA_SMALL_NUMBER
			? CurrentRadius / SphereMeshRadius
			: FMath::Max(0.1f, CurrentRadius / 100.0f);
		SphereMeshComp->SetRelativeScale3D(FVector(SphereScale));
	}
}


float ARogueRadiusIndicatorActor::GetMeshRadius(const UStaticMeshComponent* InMeshComp) const
{
	if (!InMeshComp || !InMeshComp->GetStaticMesh())
	{
		return 0.0f;
	}

	const FVector MeshBounds = InMeshComp->GetStaticMesh()->GetBounds().BoxExtent;
	return FMath::Max3(MeshBounds.X, MeshBounds.Y, MeshBounds.Z);
}


void ARogueRadiusIndicatorActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARogueRadiusIndicatorActor, IndicatorRadius);
	DOREPLIFETIME(ARogueRadiusIndicatorActor, IndicatorLifetime);
}
