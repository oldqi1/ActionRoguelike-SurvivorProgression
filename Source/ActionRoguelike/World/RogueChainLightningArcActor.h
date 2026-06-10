// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RogueChainLightningArcActor.generated.h"

class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class ACTIONROGUELIKE_API ARogueChainLightningArcActor : public AActor
{
	GENERATED_BODY()

public:
	ARogueChainLightningArcActor();

	void InitializeArc(const FVector& StartLocation, const FVector& EndLocation);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	TObjectPtr<UStaticMesh> BeamMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	TObjectPtr<UMaterialInterface> BeamMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	FLinearColor BeamColor = FLinearColor(0.02f, 0.55f, 1.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals", meta = (ClampMin = "1"))
	int32 SegmentCount = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals", meta = (ClampMin = "0.1"))
	float MainBeamRadius = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals", meta = (ClampMin = "0.1"))
	float BranchBeamRadius = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals", meta = (ClampMin = "0.0"))
	float ArcJitter = 58.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals", meta = (ClampMin = "0.01"))
	float ArcLifetime = 0.18f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> BeamSegments;

	void AddBeamSegment(const FVector& SegmentStart, const FVector& SegmentEnd, float Radius);
};
