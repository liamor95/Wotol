#include "WOTOLCaptureObject.h"
#include "DemoFlowSubsystem.h"
#include "Gameplay/Battle/TerritoryStateManager.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "WOTOLGlow.h"
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
	NameTagShadow->SetWorldSize(44.f); // fin liseré noir centré autour du nom
	NameTagShadow->SetRelativeLocation(FVector(0.f, 0.f, -140.f));
	NameTagShadow->SetTextRenderColor(FColor(0, 0, 0, 255));

	NameTag = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameTag"));
	NameTag->SetupAttachment(SceneRoot);
	NameTag->SetHorizontalAlignment(EHTA_Center);
	NameTag->SetWorldSize(40.f);
	NameTag->SetRelativeLocation(FVector(0.f, 0.f, -140.f));
}

void AWOTOLCaptureObject::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
	BuildVisual();
	if (NameTag) NameTag->SetTextRenderColor(FFactionColors::Get(OwnerFaction).ToFColor(true));

	if (bIsGhostPreview)
	{
		ApplyHologramMaterial();
		SetActorEnableCollision(false);
		if (NameTag)       NameTag->SetVisibility(false);
		if (NameTagShadow) NameTagShadow->SetVisibility(false);
	}

	if (bIsUnderConstruction && SceneRoot)
	{
		SceneRoot->SetWorldScale3D(FVector(0.05f));
	}
}

// Remplace le matériau de CHAQUE pièce (mesh principal + kitbash) par une coquille
// translucide émissive partagée (WOTOLGlow::MakeHalo) : garantit que l'aperçu a EXACTEMENT
// la même silhouette que le bâtiment réel, juste en mode holographique. Coupe aussi les
// lampes ponctuelles (une prévisualisation ne doit pas éclairer la scène comme un vrai bâtiment).
void AWOTOLCaptureObject::ApplyHologramMaterial()
{
	const FLinearColor Base = FFactionColors::Get(OwnerFaction);
	HologramMID = WOTOLGlow::MakeHalo(this, Base * 2.2f, 0.35f);
	if (!HologramMID) return;

	TArray<UStaticMeshComponent*> Meshes;
	GetComponents<UStaticMeshComponent>(Meshes);
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		if (!Mesh) continue;
		const int32 NumSlots = FMath::Max(1, Mesh->GetNumMaterials());
		for (int32 Slot = 0; Slot < NumSlots; ++Slot)
		{
			Mesh->SetMaterial(Slot, HologramMID);
		}
	}

	TArray<UPointLightComponent*> Lights;
	GetComponents<UPointLightComponent>(Lights);
	for (UPointLightComponent* Light : Lights)
	{
		if (Light) Light->SetVisibility(false);
	}
}

void AWOTOLCaptureObject::BeginConstruction(float Duration)
{
	bIsUnderConstruction = true;
	ConstructionDuration = FMath::Max(0.1f, Duration);
	ConstructionElapsed  = 0.f;
	if (SceneRoot) SceneRoot->SetWorldScale3D(FVector(0.05f));
}

void AWOTOLCaptureObject::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Aperçu holographique : pulsation lente (même langage que le feedback des éléments
	// destructibles) — pas de PV à afficher, pas de rotation face-caméra de l'étiquette.
	if (bIsGhostPreview)
	{
		if (HologramMID)
		{
			const float Pulse = 0.28f + 0.20f * FMath::Sin(GetWorld()->GetTimeSeconds() * 2.4f);
			HologramMID->SetScalarParameterValue(TEXT("Opacity"), Pulse);
		}
		return;
	}

	if (bIsUnderConstruction && SceneRoot)
	{
		ConstructionElapsed += DeltaSeconds;
		const float Alpha = FMath::Clamp(ConstructionElapsed / ConstructionDuration, 0.f, 1.f);
		SceneRoot->SetWorldScale3D(FVector(FMath::Lerp(0.05f, 1.f, Alpha)));
		if (Alpha >= 1.f)
		{
			bIsUnderConstruction = false;
			SceneRoot->SetWorldScale3D(FVector(1.f));
		}
	}

	if (!NameTag) return;

	const FText TagText = bIsUnderConstruction
		? FText::FromString(FString::Printf(TEXT("%s\nConstruction... %d %%"),
			*GetDisplayName().ToString(),
			FMath::RoundToInt(FMath::Clamp(ConstructionElapsed / ConstructionDuration, 0.f, 1.f) * 100.f)))
		: FText::FromString(FString::Printf(TEXT("%s\n%d / %d"),
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
				// Contour noir CENTRÉ (fin liseré), pas d'ombre décalée.
				NameTagShadow->SetWorldRotation(Face);
				NameTagShadow->SetWorldLocation(NameLoc - Face.Vector() * 1.5f);
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
			// Préserve le graphe, le type d'objectif et les fortifications déjà enregistrés.
			// Réinitialiser toute la structure ici supprimait silencieusement les liens de carte.
			FZoneState State = Territory->GetZoneState(ZoneID);
			State.Grade = 1;
			State.Owner = OwnerFaction;
			State.CapturingFaction = OwnerFaction;
			State.CaptureProgress = 100.f;
			State.bConquestObjectiveCompleted = true;
			State.StrategicStatus = EZoneStrategicStatus::Stable;
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
	if (!ShapeMesh || !SceneRoot) return;

	const TCHAR* M_CUBE = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* M_SPH  = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	const TCHAR* M_CYL  = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* M_CONE = TEXT("/Engine/BasicShapes/Cone.Cone");

	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// Ajoute une pièce kitbash (mesh primitif + transform + couleur) au bâtiment.
	auto AddPiece = [&](const TCHAR* MeshPath, const FVector& Loc, const FVector& Scale,
		const FRotator& Rot, const FLinearColor& Color) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		if (!C) return nullptr;
		C->SetupAttachment(SceneRoot);
		C->RegisterComponent();
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, MeshPath)) C->SetStaticMesh(M);
		C->SetRelativeLocationAndRotation(Loc, Rot);
		C->SetRelativeScale3D(Scale);
		// Corps du bâtiment MAT rugueux (fini le plastique lisse).
		if (UMaterialInstanceDynamic* MID = WOTOLGlow::MakeMatte(this, Color))
			C->SetMaterial(0, MID);
		return C;
	};

	// Variante ÉMISSIVE : la pièce (cristal/noyau) RAYONNE d'elle-même (matériau unlit),
	// ce n'est plus la lampe qui éclaire le sol mais le mesh qu'on voit briller.
	auto AddGlow = [&](const TCHAR* MeshPath, const FVector& Loc, const FVector& Scale,
		const FRotator& Rot, const FLinearColor& EmissiveHDR) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* C = AddPiece(MeshPath, Loc, Scale, Rot, FLinearColor::White);
		if (C)
			if (UMaterialInstanceDynamic* MID = WOTOLGlow::MakeGlow(this, EmissiveHDR))
				C->SetMaterial(0, MID);
		return C;
	};

	// L'acteur est spawné à +200 Z : la base du bâtiment est posée vers Z relatif -200 (sol).
	if (OwnerFaction == EFactionID::Noxeens)
	{
		// ── ABYSSALYSEUR : bâtiment organique bioluminescent (vert abyssal) ──
		const FLinearColor Dark (0.07f, 0.10f, 0.12f, 1.f);
		const FLinearColor Shell(0.10f, 0.16f, 0.18f, 1.f);
		const FLinearColor Glow (0.28f, 0.95f, 0.45f, 1.f);

		AddPiece(M_CYL, FVector(0, 0, -195), FVector(4.2f, 4.2f, 0.5f), FRotator::ZeroRotator, Dark);      // socle
		// Corps bulbeux (le mesh principal)
		ShapeMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, M_SPH));
		ShapeMesh->SetRelativeLocation(FVector(0, 0, -40));
		ShapeMesh->SetRelativeScale3D(FVector(3.0f, 3.0f, 3.2f));
		if (BaseMat)
		{
			if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this))
			{
				MID->SetVectorParameterValue(TEXT("Color"), Shell);
				ShapeMesh->SetMaterial(0, MID);
			}
		}
		AddPiece(M_CONE, FVector(0, 0, 210), FVector(1.3f, 1.3f, 3.0f), FRotator::ZeroRotator, Shell);     // flèche
		AddGlow(M_SPH,  FVector(0, 0, 130), FVector(1.1f, 1.1f, 1.1f), FRotator::ZeroRotator, Glow * 3.2f); // noyau ÉMISSIF
		// Tentacules bioluminescents (émissifs) autour de la base
		for (int32 i = 0; i < 6; ++i)
		{
			const float A = 2.f * PI * i / 6.f;
			AddGlow(M_CONE, FVector(FMath::Cos(A) * 150.f, FMath::Sin(A) * 150.f, -120.f),
				FVector(0.4f, 0.4f, 1.6f), FRotator(20.f, FMath::RadiansToDegrees(A), 0.f), Glow * 2.4f);
		}
		// Lampe VERTE discrète (léger halo, pas une flaque au sol) au cœur bulbeux.
		if (UPointLightComponent* PC = NewObject<UPointLightComponent>(this))
		{
			PC->SetupAttachment(RootComponent);
			PC->RegisterComponent();
			PC->SetRelativeLocation(FVector(0, 0, 130));
			PC->SetLightColor(FLinearColor(0.30f, 1.0f, 0.45f));
			PC->SetIntensity(2600.f);
			PC->SetAttenuationRadius(700.f);
			PC->SetCastShadows(false);
		}
	}
	else
	{
		// ── CRISTALLISEUR : bâtiment cristal-tech (bleu acier + or + énergie cyan) ──
		const FLinearColor Steel (0.12f, 0.20f, 0.42f, 1.f);
		const FLinearColor Gold  (0.95f, 0.78f, 0.25f, 1.f);
		const FLinearColor Energy(0.45f, 0.90f, 1.00f, 1.f);

		AddPiece(M_CYL, FVector(0, 0, -195), FVector(4.4f, 4.4f, 0.5f), FRotator::ZeroRotator, Steel * 0.7f); // socle
		// Tour principale (le mesh principal)
		ShapeMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, M_CYL));
		ShapeMesh->SetRelativeLocation(FVector(0, 0, -40));
		ShapeMesh->SetRelativeScale3D(FVector(2.6f, 2.6f, 2.4f));
		if (BaseMat)
		{
			if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this))
			{
				MID->SetVectorParameterValue(TEXT("Color"), Steel);
				ShapeMesh->SetMaterial(0, MID);
			}
		}
		AddPiece(M_CYL,  FVector(0, 0, 70),  FVector(3.0f, 3.0f, 0.3f), FRotator::ZeroRotator, Gold);      // anneau or
		// GRAND CRISTAL central ÉMISSIF = énergie BLEUE qui RAYONNE (c'est le cristal
		// qu'on voit briller, pas le sol autour).
		const FLinearColor CrystalHDR(0.5f * 3.0f, 1.4f * 3.0f, 2.8f * 3.0f, 1.f);
		AddGlow(M_CONE, FVector(0, 0, 240), FVector(1.5f, 1.5f, 3.2f), FRotator::ZeroRotator, CrystalHDR);
		for (int32 i = 0; i < 4; ++i)
		{
			const float A = 2.f * PI * i / 4.f + PI / 4.f;
			AddGlow(M_CONE, FVector(FMath::Cos(A) * 130.f, FMath::Sin(A) * 130.f, 30.f),
				FVector(0.5f, 0.5f, 1.8f), FRotator(-15.f, FMath::RadiansToDegrees(A), 0.f), CrystalHDR * 0.85f);
		}
		// Lampe BLEUE DISCRÈTE : léger halo autour du cristal, PAS une flaque au sol.
		if (UPointLightComponent* PC = NewObject<UPointLightComponent>(this))
		{
			PC->SetupAttachment(RootComponent);
			PC->RegisterComponent();
			PC->SetRelativeLocation(FVector(0, 0, 240));
			PC->SetLightColor(FLinearColor(0.30f, 0.65f, 1.0f));
			PC->SetIntensity(3000.f);
			PC->SetAttenuationRadius(800.f);
			PC->SetCastShadows(false);
		}
	}
}
