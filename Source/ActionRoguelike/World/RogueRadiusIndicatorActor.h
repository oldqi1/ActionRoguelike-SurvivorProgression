// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RogueRadiusIndicatorActor.generated.h"

class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class ACTIONROGUELIKE_API ARogueRadiusIndicatorActor : public AActor
{
	GENERATED_BODY()

public:
	ARogueRadiusIndicatorActor();

	void InitializeIndicator(float InRadius, float InLifetime);

	virtual void Tick(float DeltaSeconds) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> RootSceneComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RingMeshComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> SphereMeshComp;

	UPROPERTY(EditDefaultsOnly, Category = "Indicator|Ring")
	TObjectPtr<UStaticMesh> RingMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Indicator|Ring")
	TObjectPtr<UMaterialInterface> RingMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Indicator|Sphere")
	bool bShowSphereVolume = true;

	UPROPERTY(EditDefaultsOnly, Category = "Indicator|Sphere")
	TObjectPtr<UStaticMesh> SphereMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Indicator|Sphere")
	TObjectPtr<UMaterialInterface> SphereMaterial;

	UPROPERTY(ReplicatedUsing=OnRep_IndicatorRadius)
	float IndicatorRadius = 0.0f;

	UPROPERTY(ReplicatedUsing=OnRep_IndicatorLifetime)
	float IndicatorLifetime = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Indicator", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float InitialRadiusAlpha = 0.15f;

	float IndicatorElapsedTime = 0.0f;

	UFUNCTION()
	void OnRep_IndicatorRadius();

	UFUNCTION()
	void OnRep_IndicatorLifetime();

	void ApplyAnimatedScale();

	float GetMeshRadius(const UStaticMeshComponent* InMeshComp) const;
};
