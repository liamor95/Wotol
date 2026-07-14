#include "WOTOLCoverStructure.h"
#include "Gameplay/Units/UnitBase.h"
#include "Core/FactionRegistrySubsystem.h"
#include "WOTOLBubbleBurst.h"
#include "WOTOLGlow.h"
#include "WOTOLDamageNumber.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

AWOTOLCoverStructure::AWOTOLCoverStructure()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	HealthTag = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HealthTag"));
	HealthTag->SetupAttachment(SceneRoot);
	HealthTag->SetHorizontalAlignment(EHTA_Center);
	HealthTag->SetWorldSize(34.f);
	HealthTag->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	HealthTag->SetTextRenderColor(FColor(230, 210, 150, 255));
	HealthTag->SetVisibility(false);
}

void AWOTOLCoverStructure::BeginPlay()
{
	Super::BeginPlay();
	// TOUT est destructible desormais : les anciennes structures "indestructibles" deviennent
	// juste TRES resistantes (reperes solides), mais on peut FINIR par les abattre.
	if (bIndestructible)
	{
		bIndestructible = false;
		MaxHealth = FMath::Max(MaxHealth, 2600.f);
	}
	CurrentHealth = MaxHealth;
	BuildVisual();
}

void AWOTOLCoverStructure::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bFalling) { TickFall(DeltaSeconds); return; }
	if (bDestroyed || !HealthTag) return;

	// VAGUE DE LUMIÈRE À TRAVERS LE MODÈLE : cycle ~4.5 s. Pendant ~1.4 s une onde monte de la
	// base au sommet ; toute pièce dont la hauteur est proche de l'onde s'illumine (émissif +
	// bloom, intensité selon la proximité), puis revient en pierre -> la lumière ÉPOUSE la
	// forme du modèle (comme le cristal du Cristalliseur), pas un carré.
	// Onde RAPIDE et DISCRÈTE : le balayage traverse le modèle très vite (~0,4 s) et l'émissif
	// reste FAIBLE (juste assez pour distinguer la forme, pas un blob blanc). Cycle court.
	ScanTimer += DeltaSeconds;
	const float Cycle = FMath::Fmod(ScanTimer, 2.6f);
	const bool  bSweep = (Cycle < 0.4f);                       // passage TRÈS rapide
	const float SweepZ = bSweep ? (Cycle / 0.4f) * ScanTop : -100000.f;
	const float Band   = FMath::Max(90.f, ScanTop * 0.16f);   // onde fine
	for (int32 i = 0; i < Parts.Num(); ++i)
	{
		UStaticMeshComponent* C = Parts[i];
		if (!C || i >= PartPulseMID.Num()) continue;
		const float Dist = FMath::Abs(PartZ[i] - SweepZ);
		if (bSweep && Dist < Band)
		{
			const float Inten = 1.f - Dist / Band;                 // 0 (bord) -> 1 (centre de l'onde)
			if (PartPulseMID[i])
				// Cyan DOUX (luminosité fortement réduite) : on voit la lumière épouser la
				// forme, sans cramer en blanc. Peak ≈ (0.32, 0.75, 1.0) -> à peine au-dessus
				// du seuil de bloom, très léger.
				PartPulseMID[i]->SetVectorParameterValue(TEXT("Color"),
					FLinearColor(0.12f + 0.20f * Inten, 0.35f + 0.40f * Inten, 0.50f + 0.50f * Inten, 1.f));
			if (!PartGlowing[i]) { C->SetMaterial(0, PartPulseMID[i]); PartGlowing[i] = 1; }
		}
		else if (PartGlowing[i])
		{
			if (PartRestMID[i]) C->SetMaterial(0, PartRestMID[i]); // retour pierre
			PartGlowing[i] = 0;
		}
	}

	// Étiquette PV TOUJOURS visible : toute structure est destructible.
	HealthTag->SetVisibility(true);
	HealthTag->SetText(FText::FromString(FString::Printf(TEXT("Ruine  %d / %d"),
		FMath::RoundToInt(CurrentHealth), FMath::RoundToInt(MaxHealth))));
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		if (PC->PlayerCameraManager)
		{
			FRotator F = (PC->PlayerCameraManager->GetCameraLocation() - HealthTag->GetComponentLocation()).Rotation();
			F.Pitch = 0.f; F.Roll = 0.f;
			HealthTag->SetWorldRotation(F);
		}
}

// Ajoute une pièce (mesh primitif) avec collision bloquante (unités + tirs).
static UStaticMeshComponent* AddCoverPiece(AActor* Owner, USceneComponent* Root,
	const TCHAR* MeshPath, const FVector& Loc, const FVector& Scale, const FRotator& Rot,
	const FLinearColor& Color)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner);
	if (!C) return nullptr;
	C->SetupAttachment(Root);
	C->RegisterComponent();
	if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, MeshPath)) C->SetStaticMesh(M);
	C->SetRelativeLocationAndRotation(Loc, Rot);
	C->SetRelativeScale3D(Scale);
	// Bloque tout : unités (WorldStatic/Pawn) ET les tracés de tir (couverture).
	C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	C->SetCollisionObjectType(ECC_WorldStatic);
	C->SetCollisionResponseToAllChannels(ECR_Block);
	// Couleur vive (cristaux/conduits) -> émissif (brille) ; sinon -> mat rugueux.
	const bool bEmissive = (Color.R > 1.2f || Color.G > 1.2f || Color.B > 1.2f);
	if (UMaterialInstanceDynamic* MID = bEmissive
			? WOTOLGlow::MakeGlow(Owner, Color) : WOTOLGlow::MakeMatte(Owner, Color))
		C->SetMaterial(0, MID);
	return C;
}

void AWOTOLCoverStructure::BuildVisual()
{
	const TCHAR* M_CUBE = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* M_CYL  = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* M_CONE = TEXT("/Engine/BasicShapes/Cone.Cone");

	// Ruines de pierre ENCROÛTÉES (fond marin) : pierre chaude patinée + algues, PAS bleu.
	// Une pointe de variété par instance pour qu'elles ne soient pas toutes identiques.
	const float Tint = ((Variant * 37 + 13) % 100) / 100.f; // 0..1 déterministe par variante
	const FLinearColor Stone = FMath::Lerp(
		FLinearColor(0.44f, 0.38f, 0.30f, 1.f),  // grès brun chaud
		FLinearColor(0.34f, 0.40f, 0.26f, 1.f),  // pierre verdie par les algues
		Tint);
	const FLinearColor Dark (0.24f, 0.22f, 0.18f, 1.f); // pierre humide sombre
	const FLinearColor Glow (0.35f, 1.60f, 1.20f, 1.f); // cristal/conduit bioluminescent (émissif)

	auto Add = [&](const TCHAR* Mesh, const FVector& L, const FVector& S, const FRotator& R, const FLinearColor& Col)
	{
		if (UStaticMeshComponent* C = AddCoverPiece(this, SceneRoot, Mesh, L, S, R, Col)) Parts.Add(C);
	};

	// Ruines d'une CITÉ ENGLOUTIE (réf. images) : temples anguleux effondrés, arches/anneaux
	// de pierre, grands escaliers, colonnades, dalles pyramidales inclinées. HAUTES (la chute
	// se voit) et bâties en MORCEAUX distincts (chaque bloc se détache à la destruction).
	if (Variant == 1) // MUR DE TEMPLE À CORNICHE (pan de mur haut + pilastres + corniche)
	{
		Add(M_CUBE, FVector(0, 0, 40),  FVector(1.3f, 6.0f, 0.5f), FRotator::ZeroRotator, Dark);   // socle
		Add(M_CUBE, FVector(0, 0, 330), FVector(0.6f, 5.4f, 5.4f), FRotator::ZeroRotator, Stone);  // grand mur
		Add(M_CUBE, FVector(0, -230, 320), FVector(0.9f, 0.8f, 5.2f), FRotator::ZeroRotator, Dark); // pilastre G
		Add(M_CUBE, FVector(0,  230, 320), FVector(0.9f, 0.8f, 5.2f), FRotator::ZeroRotator, Dark); // pilastre D
		Add(M_CUBE, FVector(0, 0, 610), FVector(1.2f, 6.2f, 0.7f), FRotator(0, 0, 3.f), Stone);     // corniche
		Add(M_CUBE, FVector(0, 190, 690), FVector(0.7f, 1.4f, 1.0f), FRotator(0, 0, 10.f), Stone);  // bloc de faîte brisé
		Add(M_CONE, FVector(60, -120, 470), FVector(0.14f, 0.14f, 1.2f), FRotator::ZeroRotator, Glow);
	}
	else if (Variant == 2) // ANNEAU / ARCHE DE PIERRE (portail rond en voussoirs, réf. images)
	{
		Add(M_CUBE, FVector(0, 0, 30), FVector(1.4f, 4.6f, 0.5f), FRotator::ZeroRotator, Dark);     // dallage
		Add(M_CUBE, FVector(0, -300, 220), FVector(0.9f, 0.9f, 4.4f), FRotator::ZeroRotator, Stone); // pied G
		Add(M_CUBE, FVector(0,  300, 220), FVector(0.9f, 0.9f, 4.4f), FRotator::ZeroRotator, Stone); // pied D
		// Demi-cercle de voussoirs (blocs le long d'un arc, plan Y-Z).
		const int32 N = 7; const float Rad = 330.f; const float Cz = 440.f;
		for (int32 i = 0; i < N; ++i)
		{
			const float a = PI * (0.10f + 0.80f * i / (N - 1)); // ~18°..162°
			const FVector L(0.f, -FMath::Cos(a) * Rad, Cz + FMath::Sin(a) * Rad);
			Add(M_CUBE, L, FVector(0.9f, 0.95f, 0.95f),
				FRotator(0.f, 0.f, FMath::RadiansToDegrees(a) - 90.f), (i % 2) ? Stone : Dark);
		}
		Add(M_CONE, FVector(0, 0, Cz + Rad + 40.f), FVector(0.2f, 0.2f, 0.7f), FRotator::ZeroRotator, Glow);
	}
	else if (Variant == 3) // TEMPLE À TOIT EN BÂTIÈRE EFFONDRÉ (grande halle engloutie, penchée)
	{
		Add(M_CUBE, FVector(0, 0, 40),  FVector(4.6f, 5.4f, 0.6f), FRotator::ZeroRotator, Dark);     // plateforme
		Add(M_CUBE, FVector(-140, -260, 260), FVector(3.2f, 0.7f, 4.2f), FRotator::ZeroRotator, Stone); // mur latéral G
		Add(M_CUBE, FVector(-140,  260, 260), FVector(3.2f, 0.7f, 4.2f), FRotator::ZeroRotator, Stone); // mur latéral D
		Add(M_CUBE, FVector(-360, 0, 300), FVector(0.7f, 4.4f, 4.6f), FRotator::ZeroRotator, Dark);   // mur du fond (porte sombre)
		// Toit en bâtière (deux pans inclinés) qui s'effondre vers l'avant -> penché.
		Add(M_CUBE, FVector(-40, -170, 560), FVector(3.6f, 2.0f, 0.5f), FRotator(0, 6.f, 42.f), Stone); // pan G
		Add(M_CUBE, FVector(-40,  170, 560), FVector(3.6f, 2.0f, 0.5f), FRotator(0, 6.f, -42.f), Stone);// pan D
		Add(M_CUBE, FVector(260, 0, 120), FVector(1.2f, 3.0f, 0.6f), FRotator(0, 0, 4.f), Dark);      // marches basses devant
		Add(M_CONE, FVector(-140, 0, 360), FVector(0.2f, 0.2f, 1.1f), FRotator::ZeroRotator, Glow);   // lueur interne
	}
	else if (Variant == 4) // GRAND ESCALIER + PLATEFORME (montée cérémonielle brisée)
	{
		for (int32 i = 0; i < 6; ++i) // volée de marches
			Add(M_CUBE, FVector(-i * 90.f, 0, 40 + i * 70.f), FVector(0.9f, 4.4f, 0.7f), FRotator::ZeroRotator, (i % 2) ? Stone : Dark);
		Add(M_CUBE, FVector(-620, 0, 470), FVector(3.0f, 4.6f, 0.7f), FRotator::ZeroRotator, Stone);  // plateforme haute
		Add(M_CYL,  FVector(-620, -150, 720), FVector(0.7f, 0.7f, 3.2f), FRotator::ZeroRotator, Stone); // colonne brisée
		Add(M_CUBE, FVector(-620, 160, 690), FVector(1.0f, 1.0f, 1.0f), FRotator(12.f, 20.f, 8.f), Dark); // bloc tombé
		Add(M_CONE, FVector(-620, 0, 560), FVector(0.16f, 0.16f, 1.0f), FRotator::ZeroRotator, Glow);
	}
	else if (Variant == 5) // DALLE PYRAMIDALE INCLINÉE (haute tour-monolithe penchée, réf. image sombre)
	{
		Add(M_CUBE, FVector(0, 0, 40), FVector(2.2f, 2.6f, 0.6f), FRotator::ZeroRotator, Dark);       // socle
		Add(M_CUBE, FVector(60, 0, 470), FVector(1.1f, 2.2f, 9.0f), FRotator(0, 0, 20.f), Stone);     // grande dalle TRES haute, penchée
		Add(M_CUBE, FVector(-120, 0, 260), FVector(0.9f, 1.8f, 4.8f), FRotator(0, 0, -8.f), Dark);    // contrefort
		Add(M_CUBE, FVector(-260, 130, 80), FVector(1.1f, 1.1f, 1.1f), FRotator(10.f, 30.f, 10.f), Stone); // bloc tombé
		Add(M_CUBE, FVector(-240, -150, 70), FVector(0.9f, 0.9f, 0.9f), FRotator(8.f, 200.f, 6.f), Dark);  // bloc tombé
		Add(M_CONE, FVector(90, 0, 860), FVector(0.24f, 0.24f, 1.2f), FRotator(20.f, 0, 0), Glow);    // cristal au sommet
	}
	else // COLONNADE DE TEMPLE (portique : rangée de colonnes + entablement) — repère majeur
	{
		Add(M_CUBE, FVector(0, 0, 40), FVector(2.4f, 7.2f, 0.6f), FRotator::ZeroRotator, Dark);       // stylobate (base)
		Add(M_CUBE, FVector(0, 0, 120), FVector(2.0f, 6.8f, 0.5f), FRotator::ZeroRotator, Stone);     // gradin
		const float Ys[4] = { -420.f, -140.f, 140.f, 420.f };
		for (int32 i = 0; i < 4; ++i) // 4 colonnes (une brisée plus courte)
		{
			const float h = (i == 2) ? 3.6f : 6.0f; const float z = 200.f + h * 50.f;
			Add(M_CYL, FVector(0, Ys[i], z), FVector(0.8f, 0.8f, h), FRotator::ZeroRotator, Stone);
		}
		Add(M_CUBE, FVector(0, -140, 730), FVector(1.4f, 6.4f, 0.8f), FRotator(0, 0, 2.f), Stone);    // architrave
		Add(M_CUBE, FVector(0, 380, 790), FVector(1.0f, 1.6f, 0.9f), FRotator(0, 0, 14.f), Stone);    // fronton brisé
		Add(M_CONE, FVector(40, 0, 560), FVector(0.18f, 0.18f, 1.4f), FRotator::ZeroRotator, Glow);
	}

	// Hauteur pour le texte / burst de sommet, selon la silhouette.
	const float TopZ = (Variant == 5) ? 900.f : (Variant == 0) ? 820.f : (Variant == 2) ? 800.f
		: (Variant == 4) ? 780.f : 700.f;
	HealthTag->SetRelativeLocation(FVector(0.f, 0.f, TopZ + 60.f));
	PillarLen = TopZ;
	ScanTop   = TopZ;

	// FEEDBACK : pour CHAQUE pièce du modèle, on garde son matériau NORMAL (pierre) + on prépare
	// un matériau ÉMISSIF (bloom) de la même pièce. La vague de lumière (Tick) fait BASCULER une
	// pièce sur son émissif quand l'onde la traverse -> le MODÈLE s'illumine dans sa forme.
	PartRestMID.Reset(); PartPulseMID.Reset(); PartZ.Reset(); PartGlowing.Reset();
	for (UStaticMeshComponent* C : Parts)
	{
		UMaterialInstanceDynamic* Rest = C ? Cast<UMaterialInstanceDynamic>(C->GetMaterial(0)) : nullptr;
		UMaterialInstanceDynamic* Pulse = WOTOLGlow::MakeGlow(this, FLinearColor(0.5f, 1.6f, 2.2f, 1.f)); // cyan lumineux
		PartRestMID.Add(Rest);
		PartPulseMID.Add(Pulse);
		PartZ.Add(C ? C->GetRelativeLocation().Z : 0.f);
		PartGlowing.Add(0);
	}
}

void AWOTOLCoverStructure::TakeCoverDamage(float Amount, AUnitBase* InstigatorUnit)
{
	if (bDestroyed || bIndestructible || Amount <= 0.f) return;
	CurrentHealth -= Amount;
	// Mémorise le SENS DU TIR (du tireur vers la structure) -> la structure tombera dans
	// ce sens (elle bascule "dans le sens dans lequel on tire").
	if (InstigatorUnit)
	{
		FVector D = GetActorLocation() - InstigatorUnit->GetActorLocation(); D.Z = 0.f;
		if (!D.IsNearlyZero()) { LastFireDir = D.GetSafeNormal(); bHasFireDir = true; }
	}
	if (CurrentHealth <= 0.f) Collapse();
}

void AWOTOLCoverStructure::Collapse()
{
	if (bDestroyed || bFalling) return;
	UWorld* W = GetWorld();
	const FVector Origin = GetActorLocation();

	// BIAIS D'ÉJECTION (les morceaux partent globalement dans ce sens) : sens du tir, sinon
	// vers l'ennemi le plus proche, sinon aléatoire.
	FVector Dir = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f).GetSafeNormal();
	if (bHasFireDir) Dir = LastFireDir;
	else if (W)
		if (UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>())
		{
			float Best = TNumericLimits<float>::Max();
			const EFactionID Facs[2] = { EFactionID::Aquiloris, EFactionID::Noxeens };
			for (EFactionID F : Facs)
				for (AUnitBase* U : Reg->GetUnitsForFaction(F))
				{
					if (!U || !U->IsAlive()) continue;
					FVector To = U->GetActorLocation() - Origin; To.Z = 0.f;
					const float D = To.Size();
					if (D > 60.f && D < Best) { Best = D; Dir = To.GetSafeNormal(); }
				}
		}
	FallDir = Dir.IsNearlyZero() ? FVector(1, 0, 0) : Dir;

	// ── DÉGÂTS D'EFFONDREMENT (une fois) : l'édifice s'abat -> zone autour de sa base. ──
	if (W)
		if (UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>())
		{
			const EFactionID Facs[2] = { EFactionID::Aquiloris, EFactionID::Noxeens };
			for (EFactionID F : Facs)
				for (AUnitBase* U : Reg->GetUnitsForFaction(F))
				{
					if (!U || !U->IsAlive()) continue;
					FVector To = U->GetActorLocation() - Origin; To.Z = 0.f;
					if (To.Size() <= DebrisRadius * 1.35f)
					{
						U->TakeDamageFromUnit(DebrisDamage, nullptr);
						U->LaunchCharacter(To.GetSafeNormal() * 620.f + FVector(0, 0, 240.f), true, true);
						AWOTOLBubbleBurst::Burst(W, U->GetActorLocation() + FVector(0, 0, 40.f),
							FLinearColor(0.6f, 0.62f, 0.68f, 1.f), 10);
					}
				}
		}

	// ── ÉCLATEMENT EN FRAGMENTS : chaque morceau reçoit SA propre vitesse (dispersion) +
	// rotation -> ils volent dans des directions differentes et retombent a des endroits
	// differents (plusieurs points de chute, pas un seul bloc). ──
	bFalling = true; FallElapsed = 0.f;
	PartVel.Reset(); PartAngAxis.Reset(); PartAngSpeed.Reset(); PartLanded.Reset();
	for (UStaticMeshComponent* C : Parts)
	{
		if (!C) { PartVel.Add(FVector::ZeroVector); PartAngAxis.Add(FVector::UpVector); PartAngSpeed.Add(0.f); PartLanded.Add(1); continue; }
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		const FVector Rel = C->GetRelativeLocation();
		FVector Out(Rel.X, Rel.Y, 0.f);                       // vers l'exterieur depuis l'axe
		if (Out.IsNearlyZero()) Out = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f);
		Out = Out.GetSafeNormal();
		const FVector Jit(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f);
		FVector HDir = (Out * 0.7f + FallDir * 0.6f + Jit * 0.5f).GetSafeNormal();
		const float HeightFactor = 1.f + Rel.Z / 700.f;       // les morceaux HAUTS sont projetes plus loin/haut
		// Vitesses REDUITES (chute plus lente = ressenti SOUS-MARIN, demande joueur).
		const float HSpeed = FMath::FRandRange(90.f, 220.f) * HeightFactor;
		const float VSpeed = FMath::FRandRange(120.f, 280.f) + Rel.Z * 0.20f;
		PartVel.Add(HDir * HSpeed + FVector(0, 0, VSpeed));
		PartAngAxis.Add(FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)).GetSafeNormal());
		PartAngSpeed.Add(FMath::FRandRange(120.f, 340.f));    // deg/s
		PartLanded.Add(0);
	}

	if (HealthTag) HealthTag->SetVisibility(false);
	if (W)
	{
		AWOTOLDamageNumber::SpawnText(W, Origin + FVector(0, 0, PillarLen), TEXT("Il s'effondre !"),
			FLinearColor(0.9f, 0.85f, 0.6f, 1.f));
		AWOTOLBubbleBurst::Burst(W, Origin + FVector(0, 0, PillarLen * 0.5f), FLinearColor(0.62f, 0.64f, 0.7f, 1.f), 26);
	}
}

void AWOTOLCoverStructure::TickFall(float Dt)
{
	UWorld* W = GetWorld();
	if (!W) return;
	FallElapsed += Dt;

	const float Gravity = 900.f;  // gravite ADOUCIE -> les debris retombent lentement (sous l'eau)
	const float RestZ   = 22.f;   // hauteur locale de repos des gravats (petit tas au sol)
	int32 Remaining = 0;

	for (int32 i = 0; i < Parts.Num(); ++i)
	{
		UStaticMeshComponent* C = Parts[i];
		if (!C || i >= PartVel.Num() || PartLanded[i]) continue;

		PartVel[i].Z -= Gravity * Dt;
		FVector Rel = C->GetRelativeLocation() + PartVel[i] * Dt;

		// Rotation propre du fragment (culbute).
		const FQuat Spin(PartAngAxis[i], FMath::DegreesToRadians(PartAngSpeed[i]) * Dt);
		C->SetRelativeRotation((Spin * C->GetRelativeRotation().Quaternion()).Rotator());

		if (Rel.Z <= RestZ)
		{
			Rel.Z = RestZ;
			C->SetRelativeLocation(Rel);
			// Aplatit un peu le fragment au sol (gravats).
			const FVector S = C->GetRelativeScale3D();
			C->SetRelativeScale3D(FVector(S.X, S.Y, FMath::Max(0.15f, S.Z * 0.5f)));
			PartLanded[i] = 1;
			AWOTOLBubbleBurst::Burst(W, C->GetComponentLocation() + FVector(0, 0, 20.f),
				FLinearColor(0.62f, 0.64f, 0.7f, 1.f), 5);
		}
		else
		{
			C->SetRelativeLocation(Rel);
			++Remaining;
		}
	}

	if (Remaining == 0 || FallElapsed >= FallDuration)
	{
		bFalling = false;
		bDestroyed = true;
	}
}
