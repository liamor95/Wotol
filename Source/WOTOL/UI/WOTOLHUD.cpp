#include "WOTOLHUD.h"
#include "WOTOLBattleWidget.h"
#include "Blueprint/UserWidget.h"

void AWOTOLHUD::BeginPlay()
{
	Super::BeginPlay();

	if (BattleWidgetClass)
	{
		BattleWidget = CreateWidget<UWOTOLBattleWidget>(
			GetOwningPlayerController(), BattleWidgetClass);
		if (BattleWidget)
		{
			BattleWidget->AddToViewport();
		}
	}
}

void AWOTOLHUD::DrawHUD()
{
	Super::DrawHUD();

	if (bDrawBoxSelect)
	{
		const FLinearColor BoxColor(0.2f, 0.8f, 0.2f, 0.25f);
		const FLinearColor BorderColor(0.2f, 0.8f, 0.2f, 0.9f);

		const float X  = FMath::Min(BoxStart.X, BoxCurrent.X);
		const float Y  = FMath::Min(BoxStart.Y, BoxCurrent.Y);
		const float W  = FMath::Abs(BoxCurrent.X - BoxStart.X);
		const float H  = FMath::Abs(BoxCurrent.Y - BoxStart.Y);

		DrawRect(BoxColor, X, Y, W, H);
		DrawLine(X,   Y,   X+W, Y,   BorderColor, 1.5f);
		DrawLine(X+W, Y,   X+W, Y+H, BorderColor, 1.5f);
		DrawLine(X+W, Y+H, X,   Y+H, BorderColor, 1.5f);
		DrawLine(X,   Y+H, X,   Y,   BorderColor, 1.5f);
	}
}

void AWOTOLHUD::SetBoxSelectState(bool bActive, FVector2D Start, FVector2D Current)
{
	bDrawBoxSelect = bActive;
	BoxStart       = Start;
	BoxCurrent     = Current;
}
