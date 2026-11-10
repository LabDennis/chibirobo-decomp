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
extern const char sEnglishInterfaceText065[];
extern const char sEnglishInterfaceText020[];
extern const char sEnglishInterfaceText021[];
extern const char sSpanishInterfaceText023[];
extern const char sEnglishInterfaceText025[];
extern const char sEnglishInterfaceText258[];

extern const char sItalianInterfaceText012[] = "Riponi";
extern const char sItalianInterfaceText013[] = "Togli il costume";
extern const char sItalianInterfaceText017[] = "S\354";
extern const char sItalianInterfaceText019[] = "Suono";
extern const char sItalianInterfaceText022[] = "Indietro";
extern const char sItalianInterfaceText024[] = "Oggetti";
extern const char sItalianInterfaceText026[] = "Stato";
extern const char sItalianInterfaceText027[] = "Adesivi";
extern const char sItalianInterfaceText029[] = "Azione";
extern const char sItalianInterfaceText030[] = "Annulla";
extern const char sItalianInterfaceText031[] = "Regolazione visuale";
extern const char sItalianInterfaceText032[] = "Menu Strumenti";
extern const char sItalianInterfaceText033[] = "Chibi-Vista";
extern const char sItalianInterfaceText034[] = "Posa";
extern const char sItalianInterfaceText035[] = "Visuale centrale";
extern const char sItalianInterfaceText036[] = "Mappa";
extern const char sItalianInterfaceText038[] = "Selezione (menu)";
extern const char sItalianInterfaceText039[] = "Controllo Chibi-Robo/Selezione (menu)";
extern const char sItalianInterfaceText040[] = "Posizione Chibi-Robo";
extern const char sItalianInterfaceText041[] = "Chibi-Porte completate";
extern const char sItalianInterfaceText042[] = "Punti Cuore";
extern const char sItalianInterfaceText043[] = "Monete";
extern const char sItalianInterfaceText044[] = "Ferraglia";
extern const char sItalianInterfaceText046[] = "Menu Chibi-PC";
extern const char sItalianInterfaceText047[] = "Negozio online";
extern const char sItalianInterfaceText057[] = "Fine";
extern const char sItalianInterfaceText060[] = "Compra";
extern const char sItalianInterfaceText061[] = "Seleziona";
extern const char sItalianInterfaceText062[] = "Affare fatto!";
extern const char sItalianInterfaceText063[] = "Vuoi acquistare?";
extern const char sItalianInterfaceText069[] = "Riciclotron";
extern const char sItalianInterfaceText070[] = "Costruisci";
extern const char sItalianInterfaceText071[] = "Cambio effettuato";
extern const char sItalianInterfaceText072[] = "Vuoi fare il cambio?";
extern const char sItalianInterfaceText073[] = "Permuta Ferraglia";
extern const char sItalianInterfaceText074[] = "Valore di cambio";
extern const char sItalianInterfaceText075[] = "Quanta ne cambi?";
extern const char sItalianInterfaceText076[] = "Non hai abbastanza Ferraglia.";
extern const char sItalianInterfaceText078[] = "Va bene?";
extern const char sItalianInterfaceText080[] = "Lo costruisci?";
extern const char sItalianInterfaceText081[] = "Riciclotron attivato.";
extern const char sItalianInterfaceText082[] = "Permuta";
extern const char sItalianInterfaceText084[] = "Il coperchio del disco \350 aperto.\012Se vuoi proseguire nel gioco,\012chiudi il "
                                               "coperchio del disco.";
extern const char sItalianInterfaceText085[] = "Inserisci il disco di gioco Chibi-Robo.";
extern const char sItalianInterfaceText087[] = "Impossibile leggere il disco di gioco.\012Per ulteriori indicazioni consulta il "
                                               "manuale\012di istruzioni del Nintendo GameCube.";
extern const char sItalianInterfaceText088[] = "Caricamento in corso...";
extern const char sItalianInterfaceText090[] = "Punteggio";
extern const char sItalianInterfaceText091[] = "Tempo";
extern const char sItalianInterfaceText092[] = "Percorso";
extern const char sItalianInterfaceText093[] = "Nemici";
extern const char sItalianInterfaceText094[] = "Nuovo gioco";
extern const char sItalianInterfaceText095[] = "Carica gioco";
extern const char sItalianInterfaceText096[] = "Opzioni";
extern const char sItalianInterfaceText097[] = "Modifica le impostazioni.";
extern const char sItalianInterfaceText098[] = "Salva";
extern const char sItalianInterfaceText099[] = "Vuoto";
extern const char sItalianInterfaceText103[] = "Nello Slot A non \350 inserita";
extern const char sItalianInterfaceText104[] = "alcuna Scheda Memoria.";
extern const char sItalianInterfaceText105[] = "Inserisci una Scheda Memoria o";
extern const char sItalianInterfaceText106[] = "premi il pulsante A per proseguire senza salvare.";
extern const char sItalianInterfaceText108[] = "L'oggetto presente nello Slot A non \350 idoneo.";
extern const char sItalianInterfaceText109[] = "Inserisci una Scheda Memoria idonea";
extern const char sItalianInterfaceText110[] = "o premi il pulsante A per proseguire";
extern const char sItalianInterfaceText111[] = "senza salvare.";
extern const char sItalianInterfaceText113[] = "La Scheda Memoria nello Slot A \350 danneggiata";
extern const char sItalianInterfaceText114[] = "e non pu\362 essere utilizzata.";
extern const char sItalianInterfaceText118[] = "La Scheda Memoria nello Slot A contiene dati";
extern const char sItalianInterfaceText119[] = "danneggiati e deve essere formattata.";
extern const char sItalianInterfaceText120[] = "Vuoi formattare?";
extern const char sItalianInterfaceText123[] = "La formattazione canceller\340 tutti i dati salvati";
extern const char sItalianInterfaceText124[] = "nella Scheda Memoria. Vuoi procedere?";
extern const char sItalianInterfaceText128[] = "Formattazione in corso. ";
extern const char sItalianInterfaceText129[] = "Non toccare la Scheda Memoria";
extern const char sItalianInterfaceText130[] = "o il pulsante POWER.";
extern const char sItalianInterfaceText133[] = "Impossibile formattare la Scheda Memoria";
extern const char sItalianInterfaceText134[] = "presente nello Slot A.";
extern const char sItalianInterfaceText138[] = "La Scheda Memoria presente nello";
extern const char sItalianInterfaceText139[] = "Slot A \350 stata formattata.";
extern const char sItalianInterfaceText143[] = "Prosegui senza salvare?";
extern const char sItalianInterfaceText148[] = "Per creare dei nuovi dati di gioco";
extern const char sItalianInterfaceText149[] = "seleziona un file.";
extern const char sItalianInterfaceText153[] = "Lo spazio libero";
extern const char sItalianInterfaceText154[] = "\350 insufficiente.";
extern const char sItalianInterfaceText158[] = "I dati sono";
extern const char sItalianInterfaceText159[] = "danneggiati.";
extern const char sItalianInterfaceText163[] = "Lo spazio libero nella Scheda Memoria";
extern const char sItalianInterfaceText164[] = "nello Slot A \350 insufficiente.";
extern const char sItalianInterfaceText165[] = "Per salvare Chibi-Robo sono necessari";
extern const char sItalianInterfaceText166[] = "1 file e 6 blocchi.";
extern const char sItalianInterfaceText168[] = "Sovrascrivi i dati di gioco?";
extern const char sItalianInterfaceText173[] = "Per organizzare i dati della Scheda Memoria";
extern const char sItalianInterfaceText174[] = "utilizza lo schermo Memory Card (Scheda Memoria).";
extern const char sItalianInterfaceText178[] = "Salvataggio in corso.";
extern const char sItalianInterfaceText180[] = "nello Slot A o il pulsante POWER.";
extern const char sItalianInterfaceText183[] = "Si \350 verificato un errore durante il salvataggio.";
extern const char sItalianInterfaceText188[] = "Seleziona un file da caricare.";
extern const char sItalianInterfaceText198[] = "I dati di gioco di Chibi-Robo che vuoi caricare";
extern const char sItalianInterfaceText199[] = "dalla Scheda Memoria nello Slot A sono";
extern const char sItalianInterfaceText200[] = "danneggiati e devono essere cancellati.";
extern const char sItalianInterfaceText203[] = "Vuoi creare dei nuovi";
extern const char sItalianInterfaceText204[] = "dati di gioco?";
extern const char sItalianInterfaceText209[] = "Inserisci una Scheda Memoria idonea.";
extern const char sItalianInterfaceText210[] = "Premi il pulsante A per proseguire";
extern const char sItalianInterfaceText219[] = "Slot A \350 danneggiata e non pu\362 essere utilizzata.";
extern const char sItalianInterfaceText220[] = "Inserisci un'altra Scheda Memoria";
extern const char sItalianInterfaceText223[] = "Inserisci una Scheda Memoria";
extern const char sItalianInterfaceText224[] = "o premi il pulsante A per proseguire senza salvare.";
extern const char sItalianInterfaceText228[] = "Salvataggio completato.";
extern const char sItalianInterfaceText233[] = "Seleziona dei dati di gioco.";
extern const char sItalianInterfaceText238[] = "I dati di gioco non possono essere caricati.";
extern const char sItalianInterfaceText243[] = "La Scheda Memoria nello Slot A";
extern const char sItalianInterfaceText244[] = "non \350 utilizzabile.";
extern const char sItalianInterfaceText259[] = "Tempo:";
extern const char sItalianInterfaceText263[] = "Questo gioco pu\362 essere ";
extern const char sItalianInterfaceText264[] = "visualizzato in modalit\340 60Hz.";
extern const char sItalianInterfaceText265[] = "Seleziona la modalit\340.";
extern const char sItalianInterfaceText266[] = "Se il televisore non dovesse";
extern const char sItalianInterfaceText267[] = "visualizzare correttamente la modalit\340 60Hz,";
extern const char sItalianInterfaceText268[] = "seleziona la modalit\340 50Hz.";
extern const char sItalianInterfaceText270[] = "%s%02X Modalit\340 60Hz %s  %s%02X Modalit\340 50Hz %s";
extern const char sItalianInterfaceText271[] = "Visualizzazione impostata a 60Hz.";
extern const char sItalianInterfaceText274[] = "Visualizzazione impostata a 50Hz.";
extern const char sItalianInterfaceText277[] = "NUOVO";

const char* gItalianInterfaceText[] = {
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sItalianInterfaceText012,      sItalianInterfaceText013,      sEnglishInterfaceText014,      sEnglishInterfaceText015,
	sEnglishInterfaceText016,      sItalianInterfaceText017,      sEnglishInterfaceText065,      sItalianInterfaceText019,
	sEnglishInterfaceText020,      sEnglishInterfaceText021,      sItalianInterfaceText022,      sSpanishInterfaceText023,
	sItalianInterfaceText024,      sEnglishInterfaceText025,      sItalianInterfaceText026,      sItalianInterfaceText027,
	gEmptyResourceDirectorySuffix, sItalianInterfaceText029,      sItalianInterfaceText030,      sItalianInterfaceText031,
	sItalianInterfaceText032,      sItalianInterfaceText033,      sItalianInterfaceText034,      sItalianInterfaceText035,
	sItalianInterfaceText036,      sSpanishInterfaceText023,      sItalianInterfaceText038,      sItalianInterfaceText039,
	sItalianInterfaceText040,      sItalianInterfaceText041,      sItalianInterfaceText042,      sItalianInterfaceText043,
	sItalianInterfaceText044,      gEmptyResourceDirectorySuffix, sItalianInterfaceText046,      sItalianInterfaceText047,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sItalianInterfaceText057,      sItalianInterfaceText022,      gEmptyResourceDirectorySuffix,
	sItalianInterfaceText060,      sItalianInterfaceText061,      sItalianInterfaceText062,      sItalianInterfaceText063,
	sItalianInterfaceText017,      sEnglishInterfaceText065,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sItalianInterfaceText069,      sItalianInterfaceText070,      sItalianInterfaceText071,
	sItalianInterfaceText072,      sItalianInterfaceText073,      sItalianInterfaceText074,      sItalianInterfaceText075,
	sItalianInterfaceText076,      gEmptyResourceDirectorySuffix, sItalianInterfaceText078,      gEmptyResourceDirectorySuffix,
	sItalianInterfaceText080,      sItalianInterfaceText081,      sItalianInterfaceText082,      gEmptyResourceDirectorySuffix,
	sItalianInterfaceText084,      sItalianInterfaceText085,      sItalianInterfaceText085,      sItalianInterfaceText087,
	sItalianInterfaceText088,      sItalianInterfaceText088,      sItalianInterfaceText090,      sItalianInterfaceText091,
	sItalianInterfaceText092,      sItalianInterfaceText093,      sItalianInterfaceText094,      sItalianInterfaceText095,
	sItalianInterfaceText096,      sItalianInterfaceText097,      sItalianInterfaceText098,      sItalianInterfaceText099,
	gEmptyResourceDirectorySuffix, sItalianInterfaceText017,      sEnglishInterfaceText065,      sItalianInterfaceText103,
	sItalianInterfaceText104,      sItalianInterfaceText105,      sItalianInterfaceText106,      gEmptyResourceDirectorySuffix,
	sItalianInterfaceText108,      sItalianInterfaceText109,      sItalianInterfaceText110,      sItalianInterfaceText111,
	gEmptyResourceDirectorySuffix, sItalianInterfaceText113,      sItalianInterfaceText114,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sItalianInterfaceText118,      sItalianInterfaceText119,
	sItalianInterfaceText120,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sItalianInterfaceText123,
	sItalianInterfaceText124,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sItalianInterfaceText128,      sItalianInterfaceText129,      sItalianInterfaceText130,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sItalianInterfaceText133,      sItalianInterfaceText134,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sItalianInterfaceText138,      sItalianInterfaceText139,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sItalianInterfaceText143,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sItalianInterfaceText148,      sItalianInterfaceText149,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sItalianInterfaceText153,      sItalianInterfaceText154,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sItalianInterfaceText158,      sItalianInterfaceText159,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sItalianInterfaceText163,
	sItalianInterfaceText164,      sItalianInterfaceText165,      sItalianInterfaceText166,      gEmptyResourceDirectorySuffix,
	sItalianInterfaceText168,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sItalianInterfaceText173,      sItalianInterfaceText174,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sItalianInterfaceText178,      sItalianInterfaceText129,
	sItalianInterfaceText180,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sItalianInterfaceText183,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sItalianInterfaceText188,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sItalianInterfaceText088,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sItalianInterfaceText198,      sItalianInterfaceText199,
	sItalianInterfaceText200,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sItalianInterfaceText203,
	sItalianInterfaceText204,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sItalianInterfaceText108,      sItalianInterfaceText209,      sItalianInterfaceText210,      sItalianInterfaceText111,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sItalianInterfaceText138,      sItalianInterfaceText219,
	sItalianInterfaceText220,      sItalianInterfaceText110,      sItalianInterfaceText111,      sItalianInterfaceText223,
	sItalianInterfaceText224,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sItalianInterfaceText228,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sItalianInterfaceText233,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sItalianInterfaceText238,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sItalianInterfaceText243,
	sItalianInterfaceText244,      gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix,
	sItalianInterfaceText138,      sItalianInterfaceText219,      sItalianInterfaceText220,      sItalianInterfaceText224,
	gEmptyResourceDirectorySuffix, sItalianInterfaceText138,      sItalianInterfaceText219,      sItalianInterfaceText220,
	sItalianInterfaceText224,      gEmptyResourceDirectorySuffix, sEnglishInterfaceText258,      sItalianInterfaceText259,
	sItalianInterfaceText017,      sEnglishInterfaceText065,      gEmptyResourceDirectorySuffix, sItalianInterfaceText263,
	sItalianInterfaceText264,      sItalianInterfaceText265,      sItalianInterfaceText266,      sItalianInterfaceText267,
	sItalianInterfaceText268,      gEmptyResourceDirectorySuffix, sItalianInterfaceText270,      sItalianInterfaceText271,
	gEmptyResourceDirectorySuffix, gEmptyResourceDirectorySuffix, sItalianInterfaceText274,      gEmptyResourceDirectorySuffix,
	gEmptyResourceDirectorySuffix, sItalianInterfaceText277,
};
#else
const char* gItalianInterfaceText[] = {
#include "game/interface_text_us_fallback.inc"
};
#endif
}
