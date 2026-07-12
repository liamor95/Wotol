#include "WOTOLGlow.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_EDITOR
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionConstant.h"
#endif

namespace
{
	// Matériaux parents mis en cache (référence FORTE -> pas collectés par le GC,
	// évite de recompiler à chaque fois).
	TStrongObjectPtr<UMaterialInterface> GGlowParent;
	TStrongObjectPtr<UMaterialInterface> GMatteParent;
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
