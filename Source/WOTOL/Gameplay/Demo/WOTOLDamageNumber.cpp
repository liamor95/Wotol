#include "WOTOLDamageNumber.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

AWOTOLDamageNumber::AWOTOLDamageNumber()
{
	PrimaryActorTick.bCanEverTick = true;

	Text = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Text"));
	RootComponent = Text;
	Text->SetHorizontalAlignment(EHTA_Center);
	Text->SetWorldSize(70.f);
}

void AWOTOLDamageNumber::Init(float Amount, const FLinearColor& Color)
{
	if (Text)
	{
		Text->SetText(FText::FromString(FString::Printf(TEXT("-%d"), FMath::RoundToInt(Amount))));
		Text->SetTextRenderColor(Color.ToFColor(true));
	}
}

AWOTOLDamageNumber* AWOTOLDamageNumber::Spawn(UWorld* World, const FVector& Loc,
	float Amount, const FLinearColor& Color)
{
	if (!World) return nullptr;
	AWOTOLDamageNumber* N = World->SpawnActor<AWOTOLDamageNumber>(
		AWOTOLDamageNumber::StaticClass(), Loc, FRotator::ZeroRotator);
	if (N) N->Init(Amount, Color);
	return N;
}

void AWOTOLDamageNumber::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;
	AddActorWorldOffset(FVector(0.f, 0.f, 100.f * DeltaSeconds)); // monte

	// Toujours face caméra (lisible)
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (PC->PlayerCameraManager && Text)
		{
			FRotator F = (GetActorLocation() - PC->PlayerCameraManager->GetCameraLocation()).Rotation();
			F.Pitch = 0.f; F.Roll = 0.f;
			SetActorRotation(F);
		}
	}

	if (Age >= Life)
	{
		Destroy();
	}
}
