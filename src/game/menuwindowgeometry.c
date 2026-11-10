#include <dolphin/mtx/mtxtypes.h>

struct MenuWindowGeometry {
	Vec _00[6];
	Vec outline[18];
	Vec _120[18];
	Vec cursorBorder[6];
	Vec _240[7];
};

const struct MenuWindowGeometry gMenuWindowGeometry = { { { 1, 1, 1 }, { 0 }, { 0 }, { 1, 1, 1 } },
	                                                    { { 0, 0, -10 },
	                                                      { 0, 0, -10 },
	                                                      { 0, 0, -10 },
	                                                      { 0, 0, -10 },
	                                                      { 0, 0, -10 },
	                                                      { 0, 0, -10 },
	                                                      { 0, 0, -10 },
	                                                      { 0, 0, -10 },
	                                                      { 0, 0, -10 },
	                                                      { 0, 0, -10 },
	                                                      { 0, 0, -10 },
	                                                      { 0, 0, -10 },
	                                                      { 0, 0, -10 },
	                                                      { 0, 0, -10 },
	                                                      { 0, 0, -10 },
	                                                      { 0, 0, -10 },
	                                                      { 0, 0, -10 },
	                                                      { 0, 0, -10 } } };
