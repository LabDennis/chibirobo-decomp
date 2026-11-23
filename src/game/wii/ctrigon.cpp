// The Wii's map floor collision, .text 0x802BA4F4-0x802BD294 (its static initializer is
// __sinit_\ctrigon_cpp). Unlike the GameCube file, whose functions sit in reverse, the Wii
// keeps them in source order, and the two selector switches stay functions of their own.
// The readers are global functions as in the GameCube map (no local scope there either).
// NONMATCHING: the NW4R shape parser and grid builder are reconstructed; stack lifetimes
// and assertion ordering still differ. TrigonGetVtxS8 differs from retail in register choice.
#include "rvl_msl/PowerPC_EABI_Support/MSL_C/MSL_Common/math_api.h"
#include "game/ctrigon.h"
#include "nw4r/g3d/g3d_scnmdl.h"
#include "game/mm.h"
#include "game/ut.h"
#include "Dolphin/gx.h"
#include <float.h>
#include <math.h>

extern "C" void PSMTXMultVecArray(const Mtx m, const Vec* srcBase, Vec* dstBase, u32 count);

// what the parser met in the display lists, one count per kind; Create clears it and
// names the pool after it
class _trigonCounter {
public:
	u32 mCount[11]; // _00
	// _2C vtable

	virtual ~_trigonCounter() { } // _08
};

static _trigonCounter sCounter;

struct ResDicNodeData {
	u16 ref;
	u16 flag;
	u16 idxLeft;
	u16 idxRight;
	s32 ofsString;
	s32 ofsData;
};

struct ResDicData {
	u32 size;
	u32 numData;
	ResDicNodeData data[1];
};

class ResDic {
public:
	void* mpData;

	ResDic(void* p = NULL)
	    : mpData(p)
	{
		if ((u32)p & 0x3) {
			nw4r::db::Panic("g3d_resdict_ac.h", 84, "NW4R:Failed assertion !((u32)p & 0x3)");
		}
	}
	void* operator[](const char* pName) const;
};

struct ResUserDataItemData {
	enum DataType { S32 };

	u32 nameID;
	s32 ofsData;
	u32 numData;
	u32 valueType;
};

class ResUserDataItem {
public:
	ResUserDataItemData* mpData;

	ResUserDataItem(void* p = NULL)
	    : mpData((ResUserDataItemData*)p)
	{
		if ((u32)p & 0x3) {
			nw4r::db::Panic("g3d_resuser_ac.h", 38, "NW4R:Failed assertion !((u32)p & 0x3)");
		}
	}
	bool IsValid() const { return mpData != NULL; }
	const ResUserDataItemData& ref() const
	{
		if (!IsValid()) {
			nw4r::db::Panic("g3d_resuser_ac.h", 38, "%s::%s: Object not valid.", "ResUserDataItem", "ref");
		}
		return *mpData;
	}
	u32 GetValueType() const { return ref().valueType; }
	s32 GetS32(u32 idx) const
	{
		if (GetValueType() != ResUserDataItemData::S32) {
			nw4r::db::Panic("g3d_resuser_ac.h", 54, "NW4R:Failed assertion GetValueType() == ResUserDataItemData::S32");
		}
		const s32* data = ref().ofsData != 0 ? (const s32*)((const u8*)mpData + ref().ofsData) : NULL;
		return data[idx];
	}
};

struct ResUserDataData {
	u32 size;
	ResDicData dic;
};

class ResUserData {
public:
	ResUserDataData* mpData;

	ResUserData(void* p = NULL)
	    : mpData((ResUserDataData*)p)
	{
	}
	bool IsValid() const { return mpData != NULL; }
	ResUserDataData& ref()
	{
		if (!IsValid()) {
			nw4r::db::Panic("g3d_resuser_ac.h", 87, "%s::%s: Object not valid.", "ResUserData", "ref");
		}
		return *mpData;
	}
	ResDic GetResDic() { return ResDic(&ref().dic); }
};

// returns the position array entry the index at dl points at; the 16 and 32 bit readers
// step by the whole vertex (6 or 12 bytes), the byte readers by three
typedef void* (*TrigonGetIdxFunc)(void* base, u8* dl);

extern "C" {
void TrigonGetVtxF32(const TRIGON_INFO* tri, Vec* out);
void TrigonGetVtxS16(const TRIGON_INFO* tri, Vec* out);
void TrigonGetVtxU16(const TRIGON_INFO* tri, Vec* out);
void TrigonGetVtxS8(const TRIGON_INFO* tri, Vec* out);
void TrigonGetVtxU8(const TRIGON_INFO* tri, Vec* out);
void* TrigonGetIdxF32U8(void* base, u8* dl);
void* TrigonGetIdxF32U16(void* base, u8* dl);
void* TrigonGetIdxS16U8(void* base, u8* dl);
void* TrigonGetIdxS16U16(void* base, u8* dl);
void* TrigonGetIdxU16U8(void* base, u8* dl);
void* TrigonGetIdxU16U16(void* base, u8* dl);
void* TrigonGetIdxS8U8(void* base, u8* dl);
void* TrigonGetIdxS8U16(void* base, u8* dl);
void* TrigonGetIdxU8U8(void* base, u8* dl);
void* TrigonGetIdxU8U16(void* base, u8* dl);
void fn_802BA678(CTrigon* trigon, nw4r::g3d::ScnMdl* model, nw4r::math::MTX34* worldMtx);
}

/**
 * @note Address: 0x802BA4F4
 * @note Size: 0x184
 */
// CTrigon::Create: reserve enough triangles for every polygon, then let the parser fill the pool and grid.
// NONMATCHING: retail's NW4R assertion strings and CTrigon allocation strings are separate literal objects;
// this unit's literals share @stringBase0, changing pool references and saving r27 instead of r26.
extern "C" TRIGON_INFO* fn_802BA4F4(CTrigon* trigon, nw4r::g3d::ScnMdl* model)
{
	nw4r::g3d::ResMdl mdl = model->GetResMdl();
	trigon->mNumMax       = 0;
	for (int i = 0; i < mdl.GetResShpNumEntries(); i++) {
		nw4r::g3d::ResShp shp = mdl.GetResShp(i);
		if (shp.ptr() == NULL) {
			nw4r::db::Panic("g3d_resshp_ac.h", 0x3A, "%s::%s: Object not valid.", "ResShp", "ref");
		}
		trigon->mNumMax += shp.ref().numPolygon;
	}
	if (trigon->mNumMax == 0) {
		return NULL;
	}
	trigon->mTri = (TRIGON_INFO*)mm::Alloc((trigon->mNumMax * sizeof(TRIGON_INFO) + 31) & ~31, "");
	trigon->mNum = 0;
	for (int i = 0; i < 11; i++) {
		sCounter.mCount[i] = 0;
	}
	trigon->_0C = trigon->_10 = 0;
	trigon->mMin.x = trigon->mMin.y = trigon->mMin.z = NAN;
	trigon->mMax.x = trigon->mMax.y = trigon->mMax.z = NAN;
	fn_802BA678(trigon, model, model->GetWldMtxArray());
	mm::SetName(trigon->mTri, FormatToStaticBuffer("(Tri %d(%d+%d)/%d)", trigon->mNum, trigon->_0C, trigon->_10, trigon->mNumMax));
	return trigon->mTri;
}

TrigonGetIdxFunc GetIdxFunc(int compType, int idxSize);
TrigonGetVtxFunc GetVtxFunc(int compType);

TrigonGetIdxFunc GetIdxFunc(int compType, int idxSize)
{
	switch (compType) {
	case GX_U8:
		return idxSize == 2 ? TrigonGetIdxU8U16 : TrigonGetIdxU8U8;
	case GX_S8:
		return idxSize == 2 ? TrigonGetIdxS8U16 : TrigonGetIdxS8U8;
	case GX_U16:
		return idxSize == 2 ? TrigonGetIdxU16U16 : TrigonGetIdxU16U8;
	case GX_S16:
		return idxSize == 2 ? TrigonGetIdxS16U16 : TrigonGetIdxS16U8;
	case GX_F32:
		return idxSize == 2 ? TrigonGetIdxF32U16 : TrigonGetIdxF32U8;
	}
	return NULL;
}

TrigonGetVtxFunc GetVtxFunc(int compType)
{
	switch (compType) {
	case GX_U8:
		return TrigonGetVtxU8;
	case GX_S8:
		return TrigonGetVtxS8;
	case GX_U16:
		return TrigonGetVtxU16;
	case GX_S16:
		return TrigonGetVtxS16;
	case GX_F32:
		return TrigonGetVtxF32;
	}
	return NULL;
}

class _trigonChecker : public TRIGON_INFO {
public:
	_trigonChecker()
	    : mNumOk(0)
	    , mNumNg(0)
	{
	}
	virtual ~_trigonChecker() { }

	bool Check();

	s32 mNumOk; // _24
	s32 mNumNg; // _28
};

inline bool _trigonChecker::Check()
{
	Vec v[3];
	Vec e1;
	Vec e2;
	Vec cross;

	mGetVtx(this, v);
	PSVECSubtract(&v[2], &v[1], &e1);
	PSVECSubtract(&v[0], &v[1], &e2);
	PSVECCrossProduct(&e1, &e2, &cross);
	if (PSVECMag(&cross) > FLT_EPSILON) {
		CVec normal;
		CalcTriangleNormal(&v[0], &v[1], &v[2], &normal);
		if (normal.y > 0.0f) {
			mNumOk++;
			return true;
		}
	}
	mNumNg++;
	return false;
}

#define SET_TRIGON(a, b, c) \
	checker.mVtx[0] = a;    \
	checker.mVtx[1] = b;    \
	checker.mVtx[2] = c;

#define ADD_TRIGON()                          \
	checker.mMtx    = mtx;                    \
	checker.mFrac   = frac;                   \
	checker.mGetVtx = getVtx;                 \
	if (checker.Check()) {                    \
		out->mVtx[0]    = checker.mVtx[0];    \
		out->mVtx[1]    = checker.mVtx[1];    \
		out->mVtx[2]    = checker.mVtx[2];    \
		out->mGetVtx    = checker.mGetVtx;    \
		out->mMtx       = checker.mMtx;       \
		out->mFrac      = checker.mFrac;      \
		out->mCell      = checker.mCell;      \
		*(u16*)out->_18 = *(u16*)checker._18; \
		out->mFloorType = checker.mFloorType; \
		*(u32*)out->_1C = *(u32*)checker._1C; \
		out++;                                \
		num++;                                \
		sCounter.mCount[counterIndex]++;      \
	}

extern "C" int fn_802BB5E4(CTrigon* trigon, nw4r::g3d::ResShp shp, TRIGON_INFO* out, MtxP mtx)
{
	nw4r::g3d::ResVtxPos pos = shp.GetResVtxPos();
	GXCompCnt cmpcnt;
	GXCompType compType;
	u8 frac;
	pos.GetVtxAttrFmt(&cmpcnt, &compType, &frac);
	void* vtx = pos.GetData();

	GXVtxDescList desc[GX_VA_TEX7 + 1];
	GXVtxAttrFmtList fmt[GX_VA_TEX7 + 1];
	shp.GXGetVtxDescv(desc);
	shp.GXGetVtxAttrFmtv(fmt);
	int posSize;
	int stride              = trigon->GetVtxSize(desc, fmt, &posSize, NULL, NULL, NULL);
	TrigonGetIdxFunc getIdx = GetIdxFunc(compType, posSize);
	TrigonGetVtxFunc getVtx = GetVtxFunc(compType);

	u8* dl  = shp.GetPrimDLTag().GetDL();
	u8* end = dl + (shp.ref().numPolygon << 5);
	_trigonChecker checker;
	int num = 0;
	int i;
	int counterIndex;

	while (dl < end) {
		u8 cmd = dl[0];
		int n  = dl[1] << 8 | dl[2];
		dl += 3;

		switch (cmd & GX_OPCODE_MASK) {
		case 0:
			return num;
		case GX_POINTS:
			dl += stride * n;
			break;
		case GX_LINES:
			sCounter.mCount[1] += n / 2;
			dl += stride * n;
			break;
		case GX_LINESTRIP:
			sCounter.mCount[2] += n - 1;
			dl += stride * n;
			break;
		case GX_TRIANGLES:
			counterIndex = 3;
			sCounter.mCount[4] += n / 3;
			for (i = 0; i < n / 3; i++) {
				SET_TRIGON(getIdx(vtx, dl), getIdx(vtx, dl + stride), getIdx(vtx, dl + stride * 2));
				ADD_TRIGON();
				dl += stride * 3;
			}
			break;
		case GX_TRIANGLESTRIP:
			counterIndex = 5;
			sCounter.mCount[6] += n - 2;
			for (i = 2; i < n; i++) {
				if (i & 1) {
					SET_TRIGON(getIdx(vtx, dl), getIdx(vtx, dl + stride * 2), getIdx(vtx, dl + stride));
				} else {
					SET_TRIGON(getIdx(vtx, dl), getIdx(vtx, dl + stride), getIdx(vtx, dl + stride * 2));
				}
				ADD_TRIGON();
				dl += stride;
			}
			dl += stride * 2;
			break;
		case GX_TRIANGLEFAN: {
			counterIndex = 7;
			sCounter.mCount[8] += n - 2;
			void* first = getIdx(vtx, dl);
			for (i = 2; i < n; i++) {
				SET_TRIGON(first, getIdx(vtx, dl + stride), getIdx(vtx, dl + stride * 2));
				ADD_TRIGON();
				dl += stride;
			}
			dl += stride * 2;
			break;
		}
		case GX_QUADS:
			counterIndex = 9;
			sCounter.mCount[10] += n / 4;
			for (i = 0; i < n / 4; i++) {
				SET_TRIGON(getIdx(vtx, dl), getIdx(vtx, dl + stride), getIdx(vtx, dl + stride * 2));
				ADD_TRIGON();
				SET_TRIGON(getIdx(vtx, dl), getIdx(vtx, dl + stride * 2), getIdx(vtx, dl + stride * 3));
				ADD_TRIGON();
				dl += stride * 4;
			}
			break;
		}
	}
	return num;
}

struct TrigonNodeInfo {
	u16 mNum;
	u16 mShapes[400];
	u16 mMaterials[200];
};
STATIC_ASSERT(sizeof(TrigonNodeInfo) == 0x4B2);

extern "C" void fn_802BA678(CTrigon* trigon, nw4r::g3d::ScnMdl* model, nw4r::math::MTX34* worldMtx)
{
	nw4r::g3d::ResMdl mdl = model->GetResMdl();
	u32 numNodes          = mdl.GetResNodeNumEntries();
	u32 numShapes         = mdl.GetResShpNumEntries();
	u32 numMaterials      = mdl.GetResMatNumEntries();
	if (numShapes >= 400) {
		OSPanic("ctrigon.cpp", 0x60, "num_resShp(%d) >= NODE_TBL_SHP_MAXNUM(%d)\n", numShapes, 400);
	}
	if (numMaterials >= 200) {
		OSPanic("ctrigon.cpp", 0x63, "num_resMat(%d) >= NODE_TBL_MAT_MAXNUM(%d)\n", numMaterials, 200);
	}

	trigon->_2C           = mm::Alloc((numNodes * sizeof(TrigonNodeInfo) + 31) & ~31, FormatToStaticBuffer("(Node  %d)", numNodes));
	trigon->mCells        = (TRIGON_CELL*)mm::Alloc((numNodes * (sizeof(TRIGON_CELL) + sizeof(u16)) + 31) & ~31,
	                                                FormatToStaticBuffer("(Layer %d)", numNodes));
	trigon->mCellOrder    = (u16*)(trigon->mCells + numNodes);
	TrigonNodeInfo* nodes = (TrigonNodeInfo*)trigon->_2C;
	for (u32 i = 0; i < numNodes; i++) {
		nodes[i].mNum = 0;
	}

	const char* drawNames[] = { "DrawOpa", "DrawXlu" };
	for (s32 pass = 0; pass < 2; pass++) {
		const u8* code = mdl.GetResByteCode(drawNames[pass]);
		if (code == NULL) {
			continue;
		}
		while (*code == 1) {
			const u8* draw = code;
			code += 8;
			if (*code != 4) {
				nw4r::db::Panic("ctrigon.cpp", 0x82, "NW4R:Failed assertion pByteCode[0] == cResByteCodeData::DRAW");
			}
			u16 material                 = draw[1] << 8 | draw[2];
			u16 shape                    = draw[3] << 8 | draw[4];
			u16 node                     = draw[5] << 8 | draw[6];
			TrigonNodeInfo* info         = &nodes[node];
			info->mShapes[info->mNum]    = shape;
			info->mMaterials[info->mNum] = material;
			info->mNum++;
			mdl.GetResNode(node);
			mdl.GetResShp(shape);
			mdl.GetResMat(material);
		}
	}

	trigon->mNumCells = 0;
	for (u32 nodeIndex = 0; nodeIndex < numNodes; nodeIndex++) {
		TrigonNodeInfo* info                   = &nodes[nodeIndex];
		trigon->mCells[trigon->mNumCells].mTri = &trigon->mTri[trigon->mNum];
		trigon->mCells[trigon->mNumCells].mNum = 0;
		nw4r::g3d::ResNode node                = mdl.GetResNode((int)nodeIndex);
		nw4r::g3d::ScnMdl::CopiedVisAccess visibility(model, nodeIndex);
		if (!visibility.IsVisible()) {
			continue;
		}
		nw4r::g3d::ResUserData nodeUserData = node.GetResUserData();
		if (nodeUserData.IsValid()) {
			::ResDic dic = ::ResUserData(nodeUserData.ptr()).GetResDic();
			::ResUserDataItem item(dic["atr"]);
			if (item.IsValid() && item.ref().numData != 0 && item.GetS32(0) == -1) {
				continue;
			}
		}

		for (u32 shapeIndex = 0; shapeIndex < info->mNum; shapeIndex++) {
			nw4r::g3d::ResShp shp           = mdl.GetResShp(info->mShapes[shapeIndex]);
			nw4r::g3d::ResMat mat           = mdl.GetResMat(info->mMaterials[shapeIndex]);
			s32 flags[3]                    = { 0, 0, 0 };
			nw4r::g3d::ResUserData userData = mat.GetResUserData();
			if (userData.IsValid()) {
				const char* names[] = { "atr", "mat" };
				for (s32 kind = 0; kind < 2; kind++) {
					::ResDic dic = ::ResUserData(userData.ptr()).GetResDic();
					::ResUserDataItem item(dic[names[kind]]);
					if (item.IsValid()) {
						if (item.ref().numData != 0) {
							flags[kind] = item.GetS32(0);
						}
						for (u32 i = 1; i < item.ref().numData; i++) {
							flags[2] = item.GetS32(i);
						}
					}
				}
			}
			if (flags[0] == -1) {
				continue;
			}

			u32 count = fn_802BB5E4(trigon, shp, &trigon->mTri[trigon->mNum], worldMtx[node.GetMtxID()].mtx);
			for (u32 i = 0; i < count; i++) {
				TRIGON_INFO* tri = &trigon->mTri[trigon->mNum + i];
				tri->mCell       = trigon->mNumCells;
				*(u16*)tri->_18  = flags[0];
				tri->mFloorType  = flags[1];
				*(u32*)tri->_1C  = flags[2];
				if (flags[0] < 2) {
					trigon->_0C++;
				}
			}
			trigon->mNum += count;
			trigon->mCells[trigon->mNumCells].mNum += count;
		}

		TRIGON_CELL* cell = &trigon->mCells[trigon->mNumCells];
		if (cell->mNum == 0) {
			continue;
		}
		f32* minMax = (f32*)cell->_08;
		minMax[0] = minMax[1] = NAN;
		for (u32 i = 0; i < cell->mNum; i++) {
			TRIGON_INFO* tri = &cell->mTri[i];
			Vec v[3];
			tri->mGetVtx(tri, v);
			f32 minY = v[0].y;
			if (v[1].y < minY) {
				minY = v[1].y;
			}
			if (v[2].y < minY) {
				minY = v[2].y;
			}
			if (__fpclassifyf(minMax[0]) == FP_NAN || minY < minMax[0]) {
				minMax[0] = minY;
			}
			f32 maxY = v[0].y;
			if (v[1].y > maxY) {
				maxY = v[1].y;
			}
			if (v[2].y > maxY) {
				maxY = v[2].y;
			}
			if (__fpclassifyf(minMax[1]) == FP_NAN || maxY > minMax[1]) {
				minMax[1] = maxY;
			}
			f32 minX = v[0].x;
			if (v[1].x < minX) {
				minX = v[1].x;
			}
			if (v[2].x < minX) {
				minX = v[2].x;
			}
			if (__fpclassifyf(trigon->mMin.x) == FP_NAN || minX < trigon->mMin.x) {
				trigon->mMin.x = minX;
			}
			minY = v[0].y;
			if (v[1].y < minY) {
				minY = v[1].y;
			}
			if (v[2].y < minY) {
				minY = v[2].y;
			}
			if (__fpclassifyf(trigon->mMin.y) == FP_NAN || minY < trigon->mMin.y) {
				trigon->mMin.y = minY;
			}
			f32 minZ = v[0].z;
			if (v[1].z < minZ) {
				minZ = v[1].z;
			}
			if (v[2].z < minZ) {
				minZ = v[2].z;
			}
			if (__fpclassifyf(trigon->mMin.z) == FP_NAN || minZ < trigon->mMin.z) {
				trigon->mMin.z = minZ;
			}
			f32 maxX = v[0].x;
			if (v[1].x > maxX) {
				maxX = v[1].x;
			}
			if (v[2].x > maxX) {
				maxX = v[2].x;
			}
			if (__fpclassifyf(trigon->mMax.x) == FP_NAN || maxX > trigon->mMax.x) {
				trigon->mMax.x = maxX;
			}
			maxY = v[0].y;
			if (v[1].y > maxY) {
				maxY = v[1].y;
			}
			if (v[2].y > maxY) {
				maxY = v[2].y;
			}
			if (__fpclassifyf(trigon->mMax.y) == FP_NAN || maxY > trigon->mMax.y) {
				trigon->mMax.y = maxY;
			}
			f32 maxZ = v[0].z;
			if (v[1].z > maxZ) {
				maxZ = v[1].z;
			}
			if (v[2].z > maxZ) {
				maxZ = v[2].z;
			}
			if (__fpclassifyf(trigon->mMax.z) == FP_NAN || maxZ > trigon->mMax.z) {
				trigon->mMax.z = maxZ;
			}
		}
		trigon->mCellOrder[trigon->mNumCells] = trigon->mNumCells;
		trigon->mNumCells++;
	}

	for (u32 i = 0; i < trigon->mNumCells - 1; i++) {
		for (u32 j = i + 1; j < trigon->mNumCells; j++) {
			TRIGON_CELL* a = &trigon->mCells[trigon->mCellOrder[i]];
			TRIGON_CELL* b = &trigon->mCells[trigon->mCellOrder[j]];
			if (((f32*)a->_08)[0] < ((f32*)b->_08)[0]) {
				u16 tmp               = trigon->mCellOrder[i];
				trigon->mCellOrder[i] = trigon->mCellOrder[j];
				trigon->mCellOrder[j] = tmp;
			}
		}
	}
}

/**
 * @note Address: 0x802BC21C
 * @note Size: 0x1C8
 */
// the bytes one vertex takes in the display list, and the index size of each attribute. the
// color's direct size comes from the first format entry
int CTrigon::GetVtxSize(GXVtxDescList* desc, GXVtxAttrFmtList* fmt, int* pos, int* nrm, int* tex, int* clr)
{
	int size    = 0;
	int posSize = 0;
	int nrmSize = 0;
	int texSize = 0;
	int clrSize = 0;

	// the position matrix index through the last texture coordinate
	for (int i = 0; i < GX_VA_TEX7 + 1; i++, desc++) {
		if (desc->mType == GX_NONE) {
			continue;
		}
		switch (desc->mAttr) {
		case GX_VA_PNMTXIDX: // matrix indices take a byte
		case GX_VA_TEX0MTXIDX:
		case GX_VA_TEX1MTXIDX:
		case GX_VA_TEX2MTXIDX:
		case GX_VA_TEX3MTXIDX:
		case GX_VA_TEX4MTXIDX:
		case GX_VA_TEX5MTXIDX:
		case GX_VA_TEX6MTXIDX:
		case GX_VA_TEX7MTXIDX:
			size += 1;
			break;
		case GX_VA_POS: // the index's own size, not the attribute's
			switch (desc->mType) {
			case GX_INDEX8:
				posSize = 1;
				size += 1;
				break;
			case GX_INDEX16:
				posSize = 2;
				size += 2;
				break;
			}
			break;
		case GX_VA_NRM:
		case GX_VA_NBT:
			switch (desc->mType) {
			case GX_INDEX8:
				nrmSize = 1;
				size += 1;
				break;
			case GX_INDEX16:
				nrmSize = 2;
				size += 2;
				break;
			}
			break;
		case GX_VA_TEX0:
		case GX_VA_TEX1:
		case GX_VA_TEX2:
		case GX_VA_TEX3:
		case GX_VA_TEX4:
		case GX_VA_TEX5:
		case GX_VA_TEX6:
		case GX_VA_TEX7:
			switch (desc->mType) {
			case GX_INDEX8:
				texSize = 1;
				size += 1;
				break;
			case GX_INDEX16:
				texSize = 2;
				size += 2;
				break;
			}
			break;
		case GX_VA_CLR0:
		case GX_VA_CLR1:
			switch (desc->mType) {
			case GX_INDEX8:
				clrSize = 1;
				size += 1;
				break;
			case GX_INDEX16:
				clrSize = 2;
				size += 2;
				break;
			case GX_DIRECT:
				switch (fmt->mType) {
				case GX_RGB565:
				case GX_RGBA4:
					clrSize = 2;
					size += 2;
					break;
				case GX_RGB8:
				case GX_RGBA6:
					clrSize = 3;
					size += 3;
					break;
				case GX_RGBX8:
				case GX_RGBA8:
					clrSize = 4;
					size += 4;
					break;
				}
				break;
			}
			break;
		}
	}

	if (pos) {
		*pos = posSize;
	}
	if (nrm) {
		*nrm = nrmSize;
	}
	if (tex) {
		*tex = texSize;
	}
	if (clr) {
		*clr = clrSize;
	}
	return size;
}

/**
 * @note Address: 0x802BC4A4
 * @note Size: 0x14
 */
void* TrigonGetIdxU8U16(void* base, u8* dl)
{
	return (u8*)base + *(u16*)dl * 3;
}

/**
 * @note Address: 0x802BC4B8
 * @note Size: 0x14
 */
void* TrigonGetIdxU8U8(void* base, u8* dl)
{
	return (u8*)base + *(u8*)dl * 3;
}

/**
 * @note Address: 0x802BC4CC
 * @note Size: 0x14
 */
void* TrigonGetIdxS8U16(void* base, u8* dl)
{
	return (s8*)base + *(u16*)dl * 3;
}

/**
 * @note Address: 0x802BC4E0
 * @note Size: 0x14
 */
void* TrigonGetIdxS8U8(void* base, u8* dl)
{
	return (s8*)base + *(u8*)dl * 3;
}

/**
 * @note Address: 0x802BC4F4
 * @note Size: 0x10
 */
void* TrigonGetIdxU16U16(void* base, u8* dl)
{
	return (u8*)base + *(u16*)dl * 6;
}

/**
 * @note Address: 0x802BC504
 * @note Size: 0x10
 */
void* TrigonGetIdxU16U8(void* base, u8* dl)
{
	return (u8*)base + *(u8*)dl * 6;
}

/**
 * @note Address: 0x802BC514
 * @note Size: 0x10
 */
void* TrigonGetIdxS16U16(void* base, u8* dl)
{
	return (u8*)base + *(u16*)dl * 6;
}

/**
 * @note Address: 0x802BC524
 * @note Size: 0x10
 */
void* TrigonGetIdxS16U8(void* base, u8* dl)
{
	return (u8*)base + *(u8*)dl * 6;
}

/**
 * @note Address: 0x802BC534
 * @note Size: 0x10
 */
void* TrigonGetIdxF32U16(void* base, u8* dl)
{
	return (u8*)base + *(u16*)dl * 12;
}

/**
 * @note Address: 0x802BC544
 * @note Size: 0x10
 */
void* TrigonGetIdxF32U8(void* base, u8* dl)
{
	return (u8*)base + *(u8*)dl * 12;
}

/**
 * @note Address: 0x802BC5C4
 * @note Size: 0x1AC
 */
void TrigonGetVtxU8(const TRIGON_INFO* tri, Vec* out)
{
	int i;
	int scale = 1 << tri->mFrac;

	// frac is the fixed point's fraction bits
	for (i = 0; i < 3; i++) {
		out[i].x = (f32)((u8*)tri->mVtx[i])[0] / (f32)scale;
		out[i].y = (f32)((u8*)tri->mVtx[i])[1] / (f32)scale;
		out[i].z = (f32)((u8*)tri->mVtx[i])[2] / (f32)scale;
	}
	if (tri->mMtx) {
		PSMTXMultVecArray(tri->mMtx, out, out, 3);
	}
}

/**
 * @note Address: 0x802BC770
 * @note Size: 0x1EC
 */
void TrigonGetVtxS8(const TRIGON_INFO* tri, Vec* out)
{
	int i;
	int scale = 1 << tri->mFrac;

	for (i = 0; i < 3; i++) {
		out[i].x = (f32)((s8*)tri->mVtx[i])[0] / (f32)scale;
		out[i].y = (f32)((s8*)tri->mVtx[i])[1] / (f32)scale;
		out[i].z = (f32)((s8*)tri->mVtx[i])[2] / (f32)scale;
	}
	if (tri->mMtx) {
		PSMTXMultVecArray(tri->mMtx, out, out, 3);
	}
}

/**
 * @note Address: 0x802BC95C
 * @note Size: 0x1AC
 */
void TrigonGetVtxU16(const TRIGON_INFO* tri, Vec* out)
{
	int i;
	int scale = 1 << tri->mFrac;

	for (i = 0; i < 3; i++) {
		out[i].x = (f32)((u16*)tri->mVtx[i])[0] / (f32)scale;
		out[i].y = (f32)((u16*)tri->mVtx[i])[1] / (f32)scale;
		out[i].z = (f32)((u16*)tri->mVtx[i])[2] / (f32)scale;
	}
	if (tri->mMtx) {
		PSMTXMultVecArray(tri->mMtx, out, out, 3);
	}
}

/**
 * @note Address: 0x802BCB08
 * @note Size: 0x1C8
 */
void TrigonGetVtxS16(const TRIGON_INFO* tri, Vec* out)
{
	int i;
	int scale = 1 << tri->mFrac;

	for (i = 0; i < 3; i++) {
		out[i].x = (f32)((s16*)tri->mVtx[i])[0] / (f32)scale;
		out[i].y = (f32)((s16*)tri->mVtx[i])[1] / (f32)scale;
		out[i].z = (f32)((s16*)tri->mVtx[i])[2] / (f32)scale;
	}
	if (tri->mMtx) {
		PSMTXMultVecArray(tri->mMtx, out, out, 3);
	}
}

/**
 * @note Address: 0x802BCCD0
 * @note Size: 0x74
 */
void TrigonGetVtxF32(const TRIGON_INFO* tri, Vec* out)
{
	int i;

	for (i = 0; i < 3; i++) {
		out[i] = *(Vec*)tri->mVtx[i];
	}
	if (tri->mMtx) {
		PSMTXMultVecArray(tri->mMtx, out, out, 3);
	}
}

/**
 * @note Address: 0x802BCD44
 * @note Size: 0x88
 */
void CTrigon::Release()
{
	if (mCells != NULL) {
		mm::Free(mCells);
		mCells     = NULL;
		mCellOrder = NULL;
		mNumCells  = 0;
	}
	if (_2C != NULL) {
		mm::Free(_2C);
		_2C = NULL;
	}
	if (mTri != NULL) {
		mm::Free(mTri);
		mTri = NULL;
	}
	mNumMax = 0;
	mNum    = 0;
}

/**
 * @note Address: 0x802BCDCC
 * @note Size: 0xFC
 */
// the triangle under pos, trying the one from last time first, then its cell, then all of them
int CTrigon::Check(Vec* pos, int hint, f32* floorY)
{
	if (hint >= 0 && hint < mNum) {
		Vec v[3];
		mTri[hint].mGetVtx(&mTri[hint], v);
		if (IsPointInsideTriangleXZ(pos, &v[0], &v[1], &v[2])) {
			if (floorY != NULL) {
				CVec hit;
				IntersectVerticalLineWithTrianglePlane(pos, &v[0], &v[1], &v[2], &hit);
				*floorY = hit.y;
			}
			return hint;
		}
		return SearchNear(pos, hint, floorY);
	}
	return Search(pos, floorY);
}

/**
 * @note Address: 0x802BCEC8
 * @note Size: 0x1B0
 */
// walks the hint's cell outward from the hint, one triangle above and one below per step
int CTrigon::SearchNear(Vec* pos, int hint, f32* floorY)
{
	TRIGON_CELL* cell = &mCells[mTri[hint].mCell];
	TRIGON_INFO* tri  = cell->mTri;
	int found         = -1;
	u32 num           = cell->mNum;
	if (tri == NULL || num == 0) {
		return -1;
	}

	// from here on the hint counts from the cell's first triangle
	Vec v[3];
	int base = tri - mTri;
	hint -= base;
	int i = 0;
	for (u32 n = 0; n < num; n++) {
		if (hint + i < num) {
			TRIGON_INFO* t = &tri[hint + i];
			t->mGetVtx(t, v);
			if (IsPointInsideTriangleXZ(pos, &v[0], &v[1], &v[2])) {
				found = hint + base + i;
				break;
			}
		}
		if (hint - i - 1 >= 0) {
			TRIGON_INFO* t = &tri[hint - i - 1];
			t->mGetVtx(t, v);
			if (IsPointInsideTriangleXZ(pos, &v[0], &v[1], &v[2])) {
				found = hint + base - i - 1;
				break;
			}
		}
		i++;
		if (hint + i >= num && hint - i - 1 < 0) {
			break;
		}
	}

	if (found >= 0) {
		if (floorY != NULL) {
			CVec hit;
			IntersectVerticalLineWithTrianglePlane(pos, &v[0], &v[1], &v[2], &hit);
			*floorY = hit.y;
		}
	} else {
		found = Search(pos, floorY);
	}
	return found;
}

/**
 * @note Address: 0x802BD078
 * @note Size: 0x1B4
 */
// every cell in mCellOrder: the highest floor at most 5 above pos wins
int CTrigon::Search(Vec* pos, f32* floorY)
{
	f32 top   = 5.0f + pos->y;
	f32 best  = NAN;
	int found = -1;
	for (int i = 0; i < mNumCells; i++) {
		int cell         = mCellOrder[i];
		TRIGON_INFO* tri = mCells[cell].mTri;
		u32 num          = mCells[cell].mNum;
		if (tri == NULL || num == 0) {
			return found;
		}
		for (u32 j = 0; j < num; tri++, j++) {
			Vec v[3];
			tri->mGetVtx(tri, v);
			if (IsPointInsideTriangleXZ(pos, &v[0], &v[1], &v[2])) {
				CVec hit;
				IntersectVerticalLineWithTrianglePlane(pos, &v[0], &v[1], &v[2], &hit);
				if (__fpclassifyf(best) == FP_NAN || (hit.y < top && top - hit.y < best)) {
					f32 dist = top - hit.y;
					if (dist >= 0.0f) {
						best = dist;
					}
					found = j + (mCells[cell].mTri - mTri);
					if (floorY != NULL) {
						*floorY = hit.y;
					}
				}
			}
		}
	}
	return found;
}
