#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * Matériau ÉMISSIF (unlit) partagé pour le greybox.
 *
 * BasicShapeMaterial n'a qu'une couleur de BASE (albédo) : un objet peint avec
 * ne « brille » pas, il faut une lampe à côté qui éclaire le SOL. Ici on veut
 * l'inverse : que le MESH lui-même émette la lumière (projectiles, rayon laser,
 * cristal du Cristalliseur, bulbes de corail). On fabrique donc un matériau
 * unlit dont l'entrée Emissive = paramètre « Color ». Ainsi une couleur >1
 * fait rayonner l'objet (bloom) sans dépendre d'aucune lampe.
 */
namespace WOTOLGlow
{
	/** Matériau parent émissif (construit une fois, mis en cache). */
	UMaterialInterface* GetGlowParent();

	/** MID émissif prêt à l'emploi : le mesh rayonne à la couleur HDR donnée. */
	UMaterialInstanceDynamic* MakeGlow(UObject* Outer, const FLinearColor& EmissiveHDR);

	/** Matériau parent MAT (rugueux, spéculaire ~0) : supprime l'aspect plastique/lisse
	 *  -> la roche, le sable et les unités accrochent la lumière en relief (contraste). */
	UMaterialInterface* GetMatteParent();

	/** MID mat prêt à l'emploi (couleur de base donnée, surface rugueuse non brillante). */
	UMaterialInstanceDynamic* MakeMatte(UObject* Outer, const FLinearColor& BaseColor);
}
