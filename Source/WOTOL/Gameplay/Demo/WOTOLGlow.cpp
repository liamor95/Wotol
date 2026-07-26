#include "WOTOLGlow.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_EDITOR
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionMultiply.h"
#endif

namespace
{
	// Matériaux parents mis en cache (référence FORTE -> pas collectés par le GC,
	// évite de recompiler à chaque fois).
	TStrongObjectPtr<UMaterialInterface> GGlowParent;
	TStrongObjectPtr<UMaterialInterface> GMatteParent;
	TStrongObjectPtr<UMaterialInterface> GHaloParent;
	TStrongObjectPtr<UMaterialInterface> GSpriteParent;
}

namespace WOTOLGlow { bool bLowGpuVFX = false; }

UMaterialInterface* WOTOLGlow::GetGlowParent()
{
	if (GGlowParent.IsValid())
	{
		return GGlowParent.Get();
	}

	UMaterialInterface* Result = nullptr;

#if WITH_EDITOR
	// Construit un matériau UNLIT dont l'Emissive = paramètre vecteur « Color ».
	// (La démo tourne dans l'éditeur : la compilation runtime du matériau est OK.)
	if (UMaterial* M = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient))
	{
		M->SetShadingModel(MSM_Unlit);

		UMaterialExpressionVectorParameter* P = NewObject<UMaterialExpressionVectorParameter>(M);
		P->ParameterName = TEXT("Color");
		P->DefaultValue = FLinearColor::White;
		M->GetExpressionCollection().AddExpression(P);
		M->GetEditorOnlyData()->EmissiveColor.Expression = P;

		M->PostEditChange();
		Result = M;
	}
#endif

	// Repli (build packagé, ou échec de construction) : couleur de base classique.
	if (!Result)
	{
		Result = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}

	GGlowParent.Reset(Result);
	return Result;
}

UMaterialInstanceDynamic* WOTOLGlow::MakeGlow(UObject* Outer, const FLinearColor& EmissiveHDR)
{
	UMaterialInterface* Parent = GetGlowParent();
	if (!Parent)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Parent, Outer);
	if (MID)
	{
		MID->SetVectorParameterValue(TEXT("Color"), EmissiveHDR);
	}
	return MID;
}

UMaterialInterface* WOTOLGlow::GetMatteParent()
{
	if (GMatteParent.IsValid())
	{
		return GMatteParent.Get();
	}

	UMaterialInterface* Result = nullptr;

#if WITH_EDITOR
	// Matériau LIT mais MAT : Roughness=1 et Specular=0 -> plus de reflet plastique.
	// BaseColor = paramètre vecteur « Color ».
	if (UMaterial* M = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient))
	{
		M->SetShadingModel(MSM_DefaultLit);

		UMaterialExpressionVectorParameter* P = NewObject<UMaterialExpressionVectorParameter>(M);
		P->ParameterName = TEXT("Color");
		P->DefaultValue = FLinearColor(0.3f, 0.3f, 0.3f, 1.f);
		M->GetExpressionCollection().AddExpression(P);
		M->GetEditorOnlyData()->BaseColor.Expression = P;

		UMaterialExpressionConstant* Rough = NewObject<UMaterialExpressionConstant>(M);
		Rough->R = 1.0f;
		M->GetExpressionCollection().AddExpression(Rough);
		M->GetEditorOnlyData()->Roughness.Expression = Rough;

		UMaterialExpressionConstant* Spec = NewObject<UMaterialExpressionConstant>(M);
		Spec->R = 0.0f;
		M->GetExpressionCollection().AddExpression(Spec);
		M->GetEditorOnlyData()->Specular.Expression = Spec;

		M->PostEditChange();
		Result = M;
	}
#endif

	if (!Result)
	{
		Result = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}

	GMatteParent.Reset(Result);
	return Result;
}

UMaterialInstanceDynamic* WOTOLGlow::MakeMatte(UObject* Outer, const FLinearColor& BaseColor)
{
	UMaterialInterface* Parent = GetMatteParent();
	if (!Parent)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Parent, Outer);
	if (MID)
	{
		MID->SetVectorParameterValue(TEXT("Color"), BaseColor);
	}
	return MID;
}

UMaterialInterface* WOTOLGlow::GetHaloParent()
{
	if (GHaloParent.IsValid())
	{
		return GHaloParent.Get();
	}

	UMaterialInterface* Result = nullptr;

#if WITH_EDITOR
	// Matériau UNLIT + TRANSLUCIDE : Emissive = « Color », Opacity = « Opacity ».
	// -> coquille lumineuse transparente qui enveloppe le modèle sans le cacher.
	if (UMaterial* M = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient))
	{
		M->SetShadingModel(MSM_Unlit);
		M->BlendMode = BLEND_Translucent;

		UMaterialExpressionVectorParameter* P = NewObject<UMaterialExpressionVectorParameter>(M);
		P->ParameterName = TEXT("Color");
		P->DefaultValue = FLinearColor::White;
		M->GetExpressionCollection().AddExpression(P);
		M->GetEditorOnlyData()->EmissiveColor.Expression = P;

		UMaterialExpressionScalarParameter* O = NewObject<UMaterialExpressionScalarParameter>(M);
		O->ParameterName = TEXT("Opacity");
		O->DefaultValue = 0.25f;
		M->GetExpressionCollection().AddExpression(O);
		M->GetEditorOnlyData()->Opacity.Expression = O;

		M->PostEditChange();
		Result = M;
	}
#endif

	if (!Result)
	{
		Result = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}

	GHaloParent.Reset(Result);
	return Result;
}

UMaterialInstanceDynamic* WOTOLGlow::MakeHalo(UObject* Outer, const FLinearColor& EmissiveHDR, float Opacity)
{
	UMaterialInterface* Parent = GetHaloParent();
	if (!Parent)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Parent, Outer);
	if (MID)
	{
		MID->SetVectorParameterValue(TEXT("Color"), EmissiveHDR);
		MID->SetScalarParameterValue(TEXT("Opacity"), Opacity);
	}
	return MID;
}

UMaterialInterface* WOTOLGlow::GetSpriteParent()
{
	if (GSpriteParent.IsValid())
	{
		return GSpriteParent.Get();
	}

	UMaterialInterface* Result = nullptr;

#if WITH_EDITOR
	// UNLIT + MASQUÉ (l'alpha de la texture découpe net, pas de tri de transparence) +
	// DEUX FACES (bTwoSided : le plan reste visible même si l'orientation calculée pour
	// faire face à la caméra isométrique fixe n'est pas parfaitement exacte -> filet de
	// sécurité, jamais de face invisible). BaseColor ET Emissive lisent la texture (rendu
	// fidèle aux couleurs de la planche, pas assombri par un éclairage de scène).
	if (UMaterial* M = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient))
	{
		M->SetShadingModel(MSM_Unlit);
		M->BlendMode = BLEND_Masked;
		M->TwoSided = true;

		UMaterialExpressionTextureSampleParameter2D* Tex =
			NewObject<UMaterialExpressionTextureSampleParameter2D>(M);
		Tex->ParameterName = TEXT("Texture");
		M->GetExpressionCollection().AddExpression(Tex);

		// Multiplie la texture par un scalaire "Brightness" (1 = actif/couleurs pleines,
		// <1 = terne pour verrouillé/pas-encore-construit) — même sémantique visuelle que
		// le repli kitbash (Color * 0.35 vs Color * 2.0 dans Refresh()).
		UMaterialExpressionScalarParameter* Brightness = NewObject<UMaterialExpressionScalarParameter>(M);
		Brightness->ParameterName = TEXT("Brightness");
		Brightness->DefaultValue = 1.0f;
		M->GetExpressionCollection().AddExpression(Brightness);

		UMaterialExpressionMultiply* Mul = NewObject<UMaterialExpressionMultiply>(M);
		Mul->A.Expression = Tex;
		Mul->B.Expression = Brightness;
		M->GetExpressionCollection().AddExpression(Mul);

		M->GetEditorOnlyData()->EmissiveColor.Expression = Mul;
		M->GetEditorOnlyData()->OpacityMask.Expression = Tex;
		M->GetEditorOnlyData()->OpacityMask.Mask = 0;
		M->GetEditorOnlyData()->OpacityMask.MaskA = 1; // canal alpha = decoupe

		M->PostEditChange();
		Result = M;
	}
#endif

	if (!Result)
	{
		Result = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}

	GSpriteParent.Reset(Result);
	return Result;
}

UMaterialInstanceDynamic* WOTOLGlow::MakeSprite(UObject* Outer, UTexture2D* Texture)
{
	UMaterialInterface* Parent = GetSpriteParent();
	if (!Parent || !Texture)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Parent, Outer);
	if (MID)
	{
		MID->SetTextureParameterValue(TEXT("Texture"), Texture);
	}
	return MID;
}
