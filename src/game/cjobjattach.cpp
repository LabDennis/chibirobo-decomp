#include "game/cjobj.h"

void CJObj::Attach(HSD_JObj* jobj)
{
	Quaternion rotation;
	mJObj = jobj;
	HSD_JObjGetTranslation(jobj, &mTranslate);
	HSD_JObjGetRotation(jobj, &rotation);
	mRotate.Set(rotation.x, rotation.y, rotation.z);
	HSD_JObjGetScale(jobj, &mScale);
}
