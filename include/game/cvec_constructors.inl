inline CVec::CVec() {}

inline CVec::CVec(f32 x, f32 y, f32 z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

inline CVec::CVec(const Vec& v)
{
	x = v.x;
	y = v.y;
	z = v.z;
}

inline CVec::CVec(const CVec& v) : Vec(v) {}

#ifndef DEFER_CVEC_DESTRUCTOR
inline CVec::~CVec() {}
#endif
