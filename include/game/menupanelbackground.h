#ifndef _GAME_MENUPANELBACKGROUND_H
#define _GAME_MENUPANELBACKGROUND_H
#include "types.h"

enum PanelLayout { ItemStatusPanel, SavePanel };

template <PanelLayout layout>
void DrawPanelBackground(f32 alpha);

#endif
