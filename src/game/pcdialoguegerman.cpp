#include "Version.h"
#include "game/pcdialogue.h"

extern "C" {
#if defined(VERSION_PAL)
extern const char sGermanPCWeather[]        = "Und nun zum Wetter. Ein starker Taifun n\344hert sich der K\374ste. "
                                              "Achten Sie auf starke Winde und HOHE Wellen! Und vergessen Sie "
                                              "nicht:  Der Herbst steht schon vor der T\374r!           ";
extern const char sGermanPCSports[]         = "Und nun zum Sport! Ich hoffe, Sie haben sich ausgeruht, denn die "
                                              "Chibilympiade steht schon vor der T\374r! Richtig geh\366rt! Citrusoft "
                                              "pr\344sentiert die gr\366\337te Darbietung von Robo-Athletik, welche die "
                                              "Menschheit jemals gesehen hat! Chibi-Robos aus allen Teilen der "
                                              "Welt kommen zusammen, um zu testen, wer der beste Helfer von "
                                              "allen ist! Wissen Sie, worauf ich mich besonders freue? Auf den "
                                              "M\374lleimer-Weitwurf! Ich krieg einfach nicht genug von den kleinen"
                                              " Kerlchen! Der Wettbewerb beginnt in einer Woche! Verpassen Sie "
                                              "ihn nicht! Und jetzt zur\374ck zu Ronny!           ";
extern const char sGermanPCShopping[]       = "Einkaufszeit, Einkaufszeit! Was soll ich heute kaufen?           ";
extern const char sGermanPCBuildUtilibots[] = "Sammeln Sie Schrott, um Praktibots zu bauen!           ";
extern const char sGermanPCExchangeScrap[]  = "Tauschen Sie \374bersch\374ssigen Schrott gegen bare Chibs ein. Helfen "
                                              "Sie mit, eine bessere Welt zu gestalten!           ";
extern const char sGermanPCWelcome[]        = "Hallo und danke, dass Sie den Chibi-PC verwenden. Citrusoft "
                                              "Robotics ist stolz darauf, Ihnen diese einzigartige "
                                              "Einkaufserfahrung pr\344sentieren zu k\366nnen: Ein PC, der Ihnen jeden"
                                              " Artikel bereits Sekunden nach der Bestellung frei Haus liefert! "
                                              "Eine gro\337e Einkaufswelt erwartet Sie. Und wenn Sie \366kologisch "
                                              "denken, werden Sie den Recyclotron lieben, in dem Ihr Schrott "
                                              "unsere Freude ist! Was w\374rden Sie heute gerne tun? Bitte w\344hlen "
                                              "Sie aus dem folgenden Men\374.           ";

const char* gGermanPCDialogue[] = { PC_DIALOGUE_ENTRIES(sGermanPCWeather, sGermanPCSports, sEnglishPCBlank, sGermanPCShopping,
	                                                    sGermanPCBuildUtilibots, sGermanPCExchangeScrap, sGermanPCWelcome) };
#else
const char* gGermanPCDialogue[] = { JAPANESE_PC_DIALOGUE_ENTRIES };
#endif
}
