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
	NameTag->SetRelativeLocation(FVector(0.f, 0.f, -140.f));
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
		if (BaseMat)
		{
			if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this))
			{
				MID->SetVectorParameterValue(TEXT("Color"), Color);
				C->SetMaterial(0, MID);
			}
		}
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
		AddPiece(M_SPH,  FVector(0, 0, 130), FVector(1.1f, 1.1f, 1.1f), FRotator::ZeroRotator, Glow);      // noyau lumineux
		// Tentacules bioluminescents autour de la base
		for (int32 i = 0; i < 6; ++i)
		{
			const float A = 2.f * PI * i / 6.f;
			AddPiece(M_CONE, FVector(FMath::Cos(A) * 150.f, FMath::Sin(A) * 150.f, -120.f),
				FVector(0.4f, 0.4f, 1.6f), FRotator(20.f, FMath::RadiansToDegrees(A), 0.f), Glow);
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
		AddPiece(M_CONE, FVector(0, 0, 240), FVector(1.5f, 1.5f, 3.2f), FRotator::ZeroRotator, Energy);    // grand cristal
		// Cristaux secondaires autour de la tour
		for (int32 i = 0; i < 4; ++i)
		{
			const float A = 2.f * PI * i / 4.f + PI / 4.f;
			AddPiece(M_CONE, FVector(FMath::Cos(A) * 130.f, FMath::Sin(A) * 130.f, 30.f),
				FVector(0.5f, 0.5f, 1.8f), FRotator(-15.f, FMath::RadiansToDegrees(A), 0.f), Energy);
		}
	}
}
