#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Units/UnitBase.h"
#include "WOTOLDemoUnit.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class USceneComponent;

// ─────────────────────────────────────────────────────────────────────────────
// UNITÉ GREYBOX CONTEXTUALISÉE (sections 3-5 du cahier des charges).
// Forme symbolique selon la CATÉGORIE (rôle), TAILLE réelle du GDD, COULEUR de
// faction (bleu Aquiloris / vert Noxéen). Nom lisible. 100 % additif : hérite de
// AUnitBase, ne modifie rien. Remplaçable par un vrai mesh Meshy plus tard.
//
// Formes : Chef = cylindre haut · Infanterie = cylindre · Montée = bloc massif ·
//          Distance = cône orienté · Spéciale = cylindre fin · Mythique = grand bloc.
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class WOTOL_API AWOTOLDemoUnit : public AUnitBase
{
	GENERATED_BODY()

public:
	AWOTOLDemoUnit();

	// Hauteur réelle approximative en mètres (valeurs du GDD/doc), pour l'échelle
	UFUNCTION(BlueprintPure, Category = "Demo|Greybox")
	static float GetUnitHeightMeters(FName UnitID);

	// Si vrai : comportement de créature/boss autonome (cherche, avance, attaque)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Greybox")
	bool bCreatureBrain = false;

	// Si vrai : c'est le boss "Kraken" (étiquette + couleur), même avant le combat.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Greybox")
	bool bIsBoss = false;

	// Créature géante occupant 2 NIVEAUX de verticalité : sa base repose sur son niveau
	// courant et le corps s'étend sur ~2 niveaux (capteur de clic sur toute la colonne).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Greybox")
	bool bTwoLayerCreature = false;

	// Multiplicateur de PV (1 = stats normales ; >1 pour un boss coriace)
	// À fixer AVANT BeginPlay (spawn différé).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Greybox")
	float HealthScale = 1.f;

	// ── FORMATION (groupe de ~5, style Total War) ──────────────────────────────────
	// Assigné au déploiement par le directeur : chaque unité appartient à un GROUPE (même
	// faction + même type) qui tente de garder sa formation (ligne/carré). Le groupe partage
	// UNE étiquette (PV cumulés) portée par l'unité CENTRALE. SlotOffset = position voulue
	// dans la formation, relative à l'ancre du groupe (X = vers l'ennemi, Y = latéral).
	UPROPERTY(BlueprintReadOnly, Category = "Demo|Formation") int32 FormationGroupId = -1;
	FVector2D FormationSlot = FVector2D::ZeroVector; // décalage voulu dans la formation
	bool      bFormationCenter = false;              // porte l'étiquette cumulée du groupe
	void SetFormation(int32 GroupId, const FVector2D& SlotOffset, bool bCenter)
	{ FormationGroupId = GroupId; FormationSlot = SlotOffset; bFormationCenter = bCenter; }

	// Horodatage du dernier ordre MANUEL du joueur sur cette unité. La ré-évaluation
	// tactique de l'IA (Director) laisse ces unités tranquilles pendant quelques secondes.
	UPROPERTY(BlueprintReadWrite, Category = "Demo|Greybox")
	float LastPlayerOrderTime = -1000.f;

	// PV max effectifs (en tenant compte de HealthScale)
	UFUNCTION(BlueprintPure, Category = "Demo|Greybox")
	int32 GetEffectiveMaxHealth() const;

	// Remet les PV au maximum effectif (après un changement de HealthScale à chaud,
	// ex. calibrage du boss sur l'armée du joueur au lancement de la bataille).
	UFUNCTION(BlueprintCallable, Category = "Demo|Greybox")
	void SetHealthToFull();

	// ─── Verticalité (nage) : l'unité tient une hauteur (couche) donnée ───────
	// Règle la couche verticale cible ; l'unité y monte/descend en douceur et la tient.
	// BORNÉE par MaxLayerZ : certaines unités (montées) ne peuvent pas dépasser un grade.
	UFUNCTION(BlueprintCallable, Category = "Demo|Greybox")
	void SetDesiredZ(float NewZ) { DesiredZ = FMath::Clamp(NewZ, 0.f, MaxLayerZ); }

	// Plafond de couche verticale de CETTE unité (ex. montées limitées au grade 1 = 800).
	float MaxLayerZ = 2400.f;

	UFUNCTION(BlueprintPure, Category = "Demo|Greybox")
	float GetDesiredZ() const { return DesiredZ; }

	// Vrai si l'unité porte un bouclier (Aquiloryons) : le cerveau tactique du Director s'en
	// sert pour la placer en MUR DE BOUCLIERS / TORTUE (ligne serrée ancrée au sol).
	UFUNCTION(BlueprintPure, Category = "Demo|Greybox")
	bool HasShield() const { return bHasShield; }

	// Ordonne à cette unité d'ATTAQUER une structure de décor (jusqu'à sa destruction).
	UFUNCTION(BlueprintCallable, Category = "Demo|Greybox")
	void OrderAttackCover(class AWOTOLCoverStructure* Cover);

	// % de vie calculé sur les PV EFFECTIFS (boss inclus) — pour la barre du HUD.
	// (GetHealthPercent() de base sature à 100% tant que PV > MaxHealth de base.)
	UFUNCTION(BlueprintPure, Category = "Demo|Greybox")
	float GetEffectiveHealthPercent() const;

	// Les textes flottants s'accrochent au VisualRoot (position visuelle réelle, couche
	// verticale comprise) -> chaque chiffre suit son unité et sa hauteur.
	virtual class USceneComponent* GetFloatingTextAnchor() const override;
	// Kraken : les chiffres de dégâts s'affichent près de son NOM/PV (haut du colosse), pas à
	// sa base où ils étaient invisibles.
	virtual class USceneComponent* GetDamageTextAnchor() const override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo|Greybox")
	TObjectPtr<UStaticMeshComponent> ShapeMesh;

	// Étiquette flottante : nom de l'unité + PV% (remplace une barre de vie UMG)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo|Greybox")
	TObjectPtr<UTextRenderComponent> NameTag;

	// Ombre noire derrière l'étiquette (contraste avec le décor)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo|Greybox")
	TObjectPtr<UTextRenderComponent> NameTagShadow;

	// Construit la forme greybox (mesh + échelle + couleur) selon rôle/faction/taille
	void BuildGreyboxShape();

private:
	// Matériau dynamique de la forme (pour changer la couleur à la sélection)
	UPROPERTY()
	TObjectPtr<class UMaterialInstanceDynamic> ShapeMID;

	// ─── Kitbash : pièces additionnelles formant la silhouette de l'unité ─────
	// Ajoute une primitive enfant (mesh + transform + couleur) à l'unité.
	UStaticMeshComponent* AddPart(const TCHAR* MeshPath, const FVector& RelLoc,
		const FVector& RelScale, const FRotator& RelRot, const FLinearColor& Color);
	// Configure le corps principal (ShapeMesh) comme une pièce kitbash.
	void SetupMainPart(const TCHAR* MeshPath, const FVector& RelLoc,
		const FVector& RelScale, const FRotator& RelRot, const FLinearColor& Color);
	// Assemble la silhouette selon l'unité (corps + tête + accessoires).
	void AssembleSilhouette(FName UnitID, EUnitRole UnitRole, float HeightU,
		const FLinearColor& Base, const FLinearColor& Accent);
	// Construit le KRAKEN (céphalopode + 2 fouets) — TOUJOURS le même design, quelle
	// que soit la faction rivale (le boss est une créature neutre, pas un mythique).
	void BuildKrakenCephalopod(float HeightU);

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PartMIDs;
	TArray<FLinearColor> PartBaseColors;

	// Proxy de clic : sphère de collision qui SUIT la couche visuelle (VisualRoot),
	// pour pouvoir sélectionner/cibler une unité affichée en hauteur (le corps
	// physique, lui, reste au sol pour la navigation).
	UPROPERTY()
	TObjectPtr<class USphereComponent> ClickProxy;

	// Disque d'équipe sous les pieds (bleu/vert) — toujours visible, lisibilité RTS.
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> TeamMarker;
	void AddTeamMarker(float Radius, float ZFeet, const FLinearColor& Color);

	// ─── Membres articulés + animation procédurale (preuve : Aquiloryons) ─────
	// Crée une articulation (pivot) enfant ; on la fait tourner pour animer.
	class USceneComponent* MakeJoint(USceneComponent* Parent, const FVector& RelLoc);
	// Crée un "os" (mesh) suspendu à une articulation (offset = pend depuis le pivot).
	UStaticMeshComponent* MakeBone(USceneComponent* Joint, const TCHAR* MeshPath,
		const FVector& Offset, const FVector& Scale, const FRotator& Rot, const FLinearColor& Color);
	// Squelette humanoïde générique : torse + cou + tête + 2 bras (épaule/coude/main)
	// + 2 jambes (hanche/genou/pied). Renseigne les articulations pour l'animation ;
	// le mains sont accessibles via JRElbow / JLElbow pour y accrocher les armes.
	void BuildArticulatedHumanoid(float HeightU, const FLinearColor& Body, float BodyW);
	// Aquiloryons = humanoïde + épée + bouclier
	void BuildArticulatedAquiloryons(float HeightU, const FLinearColor& Armor, const FLinearColor& Energy);
	// Anime les articulations selon l'état (idle / marche / attaque / bouclier).
	void AnimateArticulated(float DeltaSeconds);

	UPROPERTY() TObjectPtr<USceneComponent> JRShoulder;
	UPROPERTY() TObjectPtr<USceneComponent> JRElbow;
	UPROPERTY() TObjectPtr<USceneComponent> JLShoulder;
	UPROPERTY() TObjectPtr<USceneComponent> JLElbow;
	UPROPERTY() TObjectPtr<USceneComponent> JRHip;
	UPROPERTY() TObjectPtr<USceneComponent> JLHip;
	UPROPERTY() TObjectPtr<USceneComponent> JRKnee;
	UPROPERTY() TObjectPtr<USceneComponent> JLKnee;

	// Couche verticale = décalage VISUEL (l'unité apparaît en hauteur) ; le corps
	// physique reste au sol -> déplacement + attaques fonctionnent à toute hauteur.
	float DesiredZ     = 0.f;   // décalage de couche cible (0 = fond)
	float CurLayer     = 0.f;   // décalage courant (interpolé)
	// Décalage VERTICAL de base du visuel (constant). Sert au Kraken : son corps est
	// construit au-dessus de l'origine ; on le rabaisse pour qu'il REPOSE au niveau du
	// sol (niveau 1) au lieu de léviter, aligné avec son capteur de clic/ciblage.
	float VisualBaseZ  = 0.f;
	bool  bArticulated = false;
	// Retourne le visuel de 180° (humanoïdes construits "dos à l'avant") : corps + rig
	// tournent ensemble -> le personnage regarde et frappe enfin vers l'AVANT.
	bool  bVisualYawFlip = false;
	// Corps construit HORIZONTALEMENT (créatures quadrupèdes/serpentines : avant = +X,
	// tête en hauteur). À la MORT, ces corps ne doivent PAS pointer le museau vers le ciel :
	// ils s'affalent SUR LE FLANC (roll), à plat sur le sol -> vrai cadavre allongé.
	// Les humanoïdes (construits debout le long de +Z) basculent, eux, vers l'avant (pitch).
	bool  bHorizontalBody = false;

	// ── Étiquettes de champ de bataille GROUPÉES (style Total War) ──
	// Quand plusieurs unités du MÊME type sont proches, on n'affiche qu'UNE étiquette
	// (nom + PV CUMULÉS du groupe) portée par un « représentant » ; les autres masquent la
	// leur -> beaucoup moins de texte à l'écran ET moins de mises à jour (gain visuel + perf,
	// crucial en phase 3). Une unité qui S'ÉLOIGNE du groupe (ou sélectionnée) retrouve sa
	// propre étiquette (nom + PV individuels).
	float GroupTagTimer = 0.f;      // throttle du recalcul (staggeré)
	bool  bTagSuppressed = false;   // masquée : couverte par le représentant du groupe
	bool  bTagIsRep      = false;   // porte l'étiquette CUMULÉE du groupe
	int32 GroupTagCount  = 1;       // effectif agrégé
	int32 GroupTagCur    = 0;       // PV cumulés courants
	int32 GroupTagMax    = 0;       // PV cumulés max
	void  ComputeGroupTag();        // recalcul du voisinage même-type (throttlé)
public:
	// Lus par le HUD pour dessiner les MARQUEURS de groupe (icône + effectif + barre de vie).
	bool  IsTagRep() const        { return bTagIsRep; }        // porte le marqueur du groupe
	bool  IsTagSuppressed() const { return bTagSuppressed; }   // couverte -> aucun marqueur
	int32 GetTagCount() const     { return GroupTagCount; }
	int32 GetTagCur() const       { return GroupTagCur; }
	int32 GetTagMax() const       { return GroupTagMax; }
private:

	float AnimPhase    = 0.f;
	float SwingProgress = 0.f; // 0..1 avancement d'un coup d'épée
	float AttackAnimTimer = 0.f; // >0 = un coup vient d'être porté -> jouer l'anim d'attaque

	// Déclenché à chaque coup réellement porté -> arme l'anim d'attaque (fenêtre courte).
	virtual void OnAttackAnimTrigger() override;

	// Interception des dégâts entrants (Aquis : parade + remplissage de la jauge d'impact).
	virtual float TakeDamageFromUnit(float Damage, AUnitBase* InstigatorUnit) override;

	// Aquilombres furtive : cachée aux ennemis (l'IA hostile ne la cible pas ; le joueur, lui,
	// la voit en fantôme).
	virtual bool IsHiddenFromEnemies() const override { return bStealthed; }

	// ─── Animation générique (toutes unités) : nage + inclinaison + appendices ──
	// Conteneur visuel : on le fait flotter/incliner pour animer TOUTE la silhouette.
	UPROPERTY() TObjectPtr<USceneComponent> VisualRoot;
	float AnimClock = 0.f;
	float BobSeed   = 0.f;
	void AnimateBody(float DeltaSeconds);
	// Enregistre un appendice (tentacule) à faire onduler autour de sa base.
	void RegisterWiggle(USceneComponent* Comp, float Phase);
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> WiggleComps;
	TArray<FRotator> WiggleBase;
	TArray<float>    WigglePhase;

	// Réagit à la sélection joueur : surligne l'unité
	UFUNCTION()
	void HandleSelected(bool bSel);

	// Affiche un chiffre de dégâts flottant quand l'unité perd des PV
	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float MaxHealth);

	// MORT : stoppe tout mouvement, rend l'unité non sélectionnable/ciblable ; elle coule.
	UFUNCTION()
	void HandleDeath(AUnitBase* Unit);

	// Cerveau autonome de créature/boss (cherche l'ennemi, avance, attaque)
	void CreatureBrainTick(float DeltaSeconds);

	// ─── Compétences ACTIVES (déclenchées périodiquement, cooldown du tableur) ─────
	// RÈGLE GÉNÉRALE : une compétence n'est lancée QUE si elle est LÉGITIME (une cible/
	// un motif réel). Sinon l'unité la GARDE (retente vite) et continue ses coups basiques.
	// Chaque Ability_* renvoie true si elle a RÉELLEMENT été lancée, false sinon.
	void  TickAbility(float DeltaSeconds); // décrémente le cooldown + déclenche si légitime
	bool  UseAbility();                    // dispatch selon l'unité (true = lancée)
	bool  Ability_Shockwave();             // Aquis : 2 modes (sol autour / sur la cible) selon la situation
	bool  Ability_Laser();                 // Noxar : rayon laser sur l'objectif / le + proche
	bool  Ability_ProjectileBurst();       // Noxeblast : rafale de projectiles
	bool  Ability_BlindFlash();            // Noxeflare : éblouit les ennemis proches
	bool  Ability_Hydrolaser();            // Aquisphères : grosse boule mono OU rafale de zone
	bool  Ability_Charge();                // Noxebeast : charge frontale (repousse + interrompt)
	bool  Ability_LaserBig();              // Noxedrake : ÉNORME rayon mono-cible (décor OU unité)

	// NOXEDRAKE — Surcharge reçue du Noxar : multiplie le prochain Souffle (1 = normal).
	float NoxedrakeCharge = 1.f;
	bool  Ability_ShadowStrike();          // Aquilombres : bond furtif dans le dos + crit + ombre
	bool  Ability_Resonance();             // Léviaphénix : pulse d'aura (soin + amplification alliés)
	// Tir de base des Aquisphères : boule Hydrolaser (traînée de bulles) OU coup de crosse
	// au corps-à-corps très rapproché. Appelé au moment d'un vrai coup.
	void  FireHydrolaserOrMelee();

	// AQUILOMBRES — PASSIF « invisible si immobile » : dissimulée tant qu'elle reste immobile
	// (arrière-ligne protégée). Réapparaît dès qu'elle BOUGE (défense / changement de couche)
	// ou qu'elle ATTAQUE, puis se re-dissimule si elle redevient immobile.
	bool  bStealthed   = false;
	float StealthTimer = 0.f;
	void  UpdateStealth(float DeltaSeconds);
	void  SetStealthVisual(bool bOn);

	// LÉVIAPHÉNIX — AURA passive : amplifie les alliés proches (dégâts/défense) et accélère
	// leurs recharges. Réévaluée périodiquement.
	void  TickAura(float DeltaSeconds);
	float AuraTimer = 0.f;
	// Accélération de recharge reçue d'une aura (1 = normal, >1 = plus rapide). Décroît seule.
	float AuraCooldownRate = 1.f;

	// LÉVIAPHÉNIX — PASSIF de DÉFENSE : attaqué au corps-à-corps, il se défend seul par un
	// COUP DE NAGEOIRE (grandes nageoires pectorales) ou un COUP DE QUEUE qui REPOUSSE les
	// ennemis proches (mécanique des fluides). Nageoires + queue sur articulations animables.
	UPROPERTY() TObjectPtr<USceneComponent> LeviFinL;
	UPROPERTY() TObjectPtr<USceneComponent> LeviFinR;
	UPROPERTY() TObjectPtr<USceneComponent> LeviTail;
	float LeviDefTimer    = 0.f;   // >0 = balayage en cours
	float LeviDefCooldown = 0.f;   // délai avant la prochaine parade
	int32 LeviDefKind     = 0;     // 0 = coup de nageoire, 1 = coup de queue
	float LeviTailDir     = 1.f;   // sens du balayage de queue
	void  LeviphenixDefenseTick(float DeltaSeconds); // détecte le contact -> déclenche
	void  AnimateLeviphenix(float DeltaSeconds);      // ondulation nageoires/queue + balayage
	float GetAbilityCooldownFor(FName Id) const; // CD du tableur par unité
	float AbilityCooldown = 6.f;           // temps avant la prochaine compétence
	bool  bAbilityInit = false;

	// AQUIS — jauge d'impact : la lame photonique PARE une part des dégâts entrants et
	// BANQUE l'énergie bloquée ici ; l'onde de choc la relâche (dégâts proportionnels).
	float ImpactGauge = 0.f;

	// AQUILORYONS — bouclier énergétique : l'unité PORTE un bouclier (pose de garde,
	// blocage renforcé). L'efficacité dépend de l'ANCRAGE AU SOL (voir GetGroundedFactor).
	bool  bHasShield = false;
	float ShieldGuardTimer = 0.f; // >0 = vient de bloquer -> tient le bouclier levé un court instant

	// AQUISPHÈRES — porte un CANON tenu à DEUX MAINS (pose de port/tir des deux bras).
	bool  bTwoHandWeapon = false;

	// NOXEBEAST — « Carapace Pressurisée » : plus il subit de coups rapprochés, plus sa
	// résistance monte (0..~0.45) ; décroît seule quand on cesse de le frapper.
	float Carapace = 0.f;

	// NOXÉONS — « Émergence Luminale » : zone bioluminescente qui amplifie les Noxéens
	// proches (dégâts + recharges). Réévaluée périodiquement.
	void  TickNoxeonZone(float DeltaSeconds);
	float NoxeonZoneTimer = 0.f;

	// Anneau visuel de zone/aura (Léviaphénix, Noxéons) : MASQUÉ à la mort (on ne doit plus
	// voir la zone de pouvoir d'une unité morte -> il ne reste que le corps qui coule).
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> AuraRingParts;

	// Anneau de SÉLECTION au sol : contour lumineux LÉGER (couleur de faction) qui
	// entoure l'unité sélectionnée sans repeindre le modèle -> on distingue toujours
	// la couleur des unités. Construit à la 1re sélection puis simplement masqué/affiché.
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> SelectionRingParts;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SelHaloMID; // matériau du halo (opacité animée)
	float SelHaloAlpha  = 0.f;   // 0..1 : apparition RAPIDE (~0.4 s) puis stable — juste suggéré
	float SelHaloTarget = 0.f;   // cible (1 = sélectionnée)
	void BuildSelectionRing();
	void TickSelectionHalo(float Dt); // apparition/disparition rapide et douce du halo

	// Facteur d'ancrage au sol : 1 = bien planté au sol (couche 0), 0 = en pleine hauteur.
	// Sert au blocage (meilleur au sol) et au recul (part plus loin en l'air).
	float GetGroundedFactor() const;
	// Échelle de recul (knockback) subie : les unités bien ancrées au sol reculent MOINS
	// (bouclier planté) ; en hauteur elles sont projetées PLUS loin (pas d'appui).
	float GetKnockbackScale() const;
	// Nombre d'Aquiloryons alliés proches (mur de boucliers) -> bonus de synergie de faction.
	int32 CountNearbyShieldAllies(float Radius) const;

	// SÉPARATION DOUCE (lisibilité) : écarte gentiment les unités d'une MÊME couche
	// verticale pour éviter l'amas illisible, sans bloquer les couches différentes.
	void ApplySoftSeparation(float DeltaSeconds);
	// COHÉSION DE FORMATION : ramène doucement l'unité à sa position (slot) dans son groupe
	// tant qu'aucun ennemi n'est en portée (voyage/attente) -> les blocs gardent leur forme
	// (ligne/carré) et se déplacent ensemble. En mêlée, la formation se libère.
	void ApplyFormationCohesion(float DeltaSeconds);
	float CohTimer = 0.f;
	// TACTIQUE DE RÔLE : chaque unité se comporte selon son RÔLE + ses compétences (les unités
	// à distance/artillerie gardent leurs distances et prennent la hauteur pour tirer par-dessus
	// la mêlée ; les chargeurs/assassins plongent sur l'arrière-garde via FindHiveTargetEnemy).
	void TickRoleTactics(float DeltaSeconds);
	float RoleTimer = 0.f;

	// AQUILANCES — lance sur articulation (coup de lance = poussée vers l'avant) + son ancrage.
	UPROPERTY() TObjectPtr<USceneComponent> LanceJoint;
	FVector LanceHome = FVector::ZeroVector;
	// Bascule GARDE PASSIVE (lance en diagonale, au repos) <-> GARDE AGRESSIVE (lance
	// couchée vers l'avant, prête à frapper). 0 = passive, 1 = agressive ; interpolé.
	float LanceAggro       = 0.f;
	float LanceAggroTarget = 0.f;

	// SYNERGIE AQUILORIS (lance ↔ bouclier) : les Aquilances placées DERRIÈRE un bouclier
	// Aquiloryon sont PROTÉGÉES (bLanceGuarded) et, en retour, dopent l'ATTAQUE de ce
	// bouclier (SynergyDamageMult sur l'Aquiloryon). Réévalué périodiquement.
	void  UpdateAquilorisSynergy();
	float SynergyTimer   = 0.f;
	bool  bLanceGuarded  = false;

	// ─── Fouets du Kraken (2 grands tentacules articulés) ─────────────────────
	// Chaîne de pivots (base → pointe) formant un tentacule capable de "claquer".
	void BuildWhipTentacle(const FVector& RootLoc, float SideSign,
		const FLinearColor& Color, float H);
	// Anime les deux fouets : ondulation au repos, déroulé rapide pendant un coup.
	void AnimateWhips(float DeltaSeconds);
	// Déclenche un coup de fouet : repousse et blesse les unités devant le Kraken.
	void DoWhipStrike();
	void DoInkJet(AUnitBase* Target); // jet d'encre -> flaque ralentissante/aveuglante

	UPROPERTY() TArray<TObjectPtr<USceneComponent>> WhipJointsL;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> WhipJointsR;
	float WhipCooldown = 2.f;   // temps avant le prochain coup
	float WhipStrike   = -1.f;  // <0 = repos ; 0..1 = déroulé du coup en cours
	float CritCooldown = 3.f;   // (boss) temps avant la prochaine attaque critique possible
	float InkCooldown  = 6.f;   // (boss) temps avant le prochain JET D'ENCRE

	// (Boss) REPOSITIONNEMENT dynamique : le Kraken n'est pas figé, il tourne autour de
	// l'ennemi, change d'angle et de couche verticale pour anticiper/attaquer à découvert.
	float BossRepositionCD  = 4.f;   // délai avant la prochaine manœuvre
	float BossRepoTimer     = 0.f;   // temps restant de la manœuvre en cours
	float BossStrafeDir     = 1.f;   // sens du contournement (gauche/droite)
	float BossLayerGoal     = 0.f;   // couche visée pendant la manœuvre (base, 0..800)

	// Combat vertical : quand l'unité poursuit/attaque, elle rejoint la couche
	// (hauteur) de sa cible — mais avec un TEMPS D'ADAPTATION (pas instantané), pour que
	// l'ennemi ne "colle" pas la hauteur du joueur en même temps que lui.
	void UpdateCombatLayer(float DeltaSeconds);
	// Fait tendre DesiredZ vers GoalZ après un délai de réaction (temps d'adaptation).
	void AdaptLayerTo(float GoalZ, float DeltaSeconds);
	float LayerAdaptGoal  = -1.f;  // dernière hauteur de cible observée
	float LayerReactTimer = 0.f;   // compte à rebours avant de s'adapter
	// Unité ennemie la plus proche (partagée par le cerveau boss et le combat vertical)
	class AUnitBase* FindNearestEnemyUnit() const;
	// Cible coordonnée « esprit de ruche » (ennemi affaibli/proche du front à concentrer).
	class AUnitBase* FindHiveTargetEnemy() const;

	// SOIN sûr : rend des PV en respectant le MAX EFFECTIF (avec HealthScale/équilibrage) ->
	// contrairement au soin par dégâts négatifs qui plafonne au MaxHealth de base (et pouvait
	// « soigner » en négatif = blesser une unité mise à l'échelle). Utilisé par les capacités
	// de soin (Léviaphénix Rayonnement Vital, Noxéons Ancrage Abyssal).
	void HealEffective(float Amount);
	// Affichage du SOIN reçu (nombre vert flottant, comme les dégâts) : on ACCUMULE les petits
	// soins de régénération et on émet un « +N » périodique -> lisible sans spammer l'écran.
	float HealAccum = 0.f;
	float HealTextTimer = 0.f;

	// Cible de décor imposée par le joueur (attaquer une ruine/pilier jusqu'à destruction).
	TWeakObjectPtr<class AWOTOLCoverStructure> TargetCover;
	void TickAttackCover(float DeltaSeconds);
	float CoverAttackTimer = 0.f;

	// ── IA TACTIQUE : utiliser le décor destructible (faire s'effondrer une structure
	// sur un groupe d'ennemis placés derrière). ──
	float CoverTacticTimer = 0.f;
	bool  bCoverTactic = false; // le TargetCover courant est un choix TACTIQUE de l'IA
	bool  bPlayerCoverOrder = false; // ORDRE JOUEUR de detruire une ruine : PRIORITAIRE (l'unite
	                                 // ne se laisse PAS detourner par un ennemi tant que ce n'est
	                                 // pas detruit).
	class AWOTOLCoverStructure* FindTacticalCover() const;

	float LastKnownHealth = -1.f;
	bool  bCreatureStyled = false;

	// ── PERF : le sous-système de flux (écran de jeu) est interrogé plusieurs fois par frame
	// et par unité. Avec 160 unités en phase 3, on le MET EN CACHE (résolu une fois). ──
	TWeakObjectPtr<class UDemoFlowSubsystem> CachedFlow;
	bool IsBattleLive();
	// Multiplicateur de dégâts ENNEMIS lié à la difficulté (utilisé par le Kraken) :
	// Facile 0.8 / Normal 1.0 / Difficile 1.3.
	float DifficultyEnemyDamageMult();
	// Séparation douce coûteuse (O(n²) sur toutes les unités) : on l'ÉTALE dans le temps
	// (quelques fois par seconde) au lieu de chaque frame -> gros gain CPU, rendu identique.
	float SepTimer = 0.f;

	// ── ANTI-BLOCAGE (lecture du terrain) : si l'unité VEUT avancer mais ne bouge quasiment
	// plus (coincée contre un rocher/ruine), elle se DÉGAGE d'elle-même par un pas latéral. ──
	FVector StuckLastPos = FVector::ZeroVector;
	float   StuckCheckTimer = 0.f;   // cadence d'échantillonnage de la position
	float   UnstickTimer = 0.f;      // >0 = manœuvre de dégagement en cours
	FVector UnstickDir = FVector::ZeroVector;
	int32   StuckCount = 0;          // nb d'échantillons consécutifs bloqués -> escalade
	float   UnstickSide = 1.f;       // côté de contournement courant (garde le même sens)
	float   UnstickZBoost = 0.f;     // remontée temporaire (nage AU-DESSUS de l'obstacle)
	void    TickUnstick(float Dt, bool bWantsToMove);
};
