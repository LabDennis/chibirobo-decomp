#include "Version.h"
#if defined(VERSION_US)
#include "game/interface_text_us_fallback.h"
#endif

extern "C" {
#if defined(VERSION_PAL)
extern const char gEmptyResourceDirectorySuffix[];
extern const char sEnglishInterfaceText014[];
extern const char sEnglishInterfaceText015[];
extern const char sEnglishInterfaceText021[];
extern const char sEnglishInterfaceText023[];
extern const char sEnglishInterfaceText029[];
extern const char sEnglishInterfaceText034[];
extern const char sEnglishInterfaceText069[];
extern const char sEnglishInterfaceText090[];
extern const char sEnglishInterfaceText096[];
extern const char sEnglishInterfaceText258[];

extern const char sFrenchInterfaceText012[] = "Ranger";
extern const char sFrenchInterfaceText013[] = "Enlever";
extern const char sFrenchInterfaceText016[] = "Vibrations:";
extern const char sFrenchInterfaceText017[] = "Marche";
extern const char sFrenchInterfaceText018[] = "Arr\352t";
extern const char sFrenchInterfaceText019[] = "Son:";
extern const char sFrenchInterfaceText020[] = "St\351r\351o";
extern const char sFrenchInterfaceText022[] = "Retour";
extern const char sFrenchInterfaceText024[] = "Objets";
extern const char sFrenchInterfaceText025[] = "Manette";
extern const char sFrenchInterfaceText026[] = "Statut";
extern const char sFrenchInterfaceText027[] = "Autocollants";
extern const char sFrenchInterfaceText030[] = "Annuler";
extern const char sFrenchInterfaceText031[] = "Pivoter la cam\351ra";
extern const char sFrenchInterfaceText032[] = "Menu \351quipement";
extern const char sFrenchInterfaceText033[] = "Chibi-Scope";
extern const char sFrenchInterfaceText035[] = "Centrer la cam\351ra";
extern const char sFrenchInterfaceText036[] = "Carte";
extern const char sFrenchInterfaceText038[] = "Choisir";
extern const char sFrenchInterfaceText039[] = "Diriger Chibi-Robo / Choisir";
extern const char sFrenchInterfaceText040[] = "Chibi-Classement";
extern const char sFrenchInterfaceText041[] = "Chibi-Portes utilis\351es";
extern const char sFrenchInterfaceText042[] = "Points Bonheur";
extern const char sFrenchInterfaceText043[] = "Monis";
extern const char sFrenchInterfaceText044[] = "Ferraille";
extern const char sFrenchInterfaceText046[] = "Menu Chibi-Serveur";
extern const char sFrenchInterfaceText047[] = "Magasin en ligne";
extern const char sFrenchInterfaceText057[] = "Sortir";
extern const char sFrenchInterfaceText060[] = "Acheter";
extern const char sFrenchInterfaceText061[] = "S\351lectionner";
extern const char sFrenchInterfaceText062[] = "Transaction termin\351e.";
extern const char sFrenchInterfaceText063[] = "Acheter?";
extern const char sFrenchInterfaceText064[] = "Oui";
extern const char sFrenchInterfaceText065[] = "Non";
extern const char sFrenchInterfaceText070[] = "Fabriquer";
extern const char sFrenchInterfaceText072[] = "Revendre?";
extern const char sFrenchInterfaceText073[] = "Ferrailleur";
extern const char sFrenchInterfaceText074[] = "Taux d'achat";
extern const char sFrenchInterfaceText075[] = "Quelle quantit\351?";
extern const char sFrenchInterfaceText076[] = "Vous n'avez pas assez de ferraille.";
extern const char sFrenchInterfaceText077[] = "C'est";
extern const char sFrenchInterfaceText078[] = "d'accord?";
extern const char sFrenchInterfaceText080[] = "Construire?";
extern const char sFrenchInterfaceText081[] = "Pr\352t";
extern const char sFrenchInterfaceText082[] = "Revendre";
extern const char sFrenchInterfaceText084[] = "Le couvercle est ouvert.\012Pour continuer \340 jouer,\012veuillez fermer le couvercle.";
extern const char sFrenchInterfaceText085[] = "Veuillez ins\351rer le disque Chibi-Robo.";
extern const char sFrenchInterfaceText087[]
    = "La lecture du disque a \351chou\351.\012Veuillez vous r\351f\351rer au manuel\012d'instructions "
      "Nintendo GameCube\012pour de plus amples informations.";
extern const char sFrenchInterfaceText088[] = "Chargement";
extern const char sFrenchInterfaceText091[] = "Temps";
extern const char sFrenchInterfaceText092[] = "Parcours";
extern const char sFrenchInterfaceText093[] = "Ennemi";
extern const char sFrenchInterfaceText094[] = "Nouvelle partie";
extern const char sFrenchInterfaceText095[] = "Charger";
extern const char sFrenchInterfaceText097[] = "Param\350tres";
extern const char sFrenchInterfaceText098[] = "Sauvegarder";
extern const char sFrenchInterfaceText099[] = "Vide";
extern const char sFrenchInterfaceText103[] = "Aucune carte m\351moire n'est d\351tect\351e dans";
extern const char sFrenchInterfaceText104[] = "le Slot A. Ins\351rez une carte m\351moire ou";
extern const char sFrenchInterfaceText105[] = "appuyez sur le bouton A pour continuer";
extern const char sFrenchInterfaceText106[] = "sans sauvegarder.";
extern const char sFrenchInterfaceText108[] = "L'objet ins\351r\351 dans le Slot A n'est pas";
extern const char sFrenchInterfaceText109[] = "valide. Remplacez-le par une carte";
extern const char sFrenchInterfaceText110[] = "m\351moire ou appuyez sur le bouton A";
extern const char sFrenchInterfaceText111[] = "pour continuer sans sauvegarder.";
extern const char sFrenchInterfaceText113[] = "La carte m\351moire dans le Slot A est";
extern const char sFrenchInterfaceText114[] = "endommag\351e et ne peut pas \352tre utilis\351e.";
extern const char sFrenchInterfaceText118[] = "La carte m\351moire dans le Slot A contient";
extern const char sFrenchInterfaceText119[] = "des donn\351es corrompues et a besoin";
extern const char sFrenchInterfaceText120[] = "d'\352tre format\351e. Voulez-vous la formater?";
extern const char sFrenchInterfaceText123[] = "Formater la carte m\351moire entra\356nera la";
extern const char sFrenchInterfaceText124[] = "perte de toutes les donn\351es sauvegard\351es.";
extern const char sFrenchInterfaceText125[] = "Voulez-vous continuer le formatage?";
extern const char sFrenchInterfaceText128[] = "Formatage en cours...";
extern const char sFrenchInterfaceText129[] = "Ne touchez pas \340 la carte m\351moire";
extern const char sFrenchInterfaceText130[] = "dans le Slot A ni au bouton POWER.";
extern const char sFrenchInterfaceText133[] = "Impossible de formater la carte";
extern const char sFrenchInterfaceText134[] = "m\351moire dans le Slot A.";
extern const char sFrenchInterfaceText138[] = "La carte m\351moire dans le";
extern const char sFrenchInterfaceText139[] = "Slot A a \351t\351 format\351e.";
extern const char sFrenchInterfaceText143[] = "Continuer sans sauvegarder?";
extern const char sFrenchInterfaceText148[] = "S\351lectionner un fichier pour sauvegarder";
extern const char sFrenchInterfaceText149[] = "les donn\351es de la nouvelle partie.";
extern const char sFrenchInterfaceText153[] = "Il n'y a pas assez";
extern const char sFrenchInterfaceText154[] = "d'espace libre.";
extern const char sFrenchInterfaceText158[] = "Ces donn\351es sont";
extern const char sFrenchInterfaceText159[] = "corrompues.";
extern const char sFrenchInterfaceText163[] = "Il n'y a pas assez d'espace libre sur";
extern const char sFrenchInterfaceText164[] = "la carte m\351moire dans le Slot A.";
extern const char sFrenchInterfaceText165[] = "Chibi-Robo n\351cessite 1 fichier";
extern const char sFrenchInterfaceText166[] = "et 6 blocs pour \352tre sauvegard\351.";
extern const char sFrenchInterfaceText168[] = "Ecraser le fichier existant?";
extern const char sFrenchInterfaceText173[] = "Pour g\351rer le contenu de votre carte";
extern const char sFrenchInterfaceText174[] = "m\351moire, utilisez l'Ecran Memory Card";
extern const char sFrenchInterfaceText175[] = "(carte m\351moire).";
extern const char sFrenchInterfaceText178[] = "Sauvegarde en cours...";
extern const char sFrenchInterfaceText183[] = "Une erreur s'est produite";
extern const char sFrenchInterfaceText184[] = "durant la sauvegarde.";
extern const char sFrenchInterfaceText188[] = "Choisissez un fichier \340 charger.";
extern const char sFrenchInterfaceText193[] = "Chargement...";
extern const char sFrenchInterfaceText198[] = "Le fichier de Chibi-Robo sur la carte";
extern const char sFrenchInterfaceText199[] = "m\351moire dans le Slot A est corrompu.";
extern const char sFrenchInterfaceText200[] = "Ce fichier a besoin d'\352tre effac\351.";
extern const char sFrenchInterfaceText203[] = "Cr\351er un nouveau fichier";
extern const char sFrenchInterfaceText204[] = "de donn\351es?";
extern const char sFrenchInterfaceText220[] = "Ins\351rez une carte m\351moire ou appuyez";
extern const char sFrenchInterfaceText221[] = "sur le bouton A pour continuer sans";
extern const char sFrenchInterfaceText222[] = "sauvegarder.";
extern const char sFrenchInterfaceText223[] = "Veuillez ins\351rer une carte m\351moire ou";
extern const char sFrenchInterfaceText228[] = "Sauvegarde termin\351e.";
extern const char sFrenchInterfaceText233[] = "Choisissez un fichier.";
extern const char sFrenchInterfaceText238[] = "Chargement des donn\351es impossible.";
extern const char sFrenchInterfaceText243[] = "La carte m\351moire dans le Slot A ";
extern const char sFrenchInterfaceText244[] = "est p\351rim\351e.";
extern const char sFrenchInterfaceText259[] = "Temps de jeu:";
extern const char sFrenchInterfaceText262[] = "Le temps de jeu de la version de d\351monstration de Chibi-Robo est \351coul\351. Appuyez"
                                              " sur le bouton A pour continuer.";
extern const char sFrenchInterfaceText263[] = "Ce jeu est compatible";
extern const char sFrenchInterfaceText264[] = "avec le mode 60 Hz.";
extern const char sFrenchInterfaceText265[] = "Veuillez choisir un mode.";
extern const char sFrenchInterfaceText266[] = "Si votre t\351l\351viseur n'affiche pas";
extern const char sFrenchInterfaceText267[] = "correctement le mode 60 Hz,";
extern const char sFrenchInterfaceText268[] = "choisissez le mode 50 Hz.";
extern const char sFrenchInterfaceText270[] = "%s%02X Mode 60 Hz %s  %s%02X Mode 50 Hz %s";
extern const char sFrenchInterfaceText271[] = "L'affichage est en mode 60 Hz.";
extern const char sFrenchInterfaceText274[] = "L'affichage est en mode 50 Hz.";
extern const char sFrenchInterfaceText277[] = "NOUVEAU";

const char* gFrenchInterfaceText[] = {
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sFrenchInterfaceText012,       sFrenchInterfaceText013,       sEnglishInterfaceText014,      sEnglishInterfaceText015,
	sFrenchInterfaceText016,       sFrenchInterfaceText017,       sFrenchInterfaceText018,       sFrenchInterfaceText019,
	sFrenchInterfaceText020,       sEnglishInterfaceText021,      sFrenchInterfaceText022,       sEnglishInterfaceText023,
	sFrenchInterfaceText024,       sFrenchInterfaceText025,       sFrenchInterfaceText026,       sFrenchInterfaceText027,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText029,      sFrenchInterfaceText030,       sFrenchInterfaceText031,
	sFrenchInterfaceText032,       sFrenchInterfaceText033,       sEnglishInterfaceText034,      sFrenchInterfaceText035,
	sFrenchInterfaceText036,       sEnglishInterfaceText023,      sFrenchInterfaceText038,       sFrenchInterfaceText039,
	sFrenchInterfaceText040,       sFrenchInterfaceText041,       sFrenchInterfaceText042,       sFrenchInterfaceText043,
	sFrenchInterfaceText044,       gEmptyResourceDirectorySuffix, sFrenchInterfaceText046,       sFrenchInterfaceText047,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sFrenchInterfaceText057,       sFrenchInterfaceText022,       gEmptyResourceDirectorySuffix,
	sFrenchInterfaceText060,       sFrenchInterfaceText061,       sFrenchInterfaceText062,       sFrenchInterfaceText063,
	sFrenchInterfaceText064,       sFrenchInterfaceText065,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText069,      sFrenchInterfaceText070,       sFrenchInterfaceText062,
	sFrenchInterfaceText072,       sFrenchInterfaceText073,       sFrenchInterfaceText074,       sFrenchInterfaceText075,
	sFrenchInterfaceText076,       sFrenchInterfaceText077,       sFrenchInterfaceText078,       gEmptyResourceDirectorySuffix,
	sFrenchInterfaceText080,       sFrenchInterfaceText081,       sFrenchInterfaceText082,       gEmptyResourceDirectorySuffix,
	sFrenchInterfaceText084,       sFrenchInterfaceText085,       sFrenchInterfaceText085,       sFrenchInterfaceText087,
	sFrenchInterfaceText088,       sFrenchInterfaceText088,       sEnglishInterfaceText090,      sFrenchInterfaceText091,
	sFrenchInterfaceText092,       sFrenchInterfaceText093,       sFrenchInterfaceText094,       sFrenchInterfaceText095,
	sEnglishInterfaceText096,      sFrenchInterfaceText097,       sFrenchInterfaceText098,       sFrenchInterfaceText099,
	gEmptyResourceDirectorySuffix, sFrenchInterfaceText064,       sFrenchInterfaceText065,       sFrenchInterfaceText103,
	sFrenchInterfaceText104,       sFrenchInterfaceText105,       sFrenchInterfaceText106,       gEmptyResourceDirectorySuffix,
	sFrenchInterfaceText108,       sFrenchInterfaceText109,       sFrenchInterfaceText110,       sFrenchInterfaceText111,
	gEmptyResourceDirectorySuffix, sFrenchInterfaceText113,       sFrenchInterfaceText114,       gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sFrenchInterfaceText118,       sFrenchInterfaceText119,
	sFrenchInterfaceText120,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sFrenchInterfaceText123,
	sFrenchInterfaceText124,       sFrenchInterfaceText125,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sFrenchInterfaceText128,       sFrenchInterfaceText129,       sFrenchInterfaceText130,       gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sFrenchInterfaceText133,       sFrenchInterfaceText134,       gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sFrenchInterfaceText138,       sFrenchInterfaceText139,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sFrenchInterfaceText143,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sFrenchInterfaceText148,       sFrenchInterfaceText149,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sFrenchInterfaceText153,       sFrenchInterfaceText154,       gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sFrenchInterfaceText158,       sFrenchInterfaceText159,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sFrenchInterfaceText163,
	sFrenchInterfaceText164,       sFrenchInterfaceText165,       sFrenchInterfaceText166,       gEmptyResourceDirectorySuffix,
	sFrenchInterfaceText168,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sFrenchInterfaceText173,       sFrenchInterfaceText174,       sFrenchInterfaceText175,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sFrenchInterfaceText178,       sFrenchInterfaceText129,
	sFrenchInterfaceText130,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sFrenchInterfaceText183,
	sFrenchInterfaceText184,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sFrenchInterfaceText188,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sFrenchInterfaceText193,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sFrenchInterfaceText198,       sFrenchInterfaceText199,
	sFrenchInterfaceText200,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sFrenchInterfaceText203,
	sFrenchInterfaceText204,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sFrenchInterfaceText108,       sFrenchInterfaceText109,       sFrenchInterfaceText110,       sFrenchInterfaceText111,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sFrenchInterfaceText113,       sFrenchInterfaceText114,
	sFrenchInterfaceText220,       sFrenchInterfaceText221,       sFrenchInterfaceText222,       sFrenchInterfaceText223,
	sFrenchInterfaceText105,       sFrenchInterfaceText106,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sFrenchInterfaceText228,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sFrenchInterfaceText233,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sFrenchInterfaceText238,       gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sFrenchInterfaceText243,
	sFrenchInterfaceText244,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sFrenchInterfaceText113,       sFrenchInterfaceText114,       sFrenchInterfaceText220,       sFrenchInterfaceText221,
	sFrenchInterfaceText222,       sFrenchInterfaceText113,       sFrenchInterfaceText114,       sFrenchInterfaceText220,
	sFrenchInterfaceText221,       sFrenchInterfaceText222,       sEnglishInterfaceText258,      sFrenchInterfaceText259,
	sFrenchInterfaceText064,       sFrenchInterfaceText065,       sFrenchInterfaceText262,       sFrenchInterfaceText263,
	sFrenchInterfaceText264,       sFrenchInterfaceText265,       sFrenchInterfaceText266,       sFrenchInterfaceText267,
	sFrenchInterfaceText268,       gEmptyResourceDirectorySuffix, sFrenchInterfaceText270,       sFrenchInterfaceText271,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sFrenchInterfaceText274,       gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sFrenchInterfaceText277,
};
#else
const char* gFrenchInterfaceText[] = {
#include "game/interface_text_us_fallback.inc"
};
#endif
}
