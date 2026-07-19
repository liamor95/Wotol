#include "WOTOLGreyboxEnvironment.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/PointLight.h"
#include "Components/PointLightComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Components/LightComponent.h"
#include "WOTOLGlow.h"
#include "Math/RandomStream.h"
#include "WOTOLAmbientFish.h"
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

	// Décor MAT rugueux (roche/sable/montagnes) : plus de reflet plastique lisse -> la
	// lumière crée des ombres/reliefs = du contraste sur les parois.
	if (UMaterialInstanceDynamic* MID = WOTOLGlow::MakeMatte(this, Color))
	{
		Comp->SetMaterial(0, MID);
	}
	return SMA;
}

// Mesh ÉMISSIF (décor bioluminescent : coraux, algues) -> il brille de lui-même.
AStaticMeshActor* AWOTOLGreyboxEnvironment::SpawnGlowBlock(const TCHAR* MeshPath, const FVector& Loc,
	const FVector& Scale, const FLinearColor& EmissiveColor, const FRotator& Rot)
{
	UWorld* W = GetWorld(); if (!W) return nullptr;
	FActorSpawnParameters P; P.Owner = this;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* SMA = W->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Loc, Rot, P);
	if (!SMA) return nullptr;
	UStaticMeshComponent* Comp = SMA->GetStaticMeshComponent();
	if (!Comp) return SMA;
	Comp->SetMobility(EComponentMobility::Movable);
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->SetCanEverAffectNavigation(false);
	if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath)) Comp->SetStaticMesh(Mesh);
	SMA->SetActorScale3D(Scale);
	if (UMaterialInstanceDynamic* MID = WOTOLGlow::MakeGlow(this, EmissiveColor))
		Comp->SetMaterial(0, MID);
	return SMA;
}

// Lampe bioluminescente « naturelle » : halo doux qui éclaire le fond autour de l'organisme.
void AWOTOLGreyboxEnvironment::SpawnBioLight(const FVector& Loc, const FLinearColor& Color, float Intensity, float Radius)
{
	UWorld* W = GetWorld(); if (!W) return;
	FActorSpawnParameters P; P.Owner = this;
	AActor* A = W->SpawnActor<AActor>(AActor::StaticClass(), Loc, FRotator::ZeroRotator, P);
	if (!A) return;
	USceneComponent* Root = NewObject<USceneComponent>(A);
	Root->RegisterComponent(); A->SetRootComponent(Root);
	UPointLightComponent* PC = NewObject<UPointLightComponent>(A);
	PC->SetupAttachment(Root); PC->RegisterComponent();
	PC->SetLightColor(Color);
	PC->SetIntensity(Intensity);
	PC->SetAttenuationRadius(Radius);
	PC->SetCastShadows(false);
}

// ─── Kitbash : rocher = amas de cubes/sphères ───────────────────────────────
void AWOTOLGreyboxEnvironment::SpawnRock(const FVector& Center, float Size, const FLinearColor& Color, int32 Seed)
{
	FRandomStream R(Seed);

	// Bloc central : BLOQUANT (les unités/le Kraken contournent le rocher, n'y rentrent pas).
	const float CoreS = Size * R.FRandRange(0.7f, 0.9f);
	SpawnBlock(MESH_CUBE, Center + FVector(0, 0, CoreS * 0.4f),
		FVector(CoreS / 100.f), Vary(Color, R.FRandRange(-0.01f, 0.02f)),
		FRotator(R.FRandRange(0.f, 40.f), R.FRandRange(0.f, 360.f), R.FRandRange(0.f, 40.f)), /*bBlocking=*/true);

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

	// ── ASPÉRITÉS : pointes/arêtes rocheuses qui hérissent la surface (silhouette
	// accidentée, pas un galet lisse) -> la lumière rasante crée des ombres portées. ──
	const int32 Spikes = R.RandRange(6, 11);
	for (int32 i = 0; i < Spikes; ++i)
	{
		const float SpikeS = Size * R.FRandRange(0.18f, 0.45f);
		const FVector Off(
			R.FRandRange(-Size, Size) * 0.55f,
			R.FRandRange(-Size, Size) * 0.55f,
			R.FRandRange(Size * 0.1f, Size * 0.7f));
		SpawnBlock(MESH_CONE, Center + Off,
			FVector(SpikeS / 130.f, SpikeS / 130.f, SpikeS / 55.f),
			Vary(Color, R.FRandRange(-0.03f, 0.01f)),
			FRotator(R.FRandRange(-40.f, 40.f), R.FRandRange(0.f, 360.f), R.FRandRange(-40.f, 40.f)), false);
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
			FRotator(0.f, R.FRandRange(0.f, 360.f), 0.f), /*bBlocking=*/true); // montagnes = mur (unités/Kraken bloqués)
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

	// VARIANT DE TERRAIN : la phase 3 (Variant>=3) est une AUTRE zone, ABYSSALE-VOLCANIQUE
	// (basalte sombre, teal, montagnes bleu-nuit) -> palette + brume + disposition différentes.
	const bool bAbyss = (Variant >= 3);

	// ── Palette FOND MARIN : terreuse (phases 1/2) OU basalte abyssal sombre (phase 3). ──
	const FLinearColor FloorColor = bAbyss ? FLinearColor(0.26f, 0.30f, 0.28f, 1.f) : FLinearColor(0.46f, 0.42f, 0.32f, 1.f); // sable/basalte
	const FLinearColor SandBright = bAbyss ? FLinearColor(0.30f, 0.36f, 0.33f, 1.f) : FLinearColor(0.52f, 0.47f, 0.35f, 1.f); // couloir central
	const FLinearColor RockColor  = bAbyss ? FLinearColor(0.17f, 0.19f, 0.22f, 1.f) : FLinearColor(0.36f, 0.30f, 0.25f, 1.f); // récif / basalte
	const FLinearColor FarColor   = bAbyss ? FLinearColor(0.19f, 0.22f, 0.30f, 1.f) : FLinearColor(0.30f, 0.28f, 0.27f, 1.f); // montagnes
	const FLinearColor SurfColor  (0.28f, 0.58f, 0.72f, 1.f); // surface éclairée (rayons)
	const FLinearColor KelpColor  = bAbyss ? FLinearColor(0.14f, 0.52f, 0.55f, 1.f) : FLinearColor(0.42f, 0.55f, 0.16f, 1.f); // algues teal / jaune-vert
	// Décalage de seed propre au variant -> rochers/coraux/champignons disposés AUTREMENT.
	const int32 VSeed = Variant * 777;

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
				// Brume SOUS-MARINE bleu-vert PRÉSENTE : c'est ELLE qui donne le sentiment
				// d'être sous l'eau (l'eau qui diffuse), pendant que les objets proches gardent
				// LEUR couleur. Plus dense + démarre plus près -> profondeur bleutée, halos
				// bioluminescents qui « bavent » dans l'eau, silhouettes lointaines noyées.
				FC->SetFogDensity(bAbyss ? 0.040f : 0.032f);
				FC->SetFogHeightFalloff(0.05f);
				// Phase 3 : brume ABYSSALE plus verte/profonde (autre ambiance que la teal claire).
				FC->SetFogInscatteringColor(bAbyss ? FLinearColor(0.02f, 0.10f, 0.09f, 1.f) : FLinearColor(0.03f, 0.11f, 0.16f, 1.f));
				FC->SetStartDistance(bAbyss ? 350.f : 500.f);
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
			// JUSTE MILIEU : ni délavé-clair ni noir. Teinte à peine bleutée (l'ambiance
			// vient surtout du brouillard), couleurs saturées mais NEUTRES en gain pour que
			// chaque élément garde SA couleur (pas de dominante bleue qui uniformise tout).
			S.bOverride_ColorGain = true;        S.ColorGain = FVector4(0.86f, 0.97f, 1.12f, 1.f); // teinte EAU bleu-vert
			S.bOverride_ColorSaturation = true;  S.ColorSaturation = FVector4(1.48f, 1.45f, 1.45f, 1.f); // couleurs franches
			S.bOverride_ColorContrast   = true;  S.ColorContrast   = FVector4(1.22f, 1.21f, 1.20f, 1.f); // relief sans écraser
			// Ambiance MODÉRÉMENT SOMBRE (pas noir) : c'est le CONTRASTE avec la pénombre qui
			// fait ressortir les organismes bioluminescents ET les attaques lumineuses.
			S.bOverride_AutoExposureBias = true; S.AutoExposureBias = -0.60f;
			S.bOverride_AutoExposureMinBrightness = true; S.AutoExposureMinBrightness = 0.30f;
			S.bOverride_AutoExposureMaxBrightness = true; S.AutoExposureMaxBrightness = 0.85f;
			S.bOverride_VignetteIntensity  = true; S.VignetteIntensity = 0.36f;
			S.bOverride_SceneFringeIntensity = true; S.SceneFringeIntensity = 0.5f;
			// Bloom DISCRET : assez pour un léger halo bioluminescent, PAS assez pour cramer
			// un gros blob blanc au centre (les parties vives des unités ne bavent plus).
			S.bOverride_BloomIntensity = true; S.BloomIntensity = 0.50f;
		}

		// Nuages masqués + SOLEIL adouci (lumière directionnelle atténuée et bleutée
		// = fini l'impression de plein-air ensoleillé, on est sous l'eau).
		for (TActorIterator<AActor> It(W); It; ++It)
		{
			const FString Cls = It->GetClass()->GetName();
			if (Cls.Contains(TEXT("VolumetricCloud")))
			{
				It->SetActorHiddenInGame(true);
			}
			else if (Cls.Contains(TEXT("DirectionalLight")))
			{
				if (ULightComponent* LC = It->FindComponentByClass<ULightComponent>())
				{
					// Key light FROIDE (bleu-vert léger) : assez de teinte pour lire « sous
					// l'eau », mais PAS au point de repeindre tout en bleu -> chaque matériau
					// garde sa couleur (roche brune, sable tan, factions). Juste milieu.
					// Atténuation appliquée UNE SEULE fois (un rebuild ne doit pas re-multiplier).
					if (!bLightTuned) { LC->SetIntensity(LC->Intensity * 0.46f); bLightTuned = true; }
					// Phase 3 : teinte de key light plus froide/verte (autre ambiance).
					LC->SetLightColor(bAbyss ? FLinearColor(0.50f, 0.78f, 0.82f) : FLinearColor(0.58f, 0.74f, 0.96f));
				}
			}
		}
	}

	// ── Sol sablonneux clair (seul élément BLOQUANT) ──
	SpawnBlock(MESH_CUBE, Center + FVector(0, 0, -50.f),
		FVector(300.f, 300.f, 1.f), FloorColor, NoRot, true);
	// (Plus de "plan de surface" lumineux : il donnait un plafond artificiel.
	//  La lumière tamisée + la brume suffisent à l'ambiance sous-marine.)
	(void)SurfColor;

	// ── CANYON DE SABLE CENTRAL (couloir de combat dégagé le long de X) ──
	SpawnPlaza(Center, bAbyss ? 12500.f : 7500.f, bAbyss ? 2600.f : 1350.f, SandBright);
	SpawnPlaza(Center + FVector(0.f, 0.f, 0.f), bAbyss ? 6200.f : 3200.f,
		bAbyss ? 1800.f : 900.f, FLinearColor(0.44f, 0.44f, 0.38f, 1.f));

	// ── Palette de coraux VIFS ──
	const FLinearColor Coral[6] = {
		FLinearColor(0.95f, 0.45f, 0.12f, 1.f), // orange
		FLinearColor(0.62f, 0.35f, 0.80f, 1.f), // violet
		FLinearColor(0.92f, 0.55f, 0.68f, 1.f), // rose
		FLinearColor(0.90f, 0.80f, 0.28f, 1.f), // jaune
		FLinearColor(0.15f, 0.62f, 0.58f, 1.f), // teal
		FLinearColor(0.85f, 0.28f, 0.28f, 1.f), // rouge
	};

	// Buisson de corail BIOLUMINESCENT = coraux-tubes/branches/cerveau qui RAYONNENT
	// (émissif) + une lampe douce -> ils habillent le fond ET l'éclairent naturellement.
	auto SpawnCoral = [&](const FVector& Pos, int32 InSeed)
	{
		FRandomStream R(InSeed);
		const FLinearColor Col = Coral[R.RandRange(0, 5)];
		const int32 N = R.RandRange(4, 8);
		for (int32 k = 0; k < N; ++k)
		{
			const float Hgt = R.FRandRange(70.f, 220.f);
			const FVector Off(R.FRandRange(-70.f, 70.f), R.FRandRange(-70.f, 70.f), Hgt * 0.5f);
			const int32 Kind = R.RandRange(0, 2);
			const TCHAR* M = (Kind == 0) ? MESH_CYL : (Kind == 1) ? MESH_CONE : MESH_SPH;
			const FLinearColor C = (R.FRand() < 0.4f) ? Coral[R.RandRange(0, 5)] : Col; // variété
			// Émissif HDR (×3, comme le cristal du Cristalliseur qui brille bien).
			SpawnGlowBlock(M, Pos + Off, FVector(0.16f, 0.16f, Hgt / 100.f), C * 3.0f,
				FRotator(R.FRandRange(-14.f, 14.f), R.FRandRange(0.f, 360.f), R.FRandRange(-14.f, 14.f)));
		}
		// Lumière bioluminescente NATURELLE du buisson (éclaire le fond autour, halo doux).
		SpawnBioLight(Pos + FVector(0, 0, 90.f), Col, 2600.f, 700.f);
	};

	// Touffe d'algues = grands brins fins jaune-vert qui montent
	auto SpawnKelp = [&](const FVector& Pos, int32 InSeed)
	{
		FRandomStream R(InSeed);
		const int32 N = R.RandRange(5, 9);
		for (int32 k = 0; k < N; ++k)
		{
			const float Hgt = R.FRandRange(300.f, 700.f);
			const FVector Off(R.FRandRange(-90.f, 90.f), R.FRandRange(-90.f, 90.f), Hgt * 0.5f);
			// Algues BIOLUMINESCENTES (émissif) : brins jaune-vert qui rayonnent.
			SpawnGlowBlock(MESH_CYL, Pos + Off, FVector(0.07f, 0.07f, Hgt / 100.f),
				Vary(KelpColor, R.FRandRange(-0.05f, 0.05f)) * 2.6f,
				FRotator(R.FRandRange(-10.f, 10.f), R.FRandRange(0.f, 360.f), R.FRandRange(-10.f, 10.f)));
		}
		// Halo bioluminescent de la touffe d'algues.
		SpawnBioLight(Pos + FVector(0, 0, 260.f), KelpColor, 2200.f, 650.f);
	};

	// UN champignon/organisme bioluminescent (tige + chapeau, OU anémone à doigts). Émissif.
	const FLinearColor MushCols[5] = {
		FLinearColor(0.20f, 0.95f, 1.00f, 1.f), // cyan
		FLinearColor(0.30f, 1.00f, 0.45f, 1.f), // vert
		FLinearColor(0.75f, 0.35f, 1.00f, 1.f), // violet
		FLinearColor(0.25f, 0.60f, 1.00f, 1.f), // bleu
		FLinearColor(1.00f, 0.55f, 0.20f, 1.f), // ambre
	};
	auto SpawnMushroom = [&](const FVector& Pos, int32 InSeed)
	{
		FRandomStream R(InSeed);
		const FLinearColor Col = MushCols[R.RandRange(0, 4)];
		const float Sc = R.FRandRange(0.8f, 1.6f);
		if (R.FRand() < 0.5f)
		{
			// Champignon / méduse : tige fine + chapeau en dôme lumineux.
			const float StalkH = R.FRandRange(160.f, 320.f) * Sc;
			SpawnGlowBlock(MESH_CONE, Pos + FVector(0, 0, StalkH * 0.5f), FVector(0.09f * Sc, 0.09f * Sc, StalkH / 100.f), Col * 1.8f);
			SpawnGlowBlock(MESH_SPH, Pos + FVector(0, 0, StalkH), FVector(0.55f * Sc, 0.55f * Sc, 0.24f * Sc), Col * 3.0f);
		}
		else
		{
			// Anémone à doigts : quelques tubes charnus à bout lumineux.
			const int32 F = R.RandRange(5, 8);
			for (int32 f = 0; f < F; ++f)
			{
				const float A = R.FRandRange(0.f, 2.f * PI);
				const float Rad = R.FRandRange(10.f, 60.f) * Sc;
				const FVector Base = Pos + FVector(FMath::Cos(A) * Rad, FMath::Sin(A) * Rad, 0.f);
				const float Hp = R.FRandRange(100.f, 200.f) * Sc;
				const float Wp = R.FRandRange(0.30f, 0.5f) * Sc;
				SpawnGlowBlock(MESH_CONE, Base + FVector(0, 0, Hp * 0.5f), FVector(Wp, Wp, Hp / 100.f), Col * 1.8f,
					FRotator(FMath::Cos(A) * 16.f, 0.f, FMath::Sin(A) * -16.f));
				SpawnGlowBlock(MESH_SPH, Base + FVector(0, 0, Hp), FVector(Wp * 1.1f, Wp * 1.1f, Wp * 1.3f), Col * 3.0f);
			}
		}
		SpawnBioLight(Pos + FVector(0, 0, 130.f * Sc), Col, 1800.f, 560.f);
	};

	// ── DEUX RÉCIFS ROCHEUX bordant le canyon (côtés +Y et -Y), couverts de coraux ──
	FRandomStream Reef(4242 + VSeed);
	const float ReefExtent = bAbyss ? 10500.f : 6500.f;
	const float ReefMinY = bAbyss ? 5200.f : 2600.f;
	const float ReefMaxY = bAbyss ? 6800.f : 3400.f;
	for (float X = -ReefExtent; X <= ReefExtent; X += 1300.f)
	{
		for (int32 side = 0; side < 2; ++side)
		{
			const float Y = (side == 0 ? 1.f : -1.f) * Reef.FRandRange(ReefMinY, ReefMaxY);
			// masse rocheuse récifale (kitbash)
			SpawnRock(Center + FVector(X + Reef.FRandRange(-200.f, 200.f), Y, -40.f),
				Reef.FRandRange(360.f, 620.f), RockColor, Reef.RandRange(1, 9999));
			// coraux accrochés sur le récif
			SpawnCoral(Center + FVector(X + Reef.FRandRange(-300.f, 300.f), Y + Reef.FRandRange(-250.f, 250.f), -20.f),
				Reef.RandRange(1, 9999));
		}
	}

	// ── Coraux + rochers plus loin (remplissent les flancs, hors du couloir) ──
	FRandomStream Side(707 + VSeed);
	for (int32 i = 0; i < 30; ++i)
	{
		const float YMin = bAbyss ? 7000.f : 3400.f;
		const float YMax = bAbyss ? 11500.f : 6500.f;
		const float XMax = bAbyss ? 11000.f : 6500.f;
		const float Y = (Side.FRand() < 0.5f ? 1.f : -1.f) * Side.FRandRange(YMin, YMax);
		const float X = Side.FRandRange(-XMax, XMax);
		if (Side.FRand() < 0.6f)
			SpawnCoral(Center + FVector(X, Y, -20.f), Side.RandRange(1, 9999));
		else
			SpawnRock(Center + FVector(X, Y, -40.f), Side.FRandRange(160.f, 420.f), RockColor, Side.RandRange(1, 9999));
	}

	// ── CHAMPIGNONS / ORGANISMES BIOLUMINESCENTS dispersés ALÉATOIREMENT sur toute la
	// carte (même principe que les rochers), un par un. Le CENTRE (emplacement du bâtiment,
	// rayon 1600) est STRICTEMENT exclu -> jamais rien au milieu. ──
	FRandomStream Shr(555 + VSeed);
	for (int32 i = 0; i < 40; ++i)
	{
		const float x = Shr.FRandRange(-6200.f, 6200.f);
		const float y = Shr.FRandRange(-6200.f, 6200.f);
		if (FMath::Sqrt(x * x + y * y) < 1600.f) continue; // JAMAIS au centre / sur le bâtiment
		SpawnMushroom(Center + FVector(x, y, -20.f), Shr.RandRange(1, 9999));
	}

	// ── ALGUES (côté droit surtout, comme la réf) ──
	FRandomStream Kel(313 + VSeed);
	for (int32 i = 0; i < 10; ++i)
	{
		const float X = Kel.FRandRange(1500.f, 6500.f);
		const float Y = Kel.FRandRange(2200.f, 5200.f) * (Kel.FRand() < 0.7f ? 1.f : -1.f);
		SpawnKelp(Center + FVector(X, Y, -30.f), Kel.RandRange(1, 9999));
	}

	// ── Petits rochers BAS dans le couloir (galets). PAS de corail au centre : la zone
	// de combat centrale (|X|<2500) reste dégagée -> plus de pâté bioluminescent au milieu.
	FRandomStream Mid(151 + VSeed);
	for (int32 i = 0; i < 16; ++i)
	{
		const float MX = Mid.FRandRange(-6500.f, 6500.f);
		const FVector P = Center + FVector(MX, Mid.FRandRange(-1100.f, 1100.f), -30.f);
		// Couloir de combat : UNIQUEMENT des galets (rochers), AUCUN corail bioluminescent.
		SpawnRock(P, Mid.FRandRange(90.f, 200.f), RockColor, Mid.RandRange(1, 9999));
	}

	// ── HORIZON : chaînes de reliefs de TAILLES VARIÉES tout autour (pas un mur droit) ──
	FRandomStream Hor(2024 + VSeed);
	const int32 Rings = 14;
	for (int32 i = 0; i < Rings; ++i)
	{
		const float Ang = 2.f * PI * i / Rings + Hor.FRandRange(-0.15f, 0.15f);
		const float Dist = bAbyss ? Hor.FRandRange(13500.f, 17500.f)
			: Hor.FRandRange(8500.f, 11500.f);
		const FVector A = Center + FVector(FMath::Cos(Ang) * Dist, FMath::Sin(Ang) * Dist, 0.f);
		const float Ang2 = Ang + (2.f * PI / Rings) * 0.7f;
		const FVector B = Center + FVector(FMath::Cos(Ang2) * Dist, FMath::Sin(Ang2) * Dist, 0.f);
		const float Height = Hor.FRandRange(1200.f, 3400.f);   // hauteurs variées
		const float Width  = Hor.FRandRange(700.f, 1500.f);
		SpawnRidge(A, B, Height, Width, FarColor, 400 + i);
	}
	// Quelques gros massifs isolés à mi-distance (brise la régularité)
	SpawnRock(Center + FVector(-7000.f, -5500.f, -40.f), 900.f, RockColor, 811);
	SpawnRock(Center + FVector(6800.f, 5200.f, -40.f), 1100.f, RockColor, 812);
	SpawnRock(Center + FVector(-6000.f, 6500.f, -40.f), 700.f, RockColor, 813);
	SpawnRock(Center + FVector(7200.f, -5800.f, -40.f), 800.f, RockColor, 814);
		for (int32 i = 0; i < 16; ++i)
		{
			const float Ang = 2.f * PI * i / 16 + (PI / 16.f) + Hor.FRandRange(-0.12f, 0.12f);
			const float Dist = bAbyss ? Hor.FRandRange(10500.f, 12800.f)
				: Hor.FRandRange(5200.f, 6800.f);
			const FVector A = Center + FVector(FMath::Cos(Ang) * Dist, FMath::Sin(Ang) * Dist, 0.f);
			const float Ang2 = Ang + (2.f * PI / 16.f) * 0.6f;
			const FVector B = Center + FVector(FMath::Cos(Ang2) * Dist, FMath::Sin(Ang2) * Dist, 0.f);
			SpawnRidge(A, B, Hor.FRandRange(700.f, 1900.f), Hor.FRandRange(500.f, 1100.f), FarColor, 460 + i);
		}
		for (int32 i = 0; i < 10; ++i)
		{
			const float Ang = Hor.FRandRange(0.f, 2.f * PI);
			const float Dist = bAbyss ? Hor.FRandRange(9000.f, 11200.f)
				: Hor.FRandRange(3800.f, 5600.f);
			SpawnRock(Center + FVector(FMath::Cos(Ang) * Dist, FMath::Sin(Ang) * Dist, -40.f), Hor.FRandRange(500.f, 1200.f), RockColor, 820 + i);
		}
		// ── CORAUX/ORGANISMES BIOLUMINESCENTS : chaque source de lumière est JUSTIFIÉE par
		// un élément de décor bio-organique (amas de corail/champignon marin luminescent).
		// Le corail est une grappe de pousses colorées + une lampe accrochée à sa base. ──
		if (UWorld* Wl = GetWorld())
		{
			const FLinearColor BioCols[5] = {
				FLinearColor(0.20f, 0.95f, 1.00f), // cyan
				FLinearColor(0.30f, 1.00f, 0.45f), // vert
				FLinearColor(0.75f, 0.35f, 1.00f), // violet
				FLinearColor(0.25f, 0.60f, 1.00f), // bleu
				FLinearColor(1.00f, 0.55f, 0.20f), // ambre (rare)
			};
			UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
			UStaticMesh* Sph  = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
			UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
				nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
			(void)BaseMat;
			FRandomStream Bs(77 + VSeed);

			// ── ÉPARPILLÉ = de NOMBREUSES TOUFFES réparties sur TOUTE l'arène (réf.).
			// Chaque touffe = un actor posé au sol, contenant PLUSIEURS organismes proches
			// (comme les massifs de corail qui poussent en bouquet sur les roches). ──
			// ── RÉPARTITION GARANTIE : une GRILLE jitterée qui couvre TOUTE l'arène.
			// Chaque cellule pose une touffe (avec un peu d'aléa) -> IMPOSSIBLE de
			// s'agglutiner au centre. On saute juste la zone de déploiement centrale. ──
			const int32 GridN = 15;                 // 15x15 = 225 cellules (grille fine)
			const float HalfSpan = 4400.f;          // demi-étendue couverte
			const float CellSz = (2.f * HalfSpan) / GridN;
			// ⛔ GRILLE BIOLUMINESCENTE DÉFINITIVEMENT DÉSACTIVÉE (demande du joueur).
			// Elle reposait un organisme à l'emplacement du bâtiment central -> plus JAMAIS
			// aucun organisme de grille, nulle part.
			const bool bGridBioEnabled = false;
			for (int32 c = 0; bGridBioEnabled && c < GridN * GridN; ++c)
			{
				const int32 gx = c % GridN;
				const int32 gy = c / GridN;
				// Centre de la cellule + jitter (~40% de la cellule) = réparti mais naturel.
				const float cx = -HalfSpan + (gx + 0.5f) * CellSz + Bs.FRandRange(-CellSz * 0.4f, CellSz * 0.4f);
				const float cy = -HalfSpan + (gy + 0.5f) * CellSz + Bs.FRandRange(-CellSz * 0.4f, CellSz * 0.4f);
				// ~30% de cellules sautées (aspect naturel).
				if (Bs.FRand() < 0.30f) continue;
				// On garde UNIQUEMENT l'emplacement du bâtiment de phase 2 dégagé (rayon 700
				// autour du centre), là où se pose le Cristalliseur/Abyssalyseur.
				if (FMath::Sqrt(cx * cx + cy * cy) < 700.f) continue;
				const FVector PatchP = Center + FVector(cx, cy, -20.f);

				FActorSpawnParameters LP; LP.Owner = this;
				LP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AActor* CoralAct = Wl->SpawnActor<AActor>(AActor::StaticClass(), PatchP, FRotator(0, Bs.FRandRange(0.f, 360.f), 0.f), LP);
				if (!CoralAct) continue;
				USceneComponent* Root = NewObject<USceneComponent>(CoralAct);
				Root->RegisterComponent(); CoralAct->SetRootComponent(Root);

				// Pièce émissive (l'organisme rayonne LUI-MÊME, comme le cristal du Cristalliseur).
				// Émissif ATTÉNUÉ -> couleur FRANCHE (fini le blanc cramé).
				auto MakePiece = [&](UStaticMesh* Mesh, const FVector& Loc, const FVector& Scale,
					const FRotator& Rot, const FLinearColor& Emissive) -> void
				{
					if (!Mesh) return;
					UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(CoralAct);
					if (!M) return;
					M->SetupAttachment(Root); M->RegisterComponent();
					M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					M->SetCanEverAffectNavigation(false);
					M->SetStaticMesh(Mesh);
					M->SetRelativeScale3D(Scale);
					M->SetRelativeLocation(Loc);
					M->SetRelativeRotation(Rot);
					if (UMaterialInstanceDynamic* MID = WOTOLGlow::MakeGlow(CoralAct, Emissive))
						M->SetMaterial(0, MID);
				};

				// UN SEUL organisme par cellule -> un POINT individuel espacé (jamais un paquet).
				const int32 Organisms = 1;
				FLinearColor PatchCol = BioCols[Bs.RandRange(0, 4)];
				for (int32 o = 0; o < Organisms; ++o)
				{
					const FLinearColor Col = PatchCol;
					const float Sc = Bs.FRandRange(0.7f, 1.4f);
					const FVector O(0.f, 0.f, 0.f);

					// 5 TYPES d'organismes bioluminescents tirés au hasard (variété façon réf) :
					// 0 méduse/champignon, 1 corail à polypes, 2 anémone à doigts, 3 herbe en
					// éventail, 4 grappe de bulbes. Répartis par la grille -> jamais groupés.
					const int32 Type = Bs.RandRange(0, 4);
					if (Type == 0)
					{
						// CHAMPIGNON / MÉDUSE : tige fine + chapeau en dôme lumineux.
						const float StalkH = Bs.FRandRange(150.f, 300.f) * Sc;
						MakePiece(Cone, O + FVector(0, 0, StalkH * 0.5f), FVector(0.09f * Sc, 0.09f * Sc, StalkH / 100.f),
							FRotator::ZeroRotator, Col * 1.8f);
						MakePiece(Sph, O + FVector(0, 0, StalkH), FVector(0.55f * Sc, 0.55f * Sc, 0.24f * Sc),
							FRotator::ZeroRotator, Col * 3.2f);
					}
					else if (Type == 1)
					{
						// TOUFFE DE CORAIL : petit socle + quelques polypes à bulbe lumineux.
						MakePiece(Sph, O + FVector(0, 0, 10.f * Sc), FVector(0.7f * Sc, 0.7f * Sc, 0.4f * Sc),
							FRotator::ZeroRotator, Col * 1.2f);
						const int32 Polyps = Bs.RandRange(3, 5);
						for (int32 sIdx = 0; sIdx < Polyps; ++sIdx)
						{
							const float A   = Bs.FRandRange(0.f, 2.f * PI);
							const float Rad = Bs.FRandRange(0.f, 45.f) * Sc;
							const FVector Base = O + FVector(FMath::Cos(A) * Rad, FMath::Sin(A) * Rad, 0.f);
							const float Hp  = Bs.FRandRange(70.f, 150.f) * Sc;
							const float Wp  = Bs.FRandRange(0.40f, 0.65f) * Sc;
							const FRotator Tilt(Bs.FRandRange(-16.f, 16.f), 0.f, Bs.FRandRange(-16.f, 16.f));
							MakePiece(Cone, Base + FVector(0, 0, Hp * 0.5f), FVector(Wp, Wp, Hp / 100.f), Tilt, Col * 1.8f);
							MakePiece(Sph, Base + FVector(0, 0, Hp), FVector(Wp * 0.9f, Wp * 0.9f, Wp * 0.9f),
								FRotator::ZeroRotator, Col * 3.2f);
						}
					}
					else if (Type == 2)
					{
						// ANÉMONE À DOIGTS : bouquet de tubes charnus dressés, à bout lumineux
						// (les gros massifs cyan de la réf). Les doigts s'évasent vers le haut.
						const int32 Fingers = Bs.RandRange(6, 10);
						for (int32 f = 0; f < Fingers; ++f)
						{
							const float A   = Bs.FRandRange(0.f, 2.f * PI);
							const float Rad = Bs.FRandRange(10.f, 70.f) * Sc;
							const FVector Base = O + FVector(FMath::Cos(A) * Rad, FMath::Sin(A) * Rad, 0.f);
							const float Hp  = Bs.FRandRange(110.f, 220.f) * Sc;
							const float Wp  = Bs.FRandRange(0.30f, 0.5f) * Sc;
							// évasement : le doigt penche vers l'extérieur
							const FRotator Tilt(FMath::Cos(A) * 18.f, 0.f, FMath::Sin(A) * -18.f);
							MakePiece(Cone, Base + FVector(0, 0, Hp * 0.5f), FVector(Wp, Wp, Hp / 100.f), Tilt, Col * 1.5f);
							MakePiece(Sph, Base + FVector(FMath::Cos(A) * Hp * 0.14f, FMath::Sin(A) * Hp * 0.14f, Hp),
								FVector(Wp * 1.1f, Wp * 1.1f, Wp * 1.3f), FRotator::ZeroRotator, Col * 3.2f); // bout lumineux
						}
					}
					else if (Type == 3)
					{
						// HERBE BIOLUMINESCENTE EN ÉVENTAIL : brins fins qui rayonnent d'un point
						// (les plantes-étoiles vert-cyan de la réf).
						const int32 Blades = Bs.RandRange(9, 16);
						for (int32 b = 0; b < Blades; ++b)
						{
							const float A   = Bs.FRandRange(0.f, 2.f * PI);
							const float Hb  = Bs.FRandRange(120.f, 260.f) * Sc;
							const float Lean = Bs.FRandRange(20.f, 55.f); // très ouvert = éventail
							const FRotator Tilt(FMath::Cos(A) * Lean, 0.f, FMath::Sin(A) * -Lean);
							MakePiece(Cone, O + FVector(0, 0, Hb * 0.42f), FVector(0.05f * Sc, 0.05f * Sc, Hb / 100.f),
								Tilt, Col * 2.2f);
						}
					}
					else
					{
						// GRAPPE DE BULBES / ŒUFS lumineux posés bas au sol.
						const int32 Eggs = Bs.RandRange(3, 6);
						for (int32 e = 0; e < Eggs; ++e)
						{
							const float A   = Bs.FRandRange(0.f, 2.f * PI);
							const float Rad = Bs.FRandRange(0.f, 55.f) * Sc;
							const float R2  = Bs.FRandRange(0.25f, 0.5f) * Sc;
							const FVector Base = O + FVector(FMath::Cos(A) * Rad, FMath::Sin(A) * Rad, R2 * 55.f);
							MakePiece(Sph, Base, FVector(R2, R2, R2), FRotator::ZeroRotator, Col * 2.6f);
						}
					}
				}

				// Lumière bioluminescente NATURELLE (halo doux) : ~1 organisme sur 2 en porte
				// (ils sont nombreux et dispersés -> inutile d'en mettre partout, ça sur-éclairerait).
				if (Bs.FRand() < 0.55f)
				{
					UPointLightComponent* PC = NewObject<UPointLightComponent>(CoralAct);
					PC->SetupAttachment(Root); PC->RegisterComponent();
					PC->SetRelativeLocation(FVector(0, 0, 120.f));
					PC->SetLightColor(PatchCol);
					PC->SetIntensity(1200.f);
					PC->SetAttenuationRadius(450.f);
					PC->SetCastShadows(false);
				}
			}
		}

	// ── ARÈNE FERMÉE : MUR DE COLLISION INVISIBLE (anneau) au pied des montagnes ──
	// Les chaînes de montagnes forment l'arène ; ce mur les rend TANGIBLES : aucune unité
	// ni le Kraken ne peut sortir de l'enceinte -> l'action reste concentrée à l'intérieur.
	{
		const float WallR = bAbyss ? 9000.f : 4700.f;
		const int32 Seg   = 32;       // segments qui se chevauchent -> paroi continue
		for (int32 i = 0; i < Seg; ++i)
		{
			const float Ang = 2.f * PI * i / Seg;
			const FVector Pos = Center + FVector(FMath::Cos(Ang) * WallR, FMath::Sin(Ang) * WallR, 1400.f);
			const FRotator Rot(0.f, FMath::RadiansToDegrees(Ang), 0.f); // face tournée vers le centre
			// Dalle HAUTE (couvre toutes les couches de verticalité) et LARGE (chevauche la voisine).
			if (AStaticMeshActor* Wseg = SpawnBlock(MESH_CUBE, Pos, FVector(1.2f, 11.f, 34.f), FarColor, Rot, /*bBlocking=*/true))
				if (UStaticMeshComponent* Wc = Wseg->GetStaticMeshComponent())
					Wc->SetVisibility(false); // invisible : seule la collision compte (les montagnes font le visuel)
		}
	}

	// ── FAUNE AMBIANTE : bancs de poissons qui nagent en boucle (décoratif) ──
	if (UWorld* W = GetWorld())
	{
		FRandomStream Fs(1234);
		const FLinearColor FishColors[5] = {
			FLinearColor(0.95f, 0.65f, 0.20f, 1.f), // jaune-orange (récif)
			FLinearColor(0.30f, 0.60f, 0.90f, 1.f), // bleu vif
			FLinearColor(0.85f, 0.85f, 0.90f, 1.f), // argenté
			FLinearColor(0.20f, 0.75f, 0.60f, 1.f), // vert d'eau
			FLinearColor(0.90f, 0.40f, 0.45f, 1.f), // corail
		};
		// 5 bancs de PETITS POISSONS proches (même cercle, phases décalées) qui ondulent.
		for (int32 s = 0; s < 5; ++s)
		{
			const FVector SchoolCenter = Center + FVector(
				Fs.FRandRange(-6000.f, 6000.f), Fs.FRandRange(-6000.f, 6000.f), 0.f);
			const float Radius = Fs.FRandRange(900.f, 2200.f);
			const float Speed  = Fs.FRandRange(0.25f, 0.6f) * (Fs.FRand() < 0.5f ? 1.f : -1.f);
			const float BaseZ  = Fs.FRandRange(400.f, 1600.f);
			const FLinearColor Col = FishColors[Fs.RandRange(0, 4)];
			const int32 Count = Fs.RandRange(6, 10);
			for (int32 f = 0; f < Count; ++f)
			{
				FActorSpawnParameters P; P.Owner = this;
				if (AWOTOLAmbientFish* Fish = W->SpawnActor<AWOTOLAmbientFish>(
						AWOTOLAmbientFish::StaticClass(), SchoolCenter, FRotator::ZeroRotator, P))
				{
					Fish->Configure(SchoolCenter, Radius + Fs.FRandRange(-120.f, 120.f), Speed,
						(2.f * PI * f / Count) + Fs.FRandRange(-0.2f, 0.2f),
						Fs.FRandRange(80.f, 220.f), BaseZ, Col, Fs.FRandRange(0.7f, 1.3f),
						EFishSpecies::SmallFish);
				}
			}
		}

		// ── GRANDES CRÉATURES avec une VRAIE identité (requin, dauphin, orque, baleine, raie) :
		// silhouettes reconnaissables, lentes, réparties en hauteur (fond de scène). ──
		struct FBig { EFishSpecies Sp; FLinearColor Col; float SizeMin, SizeMax, SpeedMin, SpeedMax; };
		// (Raie manta RETIREE : mal generee/trop grande en greybox.)
		const FBig Kinds[] = {
			{ EFishSpecies::Shark,   FLinearColor(0.16f, 0.19f, 0.22f, 1.f), 4.5f, 6.5f, 0.10f, 0.18f },
			{ EFishSpecies::Dolphin, FLinearColor(0.35f, 0.45f, 0.55f, 1.f), 3.5f, 4.8f, 0.16f, 0.26f },
			{ EFishSpecies::Orca,    FLinearColor(0.06f, 0.07f, 0.09f, 1.f), 5.0f, 7.0f, 0.12f, 0.20f },
			{ EFishSpecies::Whale,   FLinearColor(0.22f, 0.32f, 0.42f, 1.f), 8.0f, 11.0f, 0.05f, 0.10f },
		};
		for (int32 k = 0; k < UE_ARRAY_COUNT(Kinds); ++k)
		{
			const FBig& B = Kinds[k];
			// Dauphins en petit groupe (2-3), les autres en solitaire/paire.
			const int32 Cnt = (B.Sp == EFishSpecies::Dolphin) ? Fs.RandRange(2, 3) : Fs.RandRange(1, 2);
			const FVector GroupCenter = Center + FVector(Fs.FRandRange(-3000.f, 3000.f), Fs.FRandRange(-3000.f, 3000.f), 0.f);
			const float GRadius = Fs.FRandRange(4500.f, 7500.f);
			const float GSpeed  = Fs.FRandRange(B.SpeedMin, B.SpeedMax) * (k % 2 ? 1.f : -1.f);
			const float GZ = (B.Sp == EFishSpecies::Whale) ? Fs.FRandRange(2800.f, 3600.f)
				: (B.Sp == EFishSpecies::Ray) ? Fs.FRandRange(300.f, 900.f)   // les raies rasent le fond
				: Fs.FRandRange(1800.f, 3200.f);
			for (int32 i = 0; i < Cnt; ++i)
			{
				FActorSpawnParameters P; P.Owner = this;
				if (AWOTOLAmbientFish* Big = W->SpawnActor<AWOTOLAmbientFish>(
						AWOTOLAmbientFish::StaticClass(), GroupCenter, FRotator::ZeroRotator, P))
				{
					Big->Configure(GroupCenter, GRadius + Fs.FRandRange(-200.f, 200.f), GSpeed,
						(2.f * PI * i / FMath::Max(1, Cnt)) + Fs.FRandRange(-0.15f, 0.15f),
						Fs.FRandRange(80.f, 240.f), GZ, B.Col, Fs.FRandRange(B.SizeMin, B.SizeMax), B.Sp);
				}
			}
		}
	}

	// ── REPÈRES D'EXPLORATION (phase 1 uniquement) ───────────────────────────────
	// Le trajet héros -> Kraken (BeginOpeningExploration, offsets ~(-2600,0) et
	// ~(2200,0) autour du centre) était une quasi ligne droite sur X : rien n'invitait
	// à explorer, juste à avancer. On ajoute des repères NON BLOQUANTS (traversables,
	// silhouette uniquement) décalés en Y de part et d'autre du couloir, pour donner
	// une vraie sensation de zone à parcourir avant de tomber sur la créature — sans
	// gêner la bataille qui suit dans cette même arène (aucune collision ajoutée).
	if (!bAbyss)
	{
		// Arche de pierre à mi-chemin, légèrement excentrée : le joueur doit dévier de
		// la ligne droite pour la traverser à la nage (repère d'échelle ~5 m de diamètre,
		// cf. Docs/DOCUMENT_MAITRE_WOTOL.md §11).
		SpawnArch(Center + FVector(-300.f, 750.f, -30.f), 260.f, 90.f, FarColor);

		// Ruine à gradins excentrée de l'autre côté : point d'intérêt qui attire l'œil
		// hors de l'axe direct, sans jamais bloquer le passage (tout est traversable).
		SpawnZiggurat(Center + FVector(950.f, -1550.f, -40.f), 420.f, 3, 130.f, 200.f, RockColor);
		SpawnColonnade(Center + FVector(280.f, -1250.f, -30.f), FVector(260.f, -80.f, 0.f),
			4, 360.f, FarColor, 901);
	}
}

// ─── Nettoyage / reconstruction du décor (transitions de phase) ──────────────
// Détruit tout ce que cet environnement a généré : tous les acteurs dont l'Owner est cet
// environnement (blocs, rochers, coraux, lampes, brouillard, post-process, poissons...).
void AWOTOLGreyboxEnvironment::ClearArena()
{
	UWorld* W = GetWorld(); if (!W) return;
	TArray<AActor*> ToKill;
	for (TActorIterator<AActor> It(W); It; ++It)
	{
		if (*It == this) continue;
		if (It->GetOwner() == this) ToKill.Add(*It);
	}
	for (AActor* A : ToKill) { if (IsValid(A)) A->Destroy(); }
}

// Reconstruit l'arène pour une PHASE donnée : nettoie l'ancien décor puis régénère avec le
// variant correspondant (phase 3 -> zone abyssale : palette/brume/disposition différentes).
void AWOTOLGreyboxEnvironment::RebuildForPhase(int32 Phase)
{
	ClearArena();
	Variant = Phase;
	BuildArena();
}
