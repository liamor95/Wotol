#include "WOTOLDemoHUD.h"
#include "DemoFlowSubsystem.h"
#include "Gameplay/Battle/WOTOLPlayerController_Battle.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void AWOTOLDemoHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;

	const float W = Canvas->SizeX;
	const float H = Canvas->SizeY;

	// ─── 1) Boîte de sélection (rectangle de drag) ───────────────────────────
	if (AWOTOLPlayerController_Battle* PC =
			Cast<AWOTOLPlayerController_Battle>(GetOwningPlayerController()))
	{
		if (PC->IsBoxSelecting())
		{
			const FVector2D S = PC->GetBoxStart();
			const FVector2D C = PC->GetBoxCurrent();
			const float X = FMath::Min(S.X, C.X);
			const float Y = FMath::Min(S.Y, C.Y);
			const float BW = FMath::Abs(C.X - S.X);
			const float BH = FMath::Abs(C.Y - S.Y);

			DrawRect(FLinearColor(0.2f, 0.7f, 1.f, 0.12f), X, Y, BW, BH);
			const FLinearColor Border(0.4f, 0.85f, 1.f, 1.f);
			DrawLine(X, Y, X + BW, Y, Border, 1.5f);
			DrawLine(X, Y + BH, X + BW, Y + BH, Border, 1.5f);
			DrawLine(X, Y, X, Y + BH, Border, 1.5f);
			DrawLine(X + BW, Y, X + BW, Y + BH, Border, 1.5f);
		}
	}

	// ─── 2) Message d'objectif + écran de fin ────────────────────────────────
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;

	if (!Demo->CurrentMessage.IsEmpty())
	{
		DrawCenteredText(Demo->CurrentMessage, H * 0.07f, FLinearColor::White, 1.3f);
	}

	if (Demo->GetPhase() == EDemoPhase::DemoEnd)
	{
		DrawCenteredText(TEXT("— FIN DE LA DÉMO —"), H * 0.42f,
			FLinearColor(1.f, 0.85f, 0.2f, 1.f), 2.2f);
		DrawCenteredText(TEXT("Le conflit Aquiloris / Noxeens ne fait que commencer..."),
			H * 0.50f, FLinearColor::White, 1.1f);
	}
}

void AWOTOLDemoHUD::DrawCenteredText(const FString& Text, float Y,
	const FLinearColor& Color, float Scale)
{
	if (!Canvas) return;
	UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;

	float TW = 0.f, TH = 0.f;
	GetTextSize(Text, TW, TH, Font, Scale);
	const float X = (Canvas->SizeX - TW) * 0.5f;

	// Légère ombre noire pour la lisibilité
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, 0.7f), X + 2.f, Y + 2.f, Font, Scale);
	DrawText(Text, Color, X, Y, Font, Scale);
}
