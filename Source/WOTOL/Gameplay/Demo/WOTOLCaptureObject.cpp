#include "WOTOLCaptureObject.h"
#include "DemoFlowSubsystem.h"
#include "Gameplay/Battle/TerritoryStateManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/GameInstance.h"

AWOTOLCaptureObject::AWOTOLCaptureObject()
{
	PrimaryActorTick.bCanEverTick = false;

	ShapeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShapeMesh"));
	RootComponent = ShapeMesh;
}

void AWOTOLCaptureObject::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
	BuildVisual();
}

FText AWOTOLCaptureObject::GetDisplayName() const
{
	return (OwnerFaction == EFactionID::Noxeens)
		? FText::FromString(TEXT("Abyssalyseur"))
		: FText::FromString(TEXT("Cristalliseur"));
}

void AWOTOLCaptureObject::ClaimZone()
{
	// Enregistre / met la zone en Grade 1 pour la faction propriétaire
	if (UWorld* W = GetWorld())
	{
		if (UTerritoryStateManager* Territory = W->GetSubsystem<UTerritoryStateManager>())
		{
			FZoneState State;
			State.Grade = 1;
			State.Owner = OwnerFaction;
			State.CapturingFaction = OwnerFaction;
			State.CaptureProgress = 100.f;
			Territory->RegisterZone(ZoneID, State);
		}
	}

	// Notifie la progression de la démo
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
		{
			Demo->MarkZoneCaptured();
		}
	}
}

void AWOTOLCaptureObject::ApplyDamage(float Amount)
{
	if (Amount <= 0.f) return;
	CurrentHealth = FMath::Max(0.f, CurrentHealth - Amount);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
		{
			Demo->MarkZoneDamaged();
		}
	}

	if (CurrentHealth <= 0.f)
	{
		OnDestroyed.Broadcast();
	}
}

void AWOTOLCaptureObject::Repair(float Amount)
{
	if (Amount <= 0.f) return;
	CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + Amount);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	if (CurrentHealth >= MaxHealth)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
			{
				Demo->MarkZoneRepaired();
			}
		}
	}
}

float AWOTOLCaptureObject::GetHealthPercent() const
{
	return (MaxHealth > 0.f) ? (CurrentHealth / MaxHealth) : 0.f;
}

void AWOTOLCaptureObject::BuildVisual()
{
	if (!ShapeMesh) return;

	// Aquiloris : cristal (cône) · Noxéens : organique (sphère)
	const TCHAR* MeshPath = (OwnerFaction == EFactionID::Noxeens)
		? TEXT("/Engine/BasicShapes/Sphere.Sphere")
		: TEXT("/Engine/BasicShapes/Cone.Cone");

	if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath))
	{
		ShapeMesh->SetStaticMesh(Mesh);
	}
	ShapeMesh->SetRelativeScale3D(FVector(3.f, 3.f, 4.f)); // structure imposante

	if (UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this))
		{
			MID->SetVectorParameterValue(TEXT("Color"), FFactionColors::Get(OwnerFaction));
			ShapeMesh->SetMaterial(0, MID);
		}
	}
}
