#include "VerticalLayerComponent.h"
#include "Core/PlayerProfileSubsystem.h"
#include "Kismet/GameplayStatics.h"

UVerticalLayerComponent::UVerticalLayerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UVerticalLayerComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentLayer = DefaultLayer;
}

void UVerticalLayerComponent::SetLayer(EVerticalLayer NewLayer)
{
	if (NewLayer == CurrentLayer) return;

	const EVerticalLayer Old = CurrentLayer;
	CurrentLayer = NewLayer;
	OnLayerChanged.Broadcast(Old, NewLayer);

	// Met à jour le profil joueur si c'est une unité contrôlée par le joueur
	if (GetOwner() && GetOwner()->GetInstigatorController()
		&& GetOwner()->GetInstigatorController()->IsPlayerController())
	{
		if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
		{
			if (UPlayerProfileSubsystem* Profile =
					GI->GetSubsystem<UPlayerProfileSubsystem>())
			{
				Profile->RecordLayerChange(NewLayer);
			}
		}
	}
}

float UVerticalLayerComponent::GetLayerTargetZ(EVerticalLayer Layer)
{
	switch (Layer)
	{
		case EVerticalLayer::Ground: return 0.f;
		case EVerticalLayer::Mid:    return 500.f;   // ~5m en UE units
		case EVerticalLayer::High:   return 1500.f;  // ~15m
		default:                     return 0.f;
	}
}
