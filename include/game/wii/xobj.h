#ifndef GAME_WII_XOBJ_H
#define GAME_WII_XOBJ_H

#include "types.h"

#include "Dolphin/gx.h"
#include "Dolphin/mtx.h"

#include "game/cfile.h"
#include "game/coordinfo.h"
#include "game/cstatus64.h"
#include "game/ctrigon.h"
#include "game/evt.h"
#include "game/ut.h"
#include "game/wii/cgs.h"
#include "game/wii/cresfile.h"
#include "game/wii/xobjs.h"
#include "revolution/mem.h"

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
	virtual void ExecCallbackB();                                                              // _10
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

// the scene animation (lights, fog, cameras)
class AnmScn : public G3dObj {
public:
	virtual void SetFrame(f32 frame);     // _1C
	virtual f32 GetFrame() const;         // _20
	virtual void SetUpdateRate(f32 rate); // _24
};

class AnmScnRes : public AnmScn {
public:
	static AnmScnRes* Construct(MEMAllocator* pAllocator, u32* pSize, ResAnmScn scn, bool cache);

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
class CAnmQueue {
public:
	virtual ~CAnmQueue() { }

private:
	u8 _04[0x64];
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
// fn_800BEBDC/fn_800BEBF0). their real names and members are not recovered yet
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
	virtual void ExecCallbackB();                                                                                               // _10
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

// one floor's edge, the GameCube's XObjEdge
struct XObjEdge {
	Vec mContactOffset; // _00
	Vec mFrom;          // _0C
	Vec mTo;            // _18
};
STATIC_ASSERT(sizeof(XObjEdge) == 0x24);

class XObj : public CObj {
public:
	XObj();
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
	virtual void vf1A0();
	virtual void vf1A4();
	virtual void vf1A8();
	virtual void vf1AC();
	virtual Vec GetCameraTarget();    // _1B0, where a camera looks at this object
	virtual void SetBaseMtx(Mtx mtx); // _1B4, keeps a copy of mtx and points mBaseMtxPtr at it
	virtual void ClearBaseMtx();      // _1B8
	virtual void vf1BC();
	virtual void SetCullMode(s32 mode); // _1C0, the floor reflection draws with 1
	virtual void ResetCullMode();       // _1C4, and puts it back after
	virtual u32 TestFloorCollision(const Vec* from, const Vec* to, f32 radius, f32 height, Vec* push, Vec* contact, s32 arg6,
	                               s32 arg7);                         // _1C8
	virtual s32 OnPlayerContact(s32 unused, s32 value);               // _1CC
	virtual BOOL TestFloorCrossing(const Vec& start, const Vec& end); // _1D0
	virtual void vf1D4();
	virtual f32 GetFloorXZExtent();   // _1D8
	virtual f32 GetCollisionHeight(); // _1DC
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
	virtual void vf208(const Vec* pointer, const Vec* motion, BOOL over, f32 speed);
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
	virtual void vf280();
	virtual void vf284(); // _284, calls vf280
	virtual void vf288();
	// CObj's
	virtual void ExecCallbackA(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj); // _0C
	virtual void ExecCallbackB();                                                                                               // _10
	virtual void Release();                                                                                                     // _40
	virtual nw4r::g3d::ResNode GetResNode(const char* name);                                                                    // _50
	virtual nw4r::g3d::ResNode GetResNode(u32 index);                                                                           // _54
	virtual nw4r::g3d::ResNode GetResNode();                                                                                    // _58
	virtual void SetAnim(s32 id, BOOL force, f32 frame, f32 rate, f32 blend, f32 blendRate);                                    // _108
	virtual void ClearAnim();                                                                                                   // _110
	virtual void ExecCallback_CALC_MAT(int timing, nw4r::g3d::ScnObj* obj, u32 param, void* info);                              // _148

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
	// the item index its data record names, -1 before it has one (the GameCube XObj's inline)
	s32 GetObjFlags() const { return mAttr.mObjData != NULL ? GetObjDataFlags(mAttr.mObjData) : -1; }

	CStatus64 mStatus;  // _150
	CoordInfo mCoord;   // _160
	AttrInfo mAttr;     // _168
	XObjFloor* mFloors; // _174, the object's collision floor list
	u8 _178[0x188 - 0x178];
	u8 _188; // _188, Update steps the object through vf16C alone while it is set and there is no base matrix
	u8 _189[0x18C - 0x189];
	s32 _18C; // _18C, Update lets the scope pick objects with it set (>= 0) and plays its sounds for them, -1 for none
	u8 _190[0x194 - 0x190];
	s32 mPlacementParam0; // _194, the placement entry's, copied on load; its meaning is up to the kind
	s32 mPlacementParam1; // _198
	CVec mLastPos;        // _19C
	u8 _1AC[0x1B8 - 0x1AC];
	f32 mLastAnimFrame;  // _1B8, the frame the animation had after the last update
	s32 mHitFloorIndex;  // _1BC
	XObjEdge mEdges[32]; // _1C0, one per floor
	f32 mFloorXZExtent;  // _640
	u8 _644[0x648 - 0x644];
	f32 mFloorHeights[32];    // _648, a height per floor
	f32 mMaxFloorHeight;      // _6C8, NAN while there is none
	f32 mSelectedFloorHeight; // _6CC, highest active floor, NaN when none is active
	s32 mSelectedFloorIndex;  // _6D0
	BOOL mScreenBoundsValid;  // _6D4, the four below hold the object's bounds on screen
	u8 _6D8[4];
	s32 _6DC;          // _6DC, set when the pointer swept across the bounds within one frame
	f32 mScreenTop;    // _6E0
	f32 mScreenBottom; // _6E4
	f32 mScreenLeft;   // _6E8
	f32 mScreenRight;  // _6EC
	u8 _6F0[0x6F8 - 0x6F0];
	CVec mToppleOffset; // _6F8
	u8 _708[0xC];
	CVec mVec714;             // _714
	CEvt mEvt;                // _724
	const void* mEventScript; // _790, the event script the object starts when used
	u8 _794[0x24];
	CAnmQueue mAnmQueue; // _7B8
	CJArchive mArchive;  // _820
	Mtx mBaseMtx;        // _968, SetBaseMtx's copy
	MtxP mBaseMtxPtr;    // _998, NULL or mBaseMtx
	u8 _99C[0xC];
};
STATIC_ASSERT(sizeof(XObj) == 0x9A8);

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
	u8 _9AC[0x9C0 - 0x9AC];
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

private:
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
WII_XOBJ_DECL(XObjSound, 0xA10);
WII_XOBJ_DECL(XObjCoin, 0xA30);
WII_XOBJ_DECL(XObjItem, 0x9B0);
WII_XOBJ_DECL_LATE_DRAW(XObjLeticker, 0xAC8);
WII_XOBJ_DECL(XObjActorToy, 0x2208);
WII_XOBJ_DECL(XObjActorJenny, 0xD98);
WII_XOBJ_DECL(XObjActorMama, 0xD88);
WII_XOBJ_DECL(XObjActorPapa, 0xD90);
WII_XOBJ_DECL(XObjActorTao, 0xDF8);
WII_XOBJ_DECL(XObjActorDeca, 0xDA0);
WII_XOBJ_DECL_LATE_DRAW(XObjArmy, 0xB30);
WII_XOBJ_DECL_LATE_DRAW(XObjSpider, 0xDF8);
WII_XOBJ_DECL_LATE_DRAW(XObjSpiderSmall, 0xB50);
WII_XOBJ_DECL(XObjSpiderBoss, 0xCE0);
WII_XOBJ_DECL(XObjDust, 0xA00);
WII_XOBJ_DECL(XObjHBox, 0x1368);
WII_XOBJ_DECL(XObjFree, 0x9B8);

// a small shadow helper the unit defines; only its trivial dtor is verified so
// far (retail 0x800BD66C, the deleting variant with no member calls)
class CustomShadow {
public:
	virtual ~CustomShadow();
};

// the Wii object readers: a thread-like prefix of 0x320 bytes (an OSThread plus
// the stack fields, like the GameCube CThread), then the vtable at _320. The
// queue and request members behind it are not yet recovered; the Wii knows only
// the all-list and single readers, no XObjReadAnm.
class XObjRead {
	u8 _000[0x320];
	// _320 vtable

public:
	virtual ~XObjRead() { }                                                                        // _08
	virtual BOOL Create(void* (*func)(void*), void* param, u32 stackSize, s32 priority, u16 attr); // _0C

private:
	u8 _324[0x3A0 - 0x324]; // vtable pad + CMsgQueue + requests, not yet recovered
};
STATIC_ASSERT(sizeof(XObjRead) == 0x3A0);

class XObjReadAll : public XObjRead { };
class XObjReadOne : public XObjRead { };

extern "C" {
void InitializeObjectPools(XOBJS* list);
void* ReadObjectListThread(void* arg);
void* ReadSingleObjectThread(void* arg);
extern XObjReadAll gAllObjectsReader;
extern XObjReadOne gSingleObjectReader;
}

#endif
