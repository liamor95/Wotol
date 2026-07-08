#include "WOTOLGlow.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_EDITOR
#include "Materials/MaterialExpressionVectorParameter.h"
#endif

namespace
{
	// Matériau parent mis en cache (référence FORTE -> pas collecté par le GC,
	// évite de recompiler le matériau émissif à chaque fois).
	TStrongObjectPtr<UMaterialInterface> GGlowParent;
}

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
