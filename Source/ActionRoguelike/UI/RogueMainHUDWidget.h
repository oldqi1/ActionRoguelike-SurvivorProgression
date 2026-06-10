// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RogueMainHUDWidget.generated.h"

class ARoguePlayerState;
class UCanvasPanel;
class UProgressBar;
class USizeBox;
class UTextBlock;
class UWidget;
class URogueUpgradeChoiceWidget;
/**
 * 
 */
UCLASS()
class ACTIONROGUELIKE_API URogueMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	/* Primary Canvas to add all projected widgets such as damage numbers */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCanvasPanel> MainCanvasPanel;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UWidget> PlayerHealth_Widget;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UWidget> PlayerRage_Widget;

protected:
	virtual void NativeConstruct() override;

	virtual void NativeDestruct() override;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> ProgressionPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> ExperienceBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ExperienceText;

	UPROPERTY(Transient)
	TObjectPtr<ARoguePlayerState> BoundPlayerState;

	UPROPERTY(Transient)
	TObjectPtr<URogueUpgradeChoiceWidget> UpgradeChoiceWidget;

	bool bWasGamePausedBeforeUpgradeChoice = false;

	bool bAppliedUpgradePause = false;

	UFUNCTION()
	void HandleExperienceChanged(ARoguePlayerState* PlayerState, int32 NewExperience, int32 ExperienceToNextLevel, int32 Delta);

	UFUNCTION()
	void HandleLevelChanged(ARoguePlayerState* PlayerState, int32 NewLevel, int32 OldLevel);

	UFUNCTION()
	void HandleUpgradeChoicesGenerated(ARoguePlayerState* PlayerState, int32 NewLevel);

	UFUNCTION()
	void HandleUpgradeChoiceWidgetClosed();

	void BuildProgressionPanel();

	void BindToPlayerState();

	void UnbindFromPlayerState();

	void RefreshProgression();

	void RefreshUpgradeChoices();

	void ShowUpgradeChoiceWidget();

	void HideUpgradeChoiceWidget();

	void SetUpgradeChoiceInputMode(bool bOpen);
};
