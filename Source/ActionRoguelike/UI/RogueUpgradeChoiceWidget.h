// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player/RoguePlayerState.h"
#include "RogueUpgradeChoiceWidget.generated.h"

class ARoguePlayerState;
class UButton;
class UHorizontalBox;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpgradeChoiceWidgetClosed);

UCLASS()
class ACTIONROGUELIKE_API URogueUpgradeChoiceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	URogueUpgradeChoiceWidget(const FObjectInitializer& ObjectInitializer);

	void InitializeChoices(ARoguePlayerState* InPlayerState);

	UPROPERTY(BlueprintAssignable)
	FOnUpgradeChoiceWidgetClosed OnChoiceWidgetClosed;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	virtual void NativeConstruct() override;

	UPROPERTY(Transient)
	TObjectPtr<ARoguePlayerState> BoundPlayerState;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> ChoiceList;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Audio")
	TObjectPtr<USoundBase> HoverSound;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Audio")
	TObjectPtr<USoundBase> SelectSound;

	UFUNCTION()
	void SelectChoice0();

	UFUNCTION()
	void SelectChoice1();

	UFUNCTION()
	void SelectChoice2();

	UFUNCTION()
	void HandleChoiceHovered();

	void BuildWidget();

	void RebuildChoices();

	void SelectChoice(int32 ChoiceIndex);

	UButton* CreateChoiceButton(int32 ChoiceIndex, const FRogueUpgradeChoice& Choice);

	FText GetStackDisplayText(const FRogueUpgradeChoice& Choice) const;

	FText GetRarityDisplayText(ERogueUpgradeRarity Rarity) const;

	FText GetUpgradeIconText(const FRogueUpgradeChoice& Choice) const;

	FLinearColor GetRarityColor(ERogueUpgradeRarity Rarity) const;
};
