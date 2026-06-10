// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/RogueUpgradeChoiceWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueUpgradeChoiceWidget)


URogueUpgradeChoiceWidget::URogueUpgradeChoiceWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<USoundBase> HoverSoundFinder(TEXT("/Game/ActionRoguelike/Audio/UI/UI_Upgrade_Hover_Kenney_Select_001.UI_Upgrade_Hover_Kenney_Select_001"));
	if (HoverSoundFinder.Succeeded())
	{
		HoverSound = HoverSoundFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> SelectSoundFinder(TEXT("/Game/ActionRoguelike/Audio/UI/UI_Upgrade_Select_Kenney_Confirmation_001.UI_Upgrade_Select_Kenney_Confirmation_001"));
	if (SelectSoundFinder.Succeeded())
	{
		SelectSound = SelectSoundFinder.Object;
	}
}


void URogueUpgradeChoiceWidget::InitializeChoices(ARoguePlayerState* InPlayerState)
{
	BoundPlayerState = InPlayerState;
	RebuildChoices();
}


TSharedRef<SWidget> URogueUpgradeChoiceWidget::RebuildWidget()
{
	BuildWidget();
	return Super::RebuildWidget();
}


void URogueUpgradeChoiceWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
	RebuildChoices();
}


void URogueUpgradeChoiceWidget::SelectChoice0()
{
	SelectChoice(0);
}


void URogueUpgradeChoiceWidget::SelectChoice1()
{
	SelectChoice(1);
}


void URogueUpgradeChoiceWidget::SelectChoice2()
{
	SelectChoice(2);
}


void URogueUpgradeChoiceWidget::HandleChoiceHovered()
{
	if (HoverSound)
	{
		UGameplayStatics::PlaySound2D(this, HoverSound, 0.35f);
	}
}


void URogueUpgradeChoiceWidget::BuildWidget()
{
	if (ChoiceList)
	{
		return;
	}

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("UpgradeChoiceRoot"));
	WidgetTree->RootWidget = RootOverlay;

	UBorder* DimBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("UpgradeChoiceDim"));
	DimBackground->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.48f));
	if (UOverlaySlot* DimSlot = RootOverlay->AddChildToOverlay(DimBackground))
	{
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* PanelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("UpgradeChoicePanel"));
	PanelBox->SetWidthOverride(980.0f);
	if (UOverlaySlot* PanelSlot = RootOverlay->AddChildToOverlay(PanelBox))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
	}

	UBorder* PanelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("UpgradeChoiceBackground"));
	PanelBackground->SetBrushColor(FLinearColor(0.005f, 0.007f, 0.011f, 0.50f));
	PanelBackground->SetPadding(FMargin(16.0f, 12.0f));
	PanelBox->SetContent(PanelBackground);

	UVerticalBox* PanelStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("UpgradeChoiceStack"));
	PanelBackground->SetContent(PanelStack);

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UpgradeChoiceTitle"));
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 28;
	TitleText->SetFont(TitleFont);
	TitleText->SetText(NSLOCTEXT("RogueHUD", "UpgradeChoiceTitle", "Choose Upgrade"));
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.97f, 1.0f, 1.0f)));
	TitleText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	if (UVerticalBoxSlot* TitleSlot = PanelStack->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	ChoiceList = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("UpgradeChoiceList"));
	PanelStack->AddChildToVerticalBox(ChoiceList);
}


void URogueUpgradeChoiceWidget::RebuildChoices()
{
	if (!ChoiceList)
	{
		return;
	}

	ChoiceList->ClearChildren();
	if (!BoundPlayerState)
	{
		return;
	}

	const TArray<FRogueUpgradeChoice> Choices = BoundPlayerState->GetPendingUpgradeChoices();
	for (int32 ChoiceIndex = 0; ChoiceIndex < Choices.Num(); ++ChoiceIndex)
	{
		UButton* ChoiceButton = CreateChoiceButton(ChoiceIndex, Choices[ChoiceIndex]);
		if (UHorizontalBoxSlot* ChoiceSlot = ChoiceList->AddChildToHorizontalBox(ChoiceButton))
		{
			ChoiceSlot->SetPadding(FMargin(ChoiceIndex == 0 ? 0.0f : 12.0f, 0.0f, 0.0f, 0.0f));
			ChoiceSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
	}
}


void URogueUpgradeChoiceWidget::SelectChoice(int32 ChoiceIndex)
{
	if (SelectSound)
	{
		UGameplayStatics::PlaySound2D(this, SelectSound, 0.75f);
	}

	if (BoundPlayerState)
	{
		BoundPlayerState->SelectUpgradeChoice(ChoiceIndex);
	}

	OnChoiceWidgetClosed.Broadcast();
	RemoveFromParent();
}


UButton* URogueUpgradeChoiceWidget::CreateChoiceButton(int32 ChoiceIndex, const FRogueUpgradeChoice& Choice)
{
	UButton* ChoiceButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *FString::Printf(TEXT("UpgradeChoiceButton_%d"), ChoiceIndex));
	FButtonStyle ButtonStyle = ChoiceButton->GetStyle();
	ButtonStyle.Normal.TintColor = FSlateColor(FLinearColor(0.055f, 0.042f, 0.16f, 0.98f));
	ButtonStyle.Hovered.TintColor = FSlateColor(FLinearColor(0.15f, 0.12f, 0.29f, 1.0f));
	ButtonStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.025f, 0.020f, 0.08f, 1.0f));
	ChoiceButton->SetStyle(ButtonStyle);
	ChoiceButton->OnHovered.AddDynamic(this, &ThisClass::HandleChoiceHovered);

	switch (ChoiceIndex)
	{
		case 0:
			ChoiceButton->OnClicked.AddDynamic(this, &ThisClass::SelectChoice0);
			break;
		case 1:
			ChoiceButton->OnClicked.AddDynamic(this, &ThisClass::SelectChoice1);
			break;
		case 2:
			ChoiceButton->OnClicked.AddDynamic(this, &ThisClass::SelectChoice2);
			break;
		default:
			break;
	}

	USizeBox* CardBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("UpgradeChoiceCardBox_%d"), ChoiceIndex));
	CardBox->SetWidthOverride(304.0f);
	CardBox->SetHeightOverride(392.0f);
	ChoiceButton->SetContent(CardBox);

	UBorder* CardBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("UpgradeChoiceCard_%d"), ChoiceIndex));
	CardBackground->SetBrushColor(FLinearColor(0.026f, 0.022f, 0.070f, 0.96f));
	CardBackground->SetPadding(FMargin(16.0f, 14.0f, 16.0f, 14.0f));
	CardBox->SetContent(CardBackground);

	UVerticalBox* TextStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *FString::Printf(TEXT("UpgradeChoiceText_%d"), ChoiceIndex));
	CardBackground->SetContent(TextStack);

	USizeBox* RaritySwatchBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("UpgradeRarityBox_%d"), ChoiceIndex));
	RaritySwatchBox->SetHeightOverride(5.0f);
	UBorder* RaritySwatch = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("UpgradeRarity_%d"), ChoiceIndex));
	RaritySwatch->SetBrushColor(GetRarityColor(Choice.Rarity));
	RaritySwatchBox->SetContent(RaritySwatch);
	if (UVerticalBoxSlot* SwatchSlot = TextStack->AddChildToVerticalBox(RaritySwatchBox))
	{
		SwatchSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *FString::Printf(TEXT("UpgradeChoiceHeader_%d"), ChoiceIndex));
	if (UVerticalBoxSlot* HeaderSlot = TextStack->AddChildToVerticalBox(HeaderRow))
	{
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("UpgradeChoiceIconBox_%d"), ChoiceIndex));
	IconBox->SetWidthOverride(78.0f);
	IconBox->SetHeightOverride(58.0f);
	if (UHorizontalBoxSlot* IconSlot = HeaderRow->AddChildToHorizontalBox(IconBox))
	{
		IconSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
		IconSlot->SetVerticalAlignment(VAlign_Center);
	}

	UBorder* IconBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("UpgradeChoiceIcon_%d"), ChoiceIndex));
	const FLinearColor RarityColor = GetRarityColor(Choice.Rarity);
	IconBackground->SetBrushColor(FLinearColor(RarityColor.R * 0.30f, RarityColor.G * 0.30f, RarityColor.B * 0.30f, 0.92f));
	IconBackground->SetPadding(FMargin(4.0f));
	IconBox->SetContent(IconBackground);

	UTextBlock* IconText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("UpgradeChoiceIconText_%d"), ChoiceIndex));
	FSlateFontInfo IconFont = IconText->GetFont();
	IconFont.Size = 16;
	IconText->SetFont(IconFont);
	IconText->SetText(GetUpgradeIconText(Choice));
	IconText->SetJustification(ETextJustify::Center);
	IconText->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 1.0f, 1.0f, 1.0f)));
	IconText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	IconBackground->SetContent(IconText);

	UVerticalBox* NameStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *FString::Printf(TEXT("UpgradeChoiceNameStack_%d"), ChoiceIndex));
	if (UHorizontalBoxSlot* NameStackSlot = HeaderRow->AddChildToHorizontalBox(NameStack))
	{
		NameStackSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameStackSlot->SetVerticalAlignment(VAlign_Center);
	}

	UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("UpgradeChoiceName_%d"), ChoiceIndex));
	FSlateFontInfo NameFont = NameText->GetFont();
	NameFont.Size = 18;
	NameText->SetFont(NameFont);
	NameText->SetText(Choice.DisplayName);
	NameText->SetJustification(ETextJustify::Left);
	NameText->SetAutoWrapText(true);
	NameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.99f, 1.0f, 1.0f)));
	if (UVerticalBoxSlot* NameSlot = NameStack->AddChildToVerticalBox(NameText))
	{
		NameSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	UTextBlock* RarityText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("UpgradeChoiceRarity_%d"), ChoiceIndex));
	FSlateFontInfo RarityFont = RarityText->GetFont();
	RarityFont.Size = 11;
	RarityText->SetFont(RarityFont);
	RarityText->SetText(GetRarityDisplayText(Choice.Rarity));
	RarityText->SetJustification(ETextJustify::Left);
	RarityText->SetColorAndOpacity(FSlateColor(RarityColor));
	RarityText->SetTextTransformPolicy(ETextTransformPolicy::ToUpper);
	if (UVerticalBoxSlot* RaritySlot = NameStack->AddChildToVerticalBox(RarityText))
	{
		RaritySlot->SetPadding(FMargin(0.0f));
	}

	UTextBlock* DescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("UpgradeChoiceDescription_%d"), ChoiceIndex));
	FSlateFontInfo DescriptionFont = DescriptionText->GetFont();
	DescriptionFont.Size = 14;
	DescriptionText->SetFont(DescriptionFont);
	DescriptionText->SetText(Choice.Description);
	DescriptionText->SetAutoWrapText(true);
	DescriptionText->SetJustification(ETextJustify::Left);
	DescriptionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.92f, 0.98f, 1.0f)));
	if (UVerticalBoxSlot* DescriptionSlot = TextStack->AddChildToVerticalBox(DescriptionText))
	{
		DescriptionSlot->SetPadding(FMargin(2.0f, 4.0f, 2.0f, 0.0f));
		DescriptionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		DescriptionSlot->SetVerticalAlignment(VAlign_Top);
	}

	UTextBlock* DetailText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("UpgradeChoiceDetail_%d"), ChoiceIndex));
	FSlateFontInfo DetailFont = DetailText->GetFont();
	DetailFont.Size = 13;
	DetailText->SetFont(DetailFont);
	DetailText->SetText(BoundPlayerState ? BoundPlayerState->GetUpgradePreviewText(Choice) : Choice.Description);
	DetailText->SetAutoWrapText(true);
	DetailText->SetJustification(ETextJustify::Left);
	DetailText->SetColorAndOpacity(FSlateColor(FLinearColor(0.66f, 0.90f, 1.0f, 1.0f)));
	if (UVerticalBoxSlot* DetailSlot = TextStack->AddChildToVerticalBox(DetailText))
	{
		DetailSlot->SetPadding(FMargin(2.0f, 10.0f, 2.0f, 0.0f));
	}

	UTextBlock* StackText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("UpgradeChoiceStack_%d"), ChoiceIndex));
	FSlateFontInfo StackFont = StackText->GetFont();
	StackFont.Size = 13;
	StackText->SetFont(StackFont);
	StackText->SetText(GetStackDisplayText(Choice));
	StackText->SetJustification(ETextJustify::Left);
	StackText->SetColorAndOpacity(FSlateColor(FLinearColor(0.84f, 0.88f, 0.94f, 1.0f)));
	if (UVerticalBoxSlot* StackSlot = TextStack->AddChildToVerticalBox(StackText))
	{
		StackSlot->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 0.0f));
	}

	USizeBox* FooterBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("UpgradeChoiceFooterBox_%d"), ChoiceIndex));
	FooterBox->SetHeightOverride(30.0f);
	if (UVerticalBoxSlot* FooterSlot = TextStack->AddChildToVerticalBox(FooterBox))
	{
		FooterSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));
	}

	UBorder* FooterBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("UpgradeChoiceFooter_%d"), ChoiceIndex));
	FooterBackground->SetBrushColor(FLinearColor(RarityColor.R * 0.20f, RarityColor.G * 0.20f, RarityColor.B * 0.20f, 0.72f));
	FooterBackground->SetPadding(FMargin(8.0f, 4.0f));
	FooterBox->SetContent(FooterBackground);

	UTextBlock* FooterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("UpgradeChoiceFooterText_%d"), ChoiceIndex));
	FSlateFontInfo FooterFont = FooterText->GetFont();
	FooterFont.Size = 12;
	FooterText->SetFont(FooterFont);
	FooterText->SetText(NSLOCTEXT("RogueHUD", "UpgradeChoiceFooter", "SELECT"));
	FooterText->SetJustification(ETextJustify::Center);
	FooterText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.98f, 1.0f, 1.0f)));
	FooterText->SetTextTransformPolicy(ETextTransformPolicy::ToUpper);
	FooterBackground->SetContent(FooterText);

	return ChoiceButton;
}


FText URogueUpgradeChoiceWidget::GetRarityDisplayText(ERogueUpgradeRarity Rarity) const
{
	switch (Rarity)
	{
		case ERogueUpgradeRarity::Rare:
			return NSLOCTEXT("RogueHUD", "UpgradeRarityRare", "Rare");
		case ERogueUpgradeRarity::Prismatic:
			return NSLOCTEXT("RogueHUD", "UpgradeRarityPrismatic", "Prismatic");
		case ERogueUpgradeRarity::Common:
		default:
			return NSLOCTEXT("RogueHUD", "UpgradeRarityCommon", "Common");
	}
}


FText URogueUpgradeChoiceWidget::GetStackDisplayText(const FRogueUpgradeChoice& Choice) const
{
	const int32 StackCount = BoundPlayerState ? BoundPlayerState->GetUpgradeStackCount(Choice.UpgradeId) : 0;
	if (Choice.MaxStacks > 0)
	{
		return StackCount == 0
			? FText::Format(NSLOCTEXT("RogueHUD", "UpgradeNewLevelFormat", "New  Lv 0/{0}"), FText::AsNumber(Choice.MaxStacks))
			: FText::Format(NSLOCTEXT("RogueHUD", "UpgradeLevelFormat", "Lv {0}/{1}"), FText::AsNumber(StackCount), FText::AsNumber(Choice.MaxStacks));
	}

	if (Choice.bUnique)
	{
		return StackCount == 0
			? NSLOCTEXT("RogueHUD", "UpgradeNewUnique", "New")
			: NSLOCTEXT("RogueHUD", "UpgradeOwnedUnique", "Owned");
	}

	return StackCount == 0
		? NSLOCTEXT("RogueHUD", "UpgradeNoStacks", "New")
		: FText::Format(NSLOCTEXT("RogueHUD", "UpgradeStackFormat", "Stack {0}"), FText::AsNumber(StackCount));
}


FText URogueUpgradeChoiceWidget::GetUpgradeIconText(const FRogueUpgradeChoice& Choice) const
{
	if (Choice.UpgradeId == TEXT("AttackDamage"))
	{
		return NSLOCTEXT("RogueHUD", "UpgradeIconAttackDamage", "ATK");
	}
	if (Choice.UpgradeId == TEXT("HealthMax"))
	{
		return NSLOCTEXT("RogueHUD", "UpgradeIconHealthMax", "HP");
	}
	if (Choice.UpgradeId == TEXT("PickupMagnet"))
	{
		return NSLOCTEXT("RogueHUD", "UpgradeIconPickupMagnet", "MAG");
	}
	if (Choice.UpgradeId == TEXT("KillExplosion"))
	{
		return NSLOCTEXT("RogueHUD", "UpgradeIconKillExplosion", "AOE");
	}
	if (Choice.UpgradeId == TEXT("LastStandShield"))
	{
		return NSLOCTEXT("RogueHUD", "UpgradeIconLastStandShield", "SAVE");
	}
	if (Choice.UpgradeId == TEXT("ChainLightning"))
	{
		return NSLOCTEXT("RogueHUD", "UpgradeIconChainLightning", "ARC");
	}

	return NSLOCTEXT("RogueHUD", "UpgradeIconFallback", "UP");
}


FLinearColor URogueUpgradeChoiceWidget::GetRarityColor(ERogueUpgradeRarity Rarity) const
{
	switch (Rarity)
	{
		case ERogueUpgradeRarity::Rare:
			return FLinearColor(0.20f, 0.62f, 1.0f, 1.0f);
		case ERogueUpgradeRarity::Prismatic:
			return FLinearColor(1.0f, 0.72f, 0.28f, 1.0f);
		case ERogueUpgradeRarity::Common:
		default:
			return FLinearColor(0.64f, 0.72f, 0.78f, 1.0f);
	}
}
