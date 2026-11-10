#include "Version.h"
#include "game/cjobj.h"

// linked into both shun and stage07

// CGame
extern "C" u8 gGame[];

struct ObjDesc {
	s32 mId; // _00
};

struct FafaToy {
	HSD_JObj* mJObj; // _00
#if defined(VERSION_JP)
	u8 _04[0x238]; // _04
#else
	u8 _04[0x240]; // _04
#endif
	ObjDesc* mType; // _244 (JP: _23C)

	s32 GetType() { return mType != NULL ? mType->mId : -1; }
};

struct ToyArray {
	u8* mData;   // _00
	u8 _04[0xC]; // _04
	s32 mStride; // _10
};

struct FafaGame {
#if defined(VERSION_JP)
	u8 _00[0x9714]; // _00
#else
	u8 _00[0x975C]; // _00
#endif
	ToyArray mItems; // _975C (JP: _9714)
};
#if defined(VERSION_JP)
typedef char FafaGame_size_check[sizeof(FafaGame) == 0x9728 ? 1 : -1];
#else
typedef char FafaGame_size_check[sizeof(FafaGame) == 0x9770 ? 1 : -1];
#endif
#define SHUN_ITEMS() (&((FafaGame*)gGame)->mItems)

// three levels, child then N siblings at each
// todo: cleanup
/**
 * @note Address: N/A
 * @note Size: ?
 */
inline HSD_JObj* FafaFindPart(HSD_JObj* root, s32* path)
{
	HSD_JObj* jobj = root;
	s32 i;
	s32 j;

	if (jobj == NULL) {
		return NULL;
	}
	for (i = 0; i < 3; i++, path++) {
		jobj = HSD_JObjGetChild(jobj);
		for (j = 0; j < *path; j++) {
			jobj = HSD_JObjGetNext(jobj);
			if (jobj == NULL) {
				return NULL;
			}
		}
	}
	return jobj;
}

/**
 * @note Address: shun .text 0x1FB8
 * @note Size: 0x164C
 */
extern "C" void SetFafaPartVisibility(s32 mode)
{
	CJObj obj;
	s32 size = SHUN_ITEMS()->mStride;
	s32 i;

	for (i = 0; i < 15; i++) {
		FafaToy* item = (FafaToy*)(SHUN_ITEMS()->mData + i * size);
		if (item->GetType() != 0x117) {
			continue;
		}
		switch (mode) {
		case 4: {
			s32 path[3] = { 0, 0, 2 };
			obj.Attach(FafaFindPart(item->mJObj, path));
			HSD_JObjClearFlags(obj.mJObj, 0x10);
		}
			{
				s32 path[3] = { 0, 0, 3 };
				obj.Attach(FafaFindPart(item->mJObj, path));
				HSD_JObjSetFlags(obj.mJObj, 0x10);
			}
			{
				s32 path[3] = { 0, 0, 4 };
				obj.Attach(FafaFindPart(item->mJObj, path));
				HSD_JObjSetFlags(obj.mJObj, 0x10);
			}
			break;
		case 7: {
			s32 path[3] = { 0, 0, 2 };
			obj.Attach(FafaFindPart(item->mJObj, path));
			HSD_JObjSetFlags(obj.mJObj, 0x10);
		}
			{
				s32 path[3] = { 0, 0, 3 };
				obj.Attach(FafaFindPart(item->mJObj, path));
				HSD_JObjClearFlags(obj.mJObj, 0x10);
			}
			{
				s32 path[3] = { 0, 0, 4 };
				obj.Attach(FafaFindPart(item->mJObj, path));
				HSD_JObjSetFlags(obj.mJObj, 0x10);
			}
			break;
		case 6: {
			s32 path[3] = { 0, 0, 2 };
			obj.Attach(FafaFindPart(item->mJObj, path));
			HSD_JObjSetFlags(obj.mJObj, 0x10);
		}
			{
				s32 path[3] = { 0, 0, 3 };
				obj.Attach(FafaFindPart(item->mJObj, path));
				HSD_JObjSetFlags(obj.mJObj, 0x10);
			}
			{
				s32 path[3] = { 0, 0, 4 };
				obj.Attach(FafaFindPart(item->mJObj, path));
				HSD_JObjClearFlags(obj.mJObj, 0x10);
			}
			break;
		default: {
			s32 path[3] = { 0, 0, 2 };
			obj.Attach(FafaFindPart(item->mJObj, path));
			HSD_JObjSetFlags(obj.mJObj, 0x10);
		}
			{
				s32 path[3] = { 0, 0, 3 };
				obj.Attach(FafaFindPart(item->mJObj, path));
				HSD_JObjSetFlags(obj.mJObj, 0x10);
			}
			{
				s32 path[3] = { 0, 0, 4 };
				obj.Attach(FafaFindPart(item->mJObj, path));
				HSD_JObjSetFlags(obj.mJObj, 0x10);
			}
			break;
		}
		break;
	}
}
