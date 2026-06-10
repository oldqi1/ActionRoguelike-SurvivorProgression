// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueMainHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Components/WidgetSwitcherSlot.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Player/RoguePlayerState.h"
#include "ActionRoguelike.h"
#include "Styling/SlateTypes.h"
#include "UI/RogueUpgradeChoiceWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueMainHUDWidget)


void URogueMainHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuildProgressionPanel();
	BindToPlayerState();
	RefreshProgression();
	RefreshUpgradeChoices();
}

void URogueMainHUDWidget::NativeDestruct()
{
	HideUpgradeChoiceWidget();
	UnbindFromPlayerState();

	Super::NativeDestruct();
}

void URogueMainHUDWidget::HandleExperienceChanged(ARoguePlayerState* PlayerState, int32 NewExperience, int32 ExperienceToNextLevel, int32 Delta)
{
	RefreshProgression();
}

void URogueMainHUDWidget::HandleLevelChanged(ARoguePlayerState* PlayerState, int32 NewLevel, int32 OldLevel)
{
	RefreshProgression();
}

void URogueMainHUDWidget::HandleUpgradeChoicesGenerated(ARoguePlayerState* PlayerState, int32 NewLevel)
{
	RefreshUpgradeChoices();
}

void URogueMainHUDWidget::HandleUpgradeChoiceWidgetClosed()
{
	UpgradeChoiceWidget = nullptr;
	SetUpgradeChoiceInputMode(false);
}

void URogueMainHUDWidget::BuildProgressionPanel()
{
	if (!MainCanvasPanel || ProgressionPanel)
	{
		return;
	}

	ProgressionPanel = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ProgressionPanel"));
	ProgressionPanel->SetWidthOverride(240.0f);
	ProgressionPanel->SetHeightOverride(30.0f);

	UBorder* PanelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ProgressionBackground"));
	PanelBackground->SetBrushColor(FLinearColor(0.006f, 0.008f, 0.010f, 0.58f));
	PanelBackground->SetPadding(FMargin(5.0f, 3.0f));
	ProgressionPanel->SetContent(PanelBackground);

	UVerticalBox* VerticalBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ProgressionStack"));
	PanelBackground->SetContent(VerticalBox);

	UHorizontalBox* HeaderBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ProgressionHeader"));
	if (UVerticalBoxSlot* HeaderSlot = VerticalBox->AddChildToVerticalBox(HeaderBox))
	{
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 2.0f));
	}

	LevelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LevelText"));
	FSlateFontInfo LevelFont = LevelText->GetFont();
	LevelFont.Size = 11;
	LevelText->SetFont(LevelFont);
	LevelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.90f, 1.0f, 1.0f)));
	LevelText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	if (UHorizontalBoxSlot* LevelSlot = HeaderBox->AddChildToHorizontalBox(LevelText))
	{
		LevelSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
	}

	ExperienceText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ExperienceText"));
	FSlateFontInfo ExperienceFont = ExperienceText->GetFont();
	ExperienceFont.Size = 10;
	ExperienceText->SetFont(ExperienceFont);
	ExperienceText->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.86f, 0.90f, 1.0f)));
	ExperienceText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	HeaderBox->AddChildToHorizontalBox(ExperienceText);

	USizeBox* ExperienceBarBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ExperienceBarBox"));
	ExperienceBarBox->SetHeightOverride(6.0f);

	ExperienceBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ExperienceBar"));
	ExperienceBar->SetFillColorAndOpacity(FLinearColor(0.12f, 0.62f, 0.95f, 1.0f));
	ExperienceBarBox->SetContent(ExperienceBar);
	VerticalBox->AddChildToVerticalBox(ExperienceBarBox);

	UWidget* AnchorWidget = PlayerRage_Widget ? PlayerRage_Widget.Get() : PlayerHealth_Widget.Get();
	UPanelWidget* AnchorParent = AnchorWidget ? AnchorWidget->GetParent() : nullptr;
	if (UVerticalBox* StatusStack = Cast<UVerticalBox>(AnchorParent))
	{
		if (UVerticalBoxSlot* PanelSlot = StatusStack->AddChildToVerticalBox(ProgressionPanel))
		{
			PanelSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
		}
		return;
	}

	if (UHorizontalBox* StatusRow = Cast<UHorizontalBox>(AnchorParent))
	{
		if (UHorizontalBoxSlot* PanelSlot = StatusRow->AddChildToHorizontalBox(ProgressionPanel))
		{
			PanelSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
		}
		return;
	}

	if (MainCanvasPanel)
	{
		UCanvasPanelSlot* PanelSlot = MainCanvasPanel->AddChildToCanvas(ProgressionPanel);
		if (PanelSlot)
		{
			PanelSlot->SetAnchors(FAnchors(0.0f, 1.0f));
			PanelSlot->SetAlignment(FVector2D(0.0f, 1.0f));

			if (UCanvasPanelSlot* AnchorCanvasSlot = AnchorWidget ? Cast<UCanvasPanelSlot>(AnchorWidget->Slot) : nullptr)
			{
				const FVector2D AnchorPosition = AnchorCanvasSlot->GetPosition();
				const FVector2D AnchorSize = AnchorCanvasSlot->GetSize();
				PanelSlot->SetPosition(AnchorPosition + FVector2D(0.0f, AnchorSize.Y + 4.0f));
			}
			else
			{
				PanelSlot->SetPosition(FVector2D(34.0f, -98.0f));
			}

			PanelSlot->SetAutoSize(true);
		}
	}
}

void URogueMainHUDWidget::BindToPlayerState()
{
	UnbindFromPlayerState();

	APlayerController* OwningPC = GetOwningPlayer();
	if (!OwningPC)
	{
		return;
	}

	BoundPlayerState = OwningPC->GetPlayerState<ARoguePlayerState>();
	if (!BoundPlayerState)
	{
		return;
	}

	BoundPlayerState->OnExperienceChanged.AddDynamic(this, &ThisClass::HandleExperienceChanged);
	BoundPlayerState->OnLevelChanged.AddDynamic(this, &ThisClass::HandleLevelChanged);
	BoundPlayerState->OnUpgradeChoicesGenerated.AddDynamic(this, &ThisClass::HandleUpgradeChoicesGenerated);
}

void URogueMainHUDWidget::UnbindFromPlayerState()
{
	if (!BoundPlayerState)
	{
		return;
	}

	BoundPlayerState->OnExperienceChanged.RemoveDynamic(this, &ThisClass::HandleExperienceChanged);
	BoundPlayerState->OnLevelChanged.RemoveDynamic(this, &ThisClass::HandleLevelChanged);
	BoundPlayerState->OnUpgradeChoicesGenerated.RemoveDynamic(this, &ThisClass::HandleUpgradeChoicesGenerated);
	BoundPlayerState = nullptr;
}

void URogueMainHUDWidget::RefreshProgression()
{
	if (!BoundPlayerState)
	{
		BindToPlayerState();
	}

	if (!BoundPlayerState || !LevelText || !ExperienceText || !ExperienceBar)
	{
		return;
	}

	const int32 Level = BoundPlayerState->GetPlayerLevel();
	const int32 Experience = BoundPlayerState->GetExperience();
	const int32 ExperienceToNextLevel = FMath::Max(1, BoundPlayerState->GetExperienceToNextLevel());
	const float ExperiencePercent = static_cast<float>(Experience) / static_cast<float>(ExperienceToNextLevel);

	LevelText->SetText(FText::Format(NSLOCTEXT("RogueHUD", "LevelFormat", "LV {0}"), FText::AsNumber(Level)));
	ExperienceText->SetText(FText::Format(NSLOCTEXT("RogueHUD", "ExperienceFormat", "{0}/{1} XP"), FText::AsNumber(Experience), FText::AsNumber(ExperienceToNextLevel)));
	ExperienceBar->SetPercent(FMath::Clamp(ExperiencePercent, 0.0f, 1.0f));
}

void URogueMainHUDWidget::RefreshUpgradeChoices()
{
	if (!BoundPlayerState)
	{
		BindToPlayerState();
	}

	if (!BoundPlayerState)
	{
		HideUpgradeChoiceWidget();
		return;
	}

	const TArray<FRogueUpgradeChoice> Choices = BoundPlayerState->GetPendingUpgradeChoices();
	Choices.Num() > 0 ? ShowUpgradeChoiceWidget() : HideUpgradeChoiceWidget();
}

void URogueMainHUDWidget::ShowUpgradeChoiceWidget()
{
	if (!BoundPlayerState || UpgradeChoiceWidget)
	{
		return;
	}

	UpgradeChoiceWidget = CreateWidget<URogueUpgradeChoiceWidget>(GetOwningPlayer(), URogueUpgradeChoiceWidget::StaticClass());
	if (!UpgradeChoiceWidget)
	{
		return;
	}

	UpgradeChoiceWidget->InitializeChoices(BoundPlayerState);
	UpgradeChoiceWidget->OnChoiceWidgetClosed.AddDynamic(this, &ThisClass::HandleUpgradeChoiceWidgetClosed);
	UpgradeChoiceWidget->AddToViewport(200);
	UE_LOGFMT(LogGame, Log, "Upgrade choice widget shown with {ChoiceCount} choices.", BoundPlayerState->GetPendingUpgradeChoices().Num());
	SetUpgradeChoiceInputMode(true);
}

void URogueMainHUDWidget::HideUpgradeChoiceWidget()
{
	if (UpgradeChoiceWidget)
	{
		UpgradeChoiceWidget->OnChoiceWidgetClosed.RemoveDynamic(this, &ThisClass::HandleUpgradeChoiceWidgetClosed);
		UpgradeChoiceWidget->RemoveFromParent();
		UpgradeChoiceWidget = nullptr;
	}

	SetUpgradeChoiceInputMode(false);
}

void URogueMainHUDWidget::SetUpgradeChoiceInputMode(bool bOpen)
{
	APlayerController* OwningPC = GetOwningPlayer();
	if (!OwningPC)
	{
		return;
	}

	if (bOpen)
	{
		OwningPC->bShowMouseCursor = true;
		FInputModeUIOnly InputMode;
		OwningPC->SetInputMode(InputMode);

		if (UWorld* World = GetWorld(); World && World->IsNetMode(NM_Standalone))
		{
			bWasGamePausedBeforeUpgradeChoice = UGameplayStatics::IsGamePaused(this);
			if (!bWasGamePausedBeforeUpgradeChoice)
			{
				UGameplayStatics::SetGamePaused(this, true);
				bAppliedUpgradePause = true;
			}
		}
		return;
	}

	OwningPC->bShowMouseCursor = false;
	OwningPC->SetInputMode(FInputModeGameOnly());

	if (bAppliedUpgradePause)
	{
		UGameplayStatics::SetGamePaused(this, bWasGamePausedBeforeUpgradeChoice);
		bAppliedUpgradePause = false;
	}
}
