#ifndef _GAME_PCDIALOGUE_H
#define _GAME_PCDIALOGUE_H

extern "C" {
extern const char sEnglishPCBlank[];
extern const char sJapanesePCWeather[];
extern const char sJapanesePCSports[];
extern const char sJapanesePCBlank[];
extern const char sJapanesePCShopping[];
extern const char sJapanesePCBuildUtilibots[];
extern const char sJapanesePCExchangeScrap[];
extern const char sJapanesePCWelcome[];
}

// The unlocalized slots on the US disc use the same 24 Japanese messages.
#define PC_DIALOGUE_ENTRIES(weather, sports, blank, shopping, build, exchange, welcome)                                                 \
	weather, sports, blank, blank, blank, blank, blank, blank, blank, blank, blank, blank, blank, blank, blank, shopping, blank, build, \
	    exchange, blank, welcome
#define JAPANESE_PC_DIALOGUE_ENTRIES                                                                                             \
	PC_DIALOGUE_ENTRIES(sJapanesePCWeather, sJapanesePCSports, sJapanesePCBlank, sJapanesePCShopping, sJapanesePCBuildUtilibots, \
	                    sJapanesePCExchangeScrap, sJapanesePCWelcome),                                                           \
	    sJapanesePCBlank, sJapanesePCBlank, sJapanesePCBlank

#endif
