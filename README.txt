# Démineur minimal — NumWorks N0120

Projet préparé pour une application externe Epsilon en C.

Caractéristiques :
- grille 8×8
- 10 mines
- première case protégée
- déplacement avec les flèches
- OK = ouvrir
- SHIFT = drapeau
- BACK = quitter
- aucune allocation dynamique (pas de malloc/free)
- tableaux statiques uniquement, pour limiter les risques liés au problème de heap signalé sur N0120

Important :
Ce dossier contient le SOURCE du jeu et le Makefile basé sur le modèle C officiel de NumWorks.
Il ne contient pas encore le fichier `output/app.nwa`, car la toolchain ARM n'est pas disponible dans cet environnement.

Le modèle officiel indique que la compilation nécessite `arm-none-eabi-gcc` et Node.js/nwlink.
