#include "revolution/hw_regs.h"

#include "Dolphin/dvd.h"
#include "Dolphin/gx.h"
#include "Dolphin/os.h"

// other libraries
void OSFatal(GXColor fg, GXColor bg, const char* msg);
u16 OSSetFontEncode(u16 encode);
u8 SCGetLanguage(void);
// 0x801B86C0
s8 SCGetProductGameRegion(void);

void __DVDShowFatalMessage(void);

static void (*FatalFunc)(void);

// indexed by SCGetLanguage()
static const char* const DefaultMessages[7] = {
	"\n\n\nエラーが発生しました。\n\n"
	"イジェクトボタンを押してディスクを取り出してか\n"
	"ら、本体の電源をOFFにして、本体の取扱説明書の\n"
	"指示に従ってください。",

	"\n\n\nAn error has occurred.\n"
	"Press the Eject Button, remove the\n"
	"Game Disc, and turn off the power to the\n"
	"console. Please read the Wii Operations\n"
	"Manual for further instructions.",

	"\n\n\nEin Fehler ist aufgetreten.\n"
	"Dr\xFC"
	"cke den Ausgabeknopf, entnimm die\n"
	"Disc und schalte die Wii-Konsole aus.\n"
	"Bitte lies die Wii-Bedienungsanleitung,\n"
	"um weitere Informationen zu erhalten.",

	"\n\n\nUne erreur est survenue.\n"
	"Appuyez sur le bouton EJECT, retirez\n"
	"le disque et \xE9teignez la console.\n"
	"Veuillez vous r\xE9"
	"f\xE9rer au mode d'emploi\n"
	"Wii pour plus de d\xE9tails.",

	"\n\n\nSe ha producido un error.\n"
	"Pulsa el Bot\xF3n EJECT, extrae el disco y\n"
	"apaga la consola. Consulta el manual de\n"
	"instrucciones de la consola Wii para\n"
	"obtener m\xE1s informaci\xF3n.",

	"\n\n\nSi \xE8 verificato un errore.\n"
	"Premi il pulsante EJECT, estrai il disco\n"
	"e spegni la console. Per maggiori\n"
	"informazioni, consulta il manuale di\n"
	"istruzioni della console Wii.",

	"\n\n\nEr is een fout opgetreden.\n"
	"Druk op de EJECT-knop, verwijder de\n"
	"disk en zet het Wii-systeem uit. Lees\n"
	"de handleiding voor meer informatie.",
};

static const char* const EuropeMessages[7] = {
	"\n\n\nエラーが発生しました。\n\n"
	"イジェクトボタンを押してディスクを取り出してか\n"
	"ら、本体の電源をOFFにして、本体の取扱説明書の\n"
	"指示に従ってください。",

	"\n\n\nAn error has occurred.\n"
	"Press the EJECT Button, remove the\n"
	"Game Disc, and turn the power off.\n"
	"Please read the Wii Operations Manual\n"
	"for more information.",

	"\n\n\nEin Fehler ist aufgetreten.\n"
	"Dr\xFC"
	"cke den Ausgabeknopf, entnimm die\n"
	"Disc und schalte die Wii-Konsole aus.\n"
	"Bitte lies die Wii-Bedienungsanleitung,\n"
	"um weitere Informationen zu erhalten.",

	"\n\n\nUne erreur est survenue.\n"
	"Appuyez sur le bouton EJECT, retirez\n"
	"le disque et \xE9teignez la console.\n"
	"Veuillez vous r\xE9"
	"f\xE9rer au mode d'emploi\n"
	"Wii pour plus de d\xE9tails.",

	"\n\n\nSe ha producido un error.\n"
	"Pulsa el Bot\xF3n EJECT, extrae el disco y\n"
	"apaga la consola. Consulta el manual de\n"
	"instrucciones de la consola Wii para\n"
	"obtener m\xE1s informaci\xF3n.",

	"\n\n\nSi \xE8 verificato un errore.\n"
	"Premi il pulsante EJECT, estrai il disco\n"
	"e spegni la console. Per maggiori\n"
	"informazioni, consulta il manuale di\n"
	"istruzioni della console Wii.",

	"\n\n\nEr is een fout opgetreden.\n"
	"Druk op de EJECT-knop, verwijder de\n"
	"disk en zet het Wii-systeem uit. Lees\n"
	"de handleiding voor meer informatie.",
};

static const char* Error104Messages[2] = {
	"\n\nエラーコード１０４。\n"
	"エラーが発生しました。\n\n"
	"イジェクトボタンを押してディスクを取り出してか\n"
	"ら、本体の電源をOFFにして、本体の取扱説明書の\n"
	"指示に従ってください。",

	"\n\nError #104,\n"
	"An error has occurred.\n"
	"Press the EJECT Button, remove the\n"
	"Game Disc, and turn the power off.\n"
	"Please read the Wii Operations Manual\n"
	"for more information.",
};

void __DVDShowFatalMessage(void)
{
	const char* const* messages;
	const char* message;
	GXColor bg = { 0x00, 0x00, 0x00, 0x00 };
	GXColor fg = { 0xFF, 0xFF, 0xFF, 0x00 };

	if (SCGetLanguage() == 0) {
		OSSetFontEncode(1);
	} else {
		OSSetFontEncode(0);
	}

	switch (SCGetProductGameRegion()) {
	default:
		messages = DefaultMessages;
		break;
	case 2:
		messages = EuropeMessages;
		break;
	case 4:
	case 5:
		messages = Error104Messages;
		break;
	}

	if (SCGetLanguage() > 6) {
		message = messages[1];
	} else {
		message = messages[SCGetLanguage()];
	}

	OSFatal(fg, bg, message);
}

BOOL DVDSetAutoFatalMessaging(BOOL enable)
{
	BOOL enabled;
	BOOL prev;

	enabled = OSDisableInterrupts();

	prev      = FatalFunc ? TRUE : FALSE;
	FatalFunc = enable ? __DVDShowFatalMessage : NULL;

	OSRestoreInterrupts(enabled);
	return prev;
}

BOOL __DVDGetAutoFatalMessaging(void)
{
	return FatalFunc ? TRUE : FALSE;
}

void __DVDPrintFatalMessage(void)
{
	if (FatalFunc) {
		(*FatalFunc)();
	}
}
