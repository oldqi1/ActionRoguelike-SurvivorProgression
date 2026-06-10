// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoguePickupPreviewActor.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ERoguePickupPreviewType : uint8
{
	Coin,
	Experience
};

UCLASS()
class ACTIONROGUELIKE_API ARoguePickupPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	ARoguePickupPreviewActor();

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	UPROPERTY(EditAnywhere, Category = "Pickup Preview")
	ERoguePickupPreviewType PreviewType = ERoguePickupPreviewType::Experience;

	UPROPERTY(EditAnywhere, Category = "Pickup Preview", meta = (ClampMin = "0.01"))
	float ExperiencePreviewScale = 0.35f;

	void RefreshPreviewMesh();
};
