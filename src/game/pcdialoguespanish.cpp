#include "Version.h"
#include "game/pcdialogue.h"

extern "C" {
#if defined(VERSION_PAL)
extern const char sSpanishPCWeather[]        = "A continuaci\363n, la informaci\363n meteorol\363gica. Se acerca un "
                                               "potente hurac\341n; hay que tener cuidado con las r\341fagas de viento "
                                               "y el oleaje. Recuerda: \241el oto\361o ya est\341 aqu\355!           ";
extern const char sSpanishPCSports[]         = "Y ahora, la informaci\363n deportiva. Espero que hay\341is descansado "
                                               "bien, porque las Chibi-olimpiadas est\341n a punto de empezar. "
                                               "Citrusoft nos presentar\341 las mejores pruebas robo-atl\351ticas del "
                                               "mundo. Todos los Chibi-Robos se reunir\341n para demostrar qui\351n es "
                                               "el mejor. \241No os perd\341is los esperados lanzamientos de latas "
                                               "vac\355as! Yo ya tengo reservadas mis entradas en el palco. \241Las "
                                               "pruebas empiezan la semana que viene!           ";
extern const char sSpanishPCShopping[]       = "\241Hora de la compra, hora de la compra! \277Qu\351 comprar\351 hoy?"
                                               "           ";
extern const char sSpanishPCBuildUtilibots[] = "\241Recoge chatarra para construir servibots!           ";
extern const char sSpanishPCExchangeScrap[]  = "\241Cambia tu chatarra por mane y ayuda a construir un mundo mejor!"
                                               "           ";
extern const char sSpanishPCWelcome[]        = "\241Hola! Gracias por usar el Chibi-PC. Citrusoft Robotics se "
                                               "congratula de presentarte esta experiencia de compra \372nica: \241un "
                                               "ordenador personal que entrega las compras a los pocos segundos "
                                               "de haberlas realizado! Un nuevo mundo al alcance de tu mano. Y si"
                                               " t\372 eres de los que ama la naturaleza, el Reciclotr\363n se "
                                               "encargar\341 de devorar todos tus desperdicios. \277Qu\351 quieres hacer "
                                               "hoy? Selecciona el men\372 que m\341s te guste.           ";

const char* gSpanishPCDialogue[] = { PC_DIALOGUE_ENTRIES(sSpanishPCWeather, sSpanishPCSports, sEnglishPCBlank, sSpanishPCShopping,
	                                                     sSpanishPCBuildUtilibots, sSpanishPCExchangeScrap, sSpanishPCWelcome) };
#else
const char* gSpanishPCDialogue[] = { JAPANESE_PC_DIALOGUE_ENTRIES };
#endif
}
