#include "Version.h"
#include "game/pcdialogue.h"

extern "C" {
#if defined(VERSION_PAL)
extern const char sItalianPCWeather[]        = "E ora passiamo alle previsioni del tempo. Un potente tifone si "
                                               "sta avvicinando pericolosamente. Fate attenzione al forte vento e"
                                               " ai mari in burrasca. E ricordate: l'autunno \350 ormai vicino!"
                                               "           ";
extern const char sItalianPCSports[]         = "E ora passiamo allo sport. Spero che siate gi\340 tutti pronti per "
                                               "le prossime Chibi-Olimpiadi. La Citrusoft presenta, infatti, una "
                                               "fantastica competizione aperta a tutti i Chibi-Robo, in cui verr\340"
                                               " eletto il miglior aiutante per la famiglia. La pi\371 attesa \350 "
                                               "sicuramente la gara di lancio nel cestino. Quei Chibi-Robo sono "
                                               "davvero eccezionali! Le Chibi-Olimpiadi cominciano tra una "
                                               "settimana. Non perdetele!           ";
extern const char sItalianPCShopping[]       = "\310 arrivato il momento degli acquisti! Che cosa compriamo oggi di "
                                               "bello?           ";
extern const char sItalianPCBuildUtilibots[] = "Raccogli Ferraglia per costruire gli Elettrobox!           ";
extern const char sItalianPCExchangeScrap[]  = "Converti la tua Ferraglia in Monete per costruire un mondo "
                                               "migliore!           ";
extern const char sItalianPCWelcome[]        = "Salve e grazie per aver scelto il Chibi-PC. La Citrusoft Robotics"
                                               " \350 orgogliosa di presentare un modo del tutto nuovo di fare "
                                               "acquisti: questo computer ti permetter\340 di ricevere in pochi "
                                               "secondi a casa tua tutto ci\362 che acquisterai online. Un nuovo e "
                                               "sconfinato mondo degli acquisti ti attende. E se sei amico della "
                                               "natura, apprezzerai sicuramente il nostro Riciclotron. I tuoi "
                                               "rottami sono la nostra delizia! Che cosa vogliamo fare oggi? "
                                               "Seleziona una voce dal menu.           ";

const char* gItalianPCDialogue[] = { PC_DIALOGUE_ENTRIES(sItalianPCWeather, sItalianPCSports, sEnglishPCBlank, sItalianPCShopping,
	                                                     sItalianPCBuildUtilibots, sItalianPCExchangeScrap, sItalianPCWelcome) };
#else
const char* gItalianPCDialogue[] = { JAPANESE_PC_DIALOGUE_ENTRIES };
#endif
}
