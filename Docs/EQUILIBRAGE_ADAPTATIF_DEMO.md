# WOTOL — Équilibrage adaptatif de la démo

Les pertes ci-dessous sont des **centres de plage**, jamais des résultats scriptés. À chaque
tentative, un seed différent choisit une cible dans la plage ; les esquives, parades, dégâts,
positions, axes de charge, couches verticales et tempéraments individuels continuent de résoudre
le combat normalement.

| Phase | Armée joueur | Facile | Normal | Difficile |
|---|---:|---:|---:|---:|
| Kraken | 16 | 2–3 | 5–7 | 8–12 |
| Défense du bâtiment | 35 | 6–10 | 12–18 | 15–20 |
| Phase 3 — joueur Aquiloris | 60 contre 100 | centre 15, plage 10–20 | centre 25, plage 20–30 | centre 45, plage 40–50 |
| Phase 3 — joueur Noxéen | 100 contre 60 | centre 25, plage 20–30 | centre 45, plage 40–50 | centre 75, plage 70–80 |

## Fonctionnement

- La pression ennemie est réévaluée toutes les 0,75 seconde selon les pertes réelles, le temps
  écoulé et l'intensité des ordres manuels du joueur.
- La correction est progressive et bornée ; elle ne sélectionne jamais une victime précise.
- Une respiration sinusoïdale et une variance propres à la tentative évitent un DPS identique
  d'une partie à l'autre.
- Chaque unité reçoit un tempérament prudent/audacieux qui modifie légèrement sa hauteur, sa
  distance de sécurité et son repli.
- Les axes de flanc, la phase des charges et la répartition siège/chasse changent à chaque replay.
- Un garde-fou empêche seulement une victoire ou une annihilation trop précoce hors de la plage.

Le seed et le résultat réel sont écrits dans les logs `WOTOL Encounter` et `WOTOL Balance` afin
de reproduire un cas de test sans rendre toutes les parties déterministes.

