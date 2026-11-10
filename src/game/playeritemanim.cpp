#include "game/playeritemanim.h"

extern "C" {
extern const char sPathCbRobo6Dat[16]  = "cb_robo_6.dat";
extern const char sPathCbRobo7Dat[16]  = "cb_robo_7.dat";
extern const char sPathCbRobo8Dat[16]  = "cb_robo_8.dat";
extern const char sPathCbRobo9Dat[16]  = "cb_robo_9.dat";
extern const char sPathCbRobo10Dat[16] = "cb_robo_10.dat";
extern const char sPathCbRobo11Dat[16] = "cb_robo_11.dat";
extern const char sPathCbRobo12Dat[16] = "cb_robo_12.dat";

XObjChrItemAnim gPlayerItemAnimationTable[] = {
	{ 3, 8, { 0, 1, 0, 1, 2, 0, 1, 0 }, 0.0f },
	{ 3, 9, { 0, 1, 0, 1, 2, 0, 1, 0, 0 }, 0.0f },
	{ 4, 6, { 0, 1, 0, 1, 0, 0 }, 0.0f },
	{ 4, 6, { 0, 1, 0, 1, 0, 1 }, 0.0f },
	{ -1, 0, { 0 }, 0.0f },
};

const char* gAttachmentAnimationPaths[] = {
	NULL, sPathCbRobo6Dat, sPathCbRobo7Dat, sPathCbRobo8Dat, sPathCbRobo9Dat, sPathCbRobo10Dat, sPathCbRobo11Dat, sPathCbRobo12Dat, NULL,
};
}
