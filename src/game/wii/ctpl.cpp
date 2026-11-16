// the Wii unit between cfile.cpp and cresfile.cpp (ctpl.cpp by the GameCube file
// it reimplements): CTexObj with its quad draws, the .tpl file CTpl binds once it
// is read in, and the game's nw4r sound setup - the two loader threads, archive
// open/group load, the per-frame track fades and the 3D actor init - which sits
// in this one retail unit in front of cresfile.
#include "game/ctpl.h"
#include "game/cfont.h"
#include "game/wii/csound.h"
#include "Dolphin/ai.h"
#include "Dolphin/os.h"
#include "Dolphin/GX/GXFifo.h"
#include "Dolphin/GX/GXGeometry.h"
#include "Dolphin/GX/GXTransform.h"
#include "PowerPC_EABI_Support/Runtime/global_destructor_chain.h"
#include "revolution/ax.h"
#include "revolution/mem.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

extern "C" {
// the nw4r archive, player, group, heap, track and 3D calls; the library they
// belong to is still the unnamed unit block around 0x8028A000/0x80296000
BOOL fn_8028CF6C(void* archive, const char* path);
s32 fn_8028D220(void* archive, void* data, u32 size);
u32 fn_80297EF4(void* player);
u32 fn_80298040(void* player, CSound* sound);
s32 fn_80297DF0(void* player, CSound* sound, void* data, u32 dataSize, void* data2, u32 data2Size);
void fn_802985B4(void* player);
void* fn_80298978(void* player, u32 index);
u32 fn_80295D88(CSound* sound);
BOOL fn_80299D64(void* player, s32 groupId, void* heap, u32 flag);
s32 fn_8029AC28(void* group, u32 priority);
BOOL fn_8029A3E0(void* heap, void* data, u32 size);
void fn_8029A458(void* heap);
void fn_8029A4AC(void* heap, s32 level);
void fn_8029AFAC(u32 voices, u32 groups);
void fn_8029B1AC();
void fn_8028B274(void* track, f32 volume, u32 arg);
void fn_8028B318(void* track, f32 pan);
u32 fn_802958C0(void* actor, void* arg);
void fn_80295914(void* actor, void* arg, void* data, u32 size);
void fn_80295848(void* sound, f32 volume);
void fn_80295850(void* sound, f32 pan);
void fn_80295858(void* sound, f32 pitch);
void fn_80295690(void* sound, Mtx mtx);
// the nw4r list insert and its panic (file, line, message, ...)
void fn_802343E0(void* list, void* item, void* iter);
void fn_80236070(const char* file, int line, const char* message, ...);
}
extern MEMiHeapHead* gWiiGameHeap;

// the LinkList assert strings retail keeps in .data behind the vtables
static char sLinkListAssert[] = "NW4R:Pointer must not be NULL (p)";
static char sLinkListFile[]   = "LinkList.h";

void CTexObj::Init(TPLDescriptor* desc, u32 tlutName)
{
	TPLHeader* tex      = desc->textureHeader;
	TPLClutHeader* clut = desc->CLUTHeader;
	mImageData          = tex != NULL ? tex->data : NULL;
	mClutData           = clut != NULL ? clut->data : NULL;
	BOOL mipmap         = tex->minLOD != tex->maxLOD;
	if (mClutData == NULL) {
		Init(tex->data, tex->width, tex->height, (GXTexFmt)tex->format, tex->wrapS, tex->wrapT, mipmap);
	} else {
		Init(tex->data, tex->width, tex->height, (GXTexFmt)tex->format, tex->wrapS, tex->wrapT, mipmap, tlutName);
		GXInitTlutObj(&mTlut, clut->data, (GXTlutFmt)clut->format, clut->numEntries);
	}
	f32 lodBias = tex->LODBias;
	f32 maxLOD  = tex->maxLOD;
	f32 minLOD  = tex->minLOD;
	GXInitTexObjLOD(this, tex->minFilter, tex->magFilter, minLOD, maxLOD, lodBias, GX_FALSE, tex->edgeLODEnable, GX_ANISO_1);
}

void CTexObj::Init(void* image, u16 width, u16 height, GXTexFmt format, GXTexWrapMode wrapS, GXTexWrapMode wrapT, GXBool mipmap)
{
	mImageData = image;
	GXInitTexObj(this, image, width, height, format, wrapS, wrapT, mipmap);
	InitMatrices();
}

void CTexObj::Init(void* image, u16 width, u16 height, GXTexFmt format, GXTexWrapMode wrapS, GXTexWrapMode wrapT, GXBool mipmap,
                   u32 tlutName)
{
	mClutData = image;
	GXInitTexObjCI(this, image, width, height, (GXCITexFmt)format, wrapS, wrapT, mipmap, tlutName);
	mTlut.mTlutName = tlutName;
	InitMatrices();
}

void CTexObj::InitMatrices()
{
	mTexMtx.mDirty = FALSE;
	PSMTXIdentity(mTexMtx.mMtx[0]);
	PSMTXIdentity(mTexMtx.mMtx[1]);
	PSMTXIdentity(mTexMtx.mMtx[2]);
	PSMTXIdentity(mTexMtx.mMtx[3]);
}

void CTexObj::LoadTexture(GXTexMapID map)
{
	if (mClutData != NULL) {
		GXLoadTlut(&mTlut, mTlut.mTlutName);
	}
	GXLoadTexObj(this, map);
}

void CTexObj::Load(GXTexMapID map, u32 mtxId, GXTexMtxType mtxType)
{
	LoadTexture(map);
	if (mTexMtx.mDirty) {
		PSMTXConcat(mTexMtx.mMtx[0], mTexMtx.mMtx[1], mTexMtx.mMtx[3]);
		PSMTXConcat(mTexMtx.mMtx[3], mTexMtx.mMtx[2], mTexMtx.mMtx[3]);
		mTexMtx.mDirty = FALSE;
	}
	GXLoadTexMtxImm(mTexMtx.mMtx[3], mtxId, mtxType);
}

u32 CTexObj::Set(const CTexObj& other)
{
	*this = other;
	return GXGetTexObjWidth(&other);
}

u32 CTexObj::SetChar(const char* str, u32 font)
{
	CFont fnt;
	s32 width;
	CTexObj* tex = GetFontGlyphTexture(&fnt, str, font, &width);
	*this        = *tex;
	return width;
}

void CTexObj::Draw(GXTexMapID map, u32 mtxId, GXTexMtxType mtxType, const Vec* pos, u32 color0, u32 color1, u32 color2, u32 color3)
{
	Load(map, mtxId, mtxType);
	GXVtxAttrFmtList fmt[] = {
		{ GX_VA_POS, GX_POS_XYZ, GX_F32, 0 },
		{ GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 },
		{ GX_VA_TEX0, GX_TEX_ST, GX_U8, 0 },
		{ GX_VA_NULL, (GXCompCnt)0, (GXCompType)0, 0 },
	};
	GXVtxDescList desc[] = {
		{ GX_VA_POS, GX_DIRECT },
		{ GX_VA_CLR0, GX_DIRECT },
		{ GX_VA_TEX0, GX_DIRECT },
		{ GX_VA_NULL, GX_NONE },
	};
	GXClearVtxDesc();
	GXSetVtxAttrFmtv(GX_VTXFMT0, fmt);
	GXSetVtxDescv(desc);
	GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
	GXPosition3f32(pos->x - 1.0f, pos->y + 1.0f, pos->z);
	GXColor1u32(color0);
	GXTexCoord2u8(0, 0);
	GXPosition3f32(pos->x + 1.0f, pos->y + 1.0f, pos->z);
	GXColor1u32(color1);
	GXTexCoord2u8(1, 0);
	GXPosition3f32(pos->x - 1.0f, pos->y - 1.0f, pos->z);
	GXColor1u32(color2);
	GXTexCoord2u8(0, 1);
	GXPosition3f32(pos->x + 1.0f, pos->y - 1.0f, pos->z);
	GXColor1u32(color3);
	GXTexCoord2u8(1, 1);
}

static const GXVtxDescList sLitDesc[] = {
	{ GX_VA_POS, GX_DIRECT }, { GX_VA_NRM, GX_DIRECT }, { GX_VA_CLR0, GX_DIRECT }, { GX_VA_TEX0, GX_DIRECT }, { GX_VA_NULL, GX_NONE },
};

void CTexObj::DrawLit(GXTexMapID map, u32 mtxId, GXTexMtxType mtxType, const Vec* pos, u32 color0, u32 color1, u32 color2, u32 color3)
{
	Load(map, mtxId, mtxType);
	GXVtxAttrFmtList fmt[] = {
		{ GX_VA_POS, GX_POS_XYZ, GX_F32, 0 },           { GX_VA_NRM, GX_NRM_XYZ, GX_S8, 0 },
		{ GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 },       { GX_VA_TEX0, GX_TEX_ST, GX_U8, 0 },
		{ GX_VA_NULL, (GXCompCnt)0, (GXCompType)0, 0 },
	};
	GXClearVtxDesc();
	GXSetVtxAttrFmtv(GX_VTXFMT0, fmt);
	GXSetVtxDescv((GXVtxDescList*)sLitDesc);
	GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
	GXPosition3f32(pos->x - 1.0f, pos->y + 1.0f, pos->z);
	GXNormal3s8(0, 0, 1);
	GXColor1u32(color0);
	GXTexCoord2u8(0, 0);
	GXPosition3f32(pos->x + 1.0f, pos->y + 1.0f, pos->z);
	GXNormal3s8(0, 0, 1);
	GXColor1u32(color1);
	GXTexCoord2u8(1, 0);
	GXPosition3f32(pos->x - 1.0f, pos->y - 1.0f, pos->z);
	GXNormal3s8(0, 0, 1);
	GXColor1u32(color2);
	GXTexCoord2u8(0, 1);
	GXPosition3f32(pos->x + 1.0f, pos->y - 1.0f, pos->z);
	GXNormal3s8(0, 0, 1);
	GXColor1u32(color3);
	GXTexCoord2u8(1, 1);
}

s32 CTpl::ReadAsync(const char* path, u32 offset, u32 size, BOOL fromAram)
{
	return CFile::ReadAsync(path, offset, size, fromAram);
}

s32 CTpl::GetResult()
{
	s32 result = CFile::GetResult();
	if (result == 0) {
		Bind(mBuffer);
	}
	return result;
}

BOOL CTpl::Close()
{
	CFile::Close();
	mPalette = NULL;
	mBound   = FALSE;
	return TRUE;
}

void CTpl::Bind(void* data)
{
	mPalette = (TPLPalette*)data;
	BindPalette(mPalette);
	mBound = TRUE;
}

// every descriptor's two header offsets become pointers and the pixel/clut
// data offsets inside the headers do the same; the unpacked flags keep a
// shared palette from being converted twice
void CTpl::BindPalette(TPLPalette* pal)
{
	pal->descriptorArray = (TPLDescriptor*)((u32)pal->descriptorArray + (u32)pal);
	for (u32 i = 0; i < pal->numDescriptors; i++) {
		TPLDescriptor* desc = &pal->descriptorArray[i];
		if (desc->textureHeader != NULL) {
			desc->textureHeader = (TPLHeader*)((u32)desc->textureHeader + (u32)pal);
			if (!desc->textureHeader->unpacked) {
				desc->textureHeader->data += (u32)pal;
				desc->textureHeader->unpacked = TRUE;
			}
		}
		if (desc->CLUTHeader != NULL) {
			desc->CLUTHeader = (TPLClutHeader*)((u32)desc->CLUTHeader + (u32)pal);
			if (!desc->CLUTHeader->unpacked) {
				desc->CLUTHeader->data += (u32)pal;
				desc->CLUTHeader->unpacked = TRUE;
			}
		}
	}
}

// --- the Wii sound setup -----------------------------------------------------

// the loader thread context: its OSThread, the heap-allocated stack and the
// impl object the __sinit gives a vtable
struct CSoundLoaderImpl {
	virtual ~CSoundLoaderImpl() { }
	u32 _04;
};

struct CSoundLoader {
	OSThread mThread;       // _000
	void* mStack;           // _318
	u32 mStackSize;         // _31C
	CSoundLoaderImpl mImpl; // _320
	CSoundLoader() { mStack = NULL; }
};

// the argument block both loader threads read
struct CSoundLoaderArgs {
	CSound* mOwner; // _00
	u32 mArg1;      // _04
	u32 mArg2;      // _08
	u32 _0C;
};

static CSoundLoader sLoader;         // 0x8059FF80
static u8* sLoaderStack;             // 0x805A28A8
static CSoundLoaderArgs sLoaderArgs; // 0x805A02A8

static void* OpenArchiveThreadProc(void* arg)
{
	CSoundLoaderArgs* args = (CSoundLoaderArgs*)arg;
	args->mOwner->OpenArchive((const char*)args->mArg1, args->mArg2);
	return NULL;
}

static void* LoadGroupThreadProc(void* arg)
{
	CSoundLoaderArgs* args = (CSoundLoaderArgs*)arg;
	CSound* owner          = args->mOwner;
	owner->LoadGroup(args->mArg1);
	return NULL;
}

BOOL CSound::OpenArchiveFile(const char* path)
{
	char name[0x80];
	sprintf(name, "%s.brsar", path);
	if (fn_8028CF6C(this, name)) {
		u32 size                          = *(u32*)&mDvdSoundArchive[0x124];
		void* data                        = MEMAllocFromExpHeapEx(gWiiGameHeap, size, 0x20);
		*(void**)&mDvdSoundArchive[0x18C] = data;
		return fn_8028D220(this, data, size);
	}
	return FALSE;
}

static BOOL InitSoundArchivePlayer(CSoundArchivePlayer* player, CSound* sound)
{
	player->mReady      = 1;
	u32 size            = fn_80297EF4(player);
	player->mSetupData  = MEMAllocFromExpHeapEx(gWiiGameHeap, size, 4);
	u32 size2           = fn_80298040(player, sound);
	player->mSetupData2 = MEMAllocFromExpHeapEx(gWiiGameHeap, size2, 0x20);
	return fn_80297DF0(player, sound, player->mSetupData, size, player->mSetupData2, size2);
}

static BOOL InitSoundHeap(void* heap, u32 size)
{
	void* data = MEMAllocFromExpHeapEx(gWiiGameHeap, size, 0x20);
	return fn_8029A3E0(heap, data, size);
}

void SoundHeapAlloc(void** slot, u32 size)
{
	*slot = MEMAllocFromExpHeapEx(gWiiGameHeap, size, 4);
}

void SoundHeapFree(void** slot)
{
	MEMFreeToExpHeap(gWiiGameHeap, *slot);
	*slot = NULL;
}

extern "C" void InitAudio(CSound* sound, u32 heapSize)
{
	AIInit(NULL);
	AXInit();
	fn_8029AFAC(4, 3);
	sound->mSound3DManager.Init(sound, sound->mArchivePlayer);
	sLoaderStack          = (u8*)MEMAllocFromExpHeapEx(gWiiGameHeap, 0x1000, 0x20);
	sound->mSoundHeapSize = heapSize;
	sound->mLoadPending   = FALSE;
}

extern "C" void ShutdownAudio()
{
	fn_8029B1AC();
	AXQuit();
	AIStopDMA();
}

BOOL CSound::OpenArchive(const char* path, s32 initialGroup)
{
	if (path != NULL) {
		if (OpenArchiveFile(path) == 0) {
			return TRUE;
		}
		if (InitSoundArchivePlayer((CSoundArchivePlayer*)mArchivePlayer, this) == 0) {
			return TRUE;
		}
		if (InitSoundHeap(mSoundHeap, mSoundHeapSize) == 0) {
			return TRUE;
		}
	}
	if ((u32)initialGroup != (u32)-1) {
		return LoadGroup(initialGroup);
	}
	return TRUE;
}

// NONMATCHING: register allocation / store scheduling around the OSCreateThread
// argument evaluation differs from retail (93.0%).
BOOL CSound::OpenArchiveAsync(const char* path, s32 initialGroup)
{
	sLoaderArgs.mOwner = this;
	sLoaderArgs.mArg2  = initialGroup;
	sLoaderArgs.mArg1  = (u32)path;
	sLoader.mStack     = sLoaderStack;
	sLoader.mStackSize = 0x1000;
	OSCreateThread(&sLoader.mThread, OpenArchiveThreadProc, &sLoaderArgs, sLoaderStack + 0x1000, 0x1000, 0x10, 1);
	OSResumeThread(&sLoader.mThread);
	mLoadPending = TRUE;
	return TRUE;
}

// the game's archive groups; only the load priority is known
struct CSoundGroup {
	u8 _00[0x24];
	s32 mPriority; // _24
};

BOOL CSound::LoadGroup(s32 groupId)
{
	if (fn_80299D64(mArchivePlayer, groupId, mSoundHeap, 0)) {
		for (u32 i = 0; i < fn_80295D88(this); i++) {
			CSoundGroup* group = (CSoundGroup*)fn_80298978(mArchivePlayer, i);
			if (group->mPriority < 0x40) {
				fn_8029AC28(fn_80298978(mArchivePlayer, i), 0x40);
			}
		}
		return TRUE;
	}
	return TRUE;
}

BOOL CSound::LoadGroupAsync(s32 groupId)
{
	sLoaderArgs.mOwner = this;
	sLoaderArgs.mArg1  = groupId;
	sLoader.mStack     = sLoaderStack;
	sLoader.mStackSize = 0x1000;
	OSCreateThread(&sLoader.mThread, LoadGroupThreadProc, &sLoaderArgs, sLoaderStack + 0x1000, 0x1000, 0x10, 1);
	OSResumeThread(&sLoader.mThread);
	mLoadPending = TRUE;
	return TRUE;
}

void CSound::RestoreHeapLevel(s32 level)
{
	fn_8029A4AC(mSoundHeap, level);
}

// NONMATCHING: track-slot loop uses the same offsets as retail but the compiler
// picks different callee-saved registers for the slot walker (92.0%).
extern "C" void UpdateGameSound(CSound* sound, u32 step)
{
	if (sound->mLoadPending && OSIsThreadTerminated(&sLoader.mThread)) {
		sound->mLoadPending = FALSE;
		fn_8029A458(sound->mSoundHeap);
	}
	for (int i = 0; i < 0x40; i++) {
		if (sound->mTracks[i].mTrack == NULL) {
			continue;
		}
		if (sound->mTracks[i].mTrack != NULL) {
			if (sound->mTracks[i].mVolume < sound->mTracks[i].mVolumeTarget) {
				sound->mTracks[i].mVolume += sound->mTracks[i].mVolumeStep * step;
				if (sound->mTracks[i].mVolume > sound->mTracks[i].mVolumeTarget) {
					sound->mTracks[i].mVolume = sound->mTracks[i].mVolumeTarget;
				}
				if (sound->mTracks[i].mTrack != NULL) {
					fn_8028B274(sound->mTracks[i].mTrack, sound->mTracks[i].mVolume, 0);
				}
			} else if (sound->mTracks[i].mVolume > sound->mTracks[i].mVolumeTarget) {
				sound->mTracks[i].mVolume += sound->mTracks[i].mVolumeStep * step;
				if (sound->mTracks[i].mVolume < sound->mTracks[i].mVolumeTarget) {
					sound->mTracks[i].mVolume = sound->mTracks[i].mVolumeTarget;
				}
				if (sound->mTracks[i].mTrack != NULL) {
					fn_8028B274(sound->mTracks[i].mTrack, sound->mTracks[i].mVolume, 0);
				}
			}
		}
		if (sound->mTracks[i].mTrack != NULL) {
			if (sound->mTracks[i].mPan < sound->mTracks[i].mPanTarget) {
				sound->mTracks[i].mPan += sound->mTracks[i].mPanStep * step;
				if (sound->mTracks[i].mPan > sound->mTracks[i].mPanTarget) {
					sound->mTracks[i].mPan = sound->mTracks[i].mPanTarget;
				}
				if (sound->mTracks[i].mTrack != NULL) {
					fn_8028B318(sound->mTracks[i].mTrack, sound->mTracks[i].mPan);
				}
			} else if (sound->mTracks[i].mPan > sound->mTracks[i].mPanTarget) {
				sound->mTracks[i].mPan += sound->mTracks[i].mPanStep * step;
				if (sound->mTracks[i].mPan < sound->mTracks[i].mPanTarget) {
					sound->mTracks[i].mPan = sound->mTracks[i].mPanTarget;
				}
				if (sound->mTracks[i].mTrack != NULL) {
					fn_8028B318(sound->mTracks[i].mTrack, sound->mTracks[i].mPan);
				}
			}
		}
	}
	fn_802985B4(sound->mArchivePlayer);
}

void CSound3DManager::Init(CSound* owner, void* player)
{
	mOwner  = owner;
	mPlayer = player;
}

// NONMATCHING: the link-insert null check and mFlags/item ordering differ from
// retail; the original assert expression is unknown (93.6%).
void CSound3D::Init(void* arg)
{
	u32 size   = fn_802958C0(mActor, mArg);
	void* data = MEMAllocFromExpHeapEx(gWiiGameHeap, size, 0x20);
	fn_80295914(mActor, mArg, data, size);
	mFlags     = 0x20;
	void* item = &mNode[4];
	if (&mSound == NULL) {
		fn_80236070(sLinkListFile, 0x233, sLinkListAssert);
	}
	fn_802343E0(&mNode, &item, mList);
	fn_80295848(mSound, 1.0f);
	fn_80295850(mSound, 1.0f);
	fn_80295858(mSound, 1.0f);
}

// NONMATCHING: f30/f31 allocation for the look-vector locals is swapped versus
// retail (99.6%).
void CSound3D::SetListener(f32 angle, const Vec* pos)
{
	Mtx mtx;
	Vec up;
	up.x     = 0.0f;
	up.y     = 1.0f;
	up.z     = 0.0f;
	f32 z    = pos->z + (f32)cos(angle);
	f32 y    = pos->y;
	f32 x    = pos->x + (f32)sin(angle);
	Vec look = { x, y, z };
	C_MTXLookAt(mtx, pos, &up, &look);
	fn_80295690(mSound, mtx);
}

void CSound3D::SetVolumePanPitch(f32 volume, f32 pan, f32 pitch)
{
	fn_80295848(mSound, volume);
	fn_80295850(mSound, pan);
	fn_80295858(mSound, pitch);
}
