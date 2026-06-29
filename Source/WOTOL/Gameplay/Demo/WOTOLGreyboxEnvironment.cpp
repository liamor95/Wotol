#include "WOTOLGreyboxEnvironment.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Data/WOTOLTypes.h"

AWOTOLGreyboxEnvironment::AWOTOLGreyboxEnvironment()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AWOTOLGreyboxEnvironment::BeginPlay()
{
	Super::BeginPlay();
	BuildArena();
}

AStaticMeshActor* AWOTOLGreyboxEnvironment::SpawnBlock(
	const TCHAR* MeshPath, const FVector& Loc, const FVector& Scale, const FLinearColor& Color,
	const FRotator& Rot, bool bBlocking)
{
	UWorld* W = GetWorld();
	if (!W) return nullptr;

	FActorSpawnParameters P;
	P.Owner = this;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AStaticMeshActor* SMA = W->SpawnActor<AStaticMeshActor>(
		AStaticMeshActor::StaticClass(), Loc, Rot, P);
	if (!SMA) return nullptr;

	UStaticMeshComponent* Comp = SMA->GetStaticMeshComponent();
	if (!Comp) return SMA;

	Comp->SetMobility(EComponentMobility::Movable);

	// Décor non bloquant = traversable (les unités ne s'y coincent pas).
	// Le sol reste bloquant pour que les unités tiennent dessus.
	if (!bBlocking)
	{
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCanEverAffectNavigation(false);
	}

	if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath))
	{
		Comp->SetStaticMesh(Mesh);
	}
	SMA->SetActorScale3D(Scale);

	if (UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			Comp->SetMaterial(0, MID);
		}
	}
	return SMA;
}

void AWOTOLGreyboxEnvironment::BuildArena()
{
	const FVector Center = GetActorLocation();
	const EFactionID Rival = (PlayerFaction == EFactionID::Aquiloris)
		? EFactionID::Noxeens : EFactionID::Aquiloris;

	// Palette fond marin (sombre = ambiance sous-marine)
	const FLinearColor FloorColor(0.10f, 0.11f, 0.13f, 1.f); // sol : gris ardoise (pas anthracite)
	const FLinearColor RockColor (0.07f, 0.10f, 0.11f, 1.f); // roches du fond
	const FLinearColor SandColor (0.18f, 0.16f, 0.11f, 1.f); // bancs de sable
	const FLinearColor WallColor (0.05f, 0.07f, 0.08f, 1.f); // parois de la cuve
	const FLinearColor FarColor  (0.04f, 0.06f, 0.07f, 1.f); // silhouettes du fond

	const FRotator NoRot = FRotator::ZeroRotator;
	const TCHAR* Cube  = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* Plane = TEXT("/Engine/BasicShapes/Plane.Plane");
	const TCHAR* Cone  = TEXT("/Engine/BasicShapes/Cone.Cone");

	// ─── BROUILLARD SOUS-MARIN ───────────────────────────────────────────────
	// Assombrit l'horizon, masque le ciel/nuages et donne la profondeur d'eau.
	if (UWorld* W = GetWorld())
	{
		FActorSpawnParameters FP; FP.Owner = this;
		if (AExponentialHeightFog* Fog = W->SpawnActor<AExponentialHeightFog>(
				AExponentialHeightFog::StaticClass(), Center + FVector(0,0,-200.f), NoRot, FP))
		{
			if (UExponentialHeightFogComponent* FC = Fog->GetComponent())
			{
				FC->SetFogDensity(0.022f);
				FC->SetFogHeightFalloff(0.12f);
				FC->SetFogInscatteringColor(FLinearColor(0.015f, 0.06f, 0.08f, 1.f));
				FC->SetStartDistance(1200.f);
			}
		}
	}

	// ─── SOL agrandi (260m) = seul élément BLOQUANT (les unités tiennent dessus)
	SpawnBlock(Plane, Center, FVector(260.f, 260.f, 1.f), FloorColor, NoRot, true);

	// Plafond sombre tout en haut : masque le ciel bleu quand on regarde vers le haut
	SpawnBlock(Plane, Center + FVector(0.f, 0.f, 6000.f),
		FVector(320.f, 320.f, 1.f), FLinearColor(0.02f, 0.03f, 0.04f, 1.f), FRotator(180.f, 0.f, 0.f), false);

	// ─── CUVE : 4 parois hautes autour de l'arène (contexte fermé d'arène) ────
	const float WX = 9000.f;   // demi-largeur de la cuve
	const float WZ = 2000.f;   // hauteur des parois
	const float WT = 1.5f;     // épaisseur (en unités de cube *100)
	const float WL = 190.f;    // longueur des parois
	SpawnBlock(Cube, Center + FVector( WX, 0.f, WZ * 0.5f), FVector(WT, WL, WZ/100.f), WallColor, NoRot, false);
	SpawnBlock(Cube, Center + FVector(-WX, 0.f, WZ * 0.5f), FVector(WT, WL, WZ/100.f), WallColor, NoRot, false);
	SpawnBlock(Cube, Center + FVector(0.f,  WX, WZ * 0.5f), FVector(WL, WT, WZ/100.f), WallColor, NoRot, false);
	SpawnBlock(Cube, Center + FVector(0.f, -WX, WZ * 0.5f), FVector(WL, WT, WZ/100.f), WallColor, NoRot, false);

	// ─── SILHOUETTES DE FOND : grands triangles (cônes) au-delà des parois ────
	// Donnent un horizon/relief de profondeur (récifs lointains).
	const float FarRing[][3] = {
		{ 11000.f,  2500.f, 22.f}, { 11000.f, -3500.f, 30.f},
		{-11000.f,  3500.f, 28.f}, {-11000.f, -2500.f, 24.f},
		{  2500.f, 11000.f, 26.f}, { -3500.f, 11000.f, 32.f},
		{  3500.f,-11000.f, 30.f}, { -2500.f,-11000.f, 24.f},
		{  8000.f,  8000.f, 20.f}, { -8000.f, -8000.f, 20.f},
		{  8000.f, -8000.f, 18.f}, { -8000.f,  8000.f, 18.f},
	};
	for (const float* C : FarRing)
	{
		const float S = C[2];
		SpawnBlock(Cone, Center + FVector(C[0], C[1], S * 100.f * 0.5f),
			FVector(S * 0.7f, S * 0.7f, S), FarColor, NoRot, false);
	}

	// ─── Bancs de sable (planes plats, décor) ────────────────────────────────
	SpawnBlock(Plane, Center + FVector(-1600.f, 800.f, 3.f),  FVector(40.f, 30.f, 1.f), SandColor, NoRot, false);
	SpawnBlock(Plane, Center + FVector(1700.f, -900.f, 3.f),  FVector(45.f, 28.f, 1.f), SandColor, NoRot, false);
	SpawnBlock(Plane, Center + FVector(0.f, 1800.f, 3.f),     FVector(50.f, 25.f, 1.f), SandColor, NoRot, false);
	SpawnBlock(Plane, Center + FVector(-4500.f, -3000.f, 3.f),FVector(60.f, 45.f, 1.f), SandColor, NoRot, false);
	SpawnBlock(Plane, Center + FVector(4800.f, 3200.f, 3.f),  FVector(60.f, 45.f, 1.f), SandColor, NoRot, false);

	// Arche centrale (repère), décalée hors du couloir de combat
	SpawnBlock(Cube, Center + FVector(0.f, -900.f, 450.f), FVector(1.8f, 1.8f, 9.f), RockColor, NoRot, false);
	SpawnBlock(Cube, Center + FVector(0.f, 900.f, 450.f),  FVector(1.8f, 1.8f, 9.f), RockColor, NoRot, false);
	SpawnBlock(Cube, Center + FVector(0.f, 0.f, 920.f),    FVector(1.8f, 19.f, 1.f), RockColor, NoRot, false);

	// ─── Reliefs rocheux répartis sur TOUTE la map (pas seulement au centre) ──
	const float P[][4] = {
		// proches du champ de bataille
		{-2200.f, -1600.f, 300.f, 6.f}, { 2200.f,  1600.f, 300.f, 6.f},
		{-2400.f,  1400.f, 180.f, 4.5f},{ 2400.f, -1400.f, 180.f, 4.5f},
		{-1300.f, -2200.f, 220.f, 5.f}, { 1300.f,  2200.f, 220.f, 5.f},
		{ -700.f,  1700.f, 130.f, 3.5f},{  700.f, -1700.f, 130.f, 3.5f},
		{-3000.f,     0.f, 360.f, 7.f}, { 3000.f,     0.f, 360.f, 7.f},
		// alentours (remplissent le vide autour de l'arène)
		{-5200.f,  3600.f, 420.f, 9.f}, { 5200.f, -3600.f, 420.f, 9.f},
		{-5600.f, -2800.f, 300.f, 6.f}, { 5600.f,  2800.f, 300.f, 6.f},
		{-3800.f,  5200.f, 360.f, 7.f}, { 3800.f, -5200.f, 360.f, 7.f},
		{ -800.f,  5600.f, 260.f, 5.f}, {  800.f, -5600.f, 260.f, 5.f},
		{-6800.f,   600.f, 500.f,10.f}, { 6800.f,  -600.f, 500.f,10.f},
		{ 4200.f,  5400.f, 320.f, 6.f}, {-4200.f, -5400.f, 320.f, 6.f},
	};
	for (const float* R : P)
	{
		SpawnBlock(Cube, Center + FVector(R[0], R[1], R[2]),
			FVector(R[3], R[3], R[2] / 100.f), RockColor, NoRot, false);
	}

	(void)Rival; // (les zones de déploiement colorées arriveront avec l'écran de déploiement)
}
