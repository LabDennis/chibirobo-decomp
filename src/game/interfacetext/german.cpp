#include "Version.h"
#if defined(VERSION_US)
#include "game/interface_text_us_fallback.h"
#endif

extern "C" {
#if defined(VERSION_PAL)
extern const char gEmptyResourceDirectorySuffix[];
extern const char sEnglishInterfaceText014[];
extern const char sEnglishInterfaceText015[];
extern const char sEnglishInterfaceText016[];
extern const char sEnglishInterfaceText019[];
extern const char sEnglishInterfaceText020[];
extern const char sEnglishInterfaceText021[];
extern const char sEnglishInterfaceText023[];
extern const char sEnglishInterfaceText024[];
extern const char sEnglishInterfaceText025[];
extern const char sEnglishInterfaceText026[];
extern const char sEnglishInterfaceText033[];
extern const char sEnglishInterfaceText034[];
extern const char sEnglishInterfaceText069[];
extern const char sEnglishInterfaceText258[];

extern const char sGermanInterfaceText000[] = "Soll die Darstellung in den Progressive-";
extern const char sGermanInterfaceText001[] = "Modus (Vollbild) umgeschaltet werden?";
extern const char sGermanInterfaceText002[] = "%s%02XJa%s  %s%02XNein%s";
extern const char sGermanInterfaceText003[] = "Der Progressive (Vollbild)-Modus";
extern const char sGermanInterfaceText004[] = "wurde eingeschaltet.";
extern const char sGermanInterfaceText007[] = "wurde ausgeschaltet.";
extern const char sGermanInterfaceText012[] = "Item lagern";
extern const char sGermanInterfaceText013[] = "Anzug ausziehen";
extern const char sGermanInterfaceText017[] = "An";
extern const char sGermanInterfaceText018[] = "Aus";
extern const char sGermanInterfaceText022[] = "Zur\374ck";
extern const char sGermanInterfaceText027[] = "Plaketten";
extern const char sGermanInterfaceText029[] = "Aktion";
extern const char sGermanInterfaceText030[] = "Abbruch";
extern const char sGermanInterfaceText031[] = "Kamera rotieren";
extern const char sGermanInterfaceText032[] = "Ausr\374stungsbildschirm";
extern const char sGermanInterfaceText035[] = "Kamera zentrieren";
extern const char sGermanInterfaceText036[] = "Karte";
extern const char sGermanInterfaceText038[] = "Auswahl";
extern const char sGermanInterfaceText039[] = "Chibi-Steuerung/Auswahl";
extern const char sGermanInterfaceText040[] = "Chibi-Rangliste";
extern const char sGermanInterfaceText041[] = "Chibi-T\374ren komplettiert";
extern const char sGermanInterfaceText042[] = "Happy-Punkte";
extern const char sGermanInterfaceText043[] = "Chibs";
extern const char sGermanInterfaceText044[] = "Schrott";
extern const char sGermanInterfaceText046[] = "Chibi-PC-Men\374";
extern const char sGermanInterfaceText047[] = "Online-Shop";
extern const char sGermanInterfaceText057[] = "Ende";
extern const char sGermanInterfaceText060[] = "Kaufen";
extern const char sGermanInterfaceText062[] = "Gekauft";
extern const char sGermanInterfaceText063[] = "Kaufen?";
extern const char sGermanInterfaceText064[] = "Ja";
extern const char sGermanInterfaceText065[] = "Nein";
extern const char sGermanInterfaceText070[] = "Bauen";
extern const char sGermanInterfaceText071[] = "Gewechselt";
extern const char sGermanInterfaceText072[] = "Wechseln?";
extern const char sGermanInterfaceText073[] = "Schrott-Tausch";
extern const char sGermanInterfaceText074[] = "Tauschrate";
extern const char sGermanInterfaceText075[] = "Wie viel tauschen?";
extern const char sGermanInterfaceText076[] = "Du hast nicht genug Schrott.";
extern const char sGermanInterfaceText077[] = "Ist das";
extern const char sGermanInterfaceText078[] = "O.K.?";
extern const char sGermanInterfaceText080[] = "Bauen?";
extern const char sGermanInterfaceText081[] = "O.K., bitte warten.";
extern const char sGermanInterfaceText082[] = "Wechseln";
extern const char sGermanInterfaceText084[] = "Der Disc-Deckel ist ge\366ffnet.\012Bitte den Disc-Deckel schlie\337en,\012um mit dem Spiel "
                                              "fortzufahren.";
extern const char sGermanInterfaceText085[] = "Bitte lege die Chibi-Robo-Disc ein.";
extern const char sGermanInterfaceText087[] = "Diese Game Disc kann nicht gelesen werden.\012Bitte lies die Bedienungsanleitung "
                                              "des\012Nintendo GameCube, um weitere\012Informationen zu erhalten.";
extern const char sGermanInterfaceText088[] = "Ladevorgang";
extern const char sGermanInterfaceText090[] = "Punktestand";
extern const char sGermanInterfaceText091[] = "Zeit";
extern const char sGermanInterfaceText092[] = "Kurs";
extern const char sGermanInterfaceText093[] = "Gegner";
extern const char sGermanInterfaceText094[] = "Neues Spiel";
extern const char sGermanInterfaceText095[] = "Spiel laden";
extern const char sGermanInterfaceText096[] = "Optionen";
extern const char sGermanInterfaceText097[] = "Einstellungen";
extern const char sGermanInterfaceText098[] = "Speichern";
extern const char sGermanInterfaceText099[] = "Leer";
extern const char sGermanInterfaceText103[] = "Es befindet sich keine Memory Card";
extern const char sGermanInterfaceText104[] = "in Steckplatz A. Bitte stecke eine";
extern const char sGermanInterfaceText105[] = "Memory Card ein oder dr\374cke den";
extern const char sGermanInterfaceText106[] = "A-Knopf, um ohne Speichern fortzufahren.";
extern const char sGermanInterfaceText108[] = "Das Objekt in Steckplatz A wird nicht als";
extern const char sGermanInterfaceText109[] = "Memory Card anerkannt.";
extern const char sGermanInterfaceText110[] = "Bitte stecke eine Memory Card ein oder";
extern const char sGermanInterfaceText111[] = "dr\374cke den A-Knopf, um ohne Speichern";
extern const char sGermanInterfaceText112[] = "fortzufahren.";
extern const char sGermanInterfaceText113[] = "Die Memory Card in Steckplatz A kann";
extern const char sGermanInterfaceText114[] = "nicht verwendet werden.";
extern const char sGermanInterfaceText118[] = "Die Memory Card in Steckplatz A ist";
extern const char sGermanInterfaceText119[] = "fehlerhaft und muss formatiert werden.";
extern const char sGermanInterfaceText120[] = "Soll sie jetzt formatiert werden?";
extern const char sGermanInterfaceText123[] = "Durch das Formatieren der Memory Card";
extern const char sGermanInterfaceText124[] = "in Steckplatz A werden alle zuvor";
extern const char sGermanInterfaceText125[] = "gespeicherten Daten gel\366scht. Soll sie";
extern const char sGermanInterfaceText126[] = "trotzdem formatiert werden?";
extern const char sGermanInterfaceText128[] = "Formatierung l\344uft... Bitte entferne nicht die";
extern const char sGermanInterfaceText129[] = "Memory Card in Steckplatz A und bet\344tige";
extern const char sGermanInterfaceText130[] = "nicht den POWER-Schalter.";
extern const char sGermanInterfaceText133[] = "Die Memory Card in Steckplatz A konnte";
extern const char sGermanInterfaceText134[] = "nicht formatiert werden.";
extern const char sGermanInterfaceText138[] = "Die Memory Card in Steckplatz A wurde";
extern const char sGermanInterfaceText139[] = "formatiert.";
extern const char sGermanInterfaceText143[] = "Ohne Speichern fortfahren?";
extern const char sGermanInterfaceText148[] = "Bitte w\344hle eine Datei, um einen neuen";
extern const char sGermanInterfaceText149[] = "Speicherplatz zu erstellen.";
extern const char sGermanInterfaceText153[] = "Nicht gen\374gend";
extern const char sGermanInterfaceText154[] = "Platz vorhanden.";
extern const char sGermanInterfaceText158[] = "Diese Daten";
extern const char sGermanInterfaceText159[] = "sind fehlerhaft.";
extern const char sGermanInterfaceText163[] = "Es befindet sich nicht gen\374gend freier Platz";
extern const char sGermanInterfaceText164[] = "auf der Memory Card in Steckplatz A.";
extern const char sGermanInterfaceText165[] = "Chibi-Robo ben\366tigt zum Speichern";
extern const char sGermanInterfaceText166[] = "1 Datei und 6 Bl\366cke.";
extern const char sGermanInterfaceText168[] = "Bestehende Datei \374berschreiben?";
extern const char sGermanInterfaceText173[] = "Bitte rufe den Memory Card-Bildschirm auf,";
extern const char sGermanInterfaceText174[] = "um die Inhalte deiner Memory Card zu";
extern const char sGermanInterfaceText175[] = "verwalten.";
extern const char sGermanInterfaceText178[] = "Speichervorgang...";
extern const char sGermanInterfaceText179[] = "Bitte entferne nicht die Memory Card in";
extern const char sGermanInterfaceText180[] = "Steckplatz A und bet\344tige nicht den";
extern const char sGermanInterfaceText181[] = "POWER-Schalter.";
extern const char sGermanInterfaceText183[] = "W\344hrend des Speicherns ist ein Fehler";
extern const char sGermanInterfaceText184[] = "aufgetreten.";
extern const char sGermanInterfaceText188[] = "W\344hle die Datei, die geladen werden soll.";
extern const char sGermanInterfaceText193[] = "Ladevorgang...";
extern const char sGermanInterfaceText198[] = "Die Chibi-Robo-Datei auf der Memory Card";
extern const char sGermanInterfaceText199[] = "in Steckplatz A ist fehlerhaft und muss";
extern const char sGermanInterfaceText200[] = "gel\366scht werden.";
extern const char sGermanInterfaceText203[] = "Soll ein neuer Speicherstand";
extern const char sGermanInterfaceText204[] = "erstellt werden?";
extern const char sGermanInterfaceText219[] = "besch\344digt und kann nicht verwendet";
extern const char sGermanInterfaceText220[] = "werden. Bitte stecke eine Memory Card ein";
extern const char sGermanInterfaceText221[] = "oder dr\374cke den A-Knopf, um ohne";
extern const char sGermanInterfaceText222[] = "Speichern fortzufahren.";
extern const char sGermanInterfaceText228[] = "Speichern erfolgreich.";
extern const char sGermanInterfaceText233[] = "W\344hle eine Datei.";
extern const char sGermanInterfaceText238[] = "Der Speicherstand konnte nicht geladen";
extern const char sGermanInterfaceText239[] = "werden.";
extern const char sGermanInterfaceText243[] = "Die Memory Card in Steckplatz A steht nicht";
extern const char sGermanInterfaceText244[] = "mehr zur Verf\374gung.";
extern const char sGermanInterfaceText254[] = "nicht verwendet werden. Bitte stecke eine";
extern const char sGermanInterfaceText255[] = "andere Memory Card ein oder dr\374cke den";
extern const char sGermanInterfaceText259[] = "Spielzeit:";
extern const char sGermanInterfaceText262[] = "Die Chibi-Robo-Demo ist nun zu Ende.\012Bitte dr\374cke den A-Knopf, um das Spiel\012"
                                              "fortzusetzen.";
extern const char sGermanInterfaceText263[] = "Dieses Spiel unterst\374tzt";
extern const char sGermanInterfaceText264[] = "den 60Hz-Modus.";
extern const char sGermanInterfaceText265[] = "Bitte Modus ausw\344hlen.";
extern const char sGermanInterfaceText266[] = "Sollte das TV-Ger\344t im";
extern const char sGermanInterfaceText267[] = "60Hz-Modus kein einwandfreies";
extern const char sGermanInterfaceText268[] = "Bild darstellen, bitte den";
extern const char sGermanInterfaceText269[] = "50Hz-Modus ausw\344hlen.";
extern const char sGermanInterfaceText270[] = "%s%02X 60Hz-Modus %s  %s%02X 50Hz-Modus %s";
extern const char sGermanInterfaceText271[] = "Die Bildschirmdarstellung erfolgt im";
extern const char sGermanInterfaceText272[] = "60Hz-Modus.";
extern const char sGermanInterfaceText275[] = "50Hz-Modus.";
extern const char sGermanInterfaceText277[] = "NEU";

const char* gGermanInterfaceText[] = {
	sGermanInterfaceText000,       sGermanInterfaceText001,       sGermanInterfaceText002,       sGermanInterfaceText003,
	sGermanInterfaceText004,       gEmptyResourceDirectorySuffix, sGermanInterfaceText003,       sGermanInterfaceText007,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sGermanInterfaceText012,       sGermanInterfaceText013,       sEnglishInterfaceText014,      sEnglishInterfaceText015,
	sEnglishInterfaceText016,      sGermanInterfaceText017,       sGermanInterfaceText018,       sEnglishInterfaceText019,
	sEnglishInterfaceText020,      sEnglishInterfaceText021,      sGermanInterfaceText022,       sEnglishInterfaceText023,
	sEnglishInterfaceText024,      sEnglishInterfaceText025,      sEnglishInterfaceText026,      sGermanInterfaceText027,
	gEmptyResourceDirectorySuffix, sGermanInterfaceText029,       sGermanInterfaceText030,       sGermanInterfaceText031,
	sGermanInterfaceText032,       sEnglishInterfaceText033,      sEnglishInterfaceText034,      sGermanInterfaceText035,
	sGermanInterfaceText036,       sEnglishInterfaceText023,      sGermanInterfaceText038,       sGermanInterfaceText039,
	sGermanInterfaceText040,       sGermanInterfaceText041,       sGermanInterfaceText042,       sGermanInterfaceText043,
	sGermanInterfaceText044,       gEmptyResourceDirectorySuffix, sGermanInterfaceText046,       sGermanInterfaceText047,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sGermanInterfaceText057,       sGermanInterfaceText022,       gEmptyResourceDirectorySuffix,
	sGermanInterfaceText060,       sGermanInterfaceText038,       sGermanInterfaceText062,       sGermanInterfaceText063,
	sGermanInterfaceText064,       sGermanInterfaceText065,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sEnglishInterfaceText069,      sGermanInterfaceText070,       sGermanInterfaceText071,
	sGermanInterfaceText072,       sGermanInterfaceText073,       sGermanInterfaceText074,       sGermanInterfaceText075,
	sGermanInterfaceText076,       sGermanInterfaceText077,       sGermanInterfaceText078,       gEmptyResourceDirectorySuffix,
	sGermanInterfaceText080,       sGermanInterfaceText081,       sGermanInterfaceText082,       gEmptyResourceDirectorySuffix,
	sGermanInterfaceText084,       sGermanInterfaceText085,       sGermanInterfaceText085,       sGermanInterfaceText087,
	sGermanInterfaceText088,       sGermanInterfaceText088,       sGermanInterfaceText090,       sGermanInterfaceText091,
	sGermanInterfaceText092,       sGermanInterfaceText093,       sGermanInterfaceText094,       sGermanInterfaceText095,
	sGermanInterfaceText096,       sGermanInterfaceText097,       sGermanInterfaceText098,       sGermanInterfaceText099,
	gEmptyResourceDirectorySuffix, sGermanInterfaceText064,       sGermanInterfaceText065,       sGermanInterfaceText103,
	sGermanInterfaceText104,       sGermanInterfaceText105,       sGermanInterfaceText106,       gEmptyResourceDirectorySuffix,
	sGermanInterfaceText108,       sGermanInterfaceText109,       sGermanInterfaceText110,       sGermanInterfaceText111,
	sGermanInterfaceText112,       sGermanInterfaceText113,       sGermanInterfaceText114,       gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sGermanInterfaceText118,       sGermanInterfaceText119,
	sGermanInterfaceText120,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sGermanInterfaceText123,
	sGermanInterfaceText124,       sGermanInterfaceText125,       sGermanInterfaceText126,       gEmptyResourceDirectorySuffix,
	sGermanInterfaceText128,       sGermanInterfaceText129,       sGermanInterfaceText130,       gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sGermanInterfaceText133,       sGermanInterfaceText134,       gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sGermanInterfaceText138,       sGermanInterfaceText139,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sGermanInterfaceText143,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sGermanInterfaceText148,       sGermanInterfaceText149,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sGermanInterfaceText153,       sGermanInterfaceText154,       gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sGermanInterfaceText158,       sGermanInterfaceText159,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sGermanInterfaceText163,
	sGermanInterfaceText164,       sGermanInterfaceText165,       sGermanInterfaceText166,       gEmptyResourceDirectorySuffix,
	sGermanInterfaceText168,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sGermanInterfaceText173,       sGermanInterfaceText174,       sGermanInterfaceText175,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sGermanInterfaceText178,       sGermanInterfaceText179,
	sGermanInterfaceText180,       sGermanInterfaceText181,       gEmptyResourceDirectorySuffix, sGermanInterfaceText183,
	sGermanInterfaceText184,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sGermanInterfaceText188,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sGermanInterfaceText193,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sGermanInterfaceText198,       sGermanInterfaceText199,
	sGermanInterfaceText200,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sGermanInterfaceText203,
	sGermanInterfaceText204,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sGermanInterfaceText108,       sGermanInterfaceText109,       sGermanInterfaceText110,       sGermanInterfaceText111,
	sGermanInterfaceText112,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sGermanInterfaceText118,       sGermanInterfaceText219,
	sGermanInterfaceText220,       sGermanInterfaceText221,       sGermanInterfaceText222,       sGermanInterfaceText110,
	sGermanInterfaceText111,       sGermanInterfaceText112,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sGermanInterfaceText228,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sGermanInterfaceText233,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sGermanInterfaceText238,       sGermanInterfaceText239,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sGermanInterfaceText243,
	sGermanInterfaceText244,       gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sGermanInterfaceText118,       sGermanInterfaceText219,       sGermanInterfaceText220,       sGermanInterfaceText221,
	sGermanInterfaceText222,       sGermanInterfaceText113,       sGermanInterfaceText254,       sGermanInterfaceText255,
	sGermanInterfaceText106,       gEmptyResourceDirectorySuffix, sEnglishInterfaceText258,      sGermanInterfaceText259,
	sGermanInterfaceText064,       sGermanInterfaceText065,       sGermanInterfaceText262,       sGermanInterfaceText263,
	sGermanInterfaceText264,       sGermanInterfaceText265,       sGermanInterfaceText266,       sGermanInterfaceText267,
	sGermanInterfaceText268,       sGermanInterfaceText269,       sGermanInterfaceText270,       sGermanInterfaceText271,
	sGermanInterfaceText272,       gEmptyResourceDirectorySuffix, sGermanInterfaceText271,       sGermanInterfaceText275,
	gEmptyResourceDirectorySuffix, sGermanInterfaceText277,
};
#else
extern const char sGermanInterfaceText002[] = "%s%02XYES%s　　%s%02XNO%s";
extern const char sGermanInterfaceText003[] = "Screen has";
extern const char sGermanInterfaceText004[] = "been set to";
extern const char sGermanInterfaceText005[] = "Progressive Mode";
extern const char sGermanInterfaceText007[] = "has been";
extern const char sGermanInterfaceText008[] = "turned off";
extern const char sGermanInterfaceText012[] = "(store item)";
extern const char sGermanInterfaceText013[] = "(remove suit)";
extern const char sGermanInterfaceText016[] = "Vibration";
extern const char sGermanInterfaceText021[] = "Monaural";
extern const char sGermanInterfaceText023[] = "Menu";
extern const char sGermanInterfaceText024[] = "Item";
extern const char sGermanInterfaceText027[] = "Happy Stickers";
extern const char sGermanInterfaceText031[] = "Camera control";
extern const char sGermanInterfaceText032[] = "Item menu";
extern const char sGermanInterfaceText033[] = "Chibi-Eye";
extern const char sGermanInterfaceText034[] = "Z action";
extern const char sGermanInterfaceText035[] = "Behind camera";
extern const char sGermanInterfaceText038[] = "Menu select";
extern const char sGermanInterfaceText039[] = "Control of ChibiRobo/Menu select";
extern const char sGermanInterfaceText040[] = "ChibiRobo Ranking";
extern const char sGermanInterfaceText041[] = "Chibi-Doors completed";
extern const char sGermanInterfaceText042[] = "Happy points";
extern const char sGermanInterfaceText043[] = "Mane";
extern const char sGermanInterfaceText044[] = "Junks";
extern const char sGermanInterfaceText045[] = "▼";
extern const char sGermanInterfaceText046[] = "ChibiCom Menu";
extern const char sGermanInterfaceText069[] = "Junk Machine";
extern const char sGermanInterfaceText073[] = "Junk trade";
extern const char sGermanInterfaceText074[] = "Current exchange rate";
extern const char sGermanInterfaceText075[] = "Exchange how much?";
extern const char sGermanInterfaceText076[] = "There is no enough junks";
extern const char sGermanInterfaceText081[] = "Stamby ok";
extern const char sGermanInterfaceText084[] = "The Disc Cover is open. \012If you want to continue the game, please close the Disc"
                                              " Cover.";
extern const char sGermanInterfaceText085[] = "Please insert the ChibiRobo! Game Disc.";
extern const char sGermanInterfaceText087[] = "The Game Disc could not be read.\012Please read the \012Nintendo GameCube Instruction\012"
                                              "Booklet for more information.";
extern const char sGermanInterfaceText088[] = "Reading";
extern const char sGermanInterfaceText090[] = "SCORE";
extern const char sGermanInterfaceText091[] = "TIME";
extern const char sGermanInterfaceText092[] = "COURCE";
extern const char sGermanInterfaceText093[] = "ENEMY";
extern const char sGermanInterfaceText096[] = "Option";
extern const char sGermanInterfaceText100[] = "Yes No";
extern const char sGermanInterfaceText101[] = "No Memory Card";
extern const char sGermanInterfaceText102[] = "found in Slot A.";
extern const char sGermanInterfaceText103[] = "Please check.";
extern const char sGermanInterfaceText106[] = "Wrong device in Slot A. Please insert a Memory Card.";
extern const char sGermanInterfaceText112[] = "damaged and can not be used.";
extern const char sGermanInterfaceText116[] = "The Memory Card in Slot A is corrupted";
extern const char sGermanInterfaceText117[] = "and needs to be formatted.";
extern const char sGermanInterfaceText118[] = "Do you want to format?";
extern const char sGermanInterfaceText121[] = "Formatting the Memory Card will erase";
extern const char sGermanInterfaceText122[] = "all previously saved data. Continue?";
extern const char sGermanInterfaceText126[] = "Saving to the Memory Card.";
extern const char sGermanInterfaceText127[] = "Do not touch the Memory";
extern const char sGermanInterfaceText128[] = "Card or the POWER Button.";
extern const char sGermanInterfaceText131[] = "An error may occurred during formatting";
extern const char sGermanInterfaceText136[] = "The Memory Card has been formatted";
extern const char sGermanInterfaceText141[] = "Continue without";
extern const char sGermanInterfaceText142[] = "saving?";
extern const char sGermanInterfaceText146[] = "Create a new game data.";
extern const char sGermanInterfaceText147[] = "Please select a file.";
extern const char sGermanInterfaceText151[] = "There is not enough free space";
extern const char sGermanInterfaceText156[] = "The save data";
extern const char sGermanInterfaceText157[] = "is broken";
extern const char sGermanInterfaceText162[] = "on the Memory Card in Slot A.";
extern const char sGermanInterfaceText163[] = "ChibiRobo! requires 1 file and";
extern const char sGermanInterfaceText172[] = "Card, use the Memory Card Screen.";
extern const char sGermanInterfaceText181[] = "An error may occurred during saving";
extern const char sGermanInterfaceText186[] = "Select the file to load";
extern const char sGermanInterfaceText196[] = "The ChibiRobo! file on Memory Card in Slot A is corrupted. This file needs to be"
                                              " deleted.";
extern const char sGermanInterfaceText201[] = "Restart with a";
extern const char sGermanInterfaceText202[] = "new save data?";
extern const char sGermanInterfaceText206[] = "Wrong device";
extern const char sGermanInterfaceText207[] = "in Slot A.";
extern const char sGermanInterfaceText208[] = "Please insert a";
extern const char sGermanInterfaceText209[] = "Memory Card.";
extern const char sGermanInterfaceText221[] = "Please insert a Memory Card";
extern const char sGermanInterfaceText226[] = "Save completed";
extern const char sGermanInterfaceText231[] = "Select a file";
extern const char sGermanInterfaceText236[] = "The data could not be loaded";
extern const char sGermanInterfaceText241[] = "The Memory Card";

const char* gGermanInterfaceText[] = {
#include "game/interface_text_us_fallback.inc"
};
#endif
}
