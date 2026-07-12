#include "WOTOLDamageNumber.h"
#include "Components/TextRenderComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

int32 AWOTOLDamageNumber::LiveCount = 0;
int32 AWOTOLDamageNumber::MaxLive   = 60;

void AWOTOLDamageNumber::EndPlay(const EEndPlayReason::Type Reason)
{
	--LiveCount;
	Super::EndPlay(Reason);
}

AWOTOLDamageNumber::AWOTOLDamageNumber()
{
	PrimaryActorTick.bCanEverTick = true;

	Text = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Text"));
	RootComponent = Text;
	Text->SetHorizontalAlignment(EHTA_Center);
	Text->SetWorldSize(70.f); // lisible mais pas envahissant (accroché à l'unité)
}

void AWOTOLDamageNumber::SetFollow(USceneComponent* Comp, const FVector& LocalOffset)
{
	Follow = Comp;
	FollowOffset = LocalOffset;
	if (Comp)
	{
		// Positionne tout de suite à côté de l'unité (à sa hauteur de couche).
		SetActorLocation(Comp->GetComponentLocation() + LocalOffset);
	}
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
	if (!World || LiveCount >= MaxLive) return nullptr;
	AWOTOLDamageNumber* N = World->SpawnActor<AWOTOLDamageNumber>(
		AWOTOLDamageNumber::StaticClass(), Loc, FRotator::ZeroRotator);
	if (N) { ++LiveCount; N->Init(Amount, Color); }
	return N;
}

AWOTOLDamageNumber* AWOTOLDamageNumber::SpawnText(UWorld* World, const FVector& Loc,
	const FString& Label, const FLinearColor& Color)
{
	if (!World || LiveCount >= MaxLive) return nullptr;
	AWOTOLDamageNumber* N = World->SpawnActor<AWOTOLDamageNumber>(
		AWOTOLDamageNumber::StaticClass(), Loc, FRotator::ZeroRotator);
	if (N) ++LiveCount;
	if (N && N->Text)
	{
		N->Text->SetText(FText::FromString(Label));
		N->Text->SetTextRenderColor(Color.ToFColor(true));
		N->Text->SetWorldSize(55.f);
	}
	return N;
}

void AWOTOLDamageNumber::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;
	Rise += 70.f * DeltaSeconds; // le texte s'élève doucement

	// S'il SUIT une unité : reste collé à côté d'elle, à SA hauteur (couche verticale),
	// et monte. Sinon (texte libre) : simple montée à sa position d'origine.
	if (Follow.IsValid())
	{
		SetActorLocation(Follow->GetComponentLocation() + FollowOffset + FVector(0.f, 0.f, Rise));
	}
	else
	{
		AddActorWorldOffset(FVector(0.f, 0.f, 70.f * DeltaSeconds));
	}

	// Toujours face caméra (lisible)
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (PC->PlayerCameraManager && Text)
		{
			// +X du texte VERS la caméra -> lu à l'endroit (pas en miroir)
			FRotator F = (PC->PlayerCameraManager->GetCameraLocation() - GetActorLocation()).Rotation();
			F.Pitch = 0.f; F.Roll = 0.f;
			SetActorRotation(F);
		}
	}

	if (Age >= Life)
	{
		Destroy();
	}
}
