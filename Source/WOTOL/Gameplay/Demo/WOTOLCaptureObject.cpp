#include "WOTOLCaptureObject.h"
#include "DemoFlowSubsystem.h"
#include "Gameplay/Battle/TerritoryStateManager.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

AWOTOLCaptureObject::AWOTOLCaptureObject()
{
	PrimaryActorTick.bCanEverTick = true;

	// Racine NON mise à l'échelle : le mesh est agrandi, mais PAS les étiquettes.
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	ShapeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShapeMesh"));
	ShapeMesh->SetupAttachment(SceneRoot);

	// Petit label posé juste au-dessus du socle (le mesh fait ~400 de haut) — plus de
	// texte démesuré flottant très haut, qui gâchait la lisibilité de l'action.
	NameTagShadow = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameTagShadow"));
	NameTagShadow->SetupAttachment(SceneRoot);
	NameTagShadow->SetHorizontalAlignment(EHTA_Center);
	NameTagShadow->SetWorldSize(46.f);
	NameTagShadow->SetRelativeLocation(FVector(0.f, 0.f, 250.f));
	NameTagShadow->SetTextRenderColor(FColor(0, 0, 0, 255));

	NameTag = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameTag"));
	NameTag->SetupAttachment(SceneRoot);
	NameTag->SetHorizontalAlignment(EHTA_Center);
	NameTag->SetWorldSize(40.f);
	NameTag->SetRelativeLocation(FVector(0.f, 0.f, 250.f));
}

void AWOTOLCaptureObject::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
	BuildVisual();
	if (NameTag) NameTag->SetTextRenderColor(FFactionColors::Get(OwnerFaction).ToFColor(true));
}

void AWOTOLCaptureObject::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!NameTag) return;

	const FText TagText = FText::FromString(FString::Printf(TEXT("%s\n%d / %d"),
		*GetDisplayName().ToString(), FMath::RoundToInt(CurrentHealth), FMath::RoundToInt(MaxHealth)));
	NameTag->SetText(TagText);
	if (NameTagShadow) NameTagShadow->SetText(TagText);

	// Étiquette + ombre décalée face à la caméra (contraste)
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (PC->PlayerCameraManager)
		{
			const FVector NameLoc = NameTag->GetComponentLocation();
			FRotator Face = (PC->PlayerCameraManager->GetCameraLocation() - NameLoc).Rotation();
			Face.Pitch = 0.f; Face.Roll = 0.f;
			NameTag->SetWorldRotation(Face);
			if (NameTagShadow)
			{
				NameTagShadow->SetWorldRotation(Face);
				const FVector Fwd   = Face.Vector();
				const FVector Right = FRotationMatrix(Face).GetScaledAxis(EAxis::Y);
				NameTagShadow->SetWorldLocation(NameLoc - Fwd * 3.f + Right * 6.f + FVector(0, 0, -8.f));
			}
		}
	}
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
		OnCaptureDestroyed.Broadcast();
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
