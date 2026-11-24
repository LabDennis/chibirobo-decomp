#ifndef NW4R_LYT_TYPES_H
#define NW4R_LYT_TYPES_H

#include "types.h"

namespace nw4r {
namespace lyt {

struct Size {
	f32 width;
	f32 height;

	Size()
	    : width(0.0f)
	    , height(0.0f)
	{
	}
	Size(const Size& other)
	    : width(other.width)
	    , height(other.height)
	{
	}
	Size(f32 w, f32 h)
	    : width(w)
	    , height(h)
	{
	}
};

} // namespace lyt
} // namespace nw4r

#endif
