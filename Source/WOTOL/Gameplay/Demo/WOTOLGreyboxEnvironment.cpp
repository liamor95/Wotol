#include "WOTOLGreyboxEnvironment.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/PostProcessVolume.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
#include "Data/WOTOLTypes.h"

namespace
{
	const TCHAR* MESH_CUBE = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* MESH_CONE = TEXT("/Engine/BasicShapes/Cone.Cone");
	const TCHAR* MESH_CYL  = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* MESH_SPH  = TEXT("/Engine/BasicShapes/Sphere.Sphere");

	// Petite variation de teinte pour casser l'uniformité (kitbash plus crédible).
	FLinearColor Vary(const FLinearColor& C, float D)
	{
		return FLinearColor(
			FMath::Max(0.f, C.R + D), FMath::Max(0.f, C.G + D),
			FMath::Max(0.f, C.B + D), 1.f);
	}
}

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

// ─── Kitbash : rocher = amas de cubes/sphères ───────────────────────────────
void AWOTOLGreyboxEnvironment::SpawnRock(const FVector& Center, float Size, const FLinearColor& Color, int32 Seed)
{
	FRandomStream R(Seed);

	// Bloc central
	const float CoreS = Size * R.FRandRange(0.7f, 0.9f);
	SpawnBlock(MESH_CUBE, Center + FVector(0, 0, CoreS * 0.4f),
		FVector(CoreS / 100.f), Vary(Color, R.FRandRange(-0.01f, 0.02f)),
		FRotator(R.FRandRange(0.f, 40.f), R.FRandRange(0.f, 360.f), R.FRandRange(0.f, 40.f)), false);

	// Éclats autour
	const int32 Chunks = R.RandRange(5, 9);
	for (int32 i = 0; i < Chunks; ++i)
	{
		const float ChunkS = Size * R.FRandRange(0.25f, 0.6f);
		const FVector Off(
			R.FRandRange(-Size, Size) * 0.5f,
			R.FRandRange(-Size, Size) * 0.5f,
			R.FRandRange(0.f, Size * 0.6f));
		const bool bRound = R.FRand() < 0.35f;
		SpawnBlock(bRound ? MESH_SPH : MESH_CUBE, Center + Off,
			FVector(ChunkS / 100.f), Vary(Color, R.FRandRange(-0.02f, 0.02f)),
			FRotator(R.FRandRange(0.f, 360.f), R.FRandRange(0.f, 360.f), R.FRandRange(0.f, 360.f)), false);
	}
}

// ─── Kitbash : chaîne de montagnes = cônes chevauchants ─────────────────────
void AWOTOLGreyboxEnvironment::SpawnRidge(const FVector& Start, const FVector& End, float Height,
	float Width, const FLinearColor& Color, int32 Seed)
{
	FRandomStream R(Seed);
	const int32 Peaks = R.RandRange(6, 10);
	for (int32 i = 0; i <= Peaks; ++i)
	{
		const float T = (float)i / Peaks;
		FVector Pos = FMath::Lerp(Start, End, T);
		Pos += FVector(R.FRandRange(-Width, Width) * 0.4f, R.FRandRange(-Width, Width) * 0.4f, 0.f);
		const float H = Height * R.FRandRange(0.6f, 1.2f);
		const float Wd = Width * R.FRandRange(0.7f, 1.2f);
		SpawnBlock(MESH_CONE, Pos + FVector(0, 0, H * 0.5f),
			FVector(Wd / 50.f, Wd / 50.f, H / 100.f),
			Vary(Color, R.FRandRange(-0.015f, 0.015f)),
			FRotator(0.f, R.FRandRange(0.f, 360.f), 0.f), false);
	}
}

// ─── Ziggourat à gradins + escalier frontal ─────────────────────────────────
void AWOTOLGreyboxEnvironment::SpawnZiggurat(const FVector& Base, float BaseHalf, int32 Tiers,
	float TierHeight, float YawDeg, const FLinearColor& Color)
{
	for (int32 i = 0; i < Tiers; ++i)
	{
		const float Half = BaseHalf * (1.f - (float)i / (Tiers + 1));
		const float Z = i * TierHeight + TierHeight * 0.5f;
		SpawnBlock(MESH_CUBE, Base + FVector(0, 0, Z),
			FVector(Half * 2.f / 100.f, Half * 2.f / 100.f, TierHeight / 100.f),
			Vary(Color, (i % 2) ? 0.015f : -0.01f),
			FRotator(0.f, YawDeg, 0.f), false);
	}
	// Escalier frontal (vers -X local avant rotation)
	const FVector Front = FRotator(0.f, YawDeg, 0.f).RotateVector(FVector(-BaseHalf, 0.f, 0.f));
	SpawnStairs(Base + Front, YawDeg + 180.f, Tiers * 2, BaseHalf * 0.6f, Color);
}

// ─── Escalier orienté ───────────────────────────────────────────────────────
void AWOTOLGreyboxEnvironment::SpawnStairs(const FVector& Base, float YawDeg, int32 Steps,
	float Width, const FLinearColor& Color)
{
	const FRotator Yaw(0.f, YawDeg, 0.f);
	const float Depth = 140.f, StepH = 90.f;
	for (int32 i = 0; i < Steps; ++i)
	{
		const FVector Off = Yaw.RotateVector(FVector(-i * Depth, 0.f, i * StepH + StepH * 0.5f));
		SpawnBlock(MESH_CUBE, Base + Off,
			FVector(Depth / 100.f, Width * 2.f / 100.f, StepH / 100.f),
			Vary(Color, (i % 2) ? 0.01f : -0.01f), Yaw, false);
	}
}

// ─── Colonnade (cylindres, certaines brisées) ───────────────────────────────
void AWOTOLGreyboxEnvironment::SpawnColonnade(const FVector& Start, const FVector& Step, int32 Count,
	float Height, const FLinearColor& Color, int32 Seed)
{
	FRandomStream R(Seed);
	for (int32 i = 0; i < Count; ++i)
	{
		const FVector Pos = Start + Step * (float)i;
		const float H = Height * (R.FRand() < 0.3f ? R.FRandRange(0.3f, 0.6f) : 1.f); // colonne brisée
		const float Rad = 90.f;
		SpawnBlock(MESH_CYL, Pos + FVector(0, 0, H * 0.5f),
			FVector(Rad / 100.f, Rad / 100.f, H / 100.f),
			Vary(Color, R.FRandRange(-0.01f, 0.01f)), FRotator::ZeroRotator, false);
		// Chapiteau
		if (H > Height * 0.7f)
		{
			SpawnBlock(MESH_CUBE, Pos + FVector(0, 0, H),
				FVector(Rad * 2.4f / 100.f, Rad * 2.4f / 100.f, 0.4f),
				Vary(Color, 0.02f), FRotator::ZeroRotator, false);
		}
	}
}

// ─── Arche de pierre (anneau de blocs) ──────────────────────────────────────
void AWOTOLGreyboxEnvironment::SpawnArch(const FVector& Base, float Radius, float YawDeg, const FLinearColor& Color)
{
	const FRotator Yaw(0.f, YawDeg, 0.f);
	const int32 Segments = 11;
	for (int32 i = 0; i <= Segments; ++i)
	{
		const float Ang = PI * (float)i / Segments; // 0..180°
		const FVector Local(FMath::Cos(Ang) * Radius, 0.f, FMath::Sin(Ang) * Radius);
		const FVector Pos = Base + Yaw.RotateVector(Local);
		const float BlockPitch = -FMath::RadiansToDegrees(Ang) + 90.f;
		SpawnBlock(MESH_CUBE, Pos,
			FVector(2.6f, 1.4f, 1.0f), Vary(Color, (i % 2) ? 0.015f : -0.01f),
			FRotator(BlockPitch, YawDeg, 0.f), false);
	}
}

// ─── Dallage / plateau de pierre ────────────────────────────────────────────
void AWOTOLGreyboxEnvironment::SpawnPlaza(const FVector& Center, float HalfX, float HalfY, const FLinearColor& Color)
{
	SpawnBlock(MESH_CUBE, Center + FVector(0, 0, 12.f),
		FVector(HalfX * 2.f / 100.f, HalfY * 2.f / 100.f, 0.24f), Color, FRotator::ZeroRotator, false);
}

void AWOTOLGreyboxEnvironment::BuildArena()
{
	const FVector Center = GetActorLocation();

	// ── Palette inspirée de la référence (cité engloutie, eau turquoise) ──
	const FLinearColor FloorColor(0.06f, 0.11f, 0.14f, 1.f); // fond marin bleu-vert sombre
	const FLinearColor StoneColor(0.13f, 0.17f, 0.19f, 1.f); // pierre bleu-gris des ruines
	const FLinearColor RockColor (0.09f, 0.13f, 0.14f, 1.f); // rochers / gravats
	const FLinearColor SandColor (0.24f, 0.21f, 0.14f, 1.f); // sable beige (avant-plan)
	const FLinearColor FarColor  (0.04f, 0.08f, 0.11f, 1.f); // silhouettes lointaines
	const FLinearColor SurfColor (0.10f, 0.30f, 0.40f, 1.f); // surface éclairée au-dessus

	const FRotator NoRot = FRotator::ZeroRotator;

	// ── AMBIANCE SOUS-MARINE (sans plugin Water) : brouillard + post-process ──
	if (UWorld* W = GetWorld())
	{
		FActorSpawnParameters FP; FP.Owner = this;

		// Brouillard turquoise LÉGER : teinte l'horizon sans noyer la scène
		if (AExponentialHeightFog* Fog = W->SpawnActor<AExponentialHeightFog>(
				AExponentialHeightFog::StaticClass(), Center + FVector(0, 0, -200.f), NoRot, FP))
		{
			if (UExponentialHeightFogComponent* FC = Fog->GetComponent())
			{
				FC->SetFogDensity(0.015f);
				FC->SetFogHeightFalloff(0.10f);
				FC->SetFogInscatteringColor(FLinearColor(0.03f, 0.14f, 0.20f, 1.f));
				FC->SetStartDistance(1500.f);
			}
		}

		// Volume post-process GLOBAL : LÉGÈRE teinte bleu-vert (sans assombrir —
		// surtout PAS d'exposition manuelle, qui rendait l'écran noir).
		if (APostProcessVolume* PPV = W->SpawnActor<APostProcessVolume>(
				APostProcessVolume::StaticClass(), Center, NoRot, FP))
		{
			PPV->bUnbound = true;
			PPV->Priority = 100.f;
			FPostProcessSettings& S = PPV->Settings;
			// Gain proche de 1 (ne descend pas trop) avec dominante bleue douce
			S.bOverride_ColorGain = true;        S.ColorGain = FVector4(0.85f, 0.97f, 1.10f, 1.f);
			S.bOverride_ColorSaturation = true;  S.ColorSaturation = FVector4(0.92f, 0.97f, 1.05f, 1.f);
			S.bOverride_VignetteIntensity  = true; S.VignetteIntensity = 0.35f;
			S.bOverride_SceneFringeIntensity = true; S.SceneFringeIntensity = 0.8f;
		}

		// Masque UNIQUEMENT les nuages volumétriques (NE PAS toucher au SkyAtmosphere
		// ni au SkyLight : les masquer supprimait tout l'éclairage → écran noir).
		for (TActorIterator<AActor> It(W); It; ++It)
		{
			if (It->GetClass()->GetName().Contains(TEXT("VolumetricCloud")))
			{
				It->SetActorHiddenInGame(true);
			}
		}
	}

	// ── Sol (seul élément BLOQUANT) + surface lumineuse au-dessus ──
	SpawnBlock(MESH_CUBE, Center + FVector(0, 0, -50.f),
		FVector(280.f, 280.f, 1.f), FloorColor, NoRot, true); // dalle de sol épaisse
	// "Surface" turquoise éclairée tout en haut (lumière venant d'en haut, comme la réf)
	SpawnBlock(TEXT("/Engine/BasicShapes/Plane.Plane"), Center + FVector(0, 0, 7000.f),
		FVector(340.f, 340.f, 1.f), SurfColor, FRotator(180.f, 0.f, 0.f), false);

	// ── Avant-plan sablonneux (beige clair, comme le bas de l'image) ──
	SpawnPlaza(Center + FVector(-3500.f, -2200.f, 0.f), 2600.f, 1800.f, SandColor);
	SpawnPlaza(Center + FVector(-1200.f, -3800.f, 0.f), 2200.f, 1500.f, SandColor);

	// ── Dallages de pierre (places carrelées) ──
	SpawnPlaza(Center + FVector(2200.f, 1200.f, 0.f), 2400.f, 1800.f, StoneColor);
	SpawnPlaza(Center + FVector(-2600.f, 1600.f, 0.f), 2000.f, 1500.f, StoneColor);

	// ── ARCHE centrale (repère focal, comme dans la référence) ──
	SpawnArch(Center + FVector(200.f, -300.f, 0.f), 700.f, 25.f, StoneColor);

	// ── Ruines à gradins (ziggourats + escaliers) de chaque côté ──
	SpawnZiggurat(Center + FVector(-4200.f, 2600.f, 0.f), 1100.f, 5, 220.f, -20.f, StoneColor);
	SpawnZiggurat(Center + FVector(4300.f, 2200.f, 0.f),  1300.f, 6, 230.f, 200.f, StoneColor);
	SpawnZiggurat(Center + FVector(3600.f, -2600.f, 0.f), 900.f,  4, 210.f, 150.f, StoneColor);

	// ── Escaliers indépendants (descentes vers la place) ──
	SpawnStairs(Center + FVector(-1600.f, 700.f, 0.f), 0.f, 8, 700.f, StoneColor);
	SpawnStairs(Center + FVector(1500.f, -1400.f, 0.f), 180.f, 7, 600.f, StoneColor);

	// ── Colonnades (rangées de colonnes brisées) ──
	SpawnColonnade(Center + FVector(-5400.f, -200.f, 0.f), FVector(0.f, 600.f, 0.f), 6, 850.f, StoneColor, 11);
	SpawnColonnade(Center + FVector(5200.f, 400.f, 0.f),  FVector(0.f, -600.f, 0.f), 6, 850.f, StoneColor, 23);

	// ── Grands pans de murs brisés qui encadrent l'arène (gauche/droite) ──
	SpawnBlock(MESH_CUBE, Center + FVector(-7200.f, 0.f, 1300.f),
		FVector(2.f, 90.f, 26.f), StoneColor, FRotator(0.f, 0.f, 8.f), false);
	SpawnBlock(MESH_CUBE, Center + FVector(7200.f, 500.f, 1200.f),
		FVector(2.f, 80.f, 24.f), StoneColor, FRotator(0.f, 0.f, -10.f), false);

	// ── Chaînes de montagnes sous-marines en fond (cônes chevauchants) ──
	SpawnRidge(Center + FVector(-9000.f, -9000.f, 0.f), Center + FVector(-9000.f, 9000.f, 0.f),
		3000.f, 1400.f, FarColor, 101);
	SpawnRidge(Center + FVector(9000.f, -9000.f, 0.f), Center + FVector(9000.f, 9000.f, 0.f),
		3200.f, 1500.f, FarColor, 202);
	SpawnRidge(Center + FVector(-9000.f, 9500.f, 0.f), Center + FVector(9000.f, 9500.f, 0.f),
		2800.f, 1300.f, FarColor, 303);

	// ── Rochers kitbashés répartis sur TOUTE la map ──
	const float RockPos[][3] = {
		{-2200.f, -1600.f, 380.f}, { 2200.f,  1600.f, 420.f},
		{-2400.f,  1400.f, 300.f}, { 2400.f, -1400.f, 320.f},
		{-1300.f, -2200.f, 280.f}, { 1300.f,  2200.f, 300.f},
		{-3000.f,     0.f, 460.f}, { 3000.f,   200.f, 440.f},
		{-5200.f,  3600.f, 520.f}, { 5200.f, -3600.f, 520.f},
		{-5600.f, -2800.f, 360.f}, { 5600.f,  2800.f, 360.f},
		{-3800.f,  5200.f, 420.f}, { 3800.f, -5200.f, 420.f},
		{ -800.f,  5600.f, 300.f}, {  800.f, -5600.f, 300.f},
		{-6800.f,   600.f, 560.f}, { 6800.f,  -600.f, 560.f},
		{ 1600.f,  4200.f, 340.f}, {-1600.f, -4200.f, 340.f},
	};
	int32 Seed = 1;
	for (const float* Rk : RockPos)
	{
		SpawnRock(Center + FVector(Rk[0], Rk[1], -40.f), Rk[2], RockColor, Seed++);
	}

	// ── Gravats (petits rochers) éparpillés pour habiller le sol ──
	FRandomStream Rub(777);
	for (int32 i = 0; i < 40; ++i)
	{
		const FVector Pos(Rub.FRandRange(-7000.f, 7000.f), Rub.FRandRange(-7000.f, 7000.f), -40.f);
		SpawnRock(Center + Pos, Rub.FRandRange(80.f, 180.f), RockColor, 1000 + i);
	}

	// ── CORAUX : touches de vie/couleur (orange, corail, teal) ───────────────
	// Buissons coralliens = amas de petits cônes/sphères de couleurs chaudes.
	const FLinearColor CoralWarm[4] = {
		FLinearColor(0.85f, 0.35f, 0.10f, 1.f), // orange
		FLinearColor(0.90f, 0.45f, 0.45f, 1.f), // corail rose
		FLinearColor(0.95f, 0.65f, 0.15f, 1.f), // ambre
		FLinearColor(0.10f, 0.55f, 0.50f, 1.f), // teal vif
	};
	FRandomStream Cor(909);
	for (int32 i = 0; i < 34; ++i)
	{
		const FVector Base = Center + FVector(
			Cor.FRandRange(-7000.f, 7000.f), Cor.FRandRange(-7000.f, 7000.f), -40.f);
		const FLinearColor Col = CoralWarm[Cor.RandRange(0, 3)];
		const int32 Branches = Cor.RandRange(3, 6);
		for (int32 b = 0; b < Branches; ++b)
		{
			const float Hgt = Cor.FRandRange(60.f, 160.f);
			const FVector Off(Cor.FRandRange(-40.f, 40.f), Cor.FRandRange(-40.f, 40.f), Hgt * 0.5f);
			const bool bRound = Cor.FRand() < 0.3f;
			SpawnBlock(bRound ? MESH_SPH : MESH_CONE, Base + Off,
				FVector(0.18f, 0.18f, Hgt / 100.f), Col,
				FRotator(Cor.FRandRange(-20.f, 20.f), Cor.FRandRange(0.f, 360.f), Cor.FRandRange(-20.f, 20.f)),
				false);
		}
	}
}
