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
	// Valeurs en UE units représentant les profondeurs océaniques (inversées — plus bas = plus profond)
	// L'environnement de jeu est sous-marin : Épipélagique est le plus haut (proche surface)
	switch (Layer)
	{
		case EVerticalLayer::Epipelagique:   return 0.f;      // surface ~0-200m
		case EVerticalLayer::Mesopelagique:  return -2000.f;  // 200-1000m
		case EVerticalLayer::Bathypelagique: return -6000.f;  // 1000-4000m
		case EVerticalLayer::Hadal:          return -12000.f; // 4000m+
		default:                             return 0.f;
	}
}

// Modificateurs de dégâts selon la direction d'attaque (verticale)
float UVerticalLayerComponent::GetAttackDamageMultiplier(EVerticalLayer AttackerLayer,
                                                          EVerticalLayer TargetLayer)
{
	const int32 AttackerIdx = static_cast<int32>(AttackerLayer);
	const int32 TargetIdx   = static_cast<int32>(TargetLayer);

	if (AttackerIdx > TargetIdx)
	{
		// Attaque ascendante (depuis couche inférieure/plus profonde) = bonus dégâts
		const int32 Diff = AttackerIdx - TargetIdx;
		if (AttackerLayer == EVerticalLayer::Hadal) return 3.0f; // ×3 depuis Hadal
		return 1.f + (Diff * 0.25f); // +25% par couche traversée
	}
	else if (AttackerIdx < TargetIdx)
	{
		// Attaque descendante = bonus précision (géré séparément), pas de bonus dégâts ici
		return 1.0f;
	}
	return 1.0f; // Même couche
}

float UVerticalLayerComponent::GetSpeedMultiplierForLayer(EVerticalLayer Layer)
{
	// Épipélagique : vitesse +20% (lumière max, couverture réduite)
	// Hadal : unités d'élite uniquement, pas de modificateur vitesse ici
	return (Layer == EVerticalLayer::Epipelagique) ? 1.2f : 1.0f;
}
