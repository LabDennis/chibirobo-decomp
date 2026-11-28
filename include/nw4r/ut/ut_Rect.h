#ifndef NW4R_UT_RECT_H
#define NW4R_UT_RECT_H

#include "types.h"

namespace nw4r {
namespace ut {

struct Rect {
	f32 left;
	f32 top;
	f32 right;
	f32 bottom;

	Rect()
	    : left(0.0f)
	    , top(0.0f)
	    , right(0.0f)
	    , bottom(0.0f)
	{
	}
	Rect(f32 l, f32 t, f32 r, f32 b)
	    : left(l)
	    , top(t)
	    , right(r)
	    , bottom(b)
	{
	}
	~Rect() { }
};

} // namespace ut
} // namespace nw4r

#endif
