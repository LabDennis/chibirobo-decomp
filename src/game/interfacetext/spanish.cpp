#include "Version.h"
#if defined(VERSION_US)
#include "game/interface_text_us_fallback.h"
#endif

extern "C" {
#if defined(VERSION_PAL)
extern const char gEmptyResourceDirectorySuffix[];
extern const char sEnglishInterfaceText015[];
extern const char sEnglishInterfaceText065[];
extern const char sEnglishInterfaceText021[];
extern const char sEnglishInterfaceText034[];
extern const char sEnglishInterfaceText100[];
extern const char sEnglishInterfaceText258[];

extern const char sSpanishInterfaceText000[] = "\277Deseas activar el modo";
extern const char sSpanishInterfaceText001[] = "de escaneo progresivo?";
extern const char sSpanishInterfaceText002[] = "%s%02XS\355%s  %s%02XNo%s";
extern const char sSpanishInterfaceText003[] = "Modo de escaneo";
extern const char sSpanishInterfaceText004[] = "progresivo activado.";
extern const char sSpanishInterfaceText007[] = "progresivo desactivado.";
extern const char sSpanishInterfaceText012[] = "Guardar objeto";
extern const char sSpanishInterfaceText013[] = "Quitar disfraz";
extern const char sSpanishInterfaceText014[] = "(?)";
extern const char sSpanishInterfaceText016[] = "Vibraci\363n";
extern const char sSpanishInterfaceText017[] = "S\355";
extern const char sSpanishInterfaceText019[] = "Sonido";
extern const char sSpanishInterfaceText020[] = "Est\351reo";
extern const char sSpanishInterfaceText022[] = "Atr\341s";
extern const char sSpanishInterfaceText023[] = "Pausa";
extern const char sSpanishInterfaceText024[] = "Objetos";
extern const char sSpanishInterfaceText025[] = "Mando";
extern const char sSpanishInterfaceText026[] = "Estado";
extern const char sSpanishInterfaceText027[] = "Pegatinas";
extern const char sSpanishInterfaceText029[] = "Acci\363n";
extern const char sSpanishInterfaceText030[] = "Cancelar";
extern const char sSpanishInterfaceText031[] = "Girar c\341mara";
extern const char sSpanishInterfaceText032[] = "Men\372 de objetos";
extern const char sSpanishInterfaceText033[] = "Chibi-visi\363n";
extern const char sSpanishInterfaceText035[] = "Centrar c\341mara";
extern const char sSpanishInterfaceText036[] = "Mapa";
extern const char sSpanishInterfaceText038[] = "Seleccionar";
extern const char sSpanishInterfaceText039[] = "Control de Chibi-Robo / Seleccionar";
extern const char sSpanishInterfaceText040[] = "Chibi-clasificaci\363n";
extern const char sSpanishInterfaceText041[] = "Chibi-puertas completadas";
extern const char sSpanishInterfaceText042[] = "Puntos felices";
extern const char sSpanishInterfaceText043[] = "Mane";
extern const char sSpanishInterfaceText044[] = "Chatarra";
extern const char sSpanishInterfaceText046[] = "Men\372 del Chibi-PC";
extern const char sSpanishInterfaceText047[] = "Tienda online";
extern const char sSpanishInterfaceText057[] = "Finalizar";
extern const char sSpanishInterfaceText060[] = "Comprar";
extern const char sSpanishInterfaceText062[] = "Comprado";
extern const char sSpanishInterfaceText063[] = "\277Comprar?";
extern const char sSpanishInterfaceText069[] = "Reciclotr\363n";
extern const char sSpanishInterfaceText070[] = "Construir";
extern const char sSpanishInterfaceText071[] = "Cambiado";
extern const char sSpanishInterfaceText072[] = "\277Cambiar?";
extern const char sSpanishInterfaceText073[] = "Cambio de chatarra";
extern const char sSpanishInterfaceText074[] = "Tipo de cambio";
extern const char sSpanishInterfaceText075[] = "\277Cu\341nto?";
extern const char sSpanishInterfaceText076[] = "No tienes chatarra suficiente.";
extern const char sSpanishInterfaceText077[] = "\277Seguro?";
extern const char sSpanishInterfaceText080[] = "\277Construir?";
extern const char sSpanishInterfaceText081[] = "Hecho";
extern const char sSpanishInterfaceText082[] = "Cambiar";
extern const char sSpanishInterfaceText084[] = "La tapa est\341 abierta. \012Si quieres seguir jugando,\012debes cerrar la tapa.";
extern const char sSpanishInterfaceText085[] = "Coloca el disco de Chibi-Robo.";
extern const char sSpanishInterfaceText087[] = "No se puede leer el disco. Consulta\012el manual de instrucciones de Nintendo\012"
                                               "GameCube para obtener m\341s informaci\363n.";
extern const char sSpanishInterfaceText088[] = "Cargando...";
extern const char sSpanishInterfaceText090[] = "Puntuaci\363n";
extern const char sSpanishInterfaceText091[] = "Tiempo";
extern const char sSpanishInterfaceText092[] = "Recorrido";
extern const char sSpanishInterfaceText093[] = "Enemigo";
extern const char sSpanishInterfaceText094[] = "Nueva partida";
extern const char sSpanishInterfaceText095[] = "Cargar partida";
extern const char sSpanishInterfaceText096[] = "Opciones";
extern const char sSpanishInterfaceText097[] = "Configuraci\363n";
extern const char sSpanishInterfaceText098[] = "Guardar";
extern const char sSpanishInterfaceText099[] = "Vac\355o";
extern const char sSpanishInterfaceText103[] = "No hay ninguna Memory Card";
extern const char sSpanishInterfaceText104[] = "en la Ranura A. Inserta una";
extern const char sSpanishInterfaceText105[] = "o pulsa el Bot\363n A para";
extern const char sSpanishInterfaceText106[] = "continuar sin guardar.";
extern const char sSpanishInterfaceText108[] = "El objeto insertado en la Ranura A no es";
extern const char sSpanishInterfaceText109[] = "v\341lido. Inserta otra Memory Card ";
extern const char sSpanishInterfaceText110[] = "o pulsa el Bot\363n A ";
extern const char sSpanishInterfaceText111[] = "para continuar sin guardar.";
extern const char sSpanishInterfaceText113[] = "La Memory Card de la Ranura A";
extern const char sSpanishInterfaceText114[] = "no puede utilizarse.";
extern const char sSpanishInterfaceText119[] = "contiene datos da\361ados y hay";
extern const char sSpanishInterfaceText120[] = "que formatearla. \277Deseas formatearla?";
extern const char sSpanishInterfaceText123[] = "Si formateas la Memory Card";
extern const char sSpanishInterfaceText124[] = "se borrar\341n todos los datos.";
extern const char sSpanishInterfaceText125[] = "\277Deseas formatearla de todos modos?";
extern const char sSpanishInterfaceText128[] = "Formateando... No toques ni la";
extern const char sSpanishInterfaceText129[] = "Memory Card de la Ranura A";
extern const char sSpanishInterfaceText130[] = "ni el Bot\363n POWER. ";
extern const char sSpanishInterfaceText133[] = "No se puede formatear la";
extern const char sSpanishInterfaceText134[] = "Memory Card de la Ranura A.";
extern const char sSpanishInterfaceText138[] = "Se ha formateado la";
extern const char sSpanishInterfaceText143[] = "\277Continuar sin guardar?";
extern const char sSpanishInterfaceText148[] = "Para crear una nueva partida";
extern const char sSpanishInterfaceText149[] = "selecciona un archivo.";
extern const char sSpanishInterfaceText153[] = "No hay suficiente";
extern const char sSpanishInterfaceText154[] = "espacio libre.";
extern const char sSpanishInterfaceText158[] = "Los datos est\341n";
extern const char sSpanishInterfaceText159[] = "da\361ados.";
extern const char sSpanishInterfaceText163[] = "No hay suficiente espacio libre";
extern const char sSpanishInterfaceText164[] = "en la Memory Card de la Ranura A.";
extern const char sSpanishInterfaceText165[] = "Para guardar datos de Chibi-Robo,";
extern const char sSpanishInterfaceText166[] = "se requieren 1 archivo y 6 bloques.";
extern const char sSpanishInterfaceText168[] = "\277Reemplazar el archivo existente?";
extern const char sSpanishInterfaceText173[] = "Para reorganizar el contenido de la";
extern const char sSpanishInterfaceText174[] = "Memory Card, ve a la pantalla Memory Card.";
extern const char sSpanishInterfaceText178[] = "Guardando...";
extern const char sSpanishInterfaceText179[] = "No toques ni la Memory Card  ";
extern const char sSpanishInterfaceText180[] = "de la Ranura A ni el Bot\363n POWER.";
extern const char sSpanishInterfaceText183[] = "Ha ocurrido un error al guardar.";
extern const char sSpanishInterfaceText188[] = "Elige la partida que quieras cargar.";
extern const char sSpanishInterfaceText198[] = "El archivo de Chibi-Robo de la";
extern const char sSpanishInterfaceText200[] = "est\341 da\361ado y va a ser borrado.";
extern const char sSpanishInterfaceText203[] = "\277Crear un archivo";
extern const char sSpanishInterfaceText204[] = "de juego nuevo?";
extern const char sSpanishInterfaceText220[] = "Inserta otra Memory Card";
extern const char sSpanishInterfaceText221[] = "o pulsa el Bot\363n A";
extern const char sSpanishInterfaceText223[] = "Inserta otra Memory Card o pulsa el";
extern const char sSpanishInterfaceText224[] = "Bot\363n A para continuar sin guardar.";
extern const char sSpanishInterfaceText228[] = "Partida guardada.";
extern const char sSpanishInterfaceText233[] = "Elige un archivo.";
extern const char sSpanishInterfaceText238[] = "No se han podido cargar";
extern const char sSpanishInterfaceText239[] = "los datos guardados.";
extern const char sSpanishInterfaceText244[] = "ya no est\341 disponible.";
extern const char sSpanishInterfaceText249[] = "est\341 da\361ada y no puede utilizarse.";
extern const char sSpanishInterfaceText259[] = "Tiempo:";
extern const char sSpanishInterfaceText262[] = "Ha terminado la demo de Chibi-Robo.\012Pulsa el Bot\363n A para\012seguir jugando.";
extern const char sSpanishInterfaceText263[] = "Este juego es compatible";
extern const char sSpanishInterfaceText264[] = "con el modo 60 Hz.";
extern const char sSpanishInterfaceText265[] = "\277Quieres verlo en ese modo?";
extern const char sSpanishInterfaceText266[] = "Si tu televisor no reproduce ";
extern const char sSpanishInterfaceText267[] = "correctamente el modo 60 Hz,";
extern const char sSpanishInterfaceText268[] = "selecciona el modo 50 Hz.";
extern const char sSpanishInterfaceText270[] = "%s%02X Modo 60 Hz %s  %s%02X Modo 50 Hz %s";
extern const char sSpanishInterfaceText271[] = "La pantalla se ha configurado";
extern const char sSpanishInterfaceText272[] = "en modo 60 Hz.";
extern const char sSpanishInterfaceText275[] = "en modo 50 Hz.";
extern const char sSpanishInterfaceText277[] = "NUEVO";

const char* gSpanishInterfaceText[] = {
	sSpanishInterfaceText000,      sSpanishInterfaceText001,      sSpanishInterfaceText002,      sSpanishInterfaceText003,
	sSpanishInterfaceText004,      gEmptyResourceDirectorySuffix, sSpanishInterfaceText003,      sSpanishInterfaceText007,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sSpanishInterfaceText012,      sSpanishInterfaceText013,      sSpanishInterfaceText014,      sEnglishInterfaceText015,
	sSpanishInterfaceText016,      sSpanishInterfaceText017,      sEnglishInterfaceText065,      sSpanishInterfaceText019,
	sSpanishInterfaceText020,      sEnglishInterfaceText021,      sSpanishInterfaceText022,      sSpanishInterfaceText023,
	sSpanishInterfaceText024,      sSpanishInterfaceText025,      sSpanishInterfaceText026,      sSpanishInterfaceText027,
	gEmptyResourceDirectorySuffix, sSpanishInterfaceText029,      sSpanishInterfaceText030,      sSpanishInterfaceText031,
	sSpanishInterfaceText032,      sSpanishInterfaceText033,      sEnglishInterfaceText034,      sSpanishInterfaceText035,
	sSpanishInterfaceText036,      sSpanishInterfaceText023,      sSpanishInterfaceText038,      sSpanishInterfaceText039,
	sSpanishInterfaceText040,      sSpanishInterfaceText041,      sSpanishInterfaceText042,      sSpanishInterfaceText043,
	sSpanishInterfaceText044,      gEmptyResourceDirectorySuffix, sSpanishInterfaceText046,      sSpanishInterfaceText047,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sSpanishInterfaceText057,      sSpanishInterfaceText022,      gEmptyResourceDirectorySuffix,
	sSpanishInterfaceText060,      sSpanishInterfaceText038,      sSpanishInterfaceText062,      sSpanishInterfaceText063,
	sSpanishInterfaceText017,      sEnglishInterfaceText065,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sSpanishInterfaceText069,      sSpanishInterfaceText070,      sSpanishInterfaceText071,
	sSpanishInterfaceText072,      sSpanishInterfaceText073,      sSpanishInterfaceText074,      sSpanishInterfaceText075,
	sSpanishInterfaceText076,      sSpanishInterfaceText077,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sSpanishInterfaceText080,      sSpanishInterfaceText081,      sSpanishInterfaceText082,      gEmptyResourceDirectorySuffix,
	sSpanishInterfaceText084,      sSpanishInterfaceText085,      sSpanishInterfaceText085,      sSpanishInterfaceText087,
	sSpanishInterfaceText088,      sSpanishInterfaceText088,      sSpanishInterfaceText090,      sSpanishInterfaceText091,
	sSpanishInterfaceText092,      sSpanishInterfaceText093,      sSpanishInterfaceText094,      sSpanishInterfaceText095,
	sSpanishInterfaceText096,      sSpanishInterfaceText097,      sSpanishInterfaceText098,      sSpanishInterfaceText099,
	sEnglishInterfaceText100,      sSpanishInterfaceText017,      sEnglishInterfaceText065,      sSpanishInterfaceText103,
	sSpanishInterfaceText104,      sSpanishInterfaceText105,      sSpanishInterfaceText106,      gEmptyResourceDirectorySuffix,
	sSpanishInterfaceText108,      sSpanishInterfaceText109,      sSpanishInterfaceText110,      sSpanishInterfaceText111,
	gEmptyResourceDirectorySuffix, sSpanishInterfaceText113,      sSpanishInterfaceText114,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sSpanishInterfaceText113,      sSpanishInterfaceText119,
	sSpanishInterfaceText120,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sSpanishInterfaceText123,
	sSpanishInterfaceText124,      sSpanishInterfaceText125,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sSpanishInterfaceText128,      sSpanishInterfaceText129,      sSpanishInterfaceText130,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sSpanishInterfaceText133,      sSpanishInterfaceText134,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sSpanishInterfaceText138,      sSpanishInterfaceText134,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sSpanishInterfaceText143,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sSpanishInterfaceText148,      sSpanishInterfaceText149,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sSpanishInterfaceText153,      sSpanishInterfaceText154,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sSpanishInterfaceText158,      sSpanishInterfaceText159,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sSpanishInterfaceText163,
	sSpanishInterfaceText164,      sSpanishInterfaceText165,      sSpanishInterfaceText166,      gEmptyResourceDirectorySuffix,
	sSpanishInterfaceText168,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sSpanishInterfaceText173,      sSpanishInterfaceText174,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sSpanishInterfaceText178,      sSpanishInterfaceText179,
	sSpanishInterfaceText180,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sSpanishInterfaceText183,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sSpanishInterfaceText188,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sSpanishInterfaceText088,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sSpanishInterfaceText198,      sSpanishInterfaceText129,
	sSpanishInterfaceText200,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sSpanishInterfaceText203,
	sSpanishInterfaceText204,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sSpanishInterfaceText108,      sSpanishInterfaceText109,      sSpanishInterfaceText110,      sSpanishInterfaceText111,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sSpanishInterfaceText113,      sSpanishInterfaceText114,
	sSpanishInterfaceText220,      sSpanishInterfaceText221,      sSpanishInterfaceText111,      sSpanishInterfaceText223,
	sSpanishInterfaceText224,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sSpanishInterfaceText228,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sSpanishInterfaceText233,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sSpanishInterfaceText238,      sSpanishInterfaceText239,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sSpanishInterfaceText113,
	sSpanishInterfaceText244,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sSpanishInterfaceText113,      sSpanishInterfaceText249,      sSpanishInterfaceText223,      sSpanishInterfaceText224,
	gEmptyResourceDirectorySuffix, sSpanishInterfaceText113,      sSpanishInterfaceText114,      sSpanishInterfaceText223,
	sSpanishInterfaceText224,      gEmptyResourceDirectorySuffix, sEnglishInterfaceText258,      sSpanishInterfaceText259,
	sSpanishInterfaceText017,      sEnglishInterfaceText065,      sSpanishInterfaceText262,      sSpanishInterfaceText263,
	sSpanishInterfaceText264,      sSpanishInterfaceText265,      sSpanishInterfaceText266,      sSpanishInterfaceText267,
	sSpanishInterfaceText268,      gEmptyResourceDirectorySuffix, sSpanishInterfaceText270,      sSpanishInterfaceText271,
	sSpanishInterfaceText272,      gEmptyResourceDirectorySuffix, sSpanishInterfaceText271,      sSpanishInterfaceText275,
	gEmptyResourceDirectorySuffix, sSpanishInterfaceText277,
};
#else
const char* gSpanishInterfaceText[] = {
#include "game/interface_text_us_fallback.inc"
};
#endif
}
