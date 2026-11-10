#include "Version.h"
#include "game/pcdialogue.h"

extern "C" {
#if defined(VERSION_PAL)
extern const char sFrenchPCWeather[]        = "Et maintenant, passons \340 la m\351t\351o. Un terrible ouragan s'approche"
                                              " de nos c\364tes, alors faites attention aux bourrasques et aux "
                                              "\351normes vagues! Et n'oubliez pas, l'automne sera bient\364t l\340!"
                                              "           ";
extern const char sFrenchPCSports[]         = "Et maintenant, le sport! J'esp\350re que vous \352tes en forme parce "
                                              "que les Chibilympics approchent \340 grands pas! Eh oui! Citrusoft "
                                              "pr\351sente le plus grand d\351fi de robo-athl\351tisme jamais vu! Il est "
                                              "temps pour chaque Chibi-Robo de par le monde de se mesurer les "
                                              "uns aux autres et de voir qui est la meilleure f\351e du foyer! J'ai"
                                              " h\342te de voir ce que donnera la comp\351tition de lancer de d\351chets!"
                                              " Ces petits gars sont vraiment extraordinaires! La c\351r\351monie "
                                              "d'ouverture est dans une semaine! Ne la ratez pas! A vous les "
                                              "studios!           ";
extern const char sFrenchPCShopping[]       = "Shopping, shopping, shopping! Qu'allez-vous acheter aujourd'hui?"
                                              "           ";
extern const char sFrenchPCBuildUtilibots[] = "R\351cup\351rez de la Ferraille pour construire des Utilibots!"
                                              "           ";
extern const char sFrenchPCExchangeScrap[]  = "Si vous avez de la Ferraille en rabe, revendez-la au centre de "
                                              "recyclage! Gardez la plan\350te propre!           ";
extern const char sFrenchPCWelcome[]        = "Bonjour et bienvenue sur le Chibi-Serveur. Citrusoft Robotique "
                                              "est fi\350re de vous pr\351senter le shopping \340 la pointe du progr\350s: "
                                              "un serveur qui vous livre la marchandise dans les secondes qui "
                                              "suivent votre commande! Un monde de shopping en ligne vous "
                                              "attend, \340 vous le l\350che-\351cran! Et si vous avez un faible pour "
                                              "l'\351cologie, vous adorerez le Recyclotron: vos d\351chets sont notre "
                                              "plaisir! Qu'est-ce qui titille votre envie aujourd'hui? "
                                              "Choisissez dans le menu.           ";

const char* gFrenchPCDialogue[] = { PC_DIALOGUE_ENTRIES(sFrenchPCWeather, sFrenchPCSports, sEnglishPCBlank, sFrenchPCShopping,
	                                                    sFrenchPCBuildUtilibots, sFrenchPCExchangeScrap, sFrenchPCWelcome) };
#else
const char* gFrenchPCDialogue[] = { JAPANESE_PC_DIALOGUE_ENTRIES };
#endif
}
