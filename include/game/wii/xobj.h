#ifndef GAME_WII_XOBJ_H
#define GAME_WII_XOBJ_H

#include "types.h"

// first: AnimSfx brings nw4r's math types, which the fallback below must not redefine
#include "game/wii/animsfx.h"

#include "Dolphin/gx.h"
#include "Dolphin/mtx.h"

#include "game/cfile.h"
#include "game/coordinfo.h"
#include "game/cstatus64.h"
#include "game/ctrigon.h"
#include "game/mapfloorhit.h"
#include "game/evt.h"
#include "game/ut.h"
#include "game/wii/cgs.h"
#include "game/wii/cresfile.h"
#include "game/wii/xobjs.h"
#include "revolution/mem.h"

extern "C" {
#include "Dolphin/OS/OSMessage.h"
#include "Dolphin/OS/OSThread.h"
}

// one of an object's floors, a list of them ends at an id of -1; the GameCube struct with the ground type split in two
struct XObjFloor {
	s16 mId;              // _00
	u8 mMaterial;         // _02, ground types 13 and up count as none
	u8 mFlags;            // _03, bit 0 includes this floor in contact tests, bit 1 in screen bounds
	s32 mCenterOffset[3]; // _04, in hundredths
	s32 mYawDegrees;      // _10
	s32 mExtent[3];       // _14, in hundredths
};
STATIC_ASSERT(sizeof(XObjFloor) == 0x20);

// the Wii XObj family, as far as xobj_ut.cpp's pool setup verifies it: names and
// sizes come from the retail RTTI strings and the element strides the pool
// constructors pass to __construct_new_array.
//
// XObj's vtable (retail __vt__4XObj at 0x80302438) has two parts, so XObj's
// chain carries two polymorphic bases: RTTI names them nw4r::g3d::
// ICalcWorldCallback at object _00 (4 slots) and nw4r::g3d::IScnObjCallback at
// object _04 (the destructor and five callbacks, the part's header at +_18; the
// three CObj does not override are weak empty bodies at 0x80031E18-0x80031E20).
// MWCC appends a class's own virtuals behind the last part and reaches them
// through the first part's pointer, so the slot comments of CObj and XObj below
// count from the start of the whole table: CObj's own run from _38 to _148 (the
// last two are its overriders of IScnObjCallback's, which the second part
// reaches through this-adjusting thunks), XObj's from
// _14C (verified against InitializeObjectPools' virtual calls and against the
// calls inside the two classes' small virtuals at 0x800433FC-0x80044500).
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}

// the parts of nw4r::math CObj's world callback uses; math_types.h itself needs the Wii MSL's
// <cmath>, which the game's include path does not have yet (a unit built with include/rvl_msl that
// includes it first gets the real one)
#ifndef NW4R_MATH_TYPES_H
namespace math {
struct VEC3;
struct MTX34 {
	f32 m[3][4];
};
MTX34* MTX34RotXYZFIdx(MTX34* pMtx, f32 fx, f32 fy, f32 fz);
inline MTX34* MTX34RotXYZRad(MTX34* pMtx, f32 rx, f32 ry, f32 rz)
{
	return MTX34RotXYZFIdx(pMtx, rx * (128.0f / 3.1415927f), ry * (128.0f / 3.1415927f), rz * (128.0f / 3.1415927f));
}
} // namespace math
#endif

namespace g3d {

class ScnObj;
class ResMdl;

// what the world callback writes for a node, as far as the game uses it
struct ChrAnmResult {
	MtxP mMtx; // _00, the node's world matrix the callback fills

	void SetScale(const math::VEC3* pScale);        // 0x80263EB8
	void SetRotTrans(const math::MTX34* pRotTrans); // 0x80264180
};

// the callback's node binding; the members are nw4r's
class FuncObjCalcWorld {
public:
	u16 GetCallbackNodeID() const { return mNodeID; }

private:
	void* mpCallback; // _00
	u8 mTiming;       // _04
	u8 _05;
	u16 mNodeID; // _06
};

// the three hooks of nw4r's world-matrix pass; their parameters are not recovered
class ICalcWorldCallback {
public:
	virtual ~ICalcWorldCallback() { }                                                          // _08
	virtual void ExecCallbackA(ChrAnmResult* pResult, ResMdl mdl, FuncObjCalcWorld* pFuncObj); // _0C
	virtual void ExecCallbackB(ChrAnmResult* pResult, ResMdl mdl, FuncObjCalcWorld* pFuncObj); // _10
	virtual void ExecCallbackC();                                                              // _14
};

class IScnObjCallback {
public:
	virtual ~IScnObjCallback() { } // _08
	// the library's order: CALC_WORLD, CALC_MAT, CALC_VIEW, DRAW_OPA, DRAW_XLU.
	// the timing is ScnObj's Timing enum, which needs the library's header
	virtual void _unk_00C();
	virtual void ExecCallback_CALC_MAT(int timing, ScnObj* obj, u32 param, void* info); // _10
	virtual void _unk_014();
	virtual void _unk_018();
	virtual void _unk_01C();
};

// a node (bone) of a model resource, as far as the game reads it
struct ResNodeData {
	u8 _00[0x10];
	u32 mMtxID; // _10, the node's index in the model's world matrix array
	u8 _14[0x18];
	Vec mRotate;    // _2C, in degrees
	Vec mTranslate; // _38
};

// nw4r's handle to a node; the two checks carry g3d_resnode_ac.h's lines
class ResNode {
public:
	ResNodeData* mpData; // _00

	bool IsValid() const { return mpData != NULL; }
	ResNodeData* ptr() const { return mpData; }
	ResNodeData& ref() const
	{
		if (!IsValid()) {
			db::Panic("g3d_resnode_ac.h", 44, "%s::%s: Object not valid.", "ResNode", "ref");
		}
		return *mpData;
	}
	u32 GetMtxID() const
	{
		if (!IsValid()) {
			db::Panic("g3d_resnode_ac.h", 83, "NW4R:Failed assertion IsValid()");
		}
		if (IsValid()) {
			return ptr()->mMtxID;
		}
		return 0;
	}
};

// the animation resources a brres file holds, each nw4r's handle with the name accessor and the
// object check of its g3d_resanm*_ac.h; the data only as far as the name offset
struct ResAnmChrData {
	u8 _00[0x18];
	s32 name; // _18
};

class ResAnmChr {
public:
	ResAnmChrData* mpData; // _00

	ResAnmChr(void* p = NULL)
	    : mpData((ResAnmChrData*)p)
	{
	}
	bool IsValid() const { return mpData != NULL; }
	const ResAnmChrData& ref() const
	{
		if (!IsValid()) {
			db::Panic("g3d_resanmchr_ac.h", 39, "%s::%s: Object not valid.", "ResAnmChr", __FUNCTION__);
		}
		return *mpData;
	}
	const char* GetName() const
	{
		const ResAnmChrData& r = ref();
		return r.name != 0 ? (const char*)&r + r.name : NULL;
	}
};

struct ResAnmVisData {
	u8 _00[0x18];
	s32 name; // _18
};

class ResAnmVis {
public:
	ResAnmVisData* mpData; // _00

	ResAnmVis(void* p = NULL)
	    : mpData((ResAnmVisData*)p)
	{
	}
	bool IsValid() const { return mpData != NULL; }
	const ResAnmVisData& ref() const
	{
		if (!IsValid()) {
			db::Panic("g3d_resanmvis_ac.h", 39, "%s::%s: Object not valid.", "ResAnmVis", __FUNCTION__);
		}
		return *mpData;
	}
	const char* GetName() const
	{
		const ResAnmVisData& r = ref();
		return r.name != 0 ? (const char*)&r + r.name : NULL;
	}
};

struct ResAnmClrData {
	u8 _00[0x18];
	s32 name; // _18
};

class ResAnmClr {
public:
	ResAnmClrData* mpData; // _00

	ResAnmClr(void* p = NULL)
	    : mpData((ResAnmClrData*)p)
	{
	}
	bool IsValid() const { return mpData != NULL; }
	const ResAnmClrData& ref() const
	{
		if (!IsValid()) {
			db::Panic("g3d_resanmclr_ac.h", 39, "%s::%s: Object not valid.", "ResAnmClr", __FUNCTION__);
		}
		return *mpData;
	}
	const char* GetName() const
	{
		const ResAnmClrData& r = ref();
		return r.name != 0 ? (const char*)&r + r.name : NULL;
	}
};

struct ResAnmTexPatData {
	u8 _00[0x28];
	s32 name; // _28
};

class ResAnmTexPat {
public:
	ResAnmTexPatData* mpData; // _00

	ResAnmTexPat(void* p = NULL)
	    : mpData((ResAnmTexPatData*)p)
	{
	}
	bool IsValid() const { return mpData != NULL; }
	const ResAnmTexPatData& ref() const
	{
		if (!IsValid()) {
			db::Panic("g3d_resanmtexpat_ac.h", 92, "%s::%s: Object not valid.", "ResAnmTexPat", __FUNCTION__);
		}
		return *mpData;
	}
	const char* GetName() const
	{
		const ResAnmTexPatData& r = ref();
		return r.name != 0 ? (const char*)&r + r.name : NULL;
	}
};

struct ResAnmTexSrtData {
	u8 _00[0x18];
	s32 name; // _18
};

class ResAnmTexSrt {
public:
	ResAnmTexSrtData* mpData; // _00

	ResAnmTexSrt(void* p = NULL)
	    : mpData((ResAnmTexSrtData*)p)
	{
	}
	bool IsValid() const { return mpData != NULL; }
	const ResAnmTexSrtData& ref() const
	{
		if (!IsValid()) {
			db::Panic("g3d_resanmtexsrt_ac.h", 38, "%s::%s: Object not valid.", "ResAnmTexSrt", __FUNCTION__);
		}
		return *mpData;
	}
	const char* GetName() const
	{
		const ResAnmTexSrtData& r = ref();
		return r.name != 0 ? (const char*)&r + r.name : NULL;
	}
};

struct ResAnmShpData {
	u8 _00[0x1C];
	s32 name; // _1C
};

class ResAnmShp {
public:
	ResAnmShpData* mpData; // _00

	ResAnmShp(void* p = NULL)
	    : mpData((ResAnmShpData*)p)
	{
	}
	bool IsValid() const { return mpData != NULL; }
	const ResAnmShpData& ref() const
	{
		if (!IsValid()) {
			db::Panic("g3d_resanmshp_ac.h", 166, "%s::%s: Object not valid.", "ResAnmShp", __FUNCTION__);
		}
		return *mpData;
	}
	const char* GetName() const
	{
		const ResAnmShpData& r = ref();
		return r.name != 0 ? (const char*)&r + r.name : NULL;
	}
};

// a scene animation resource's data, as far as the game reads it
struct ResAnmScnData {
	u8 _00[0x34];
	u16 mNumFrame; // _34
};

// a light set of a scene animation, as far as LIGHT reads it: a plain handle like the other
// shims here, so ResAnmScn::GetResLightSet hands it back in r3
struct ResLightSetData {
	static const u32 INVALID_ID = 0xFFFF;

	u8 _00[0x1A];
	u8 numLight;              // _1A
	u8 _1B[0x3C - 0x1B];
	u16 lightId[8];           // _3C, INVALID_ID for an empty slot
};

class ResLightSet {
public:
	ResLightSetData* mpData; // _00

	bool IsValid() const { return mpData != NULL; }
	const ResLightSetData& ref() const
	{
		if (!IsValid()) {
			db::Panic("g3d_reslightset_ac.h", 39, "%s::%s: Object not valid.", "ResLightSet", "ref");
		}
		return *mpData;
	}
	u32 GetNumLight() const { return ref().numLight; }
	u32 GetLightID(u32 index) const
	{
		if (index > ref().numLight - 1) {
			db::Panic("g3d_reslightset_ac.h", 113, "index is out of bounds(%d)\n%d <= index <= %d not satisfied.", index, 0,
			          ref().numLight - 1);
		}
		if (ref().lightId[index] == ResLightSetData::INVALID_ID) {
			db::Panic("g3d_reslightset_ac.h", 114, "NW4R:Failed assertion ref().lightId[index] != DataType::INVALID_ID");
		}
		return ref().lightId[index];
	}
};

class ResAnmScn {
public:
	void* mpData; // _00

	bool IsValid() const { return mpData != NULL; }
	const ResAnmScnData& ref() const
	{
		if (!IsValid()) {
			db::Panic("g3d_resanmscn_ac.h", 40, "%s::%s: Object not valid.", "ResAnmScn", "ref");
		}
		return *(const ResAnmScnData*)mpData;
	}
	u16 GetNumFrame() const { return ref().mNumFrame; }
	ResLightSet GetResLightSet(u32 idx) const;
	u32 GetResLightSetNumEntries() const;
	bool Bind(const ResAnmScn scene); // 0x80265718
	void Release();
	bool Bind() { return Bind(*this); }
};

// the brres file itself
class ResFile {
public:
	void* mpData; // _00

	bool IsValid() const { return mpData != NULL; }

	ResAnmChr GetResAnmChr(const char* pName) const;
	ResAnmChr GetResAnmChr(int idx) const;
	u32 GetResAnmChrNumEntries() const;
	ResAnmVis GetResAnmVis(const char* pName) const;
	ResAnmVis GetResAnmVis(int idx) const;
	u32 GetResAnmVisNumEntries() const;
	ResAnmClr GetResAnmClr(const char* pName) const;
	ResAnmClr GetResAnmClr(int idx) const;
	u32 GetResAnmClrNumEntries() const;
	ResAnmTexPat GetResAnmTexPat(const char* pName) const;
	ResAnmTexPat GetResAnmTexPat(int idx) const;
	u32 GetResAnmTexPatNumEntries() const;
	ResAnmTexSrt GetResAnmTexSrt(const char* pName) const;
	ResAnmTexSrt GetResAnmTexSrt(int idx) const;
	u32 GetResAnmTexSrtNumEntries() const;
	ResAnmShp GetResAnmShp(const char* pName) const;
	ResAnmShp GetResAnmShp(int idx) const;
	u32 GetResAnmShpNumEntries() const;
	ResAnmScn GetResAnmScn(int idx) const;
	u32 GetResAnmScnNumEntries() const;

	bool HasResAnmChr() const;
	bool HasResAnmVis() const;
	bool HasResAnmClr() const;
	bool HasResAnmTexPat() const;
	bool HasResAnmTexSrt() const;
	bool HasResAnmShp() const;
	bool HasResAnmScn() const;

	void Release();
	void Terminate();
};

// the parts of a material the game's colour stage edits, each with g3d_resmat_ac.h's and
// g3d_restev_ac.h's checks
struct ResGenModeData {
	u8 nTexGens;  // _00
	u8 nChans;    // _01
	u8 nTevs;     // _02
	u8 nInds;     // _03
	u32 cullMode; // _04
};

class ResGenMode {
public:
	ResGenModeData* mpData; // _00

	ResGenMode(void* p = NULL)
	    : mpData((ResGenModeData*)p)
	{
		if ((u32)p & 0x3) {
			db::Panic("g3d_resmat_ac.h", 175, "NW4R:Failed assertion !((u32)p & 0x3)");
		}
	}
	bool IsValid() const { return mpData != NULL; }
	ResGenModeData* ptr() const { return mpData; }
	void GXSetNumTevStages(u8 num);
	u8 GXGetNumTevStages() const
	{
		if (!IsValid()) {
			db::Panic("g3d_resmat_ac.h", 209, "NW4R:Failed assertion IsValid()");
		}
		return IsValid() ? ptr()->nTevs : 0;
	}
};

class ResMatTevColor {
public:
	void* mpData; // _00

	ResMatTevColor(void* p = NULL)
	    : mpData(p)
	{
		if ((u32)p & 0x1f) {
			db::Panic("g3d_resmat_ac.h", 378, "NW4R:Failed assertion !((u32)p & 0x1f)");
		}
	}
	bool IsValid() const { return mpData != NULL; }
	void DCStore(bool sync);
	void GXSetTevColor(GXTevRegID id, GXColor color);
	void GXSetTevKColor(GXTevKColorID id, GXColor color);
	void EndEdit() { DCStore(false); }
};

struct ResTevData {
	u32 size;                     // _00
	s32 toResMdlData;             // _04
	u32 id;                       // _08
	u8 nStages;                   // _0C
	u8 PADDING_0xD[0x10 - 0xD];   // _0D
	u8 texCoordToTexMapID[8];     // _10
	u8 PADDING_0x18[0x20 - 0x18]; // _18, the game tags the last byte when it adds its colour stage
};

class ResTev {
public:
	ResTevData* mpData; // _00

	ResTev(void* p = NULL)
	    : mpData((ResTevData*)p)
	{
	}
	bool IsValid() const { return mpData != NULL; }
	ResTevData* ptr() const { return mpData; }
	const ResTevData& ref() const
	{
		if (!IsValid()) {
			db::Panic("g3d_restev_ac.h", 37, "%s::%s: Object not valid.", "ResTev", __FUNCTION__);
		}
		return *mpData;
	}
	void GXSetTevOrder(GXTevStageID stage, GXTexCoordID coord, GXTexMapID map, GXChannelID channel);
	void GXSetTevColorIn(GXTevStageID stage, GXTevColorArg a, GXTevColorArg b, GXTevColorArg c, GXTevColorArg d);
	void GXSetTevColorOp(GXTevStageID stage, GXTevOp op, GXTevBias bias, GXTevScale scale, GXBool clamp, GXTevRegID reg);
	void GXSetTevAlphaIn(GXTevStageID stage, GXTevAlphaArg a, GXTevAlphaArg b, GXTevAlphaArg c, GXTevAlphaArg d);
	void GXSetTevAlphaOp(GXTevStageID stage, GXTevOp op, GXTevBias bias, GXTevScale scale, GXBool clamp, GXTevRegID reg);
	void GXSetTevKColorSel(GXTevStageID stage, GXTevKColorSel sel);
	void SetNumTevStages(u8 num);
	void DCStore(bool sync);
	u8 GetNumTevStages() const { return ref().nStages; }
	void EndEdit() { DCStore(false); }
};

struct ResMatData {
	u32 size;               // _00
	s32 toResMdlData;       // _04
	s32 name;               // _08
	u32 id;                 // _0C
	u32 flag;               // _10
	ResGenModeData genMode; // _14
	u8 _1C[0x3C - 0x1C];
	s32 toResMatDLData; // _3C
};

struct ResMatDLData {
	u8 dlPix[0x20];      // _00
	u8 dlTevColor[0x80]; // _20
};

class ResMat {
public:
	ResMatData* mpData; // _00

	bool IsValid() const { return mpData != NULL; }
	ResMatData& ref()
	{
		if (!IsValid()) {
			db::Panic("g3d_resmat_ac.h", 621, "%s::%s: Object not valid.", "ResMat", __FUNCTION__);
		}
		return *mpData;
	}
	const ResMatData& ref() const
	{
		if (!IsValid()) {
			db::Panic("g3d_resmat_ac.h", 621, "%s::%s: Object not valid.", "ResMat", __FUNCTION__);
		}
		return *mpData;
	}
	const char* GetName() const
	{
		const ResMatData& r = ref();
		return r.name != 0 ? (const char*)&r + r.name : NULL;
	}
	ResGenMode GetResGenMode() { return ResGenMode(&ref().genMode); }
	ResMatDLData* GetResMatDLData()
	{
		s32 ofs = ref().toResMatDLData;
		return ofs != 0 ? (ResMatDLData*)((u8*)mpData + ofs) : NULL;
	}
	ResMatTevColor GetResMatTevColor() { return ResMatTevColor(&GetResMatDLData()->dlTevColor); }
	ResTev GetResTev();
};

// nw4r's handle to a model resource
class ResMdl {
public:
	void* mpData; // _00

	bool IsValid() const { return mpData != NULL; }
	ResNode GetResNode(const char* name) const; // 0x8025CFC8
	ResNode GetResNode(u32 index) const;        // 0x8025D05C
	u32 GetResNodeNumEntries() const;           // 0x8025D104
	ResMat GetResMat(int idx) const;
	ResMat GetResMat(u32 idx) const;
	u32 GetResMatNumEntries() const;
	void Release();
	void Terminate();
};

// the scene objects, as far as the game reaches them: the virtuals in nw4r's slot order and
// the members the game's model object sets up
class G3dObj {
public:
	class TypeObj;
	virtual bool IsDerivedFrom(TypeObj other) const;        // _08
	virtual void G3dProc(u32 task, u32 param, void* pInfo); // _0C
	virtual ~G3dObj();                                      // _10
	virtual const TypeObj GetTypeObj() const;               // _14
	virtual const char* GetTypeName() const;                // _18

	void Destroy();

private:
	G3dObj* mpParent;     // _04
	MEMAllocator* mpHeap; // _08
};

class ScnObj : public G3dObj {
public:
	enum Timing { CALLBACK_TIMING_A = (1 << 0), CALLBACK_TIMING_B = (1 << 1), CALLBACK_TIMING_C = (1 << 2) };

	enum ExecOp {
		EXECOP_CALC_WORLD = (1 << 0),
		EXECOP_CALC_MAT   = (1 << 1),
		EXECOP_CALC_VIEW  = (1 << 2),
		EXECOP_DRAW_OPA   = (1 << 4),
		EXECOP_DRAW_XLU   = (1 << 5)
	};

	enum ForEachResult { FOREACH_RESULT_OK, FOREACH_RESULT_GLOBAL, FOREACH_RESULT_LOCAL };
	typedef ForEachResult (*ForEachFunc)(ScnObj* pObj, void* pInfo);

	virtual ForEachResult ForEach(ForEachFunc pFunc, void* pInfo, bool postOrder); // _1C
	virtual bool SetScnObjOption(u32 option, u32 value);                           // _20
	virtual bool GetScnObjOption(u32 option, u32* pValue) const;                   // _24
	virtual f32 GetValueForSortOpa() const;                                        // _28
	virtual f32 GetValueForSortXlu() const;                                        // _2C
	virtual void CalcWorldMtx(const math::MTX34* pParent, u32* pParam);            // _30

	void SetScnObjCallback(IScnObjCallback* pCallback) { mpFuncObjExec = pCallback; }
	void EnableScnObjCallbackTiming(Timing timing);
	void EnableScnObjCallbackExecOp(ExecOp op);

private:
	u8 _0C[0xD4 - 0xC];             // the matrices, bounding volumes, flags and priorities
	IScnObjCallback* mpFuncObjExec; // _D4
	u8 mCallbackTiming;             // _D8
	u8 mCallbackDeleteOption;       // _D9
	u16 mCallbackExecOpMask;        // _DA
};

class ScnLeaf : public ScnObj {
private:
	u8 mScale[0xC]; // _DC, nw4r's VEC3
};

class AnmObj;
class AnmObjChr;

class ScnMdlSimple : public ScnLeaf {
public:
	enum AnmObjType {
		ANMOBJTYPE_CHR,
		ANMOBJTYPE_VIS,
		ANMOBJTYPE_MATCLR,
		ANMOBJTYPE_TEXPAT,
		ANMOBJTYPE_TEXSRT,
		ANMOBJTYPE_SHP,
		ANMOBJTYPE_NOT_SPECIFIED
	};

	virtual bool SetAnmObj(AnmObj* pObj, AnmObjType type);  // _34
	virtual bool RemoveAnmObj(AnmObj* pObj);                // _38
	virtual AnmObj* RemoveAnmObj(AnmObjType type);          // _3C
	virtual AnmObj* GetAnmObj(AnmObjType type);             // _40
	virtual const AnmObj* GetAnmObj(AnmObjType type) const; // _44

	void EnableScnMdlCallbackTiming(Timing timing);
	void SetScnMdlCallback(ICalcWorldCallback* pCallback) { mpCalcWorldCallback = pCallback; }
	void SetScnMdlCallbackNodeID(u32 id) { mCwcbNodeID = id; }

	ResMdl mResMdl;       // _E8
	Mtx* mpWorldMtxArray; // _EC, one per ResNodeData::mMtxID

private:
	u8 _F0[0x11C - 0xF0];
	ICalcWorldCallback* mpCalcWorldCallback; // _11C
	u8 mCwcbTiming;                          // _120
	u8 mCwcbDeleteOption;                    // _121
	u16 mCwcbNodeID;                         // _122
};

class ScnMdl : public ScnMdlSimple {
public:
	// a material's buffered copy the model keeps for editing
	class CopiedMatAccess {
	public:
		CopiedMatAccess(ScnMdl* pScnMdl, u32 id);

		ResGenMode GetResGenMode(bool markDirty);
		ResMatTevColor GetResMatTevColor(bool markDirty);
		ResTev GetResTev(bool markDirty);

	private:
		u8 _00[0x34];
	};

	static ScnMdl* Construct(MEMAllocator* pAllocator, u32* pSize, ResMdl mdl, u32 bufferOption, int numView);
};

// the animation objects, with the virtuals the game calls in nw4r's slot order
class AnmObj : public G3dObj {
public:
	virtual void SetFrame(f32 frame);     // _1C
	virtual f32 GetFrame() const;         // _20
	virtual void UpdateFrame();           // _24
	virtual void SetUpdateRate(f32 rate); // _28
	virtual f32 GetUpdateRate() const;    // _2C
	virtual bool Bind(const ResMdl mdl);  // _30
	virtual void Release();               // _34

private:
	u32 mFlags; // _0C
};

class AnmObjChrRes;

class AnmObjChr : public AnmObj {
public:
	enum BindOption { BIND_ONE, BIND_PARTIAL, NUM_OF_BIND_OPTION };

	virtual const ChrAnmResult* GetResult(ChrAnmResult* pResult, u32 idx); // _38
	virtual AnmObjChrRes* Attach(int idx, AnmObjChrRes* pRes);             // _3C
	virtual AnmObjChrRes* Detach(int idx);                                 // _40
	virtual void DetachAll();                                              // _44
	virtual void SetWeight(int idx, f32 weight);                           // _48
	virtual f32 GetWeight(int idx) const;                                  // _4C
	virtual bool Bind(const ResMdl mdl, u32 target, BindOption option);    // _50
	virtual void Release(const ResMdl mdl, u32 target, BindOption option); // _54

	using AnmObj::Bind;
	using AnmObj::Release;

protected:
	int mNumBinding;      // _10
	u16* const mpBinding; // _14
};

class AnmObjChrNode : public AnmObjChr {
public:
	// the child check carries g3d_anmchr.h's line
	AnmObjChrRes* GetChild(int idx)
	{
		(void)((idx >= 0 && idx <= mChildrenArraySize - 1)
		       || (db::Panic("g3d_anmchr.h", 152, "idx is out of bounds(%d)\n%d <= idx <= %d not satisfied.", idx, 0,
		                     mChildrenArraySize - 1),
		           0));
		return mpChildrenArray[idx];
	}

protected:
	int mChildrenArraySize;         // _18
	AnmObjChrRes** mpChildrenArray; // _1C
};

class AnmObjChrBlend : public AnmObjChrNode {
public:
	static AnmObjChrBlend* Construct(MEMAllocator* pAllocator, u32* pSize, ResMdl mdl, int numChildren);

private:
	f32* mpWeightArray; // _20
};

class AnmObjChrRes : public AnmObjChr {
public:
	static AnmObjChrRes* Construct(MEMAllocator* pAllocator, u32* pSize, ResAnmChr chr, ResMdl mdl, bool cache);

	ResAnmChr GetResAnm() { return mRes; }

private:
	u8 mFrameCtrl[0x14]; // _18, nw4r's FrameCtrl base
	ResAnmChr mRes;      // _2C
};

class AnmObjVisRes : public AnmObj {
public:
	static AnmObjVisRes* Construct(MEMAllocator* pAllocator, u32* pSize, ResAnmVis vis, ResMdl mdl);
};

class AnmObjMatClrRes : public AnmObj {
public:
	static AnmObjMatClrRes* Construct(MEMAllocator* pAllocator, u32* pSize, ResAnmClr clr, ResMdl mdl, bool cache);
};

class AnmObjTexPatRes : public AnmObj {
public:
	static AnmObjTexPatRes* Construct(MEMAllocator* pAllocator, u32* pSize, ResAnmTexPat pat, ResMdl mdl, bool cache);
};

class AnmObjTexSrtRes : public AnmObj {
public:
	static AnmObjTexSrtRes* Construct(MEMAllocator* pAllocator, u32* pSize, ResAnmTexSrt srt, ResMdl mdl, bool cache);
};

class AnmObjShpRes : public AnmObj {
public:
	static AnmObjShpRes* Construct(MEMAllocator* pAllocator, u32* pSize, ResAnmShp shp, ResMdl mdl, bool cache);
};

// nw4r's light object, as far as LIGHT reads it: the enable flag in front of the GX light
class LightObj {
public:
	LightObj()
	    : mFlag(0)
	{
	}
	~LightObj() { }

	bool IsEnable() const { return (mFlag & FLAG_ENABLE_LIGHT) ? true : false; }

private:
	enum LightObjFlag { FLAG_ENABLE_LIGHT = (1 << 2) };

	u32 mFlag;       // _00
	GXLightObj mObj; // _04
};

class AnmScnRes;

// the scene animation (lights, fog, cameras); the virtuals in nw4r's order through GetLight,
// nothing past it is needed yet
class AnmScn : public G3dObj {
public:
	virtual void SetFrame(f32 frame);                      // _1C
	virtual f32 GetFrame() const;                          // _20
	virtual void SetUpdateRate(f32 rate);                  // _24
	virtual f32 GetUpdateRate() const;                     // _28
	virtual void UpdateFrame();                            // _2C
	virtual AnmScnRes* Attach(int idx, AnmScnRes* pRes);   // _30
	virtual AnmScnRes* Detach(int idx);                    // _34
	virtual u32 GetNumLightSet() const;                    // _38
	virtual u32 GetNumAmbLight() const;                    // _3C
	virtual u32 GetNumDiffuseLight() const;                // _40
	virtual u32 GetNumSpecularLight() const;               // _44
	virtual u32 GetNumFog() const;                         // _48
	virtual u32 GetNumCamera() const;                      // _4C
	virtual u32 GetLightSetMaxRefNumber() const;           // _50
	virtual u32 GetAmbLightMaxRefNumber() const;           // _54
	virtual u32 GetDiffuseLightMaxRefNumber() const;       // _58
	virtual u32 GetFogMaxRefNumber() const;                // _5C
	virtual u32 GetCameraMaxRefNumber() const;             // _60
	virtual void vf064();                                  // _64, GetLightSet
	virtual void vf068();                                  // _68, GetAmbLightColor
	virtual void GetLight(LightObj* pDiff, LightObj* pSpec, u32 refNumber); // _6C
};

class AnmScnRes : public AnmScn {
public:
	static AnmScnRes* Construct(MEMAllocator* pAllocator, u32* pSize, ResAnmScn scn, bool cache);

	// inline: retail's only copy is in the game's range
	virtual u32 GetNumLightSet() const { return mRes.GetResLightSetNumEntries(); } // _38

	ResAnmScn GetResAnm() { return mRes; }

	u8 _0C[0x20 - 0x0C];
	ResAnmScn mRes; // _20, the resource it plays; IsAnmScnEnd reads its frame count
};

// the scene root, as far as the game calls it: the current camera's handle is reduced to its data
class ScnRoot {
public:
	MtxP GetCurrentCamera(); // 0x8027ADC8
};

} // namespace g3d
} // namespace nw4r

class CCamera;

// the Wii's CVec sibling: same {x,y,z,vtable} shape with its own vtable, base of
// the Wii CObjVec (retail __ct__7CObjVecFv at 0x800BE7C0 stores the TVec vtable
// for its first member and CVec vtables for the two behind it)
class TVec : public CVec {
public:
	TVec() { }
	TVec(f32 x, f32 y, f32 z)
	    : CVec(x, y, z)
	{
	}
	virtual ~TVec() { } // _08
};

// same shape and role as the GameCube CObjVec: a current vec with the target it
// walks to and the step's speed, only the current vec is a TVec here
// ccameraactioneye.h (which cgame.h includes) carries a second copy of the class for the camera
// code; until the two are one, a file that needs both headers includes that one first and takes it
#ifndef _GAME_CCAMERAACTIONEYE_H
class CObjVec : public TVec {
public:
	CObjVec();
	u8 Step(f32 frames); // 0x802A7FB4
	bool IsMoving() const;

	CVec mTarget; // _10
	CVec mSpeed;  // _20

	virtual ~CObjVec() { }

	// puts it at a value with nowhere to go
	void Reset(f32 x, f32 y, f32 z)
	{
		this->x   = x;
		this->y   = y;
		this->z   = z;
		mTarget.x = x;
		mTarget.y = y;
		mTarget.z = z;
		mSpeed.x  = 0.0f;
		mSpeed.y  = 0.0f;
		mSpeed.z  = 0.0f;
	}
	void Reset(const Vec& v) { Reset(v.x, v.y, v.z); }
	void SetTarget(f32 x, f32 y, f32 z)
	{
		mTarget.x = x;
		mTarget.y = y;
		mTarget.z = z;
	}
	void SetTarget(const Vec& v) { SetTarget(v.x, v.y, v.z); }
	// puts it at a value with its target there too, the speed stays
	void SetNow(const Vec& v)
	{
		Set(v);
		SetTarget(v);
	}
};
STATIC_ASSERT(sizeof(CObjVec) == 0x30);
#endif

// small polymorphic members the aggregate constructors only tag with a vtable;
// their names come from the retail RTTI strings, their members are not yet
// recovered
class CResource {
public:
	virtual ~CResource() { }
};

// the game's wrapper around an nw4r scene model, base of NW_Anm (RTTI). its
// vtable sits behind the members, at _58: CObj keeps one at _E8 with the vtable
// at _140, CJArchive an NW_Anm with the vtable at _58, and both constructors
// clear the two leading words. member names are the ones the unit's nw4r
// pointer checks print ("m_pScnMdl", "m_resources.scn")
// the cross-fade of NW_Obj's skeletal animations: nw4r's blend object with four children, the
// newest at 0, and per child the anim's name and the weight it gains or loses a frame. the
// member name is the one its pointer check prints ("m_pBlend"); the class name is not known
class NW_ChrBlend {
public:
	void Init(MEMAllocator* allocator, u32* size, nw4r::g3d::ResMdl mdl);  // 0x802B8E7C
	void Attach(nw4r::g3d::AnmObjChrRes* chr, f32 blend, f32 blendFrames); // 0x802B8FD4
	void SetUpdateRate(nw4r::g3d::AnmObj* obj, f32 rate);                  // 0x802B9238
	void Update(f32 frames);                                               // 0x802B9364
	void DetachAll();                                                      // 0x802B94F8

	nw4r::g3d::AnmObjChrBlend* m_pBlend; // _00
	const char* mNames[4];               // _04
	f32 mWeightSteps[4];                 // _14
};
STATIC_ASSERT(sizeof(NW_ChrBlend) == 0x24);

// the game's wrapper around an nw4r scene model, base of NW_Anm (RTTI). its
// vtable sits behind the members, at _58: CObj keeps one at _E8 with the vtable
// at _140, CJArchive an NW_Anm with the vtable at _58, and both constructors
// clear the two leading words. member names are the ones the unit's nw4r
// pointer checks print ("m_pScnMdl", "m_resources.chr" and the rest)
class NW_Obj {
public:
	NW_Obj()
	{
		m_resFile.mpData = NULL;
		m_resMdl.mpData  = NULL;
	}

	void CreateScnMdl(int numView, BOOL blend, BOOL noTevColor, const char* name);             // 0x802B6560
	CVec GetNodeRotate(nw4r::g3d::ResNode node) const;                                         // 0x80043518, in radians
	CVec GetNodeTranslate(nw4r::g3d::ResNode node) const;                                      // 0x80043988
	void Release(BOOL freeResource);                                                           // 0x802B6690
	nw4r::g3d::AnmObjChrRes* GetAnmObjChrRes() const;                                          // 0x802B6850
	void SetAnim(const char* name, s32 id, f32 frame, f32 blend, f32 blendFrames);             // 0x802B68D8
	const char* SetAnmChr(const char* name, s32 id, f32 frame, f32 blend, f32 blendFrames);    // 0x802B698C
	void RemoveAnmChr();                                                                       // 0x802B6B00
	const char* SetAnmVis(const char* name, s32 id, f32 frame);                                // 0x802B6B84
	void RemoveAnmVis();                                                                       // 0x802B6CD8
	const char* SetAnmClr(const char* name, s32 id, f32 frame);                                // 0x802B6D50
	void RemoveAnmClr();                                                                       // 0x802B6EA4
	const char* SetAnmTexPat(const char* name, s32 id, f32 frame);                             // 0x802B6F1C
	void RemoveAnmTexPat();                                                                    // 0x802B7070
	const char* SetAnmTexSrt(const char* name, s32 id, f32 frame);                             // 0x802B70E8
	void RemoveAnmTexSrt();                                                                    // 0x802B723C
	const char* SetAnmShp(const char* name, s32 id, f32 frame);                                // 0x802B72B4
	void RemoveAnmShp();                                                                       // 0x802B7408
	BOOL SetResAnmChr(const nw4r::g3d::ResAnmChr& chr, f32 frame, f32 blend, f32 blendFrames); // 0x802B7480
	void CreateAnmChr(const nw4r::g3d::ResAnmChr& chr, f32 frame, f32 blend, f32 blendFrames); // 0x802B74EC
	BOOL SetResAnmVis(const nw4r::g3d::ResAnmVis& vis, f32 frame);                             // 0x802B77A4
	void CreateAnmVis(const nw4r::g3d::ResAnmVis& vis, f32 frame);                             // 0x802B7810
	BOOL SetResAnmClr(const nw4r::g3d::ResAnmClr& clr, f32 frame);                             // 0x802B79FC
	void CreateAnmClr(const nw4r::g3d::ResAnmClr& clr, f32 frame);                             // 0x802B7A68
	BOOL SetResAnmTexPat(const nw4r::g3d::ResAnmTexPat& pat, f32 frame);                       // 0x802B7C58
	void CreateAnmTexPat(const nw4r::g3d::ResAnmTexPat& pat, f32 frame);                       // 0x802B7D84
	BOOL SetResAnmTexSrt(const nw4r::g3d::ResAnmTexSrt& srt, f32 frame);                       // 0x802B7F74
	void CreateAnmTexSrt(const nw4r::g3d::ResAnmTexSrt& srt, f32 frame);                       // 0x802B7FE0
	BOOL SetResAnmShp(const nw4r::g3d::ResAnmShp& shp, f32 frame);                             // 0x802B81D0
	void CreateAnmShp(const nw4r::g3d::ResAnmShp& shp, f32 frame);                             // 0x802B823C
	void RemoveAnimAll();                                                                      // 0x802B842C
	void SetUpdateRate(f32 rate);                                                              // 0x802B8490
	void UpdateBlend(f32 frames);                                                              // 0x802B857C
	void RemoveAnmChrAll();                                                                    // 0x802B8594
	void SetAnmScn(u32 id, f32 frame);                                                         // 0x802B8600
	void ReleaseWork();                                                                        // 0x802B8674, frees the scene anim
	void CreateAnmScn(nw4r::g3d::ResAnmScn& scn, f32 frame);                                   // 0x802B86B8
	void InitColorTev();                                                                       // 0x802B886C
	void SetColors(GXColor add, GXColor mul);                                                  // 0x802B8C8C

	nw4r::g3d::ResFile m_resFile; // _00
	nw4r::g3d::ResMdl m_resMdl;   // _04
	nw4r::g3d::ScnMdl* m_pScnMdl; // _08, NULL without a model
	NW_ChrBlend m_blend;          // _0C
	u8 m_flags;                   // _30, 1: the skeletal anims cross-fade through m_blend
	struct {
		nw4r::g3d::AnmObjChrRes* chr;       // _34
		nw4r::g3d::AnmObjVisRes* vis;       // _38
		nw4r::g3d::AnmObjMatClrRes* matclr; // _3C
		nw4r::g3d::AnmObjTexPatRes* texpat; // _40
		nw4r::g3d::AnmObjTexSrtRes* texsrt; // _44
		nw4r::g3d::AnmObjShpRes* shp;       // _48
		nw4r::g3d::AnmScnRes* scn;          // _4C
	} m_resources;
	const char* mDefaultTexPat; // _50, the pattern anim played while no other is asked for
	const char* mTexPatName;    // _54, the pattern anim playing

	virtual ~NW_Obj() { } // _58
};
STATIC_ASSERT(sizeof(NW_Obj) == 0x5C);

class NW_Anm : public NW_Obj {
public:
	virtual ~NW_Anm() { }
};

// anims waiting to play after the current one. unlike the GameCube layout the
// vtable leads the object on the Wii (retail stores __vt__9CAnmQueue at _7B8
// inside XObj); the queue members behind it are not yet recovered
// a material color stepping towards a target; the Wii keeps the pair inside the object's
// animation queue where the GameCube XObj kept them as its own members
struct XObjColorStep {
	Vec mValue;  // _00
	Vec mTarget; // _0C
	Vec mSpeed;  // _18
};
STATIC_ASSERT(sizeof(XObjColorStep) == 0x24);

class CAnmQueue {
public:
	virtual ~CAnmQueue() { }

	// the push anchor and the per-axis move limits the GameCube kept on XObj itself
	// (SetClampedPushPositionXZ at 0x800B61FC reads _7BC.._7D0, inside the queue)
	f32 mAnchorX;   // _04
	f32 mAnchorZ;   // _08
	f32 mLimitPosX; // _0C
	f32 mLimitNegX; // _10
	f32 mLimitPosZ; // _14
	f32 mLimitNegZ; // _18
	// _1C, the GameCube XObj's own mMarkerOverride: scripts override the object's map marker with it
	s32 mMarkerOverride;

	// _20, XObj::SetColor writes them (verified at 0x800B67F8); the rest of the queue's
	// members are not yet recovered
	XObjColorStep mColorStep[2];
};
STATIC_ASSERT(sizeof(CAnmQueue) == 0x68);

// the Wii object archive, replacing the GameCube CJArchive/CJAnim pair: the
// retail ctor (0x800BE7E0) constructs an NW_Anm and the CFile the archive reads
// go through. XObj keeps one and XObjChr a second one
class CJArchive {
public:
	CJArchive();

	NW_Anm mAnm; // _00, its vtable at _58
	CFile mFile; // _5C
};
STATIC_ASSERT(sizeof(CJArchive) == 0x148);

// element of XObjMap's two 100-entry record lists: 0xC bytes with a ctor and
// dtor in the CPtcl unit (retail fn_800BEB4C/dtor_800BEB60 and
// fn_800BEBDC/dtor_800BEBF0). their real names and members are not recovered yet
class XObjMapListA {
public:
	XObjMapListA();
	~XObjMapListA();

private:
	u8 _00[0xC];
};
STATIC_ASSERT(sizeof(XObjMapListA) == 0xC);

class XObjMapListB {
public:
	XObjMapListB();
	~XObjMapListB();

private:
	u8 _00[0xC];
};
STATIC_ASSERT(sizeof(XObjMapListB) == 0xC);

// an animation as the objects ask for it: its index in the object's own file and the name it is
// looked up by in mAnmSource. the type's name is not known
struct CObjAnim {
	s32 mId;           // _00
	const char* mName; // _04
};

// the shared base of the game's scene objects: the two nw4r callback
// interfaces plus CObj's own members. the member list below is decoded from
// the stores in the retail aggregate constructors; the nine vec members come
// in three {TVec,CVec,CVec} groups matching the GameCube CObjVec shape, so
// they are named after the three move steppers the GameCube CXObj keeps in
// CObjVecs
class CObj : public nw4r::g3d::ICalcWorldCallback, public nw4r::g3d::IScnObjCallback {
public:
	// the retail aggregate constructors show all nine vecs zeroed while the
	// vec members' own ctors stay empty, so the zeros come from here
	CObj()
	{
		mMoveTranslate.Set(0.0f, 0.0f, 0.0f);
		mMoveTarget0.Set(0.0f, 0.0f, 0.0f);
		mMoveSpeed0.Set(0.0f, 0.0f, 0.0f);
		mMoveRotate.Set(0.0f, 0.0f, 0.0f);
		mMoveTarget1.Set(0.0f, 0.0f, 0.0f);
		mMoveSpeed1.Set(0.0f, 0.0f, 0.0f);
		mMoveScale.Set(0.0f, 0.0f, 0.0f);
		mMoveTarget2.Set(0.0f, 0.0f, 0.0f);
		mMoveSpeed2.Set(0.0f, 0.0f, 0.0f);
	}
	virtual ~CObj() { }
	virtual void ExecCallbackA(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj); // _0C
	virtual void ExecCallbackB(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj); // _10
	virtual void ExecCallbackC();                                                                                               // _14
	virtual BOOL ReadAsync(ResourceFile* file, const char* path, u32 model, u32 flags, u32 offset, u32 size);                   // _38
	virtual BOOL ReadCheck();                                                                                                   // _3C
	virtual void Release();                                                                                                     // _40
	virtual u8 StepTranslate(f32 frames);                                                                                       // _44
	virtual u8 StepRotate(f32 frames);                                                                                          // _48
	virtual u8 StepScale(f32 frames);                                                                                           // _4C
	virtual nw4r::g3d::ResNode GetResNode(const char* name);                                                                    // _50
	virtual nw4r::g3d::ResNode GetResNode(u32 index);                                                                           // _54
	virtual nw4r::g3d::ResNode GetResNode();     // _58, the first node
	virtual nw4r::g3d::ResNode GetRootResNode(); // _5C
	virtual void SetTranslateX(f32 x);           // _60
	virtual void SetTranslateY(f32 y);           // _64
	virtual void SetTranslateZ(f32 z);           // _68
	virtual void SetRotationX(f32 x);            // _6C
	virtual void SetRotationY(f32 y);            // _70
	virtual void SetRotationZ(f32 z);            // _74
	virtual void SetScaleX(f32 x);               // _78
	virtual void SetScaleY(f32 y);               // _7C
	virtual void SetScaleZ(f32 z);               // _80
	virtual void SetPos(f32 x, f32 y, f32 z);    // _84
	virtual void SetRot(f32 x, f32 y, f32 z);    // _88
	virtual void SetScale(f32 x, f32 y, f32 z)   // _8C
	{
		mMoveScale.x   = x;
		mMoveScale.y   = y;
		mMoveScale.z   = z;
		mMoveTarget2.x = x;
		mMoveTarget2.y = y;
		mMoveTarget2.z = z;
	}
	virtual void SetPos(const Vec& pos);     // _90
	virtual void SetRot(const Vec& rot);     // _94
	virtual void SetScale(const Vec& scale); // _98
	virtual void SetScale(f32 scale) { SetScale(scale, scale, scale); }
	virtual MtxP GetWorldMtx(nw4r::g3d::ResNode node);                                       // _A0, NULL without a model
	virtual MtxP GetWorldMtx(const char* name);                                              // _A4
	virtual MtxP GetWorldMtx(u32 index);                                                     // _A8
	virtual MtxP GetWorldMtx();                                                              // _AC
	virtual CVec GetWorldPos(nw4r::g3d::ResNode node);                                       // _B0
	virtual CVec GetWorldPos(const char* name);                                              // _B4
	virtual CVec GetWorldPos(u32 index);                                                     // _B8
	virtual CVec GetWorldPos();                                                              // _BC
	virtual CVec GetNodeTranslate(nw4r::g3d::ResNode node);                                  // _C0, the node's own translation
	virtual CVec GetNodeTranslate(const char* name);                                         // _C4
	virtual CVec GetNodeTranslate(u32 index);                                                // _C8
	virtual CVec GetNodeTranslate();                                                         // _CC
	virtual f32 GetNodeRotateX(nw4r::g3d::ResNode node);                                     // _D0, the node's own rotation, in radians
	virtual f32 GetNodeRotateX(const char* name);                                            // _D4
	virtual f32 GetNodeRotateX(u32 index);                                                   // _D8
	virtual f32 GetNodeRotateX();                                                            // _DC
	virtual f32 GetNodeRotateY(nw4r::g3d::ResNode node);                                     // _E0
	virtual f32 GetNodeRotateY(const char* name);                                            // _E4
	virtual f32 GetNodeRotateY(u32 index);                                                   // _E8
	virtual f32 GetNodeRotateY();                                                            // _EC
	virtual f32 GetNodeRotateZ(nw4r::g3d::ResNode node);                                     // _F0
	virtual f32 GetNodeRotateZ(const char* name);                                            // _F4
	virtual f32 GetNodeRotateZ(u32 index);                                                   // _F8
	virtual f32 GetNodeRotateZ();                                                            // _FC
	virtual void SetMoveDir(const Vec* dir, f32 speed);                                      // _100
	virtual void SetRotateY(f32 target, f32 speed);                                          // _104
	virtual void SetAnim(s32 id, BOOL force, f32 frame, f32 rate, f32 blend, f32 blendRate); // _108
	virtual void SetAnim(const CObjAnim* anim, BOOL force, f32 frame, f32 rate, f32 blend, f32 blendRate); // _10C
	virtual void ClearAnim();                                                                              // _110
	virtual void SetAnimRate(f32 rate);                                                                    // _114
	virtual void UpdateAnim(f32 speed);         // _118, the GameCube's UpdateAnim
	virtual s32 GetAnimId() { return mAnimId; } // _11C
	virtual f32 GetAnimRate(s32 index);         // _120
	virtual f32 GetAnimRate();                  // _124
	virtual f32 GetAnimFrame();                 // _128
	virtual f32 GetAnimEndFrame();              // _12C
	virtual bool IsAnimEnd(f32 frames);         // _130, the animation runs out within frames
	virtual bool IsAnimDone()
	{
		f32 frame = GetAnimFrame();
		return frame + mAnimRate[1] >= GetAnimEndFrame();
	}
	// the animation crossed frame f during the last update
	BOOL PassedAnimFrame(f32 f) { return mLastAnimFrame < f && GetAnimFrame() >= f; }
	virtual void SetAnmScn(u32 id, f32 frame, f32 rate); // _138
	virtual void ReleaseModelWork();                     // _13C
	virtual bool IsAnmScnEnd();                          // _140, the scene animation has run out
	virtual void _unk_00C();                             // _144, IScnObjCallback's
	// the IScnObjCallback override: overriding a second base's virtual adds a slot here
	virtual void ExecCallback_CALC_MAT(int timing, nw4r::g3d::ScnObj* obj, u32 param, void* info); // _148

	void Init();
	BOOL Load(ResourceFile* file, const char* path, u32 model, u32 flags, u32 offset, u32 size);
	void SetAnmSource(NW_Obj* source);                // 0x802B9B64
	void ClearAnmSource();                            // 0x802B9BEC
	void SetMatAnim(const CObjAnim* anim, f32 frame); // 0x802BA134
	void SetAnmScnRate(f32 rate);

	// the node's world matrix, NULL without a model. retail expands the same code in GetWorldMtx
	// (0x8001A3C0) and in GetWorldPos (0x80043B54), with the node copied first as an argument is
	MtxP GetNodeWorldMtx(nw4r::g3d::ResNode node) const
	{
		return mObject.m_pScnMdl != NULL ? mObject.m_pScnMdl->mpWorldMtxArray[node.GetMtxID()] : NULL;
	}

	s32 mLoadState;        // _08, 1 while the resource file is being read
	u32 mModel;            // _0C
	u32 mLoadFlags;        // _10
	CFileRequest mRequest; // _14
	CResource mResource;   // _2C
	TVec mMoveTranslate;   // _30
	CVec mMoveTarget0;     // _40
	CVec mMoveSpeed0;      // _50
	TVec mMoveRotate;      // _60
	CVec mMoveTarget1;     // _70
	CVec mMoveSpeed1;      // _80
	TVec mMoveScale;       // _90
	CVec mMoveTarget2;     // _A0
	CVec mMoveSpeed2;      // _B0
	s32 mAnimId;           // _C0
	f32 mAnimRate[4];      // _C4, the animation's rate and the one last handed on, then the same for the scene anm
	CColor mColor0;        // _D4, the two colors the materials blend in
	CColor mColor1;        // _DC
	u8 mColorsChanged;     // _E4, set until the next material callback hands them to the model
	NW_Obj mObject;        // _E8
	NW_Obj* mAnmSource;    // _144, a second file the anims are looked up in by name first, or NULL
	u8 mAnimFromSource;    // _148, the playing anim came from mAnmSource
	u8 _149[7];
};
STATIC_ASSERT(sizeof(CObj) == 0x150);

// a queued anim, the same record as WiiTitleObject's WiiAnimSlot
struct XObjAnimSlot {
	s32 mAnim; // _00, -1 when the slot is free
	s16 mA;    // _04
	s16 mB;    // _06
};
STATIC_ASSERT(sizeof(XObjAnimSlot) == 0x8);

// one floor's edge, the GameCube's XObjEdge
struct XObjEdge {
	Vec mContactOffset; // _00
	Vec mFrom;          // _0C
	Vec mTo;            // _18
};
STATIC_ASSERT(sizeof(XObjEdge) == 0x24);

class XObj : public CObj {
public:
	XObj() { } // everything is the member classes' own constructors; retail inlines this at every construction site
	// the trivial body: retail's aggregate destructors inline it to nothing, the
	// standalone __dt__4XObjFv at 0x8002D57C is the deleting variant
	virtual ~XObj() { }
	virtual void InitializeObject();                       // _14C, first of XObj's own virtuals; retail's out-of-line
	                                                       // definition keeps the GameCube u32 parameter, but callers
	                                                       // here emit no argument (the callee never reads it)
	virtual void ResetForNewGame();                        // _150
	virtual void ResetObjectState();                       // _154
	virtual BOOL LoadFromPlacement(const CoordInfo& info); // _158
	virtual void vf15C();
	virtual void vf160() { mLastAnimFrame = GetAnimFrame(); }
	virtual void UpdateObject(u32 frames); // _164
	virtual void vf168();
	virtual void vf16C(); // _16C, empty
	virtual void vf170();
	virtual void DrawObject();                                       // _174
	virtual void DrawWithView(Mtx view);                             // _178
	virtual void DrawReflection(Mtx view);                           // _17C
	virtual void DrawGroundShadow(f32 opacity);                      // _180
	virtual void DrawGroundShadowAt(const Vec* origin, f32 opacity); // _184
	virtual void DrawObjectEffects();                                // _188
	virtual void DrawCollisionShapes();                              // _18C
	virtual void DrawScreenOverlay();                                // _190
	virtual void DrawDebugStatus();                                  // _194
	virtual void vf198();
	virtual void vf19C(u32 color, f32 z, f32 x, f32 y); // _19C, CGame::DrawHud calls it on its mHudObject
	// _1A0, the GameCube's ApplyMaterialPresets: SYSCALL_SET_OBJSTAT's 0x20000 passes the object's material bits
	virtual void ApplyMaterialPresets(u32 flags[5]);
	virtual void SetDestinationAlpha(s32 alpha, s32 time); // _1A4, the GameCube's
	virtual void vf1A8();
	virtual void SetDiffuseColor(GXColor color); // _1AC, the item tint the carry code passes (the GameCube set the materials' diffuse)
	virtual Vec GetCameraTarget();               // _1B0, where a camera looks at this object
	virtual void SetBaseMtx(Mtx mtx);            // _1B4, keeps a copy of mtx and points mBaseMtxPtr at it
	virtual void ClearBaseMtx();                 // _1B8
	virtual void vf1BC();
	virtual void SetCullMode(s32 mode); // _1C0, the floor reflection draws with 1
	virtual void ResetCullMode();       // _1C4, and puts it back after
	virtual u32 TestFloorCollision(const Vec* from, const Vec* to, f32 radius, f32 height, Vec* push, Vec* contact, s32* cursor,
	                               s32 arg7);                         // _1C8, cursor walks the floor list between calls, -1 ends it
	virtual s32 OnPlayerContact(s32 unused, s32 value);               // _1CC
	virtual BOOL TestFloorCrossing(const Vec& start, const Vec& end); // _1D0
	virtual void ProjectOffsetToScreen(const Vec* offset, f32* x, f32* y, f32* z, CCamera* camera); // _1D4
	virtual f32 GetFloorXZExtent();                                                                 // _1D8
	virtual f32 GetCollisionHeight();                                                               // _1DC
	// _1E0, the object's bounds on screen under camera, the GameCube's; FALSE when it is out of view
	virtual BOOL ProjectVisibleBounds(f32* width, f32* height, CCamera* camera, f32 radius, f32 objectHeight);
	virtual BOOL IsOutsideCameraDepth(CCamera* camera);                  // _1E4, TRUE when the model lies outside the camera's depth range
	virtual s32 HandleEventCommand(s32 count, s32* args);                // _1E8
	virtual const void* GetEventScript();                                // _1EC
	virtual void SetColor(s32 blend, u8 r, u8 g, u8 b, f32 time);        // _1F0
	virtual void OnPushDisplacement(const Vec& displacement);            // _1F4
	virtual BOOL ReceivePlugPower(f32 amount);                           // _1F8
	virtual void OnHitFromPosition(const Vec& position, f32 strength);   // _1FC
	virtual void OnSquirterHit(const Vec* contact, s32 value, s32 tint); // _200
	virtual BOOL OnItemUsed(const Vec* position, s32 value);             // _204
	// _208, Update tells each live object every frame whether the pointer is on it, with the pointer's
	// motion and speed on the screen
	virtual s32 vf208(const Vec* pointer, const Vec* motion, u8 over, f32 speed);
	virtual void PauseSounds(f32 fade);                       // _20C, empty; PauseGameSounds calls it on every live object
	virtual void ResumeSounds(f32 fade);                      // _210, empty; ResumeGameSounds' counterpart
	virtual void OnMoveRequest();                             // _214, runs ahead of the three requests below
	virtual void RequestRotationY(f32 y);                     // _218
	virtual void RequestRotateY(f32 target, f32 speed);       // _21C
	virtual void RequestMoveDir(const Vec* dir, f32 speed);   // _220
	virtual BOOL HasModel();                                  // _224
	virtual nw4r::g3d::ResNode GetResNodeOrFirst(u32 index);  // _228, the first node when index is out of range
	virtual nw4r::g3d::ResNode FindResNode(const char* name); // _22C
	virtual CVec GetJointPos(nw4r::g3d::ResNode node);        // _230
	virtual CVec GetJointPos(const char* name);               // _234
	virtual CVec GetJointPos(u32 index);                      // _238
	virtual CVec GetJointWorldPos(const char* name);          // _23C
	virtual CVec GetJointWorldPos(u32 index);                 // _240
	virtual CVec* GetPos();                              // _244, &mMoveTranslate; the GameCube-named reading of this slot was GetPosition
	virtual CVec GetJointTranslate(const char* name);    // _248
	virtual CVec GetJointTranslate(u32 index);           // _24C
	virtual CVec GetJointScale(nw4r::g3d::ResNode node); // _250, the scale in the node's world matrix
	virtual void GetJointScale(nw4r::g3d::ResNode node, Vec* out); // _254
	virtual CVec* GetScale();                                      // _258
	virtual f32 GetPosX();                                         // _25C
	virtual f32 GetPosY();                                         // _260
	virtual f32 GetPosZ();                                         // _264
	virtual f32 GetRotX();                                         // _268
	virtual f32 GetRotY();                                         // _26C
	virtual f32 GetRotZ();                                         // _270
	virtual f32 GetScaleX();                                       // _274
	virtual f32 GetScaleY();                                       // _278
	virtual f32 GetScaleZ();                                       // _27C
	// _280, resolves this kind's parameter record for the passed ints (cjanim's helper
	// packs them into a u64 key, four bits per arg) and hands the record's node name to
	// GetResNode(name), or GetResNode(0) when there is none
	virtual nw4r::g3d::ResNode vf280(s32 count, s32* args);
	virtual nw4r::g3d::ResNode vf284(s32 count, s32* args); // _284, forwards to vf280
	// _288, the same lookup, then GetJointWorldPos(name), or GetJointWorldPos(0) without one
	virtual CVec vf288(s32 count, s32* args);
	// CObj's
	virtual void ExecCallbackA(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj); // _0C
	virtual void ExecCallbackB(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj); // _10
	virtual void Release();                                                                                                     // _40
	virtual nw4r::g3d::ResNode GetResNode(const char* name);                                                                    // _50
	virtual nw4r::g3d::ResNode GetResNode(u32 index);                                                                           // _54
	virtual nw4r::g3d::ResNode GetResNode();                                                                                    // _58
	virtual void SetAnim(s32 id, BOOL force, f32 frame, f32 rate, f32 blend, f32 blendRate);                                    // _108
	virtual void ClearAnim();                                                                                                   // _110
	virtual void ExecCallback_CALC_MAT(int timing, nw4r::g3d::ScnObj* obj, u32 param, void* info);                              // _148

	// empties the queue of anims waiting behind the current one
	void ClearAnimQueue()
	{
		mAnimHead = 0;
		for (s32 i = 0; i < 4; i++) {
			mAnimSlots[i].mAnim = -1;
		}
	}
	// queues an anim in the first free slot from the head on, dropped when all four are taken
	void PushAnim(s32 anim, s16 a, s16 b)
	{
		for (s32 i = 0; i < 4; i++) {
			XObjAnimSlot* slot = &mAnimSlots[(mAnimHead + i) % 4];
			if (slot->mAnim == -1) {
				slot->mAnim = anim;
				slot->mA    = a;
				slot->mB    = b;
				return;
			}
		}
	}

	// the object's kind, -1 without its data record
	s16 GetCategory() const
	{
		if (mAttr.mObjData != NULL) {
			return mAttr.mObjData->GetCategory();
		}
		return -1;
	}
	// (_9C SetScale(f32) scales all three axes alike, _11C GetAnimId returns mAnimId, _134 IsAnimDone is TRUE once
	// the next step passes the end frame; see the slot comments)
	// which pool of XOBJS the object's data record puts it in, -1 before it has one (the GameCube XObj's inline)
	s32 GetPool() const { return mAttr.mObjData != NULL ? GetObjDataPool(mAttr.mObjData) : -1; }
	// the object id of its placement, -1 without one (the GameCube XObj's inline)
	s32 GetKind() const { return mAttr.mPlacement != NULL ? mAttr.mPlacement->mObjectId : -1; }
	// the item its data record names, -1 without one (the GameCube XObj's inline)
	s16 GetItem() const { return mAttr.mObjData != NULL ? mAttr.mObjData->GetItem() : -1; }
	// the item index its data record names, -1 before it has one (the GameCube XObj's inline)
	s32 GetObjFlags() const { return mAttr.mObjData != NULL ? GetObjDataFlags(mAttr.mObjData) : -1; }

	BOOL IsFloorOn(s32 i) const { return mFloorMasks[0] & (1 << i); }
	void ClearFloor(s32 i) { mFloorMasks[0] &= ~(1 << i); }

	CStatus64 mStatus;  // _150
	CoordInfo mCoord;   // _160
	AttrInfo mAttr;     // _168
	XObjFloor* mFloors; // _174, the object's collision floor list
	u32 mFloorMasks[2]; // _178, a bit per floor, one mask per group (the GameCube's _250)
	s32 mStageIndex;    // _180, the object's slot in XOBJS's object table
	s32 mPoolIndex;     // _184, its slot in its pool
	u8 _188;            // _188, Update steps the object through vf16C alone while it is set and there is no base matrix
	u8 _189[0x18C - 0x189];
	s32 _18C; // _18C, Update lets the scope pick objects with it set (>= 0) and plays its sounds for them, -1 for none
	// _190, the animation event callback for the object's kind; AssignAnimEventCallback
	// (cjanim.cpp) sets it, the animation stepper calls it at the events' frames
	void (*mAnimEventCallback)(XObj* object, s32 kind, f32 prev, f32 frame);
	s32 mPlacementParam0; // _194, the placement entry's, copied on load; its meaning is up to the kind
	s32 mPlacementParam1; // _198
	CVec mLastPos;        // _19C
	u8 _1AC[0x1B8 - 0x1AC];
	f32 mLastAnimFrame;  // _1B8, the frame the animation had after the last update
	s32 mHitFloorIndex;  // _1BC
	XObjEdge mEdges[32]; // _1C0, one per floor
	// _640, the collision floors' reach on the ground plane: [0] the wider of the x and z radii,
	// [1] the narrower; UpdateObjectFloorXZExtent fills both with NAN while there are no floors
	f32 mFloorXZExtent[2];    // xobj_chr.cpp reads them by index, [0] is GetFloorXZExtent's
	f32 mFloorHeights[32];    // _648, a height per floor
	f32 mMaxFloorHeight;      // _6C8, NAN while there is none
	f32 mSelectedFloorHeight; // _6CC, highest active floor, NaN when none is active
	s32 mSelectedFloorIndex;  // _6D0
	BOOL mScreenBoundsValid;  // _6D4, the four below hold the object's bounds on screen
	s32 _6D8;                 // _6D8, vf208 stores the pointer's over flag here
	s32 _6DC;                 // _6DC, set when the pointer swept across the bounds within one frame
	f32 mScreenTop;           // _6E0
	f32 mScreenBottom;        // _6E4
	f32 mScreenLeft;          // _6E8
	f32 mScreenRight;         // _6EC
	f32 mFloorTop; // _6F0, the highest projected floor top
	u8 _6F4[0x6F8 - 0x6F4];
	CVec mToppleOffset; // _6F8
	// _708, the map triangle under the object and its height; cjanim's floor queries refresh it
	XObjMapHit mFloorHit;
	CVec mVec714; // _714, the base the object's floor list hangs from in its local frame (the GameCube's mToppleOffset role: the cjanim
	              // floor routines add the floors' center offsets to it)
	CEvt mEvt;    // _724
	const void* mEventScript;   // _790, the event script the object starts when used
	s32 mAnimHead;              // _794, the slot PushAnim probes first
	XObjAnimSlot mAnimSlots[4]; // _798, anims queued to play after the current one
	CAnmQueue mAnmQueue;        // _7B8
	CJArchive mArchive;         // _820
	Mtx mBaseMtx;               // _968, SetBaseMtx's copy
	MtxP mBaseMtxPtr;           // _998, NULL or mBaseMtx
	u8 _99C[0x9A4 - 0x99C];
	s32 _9A4; // _9A4, the end of a script it owns clears it
};
STATIC_ASSERT(sizeof(XObj) == 0x9A8);

// the base of the people and toys, the GameCube's members shifted by +0x28
class XObjActor : public XObj {
public:
	virtual void OnPushDisplacement(const Vec& displacement); // _1F4 override

	f32 mHeading;   // _9A8, the way it walks
	f32 mMoveSpeed; // _9AC, move speed
	u8 _9B0[0x9B8 - 0x9B0];
	s32 mKindStep;    // _9B8, the kind's own step
	u32 mKindCounter; // _9BC, cleared with mKindStep

	Vec GetCameraTarget();
};

// the plain object an XObjHBox carries
struct XObjHBoxPart {
	XObj mObj;              // _00
	f32 mHit;               // _9A8, set to 1 when chibi robo hits the box
	u8 _9AC[0x9B0 - 0x9AC]; // _9AC
	f32 mRange;             // _9B0
	s32 mSfx;               // _9B4, keyed off on a hit
};
STATIC_ASSERT(sizeof(XObjHBoxPart) == 0x9B8);

// a box that reads a second object (placement 36) and draws it inside itself
class XObjHBox : public XObj {
public:
	virtual void Release();                                            // _40 override
	virtual void InitializeObject();                                   // _14C override, fn_801164D8
	virtual void ResetForNewGame();                                    // _150 override, fn_80116518
	virtual void ResetObjectState();                                   // _154 override
	virtual BOOL LoadFromPlacement(const CoordInfo& info);             // _158 override
	virtual void UpdateObject(u32 frames);                             // _164 override
	virtual void DrawWithView(Mtx view);                               // _178 override
	virtual void DrawGroundShadowAt(const Vec* origin, f32 opacity);   // _184 override
	virtual void DrawScreenOverlay();                                  // _190 override
	virtual s32 OnPlayerContact(s32 unused, s32 flags);                // _1CC override
	virtual void OnHitFromPosition(const Vec& position, f32 strength); // _1FC override

	s32 mLoadState;     // _9A8
	f32 mRing;          // _9AC, draws the two rings while above 0
	XObjHBoxPart mPart; // _9B0
};
STATIC_ASSERT(sizeof(XObjHBox) == 0x1368);

// subclass pool types: ctor/dtor are defined in their own units, the tail pad is
// the part of the stride the class adds on top of XObj
#define WII_XOBJ_DECL(name, size)      \
	class name : public XObj {         \
	public:                            \
		name();                        \
		virtual ~name();               \
                                       \
	private:                           \
		u8 _tail[size - sizeof(XObj)]; \
	};                                 \
	STATIC_ASSERT(sizeof(name) == size)

// the same with the one virtual these classes add behind XObj's (_28C): CGame::DrawScene's last pass
// calls it on the leticker, army and spider objects, each through its own class
#define WII_XOBJ_DECL_LATE_DRAW(name, size) \
	class name : public XObj {              \
	public:                                 \
		name();                             \
		virtual ~name();                    \
		virtual void DrawLate(); /* _28C */ \
                                            \
	private:                                \
		u8 _tail[size - sizeof(XObj)];      \
	};                                      \
	STATIC_ASSERT(sizeof(name) == size)

class WiiGsSceneObj;

// the stage camera object, the first of pool 0: its scene (_9A8) brings the stage's own camera, lights and fog
class XObjScnCam : public XObj {
public:
	XObjScnCam();
	virtual ~XObjScnCam();

	WiiGsSceneObj* mScene; // _9A8
	Vec mAnimOffset;       // _9AC, zeroed when an event starts a scene camera animation
	f32 mAnimAngle;        // _9B8, DEG2RAD-scaled, zeroed with mAnimOffset
	u8 _9BC[0x9C0 - 0x9BC];
};
STATIC_ASSERT(sizeof(XObjScnCam) == 0x9C0);

// the light object: its dtor (0x800BE828) destroys CLight[8] at _9AC, the
// ctor (0x800BE364) is the XObj initialization plus that array
class XObjLight : public XObj {
public:
	XObjLight();
	virtual ~XObjLight();

private:
	u8 _9A8[4];
	CLight mLights[8]; // _9AC
	u8 _D0C[0x24];
};
STATIC_ASSERT(sizeof(XObjLight) == 0xD30);

// the map object: a word and a CTrigon behind the XObj members, then two
// 100-entry record lists the CPtcl unit's element types fill
class XObjMap : public XObj {
public:
	XObjMap();
	virtual ~XObjMap();

	u8 _9A8[4];
	CTrigon mTrigon; // _9AC
	u8 _9EC[0x28];
	XObjMapListA mListA[0x64]; // _A14
	XObjMapListB mListB[0x64]; // _EC4
	u8 _1374[0x84];
};
STATIC_ASSERT(sizeof(XObjMap) == 0x13F8);
WII_XOBJ_DECL(XObjChr, 0x1CCA0);
WII_XOBJ_DECL(XObjActor3, 0x21C8);
WII_XOBJ_DECL_LATE_DRAW(XObjLeticker, 0xAC8);
WII_XOBJ_DECL(XObjActorToy, 0x2208);
WII_XOBJ_DECL(XObjActorJenny, 0xD98);
WII_XOBJ_DECL(XObjActorMama, 0xD88);
WII_XOBJ_DECL(XObjActorPapa, 0xD90);
WII_XOBJ_DECL(XObjActorTao, 0xDF8);
WII_XOBJ_DECL(XObjActorDeca, 0xDA0);
WII_XOBJ_DECL_LATE_DRAW(XObjArmy, 0xB30);

// a carried model, drawn with the object 0x13C model the spider claimed a slot for (the GameCube's CarryInfo)
struct CarryInfo {
	CVec mPos;      // _00
	CVec _10;       // _10, zeroed with the rest, never read
	f32 mAngle;     // _20
	f32 _24;        // _24
	s32 mState;     // _28, its anim, 1 opens, 2 follows mObj's joint, 3 closes
	u32 _2C;        // _2C
	CStatus mFlags; // _30, 1 drawn, 2 the robot was hit with it
	XObj* mObj;     // _38, the object whose joint it follows
};
STATIC_ASSERT(sizeof(CarryInfo) == 0x3C);

// an object an event tied to one of the spider's nodes
struct SpiderTie {
	nw4r::g3d::ResNode mJoint; // _00
	XObj* mObj;                // _04
	Mtx mMtx;                  // _08
};
STATIC_ASSERT(sizeof(SpiderTie) == 0x38);

// the spiders: the GameCube class (PAL layout) behind the Wii XObj, with the Wii AnimSfx and one word
// more after mRumbleTimer
class XObjSpider : public XObj {
public:
	XObjSpider();
	// inline: retail emits the vtable and its thunks with ExecCallbackB in xobjspider.cpp
	virtual ~XObjSpider() { }
	virtual void ExecCallbackB(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj); // _10
	virtual void SetAnim(s32 id, BOOL force, f32 frame, f32 rate, f32 blend, f32 blendRate);                                    // _108
	virtual void InitializeObject();                                                                                            // _14C
	virtual void ResetForNewGame();                                                                                             // _150
	virtual void ResetObjectState();                                                                                            // _154
	virtual BOOL LoadFromPlacement(const CoordInfo& info);                                                                      // _158
	virtual void UpdateObject(u32 frames);                                                                                      // _164
	virtual void DrawObject();                                                                                                  // _174
	virtual void DrawGroundShadow(f32 opacity);                                                                                 // _180
	virtual void DrawGroundShadowAt(const Vec* origin, f32 opacity);                                                            // _184
	virtual void DrawObjectEffects();                                                                                           // _188
	virtual void DrawScreenOverlay();                                                                                           // _190
	virtual void DrawDebugStatus();                                                                                             // _194
	virtual s32 OnPlayerContact(s32 unused, s32 value);                                                                         // _1CC
	virtual s32 HandleEventCommand(s32 count, s32* args);                                                                       // _1E8
	virtual void OnHitFromPosition(const Vec& from, f32 strength);                                                              // _1FC
	virtual void PauseSounds(f32 fade) { mSfx.Pause(fade); }                                                                    // _20C
	virtual void ResumeSounds(f32 fade) { mSfx.Resume(fade); }                                                                  // _210
	virtual void DrawLate();              // _28C, CGame::DrawScene's last pass
	virtual void UpdateAnimationSounds(); // _290, plays the sounds along the anim

	// the states, Update calls them by number
	void State0(u32 frames);
	void State1(u32 frames);
	void State2(u32 frames);
	void State3(u32 frames);
	void State4(u32 frames);
	void State5(u32 frames);
	void State6(u32 frames);
	void State7(u32 frames);
	void State8(u32 frames);
	void State9(u32 frames);

	void SetState(s32 state)
	{
		mPhase     = 0;
		mCounter   = 0;
		mPrevState = mState;
		mState     = state;
	}

	f32 mDir;                // _9A8, the way it walks
	f32 mSpeed;              // _9AC
	f32 mFacing;             // _9B0, eased toward mDir while it turns
	f32 _9B4;                // _9B4
	f32 mTurnSpeed;          // _9B8
	f32 mSavedDir;           // _9BC
	f32 mThreadAlpha;        // _9C0, 0 to 1
	f32 mRadius;             // _9C4, shadow size, also kept off the area edges
	f32 mFallSpeed;          // _9C8
	s32 mState;              // _9CC
	s32 mPhase;              // _9D0
	s32 mCounter;            // _9D4
	CStatus mFlags;          // _9D8
	s32 mPrevState;          // _9E0
	f32 mAnimStartFrame;     // _9E4, the frame SetAnim started the anim at
	s32 mWait;               // _9E8
	f32 _9EC;                // _9EC
	f32 _9F0;                // _9F0
	Vec mHitPos;             // _9F4, where it was hit from
	Vec mHome;               // _A00, y is the floor
	f32 mHitStrength;        // _A0C
	s32 mWall;               // _A10, 2 to 5 while it walks on a wall, on 3 it carries sideways
	s32 mHits;               // _A14, hits left
	s32 mMaxHits;            // _A18
	Vec mWallPos;            // _A1C
	Vec mTarget;             // _A28, where it walks to
	Vec mShotOrigin;         // _A34
	Vec mShotPosition;       // _A40
	u8 _A4C[0xA58 - 0xA4C];  // _A4C
	Vec mShotTo;             // _A58, where the carried model is shot at
	f32 mShotRange;          // _A64
	u8 _A68[0xA6C - 0xA68];  // _A68
	f32 mShotProgress;       // _A6C
	s32 mShotState;          // _A70
	s32 _A74;                // _A74
	Vec mAreaPos;            // _A78, corner of the area it stays in
	f32 mAreaWidth;          // _A84
	f32 mAreaDepth;          // _A88
	Vec mStartPos;           // _A8C
	f32 _A98;                // _A98
	s32 mCommand;            // _A9C, from the event scripts, 0 once handled
	s32 mArgs[5];            // _AA0
	u32 _AB4;                // _AB4
	CarryInfo mCarry;        // _AB8
	CarryInfo mSlotCarry;    // _AF4, mObj is the slot's model
	u8 mSlot;                // _B30, 255 for none
	u8 _B31[0xB3C - 0xB31];  // _B31
	s16 _B3C;                // _B3C
	s32 _B40[4];             // _B40
	s32 _B50[3];             // _B50
	u8 _B5C[0xB7C - 0xB5C];  // _B5C
	s16 _B7C;                // _B7C
	s32 _B80;                // _B80
	s16 mEffectId;           // _B84, a stage object it holds on its node
	XObj* mEffect;           // _B88
	s32 mEffectOn;           // _B8C
	Mtx mEffectMtx;          // _B90
	s32 mHitState;           // _BC0
	s32 mHitTimer;           // _BC4
	s32 mDroppedSpiderIndex; // _BC8, FindNearbyDroppedSpider's pool index
	s32 mRumble;             // _BCC
	s32 mRumbleTimer;        // _BD0
	s32 mSoundDelay;         // _BD4, frames after the load before its anim sounds play, half a second
	AnimSfx mSfx;            // _BD8
	SpiderTie mTies[8];      // _C34
	u8 _DF4[4];              // _DF4
};
STATIC_ASSERT(sizeof(XObjSpider) == 0xDF8);

// the fx_spider_01 a small spider carries around (the GameCube's XObjSpiderSmallFx)
struct XObjSpiderSmallFx {
	CVec mPos;       // _00
	CVec _10;        // _10
	f32 mRotY;       // _20
	f32 _24;         // _24
	u32 mState;      // _28, its anim, 1 and 3 play one, 10 puts it away
	s32 _2C;         // _2C
	CStatus mStatus; // _30, 1 shown
};
STATIC_ASSERT(sizeof(XObjSpiderSmallFx) == 0x38);

// the small spiders, npc_spider_2: the GameCube class behind the Wii XObj, with the Wii AnimSfx and one word
// more in front of it
class XObjSpiderSmall : public XObj {
public:
	XObjSpiderSmall();
	// inline: retail emits the vtable and its thunks in xobjspidersmall.cpp
	virtual ~XObjSpiderSmall() { }
	virtual void Release();                                            // _40
	virtual void InitializeObject();                                   // _14C
	virtual void ResetForNewGame();                                    // _150
	virtual void ResetObjectState();                                   // _154
	virtual BOOL LoadFromPlacement(const CoordInfo& info);             // _158
	virtual void UpdateObject(u32 frames);                             // _164
	virtual void DrawObject();                                         // _174
	virtual void DrawGroundShadow(f32 opacity);                        // _180
	virtual void DrawGroundShadowAt(const Vec* origin, f32 opacity);   // _184
	virtual void DrawObjectEffects();                                  // _188
	virtual void DrawScreenOverlay();                                  // _190
	virtual void DrawDebugStatus();                                    // _194
	virtual s32 OnPlayerContact(s32 unused, s32 value);                // _1CC
	virtual s32 HandleEventCommand(s32 count, s32* args);              // _1E8
	virtual void OnHitFromPosition(const Vec& position, f32 strength); // _1FC
	virtual void PauseSounds(f32 fade) { mAnimSfx.Pause(fade); }       // _20C
	virtual void ResumeSounds(f32 fade) { mAnimSfx.Resume(fade); }     // _210
	virtual void DrawLate();                                           // _28C, CGame::DrawScene's last pass
	virtual void UpdateAnimationSounds();                              // _290

	void State0(u32 frames);
	void State1(u32 frames);
	void State2(u32 frames);
	void State3(u32 frames);
	void State4(u32 frames);
	void State5(u32 frames);
	void State6(u32 frames);
	void State7(u32 frames);
	void State8(u32 frames);
	void State9(u32 frames);

	void FindWebs(); // collects the stage's webs, the first spider's job
	void TakeWeb();  // claims the first free web

	void SetState(s32 state)
	{
		mStep      = 0;
		mTimer     = 0;
		mPrevState = mState;
		mState     = state;
	}

	f32 mWalkHeading;       // _9A8, the way it walks
	f32 mWalkSpeed;         // _9AC
	f32 _9B0;               // _9B0
	f32 mTurnSpeed;         // _9B4
	u8 _9B8[0x9BC - 0x9B8]; // _9B8
	f32 _9BC;               // _9BC
	f32 mRadius;            // _9C0
	f32 mFallSpeed;         // _9C4
	f32 mScale;             // _9C8
	f32 mTargetScale;       // _9CC, the scale it grows to
	s32 mState;             // _9D0
	s32 mStep;              // _9D4
	s32 mTimer;             // _9D8
	CStatus mSpiderStatus;  // _9DC, 1 hit, 2 knocked over, 0x40 shadow, 0x800000 grabbed
	s32 mPrevState;         // _9E4
	u32 _9E8;               // _9E8
	f32 _9EC;               // _9EC
	f32 _9F0;               // _9F0
	Vec mHitPos;            // _9F4
	Vec mHome;              // _A00
	f32 mHitStrength;       // _A0C
	s32 mHitsLeft;          // _A10
	s32 mMaxHits;           // _A14
	Vec _A18;               // _A18
	Vec mWalkTarget;        // _A24, where it walks to
	Vec mAreaPos;           // _A30, corner of the area it walks in
	f32 mAreaWidth;         // _A3C
	f32 mAreaDepth;         // _A40
	Vec mTargetPos;         // _A44
	f32 mTargetRadius;      // _A50
	s32 mStage;             // _A54
	s32 mEvtArgs[6];        // _A58, from HandleEventCommand
	u32 _A70;               // _A70
	XObjSpiderSmallFx mFx;  // _A74
	XObj* mFxObj;           // _AAC
	u8 mFxIndex;            // _AB0, 255 for none
	u8 _AB1[0xAB4 - 0xAB1]; // _AB1
	f32 mClingAngle;        // _AB4, the angle it sits at around the robot
	f32 mShakeStrength;     // _AB8
	s32 mDrainTimer;        // _ABC, frames to the next battery drain
	f32 mHoldTime;          // _AC0, how long it holds on
	Vec mClingOffset;       // _AC4
	u8 _AD0;                // _AD0
	u8 _AD1[0xAD4 - 0xAD1]; // _AD1
	s32 mPrevBgm;           // _AD4, the bgm it took over from
	u8 mPrevBgmVolume;      // _AD8
	u8 _AD9[0xADC - 0xAD9]; // _AD9
	s32 mLoadPoolIndex;     // _ADC
	s32 mTimeMode;          // _AE0
	s32 mAreaGraceTimer;    // _AE4
	s32 mWarpsLeft;         // _AE8
	s32 mBattleBgmState;    // _AEC
	u32 _AF0;               // _AF0
	AnimSfx mAnimSfx;       // _AF4
};
STATIC_ASSERT(sizeof(XObjSpiderSmall) == 0xB50);

// the big spider, npc_spider_3: the GameCube class behind the Wii XObj with the Wii AnimSfx
class XObjSpiderBoss : public XObj {
public:
	XObjSpiderBoss();
	// inline: retail emits the vtable and its thunks in xobjspiderboss.cpp
	virtual ~XObjSpiderBoss() { }
	virtual void InitializeObject();                                   // _14C
	virtual void ResetForNewGame();                                    // _150
	virtual void ResetObjectState();                                   // _154
	virtual BOOL LoadFromPlacement(const CoordInfo& info);             // _158
	virtual void UpdateObject(u32 frames);                             // _164
	virtual void DrawObject();                                         // _174
	virtual void DrawGroundShadow(f32 opacity);                        // _180
	virtual void DrawGroundShadowAt(const Vec* origin, f32 opacity);   // _184
	virtual void DrawObjectEffects();                                  // _188
	virtual void DrawScreenOverlay();                                  // _190
	virtual void DrawDebugStatus();                                    // _194
	virtual s32 OnPlayerContact(s32 unused, s32 value);                // _1CC
	virtual s32 HandleEventCommand(s32 count, s32* args);              // _1E8
	virtual void OnHitFromPosition(const Vec& position, f32 strength); // _1FC
	virtual void PauseSounds(f32 fade) { mAnimSfx.Pause(fade); }       // _20C
	virtual void ResumeSounds(f32 fade) { mAnimSfx.Resume(fade); }     // _210
	virtual void UpdateAnimationSounds();                              // _28C

	void State0(u32 frames);
	void State1(u32 frames);
	void State2(u32 frames);
	void State3(u32 frames);
	void State4(u32 frames);

	void SetState(s32 state)
	{
		mStep      = 0;
		mTimer     = 0;
		mPrevState = mState;
		mState     = state;
	}

	f32 mHeading;             // _9A8
	f32 mSpeed;               // _9AC
	f32 mTargetRotY;          // _9B0
	f32 mRotSpeed;            // _9B4
	u8 _9B8[0x9BC - 0x9B8];   // _9B8
	f32 _9BC;                 // _9BC
	f32 mRadius;              // _9C0
	f32 mSpeedY;              // _9C4
	f32 _9C8;                 // _9C8
	f32 _9CC;                 // _9CC
	s32 mState;               // _9D0
	s32 mStep;                // _9D4
	s32 mTimer;               // _9D8
	CStatus mSpiderStatus;    // _9DC, 1 hit, 2 knocked over, 8 airborne, 0x2000000 frozen
	s32 mPrevState;           // _9E4
	u32 mActionTimer;         // _9E8
	f32 _9EC;                 // _9EC
	f32 _9F0;                 // _9F0
	Vec mHitPos;              // _9F4
	Vec mBasePos;             // _A00
	f32 mHitStrength;         // _A0C
	s32 mHits;                // _A10
	s32 mMaxHits;             // _A14
	Vec mPatrolTarget;        // _A18
	Vec mAreaPos;             // _A24
	f32 mAreaWidth;           // _A30
	f32 mAreaDepth;           // _A34
	Vec mStartPos;            // _A38
	f32 _A44;                 // _A44
	s32 mPatrolSelection;     // _A48
	s32 mAltAnimId;           // _A4C, the anim the sounds follow while 0x40000000 is set
	s32 mGrabCooldown;        // _A50
	s32 mCommand;             // _A54
	s32 mArgs[5];             // _A58
	u32 _A6C;                 // _A6C
	f32 _A70;                 // _A70
	f32 mEscapeInputStrength; // _A74
	s32 mDrainTimer;          // _A78
	f32 mEscapeRemaining;     // _A7C
	Vec _A80;                 // _A80
	s32 mGrabIntervalFrames;  // _A8C
	u32 mDamageStep;          // _A90
	s32 mBlinkTimer;          // _A94
	u8 _A98[0xABD - 0xA98];   // _A98
	u8 mPatrolEffectActive;   // _ABD
	u8 _ABE[0xAC0 - 0xABE];   // _ABE
	AnimSfx mAnimSfx;         // _AC0
	SpiderTie mParts[8];      // _B1C, the GameCube's XObjJointPart, the same shape
	u8 _CDC[4];               // _CDC
};
STATIC_ASSERT(sizeof(XObjSpiderBoss) == 0xCE0);
WII_XOBJ_DECL(XObjFree, 0x9B8);

// a small shadow helper the unit defines; only its trivial dtor is verified so
// far (retail 0x800BD66C, the deleting variant with no member calls)
class CustomShadow {
public:
	virtual ~CustomShadow();
};

// the Wii object readers: retail RTTI names XObjRead, XObjReadAll and
// XObjReadOne, with XObjRead deriving straight from the SDK OSThread (the
// GameCube used its polymorphic CThread). The stack pointer and size are
// XObjRead's own members in front of the virtuals, so the vtable lands at
// _320. The Wii knows only the all-list and single readers, no XObjReadAnm.

// an OSMessageQueue with its message buffer; RTTI shows it derives from
// OSMessageQueue (the GameCube header wraps it as a member, same layout)
class CMsgQueue : public OSMessageQueue {
public:
	// _00-_1F OSMessageQueue
	OSMessage* mMsgs; // _20
	// _24 vtable

	virtual ~CMsgQueue();               // _08
	virtual void Initialize(s32 count); // _0C, allocates mMsgs and inits the queue
};
STATIC_ASSERT(sizeof(CMsgQueue) == 0x28);

inline CMsgQueue::~CMsgQueue()
{
}

// one read waiting in a reader
struct XObjReadReq {
	void* mObj;  // _00, null when the slot is free
	void* mInfo; // _04
};

class XObjRead : public OSThread {
public:
	u8* mStack;     // _318
	u32 mStackSize; // _31C
	// _320 vtable

	XObjRead()
	    : mStack(NULL)
	{
	}

	virtual ~XObjRead();                                                                           // _08
	virtual BOOL Create(void* (*func)(void*), void* param, u32 stackSize, s32 priority, u16 attr); // _0C
	virtual BOOL QueueRead(void* object, void* info);                                              // _10
	virtual void RunRequestLoop();                                                                 // _14
	virtual void ProcessRequest(XObjReadReq* req);                                                 // _18
	virtual BOOL HasPendingReads();                                                                // _1C

	// puts one read on the queue; both QueueReads and SpawnObjectFromPlacement inline it
	BOOL ReadStart(void* obj, void* info);

	CMsgQueue mQueue;     // _324
	XObjReadReq mReqs[8]; // _34C
	u8 _38C[0x14];        // tail past the requests, not touched by any retail function
};
STATIC_ASSERT(sizeof(XObjRead) == 0x3A0);
STATIC_ASSERT(offsetof(XObjRead, mQueue) == 0x324);
STATIC_ASSERT(offsetof(XObjRead, mReqs) == 0x34C);

inline XObjRead::~XObjRead()
{
}

// reads a whole object list; its destructor is defined in xaction.cpp, which anchors the vtable
// there (retail folds the two reader vptrs off one base register in xaction's __sinit)
class XObjReadAll : public XObjRead {
public:
	virtual ~XObjReadAll();                        // _08
	virtual void ProcessRequest(XObjReadReq* req); // _18
};

// reads one object
class XObjReadOne : public XObjRead {
public:
	virtual ~XObjReadOne();                           // _08
	virtual BOOL QueueRead(void* object, void* info); // _10
	virtual void ProcessRequest(XObjReadReq* req);    // _18
};

extern "C" {
void InitializeObjectPools(XOBJS* list);
void* ReadObjectListThread(void* arg);
void* ReadSingleObjectThread(void* arg);
BOOL HasPendingObjectReads();
extern XObjReadAll gAllObjectsReader;
extern XObjReadOne gSingleObjectReader;
}

#endif
