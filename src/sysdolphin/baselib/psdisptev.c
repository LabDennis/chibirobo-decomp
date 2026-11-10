#include <global.h>

#include <dolphin/gx/gxtypes.h>
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/vec.h>
#include <sysdolphin/cobj.h>
#include <sysdolphin/jobj.h>
#include <sysdolphin/list.h>
#include <sysdolphin/memory.h>
#include <sysdolphin/mtx.h>
#include <sysdolphin/object.h>
#include <sysdolphin/random.h>

// the shared particle header still has the older layouts, these are the ones this
// game's generator and appsrt code actually use

/* size: 0xA0 */
struct HSD_Particle {
	HSD_Particle* next;                  // _00
	u32 kind;                            // _04
	u8 bank;                             // _08
	u8 texGroup;                         // _09
	u8 poseNum;                          // _0A
	u8 palNum;                           // _0B
	u16 sizeCount;                       // _0C
	u16 primColCount;                    // _0E
	u16 envColCount;                     // _10
	GXColor primCol;                     // _12
	GXColor envCol;                      // _16
	u16 cmdWait;                         // _1A
	u8 loopCount;                        // _1C
	u8 linkNo;                           // _1D
	u16 idnum;                           // _1E
	u8* cmdList;                         // _20
	u16 cmdPtr;                          // _24
	u16 cmdMarkPtr;                      // _26
	u16 cmdLoopPtr;                      // _28
	u16 life;                            // _2A
	Vec vel;                             // _2C
	f32 grav;                            // _38
	f32 fric;                            // _3C
	Vec pos;                             // _40
	f32 size;                            // _4C
	f32 rotate;                          // _50
	u16 aCmpCount;                       // _54
	u8 aCmpMode;                         // _56
	u8 aCmpParam1;                       // _57
	u8 aCmpParam2;                       // _58
	u8 pJObjOfs;                         // _59
	u16 matColCount;                     // _5A
	u16 ambColCount;                     // _5C
	u16 rotateCount;                     // _5E
	f32 sizeTarget;                      // _60
	f32 rotateTarget;                    // _64
	f32 spreadRate;                      // _68
	u16 primColRemain;                   // _6C
	u16 envColRemain;                    // _6E
	GXColor primColTarget;               // _70
	GXColor envColTarget;                // _74
	u16 matColRemain;                    // _78
	u16 ambColRemain;                    // _7A
	u16 aCmpRemain;                      // _7C
	u8 aCmpParam1Target;                 // _7E
	u8 aCmpParam2Target;                 // _7F
	u8 matRGB;                           // _80
	u8 matA;                             // _81
	u8 ambRGB;                           // _82
	u8 ambA;                             // _83
	u8 matRGBTarget;                     // _84
	u8 matATarget;                       // _85
	u8 ambRGBTarget;                     // _86
	u8 ambATarget;                       // _87
	f32 trail;                           // _88
	HSD_Generator* gen;                  // _8C
	HSD_psAppSRT* appsrt;                // _90
	f32* userdata;                       // _94
	int (*callback)(HSD_Particle* part); // _98
	u16 pauseCount;                      // _9C
	u16 pad9E;                           // _9E
};

/* size: 0x74 */
struct HSD_psAppSRT {
	HSD_psAppSRT* next;                     // _00
	HSD_Generator* gp;                      // _04
	Vec translate;                          // _08
	Vec rot;                                // _14
	u32 _20;                                // _20
	Vec scale;                              // _24
	u8 status;                              // _30
	u8 frameNum;                            // _31
	u16 usedCount;                          // _32
	Mtx mmtx;                               // _34
	f32 ssx;                                // _64
	f32 ssy;                                // _68
	void (*freefunc)(HSD_psAppSRT* appsrt); // _6C
	u16 idnum;                              // _70
	u8 xA2;                                 // _72
};

typedef struct _auxDisc {
	f32 minAngle; // _00
	f32 maxAngle; // _04
} auxDisc;

typedef struct _auxLine {
	f32 x2; // _00
	f32 y2; // _04
	f32 z2; // _08
} auxLine;

typedef struct _auxRect {
	f32 x;    // _00
	f32 y;    // _04
	f32 z;    // _08
	f32 xx;   // _0C
	f32 xy;   // _10
	f32 xz;   // _14
	f32 yx;   // _18
	f32 yy;   // _1C
	f32 yz;   // _20
	f32 zx;   // _24
	f32 zy;   // _28
	f32 zz;   // _2C
	u16 flag; // _30
} auxRect;

typedef struct _auxCone {
	f32 minAngle; // _00
	f32 maxAngle; // _04
	f32 height;   // _08
} auxCone;

typedef struct _auxSphere {
	f32 speed;    // _00
	f32 latMid;   // _04
	f32 latRange; // _08
	f32 lonMid;   // _0C
	f32 lonRange; // _10
} auxSphere;

/* size: 0x94 */
struct HSD_Generator {
	HSD_Generator* next;                  // _00
	u32 kind;                             // _04
	f32 random;                           // _08
	f32 count;                            // _0C
	HSD_JObj* jobj;                       // _10
	u16 genLife;                          // _14
	u16 type;                             // _16
	u8 bank;                              // _18
	u8 linkNo;                            // _19
	u8 texGroup;                          // _1A
	u8 dummy;                             // _1B
	u16 idnum;                            // _1C
	u16 life;                             // _1E
	u8* cmdList;                          // _20
	Vec pos;                              // _24
	Vec vel;                              // _30
	f32 grav;                             // _3C
	f32 fric;                             // _40
	f32 size;                             // _44
	f32 radius;                           // _48
	f32 angle;                            // _4C
	u32 numChild;                         // _50
	HSD_psAppSRT* appsrt;                 // _54
	void* userfunc;                       // _58
	void (*callback)(HSD_Generator* gen); // _5C
	union {
		auxDisc disc;
		auxLine line;
		auxRect rect;
		auxCone cone;
		auxSphere sphere;
	} aux; // _60
};

/* size: 0x40 */
typedef struct HSD_PSCmdList {
	u16 type;      // _00
	u16 texGroup;  // _02
	u16 genLife;   // _04
	u16 life;      // _06
	u32 kind;      // _08
	f32 grav;      // _0C
	f32 fric;      // _10
	f32 vx;        // _14
	f32 vy;        // _18
	f32 vz;        // _1C
	f32 radius;    // _20
	f32 angle;     // _24
	f32 random;    // _28
	f32 size;      // _2C
	f32 param1;    // _30
	f32 param2;    // _34
	f32 param3;    // _38
	u8 cmdList[1]; // _3C
} HSD_PSCmdList;

/* size: 0x1C */
typedef struct HSD_PSTexGroup {
	u32 num;         // _00
	u32 fmt;         // _04
	u32 tlutfmt;     // _08
	u32 width;       // _0C
	u32 height;      // _10
	u16 palnum;      // _14
	u16 palflag;     // _16
	u8* texTable[1]; // _18
} HSD_PSTexGroup;

extern int __float_nan[];
extern int __float_epsilon[];
extern int __float_min[];
#define NAN         (*(f32*)__float_nan)
#define FLT_EPSILON (*(f32*)__float_epsilon)
#define FLT_MIN     (*(f32*)__float_min)

#define M_PI   3.141592653589793
#define M_PI_2 1.5707963267948966

extern void* memset(void*, int, u32);
extern f32 __fabsf(f32 x);
extern f64 __fabs(f64 x);
extern f64 sin(f64 x);
extern f64 cos(f64 x);
extern f64 atan2(f64 y, f64 x);

// particle side
extern void psKillAllParticle(void);
extern void psKillGeneratorChild(HSD_Generator* gp);
extern HSD_Particle* psGenerateParticle(int linkNo, int bank, int reserved, u32 kind, int texGroup, u8* list, int life, f32 x, f32 y, f32 z,
                                        f32 vx, f32 vy, f32 vz, f32 size, f32 grav, f32 fric, int palflag, HSD_Generator* gp);
extern HSD_PSTexGroup** psTexGroupArray[];

// generator tables, one per bank
extern s32 psNumCmdList[64];
extern HSD_PSCmdList** psCmdListArray[64];

extern u16 lbl_8065BA84;
extern u16 lbl_8065BA86;
extern u16 lbl_8065BA8A;
extern u16 lbl_8065BA8C;
extern u16 sParticleGeneratorIdCounter;
extern HSD_psAppSRT* freeAppSRT;
u32 lbl_8065BAB8;
extern void (*lbl_8065BAB0)(HSD_Generator* gen, Mtx mtx);
extern void (*lbl_8065BAAC)(HSD_Generator* gen);
extern HSD_CObj* psCamera;
extern HSD_Generator* lbl_8065BAA4;
extern HSD_Generator* lbl_8065BAA0;
extern HSD_Generator* lbl_8065BA9C;
extern HSD_SList* lbl_8065BA98;

extern char sPathGeneratorC[];
extern char sGeneratorCameraCondition[];

void psKillAllGenerator(void);
f32 generateParticle(HSD_Generator* gen);
void psApplyGeneratorJObj(HSD_Generator* gen);
HSD_Generator* psCreateGeneratorID(int linkNo, int bank, int kind);
void psSetBillboardCamera(HSD_CObj* cobj);
HSD_psAppSRT* CreateGeneratorAppSRT(HSD_Generator* gp, s32 status);
s32 psRemoveParticleAppSRT(HSD_Particle* pp);
s32 psRemoveGeneratorAppSRT(HSD_Generator* gp);

static inline f32 gen_sinf(f32 x)
{
	return sin(x);
}

static inline f32 gen_cosf(f32 x)
{
	return cos(x);
}

static inline f32 gen_atan2f(f32 y, f32 x)
{
	return atan2(y, x);
}

static inline int gen_fpclassifyf(f32 x)
{
	switch (*(int*)&x & 0x7F800000) {
	case 0x7F800000:
		if ((*(int*)&x & 0x7FFFFF) != 0) {
			return 1;
		}
		return 2;
	case 0:
		if ((*(int*)&x & 0x7FFFFF) != 0) {
			return 5;
		}
		return 3;
	}
	return 4;
}

// msl's sqrtf with the nan checks
static inline f32 gen_sqrtf(f32 x)
{
	static const f64 _half  = 0.5;
	static const f64 _three = 3.0;

	if (x > 0.0F) {
		f64 dx    = x;
		f64 guess = __frsqrte(dx);
		guess     = _half * guess * (_three - guess * guess * dx);
		guess     = _half * guess * (_three - guess * guess * dx);
		guess     = _half * guess * (_three - guess * guess * dx);
		return (f32)(dx * guess);
	} else if (x < 0.0) {
		return NAN;
	} else if (gen_fpclassifyf(x) == 1) {
		return NAN;
	}
	return x;
}

// dst = atan2(y, x), with x near zero giving plus or minus a right angle
#define GEN_ANGLE(dst, y, x)          \
	if (__fabsf(x) < FLT_MIN) {       \
		if ((y) >= 0.0F) {            \
			(dst) = 1.5707964F;       \
		} else {                      \
			(dst) = -1.5707964F;      \
		}                             \
	} else {                          \
		(dst) = gen_atan2f((y), (x)); \
	}

static inline f32 gen_randLat(HSD_Generator* gen)
{
	if (0.0F == gen->aux.sphere.latRange) {
		return M_PI * HSD_Randf();
	}
	return gen->aux.sphere.latRange * HSD_Randf();
}

static inline f32 gen_randRadius(f32 radius)
{
	if (radius < 0.0F) {
		return -radius;
	}
	return radius * HSD_Randf();
}

static inline BOOL gen_ref_DEC(void* o)
{
	BOOL ret;
	if ((ret = (HSD_OBJ(o)->ref_count == HSD_OBJ_NOREF))) {
		return ret;
	}
	return HSD_OBJ(o)->ref_count-- == 0;
}

static inline void gen_hsdDelete(void* object)
{
	if (object != NULL) {
		HSD_CLASS_METHOD(object)->release((HSD_Class*)object);
		HSD_CLASS_METHOD(object)->destroy((HSD_Class*)object);
	}
}

static inline void gen_cobjUnref(HSD_CObj* cobj)
{
	if (cobj != NULL) {
		if (cobj != NULL && gen_ref_DEC(cobj)) {
			gen_hsdDelete(cobj);
		}
	}
}

// unlinks a dead generator, or parks it when children or a shared srt still need it
static inline void gen_killNode(HSD_Generator* gen)
{
	HSD_Generator* prev;
	HSD_psAppSRT* srt;

	prev = lbl_8065BA9C;
	if (gen->callback != NULL) {
		gen->callback(gen);
	}
	if (gen->type & 0x80) {
		psKillGeneratorChild(gen);
	}
	if (gen->numChild != 0) {
		prev         = gen;
		gen->random  = 0.0F;
		gen->genLife = 1;
	} else if ((gen->type & 0x100) && gen->jobj != NULL && (gen->type & 0x3800) && (srt = gen->appsrt) != NULL && srt->gp == gen
	           && srt->usedCount != 1) {
		prev         = gen;
		gen->random  = 0.0F;
		gen->genLife = 1;
	} else {
		if (prev == NULL) {
			lbl_8065BAA0 = gen->next;
		} else {
			prev->next = gen->next;
		}
		if (gen->appsrt != NULL) {
			psRemoveGeneratorAppSRT(gen);
		}
		gen->next    = lbl_8065BAA4;
		lbl_8065BAA4 = gen;
		lbl_8065BA8C--;
		if (gen->jobj != NULL) {
			HSD_JObjUnref(gen->jobj);
			gen->jobj = NULL;
		}
	}
	lbl_8065BA9C = prev;
}

static inline void gen_kill(HSD_Generator* gen)
{
	HSD_Generator* cur;

	cur          = lbl_8065BAA0;
	lbl_8065BA9C = NULL;
	while (cur != NULL) {
		if (cur == gen) {
			gen_killNode(gen);
			if (lbl_8065BA9C != NULL) {
				while (lbl_8065BA9C->next != NULL) {
					lbl_8065BA9C = lbl_8065BA9C->next;
				}
			} else if (lbl_8065BAA0 != NULL) {
				lbl_8065BA9C = lbl_8065BAA0;
				while (lbl_8065BA9C->next != NULL) {
					lbl_8065BA9C = lbl_8065BA9C->next;
				}
			}
			return;
		}
		lbl_8065BA9C = cur;
		cur          = cur->next;
	}
}

#define GX_TEVSTAGE0     0
#define GX_TEVSTAGE1     1
#define GX_TEVSTAGE2     2
#define GX_TEXCOORD0     0
#define GX_TEXCOORD_NULL 0xFF
#define GX_TEV_ADD       0
#define GX_TB_ZERO       0
#define GX_CS_SCALE_1    0
#define GX_TEVPREV       0
#define GX_TEV_SWAP0     0
#define GX_MODULATE      0

#define GX_CC_CPREV 0
#define GX_CC_C0    2
#define GX_CC_C1    4
#define GX_CC_C2    6
#define GX_CC_TEXC  8
#define GX_CC_RASC  10
#define GX_CC_ZERO  15

#define GX_CA_APREV 0
#define GX_CA_A0    1
#define GX_CA_A1    2
#define GX_CA_A2    3
#define GX_CA_TEXA  4
#define GX_CA_RASA  5
#define GX_CA_ZERO  7

extern void GXSetTevOrder(u8 stage, u32 coord, u32 map, u32 color);
extern void GXSetTevColorOp(u8 stage, u32 op, u32 bias, u32 scale, u32 clamp, u32 out_reg);
extern void GXSetTevAlphaOp(u8 stage, u32 op, u32 bias, u32 scale, u32 clamp, u32 out_reg);
extern void GXSetTevSwapMode(u8 stage, u32 ras_swap, u32 tex_swap);
extern void GXSetNumTevStages(u8 n);
extern void GXSetNumTexGens(u8 n);
extern void GXSetTevColorIn(u8 stage, u32 a, u32 b, u32 c, u32 d);
extern void GXSetTevAlphaIn(u8 stage, u32 a, u32 b, u32 c, u32 d);
extern void GXSetTevOp(u8 stage, u32 mode);

/**
 * @note Address: 0x801E4E00
 * @note Size: 0x130
 */
void psSetupTevCommon(void)
{
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
	GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
	GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
	GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
	GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GXSetTevColorOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
	GXSetTevAlphaOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
	GXSetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
	GXSetTevSwapMode(GX_TEVSTAGE1, GX_TEV_SWAP0, GX_TEV_SWAP0);
	GXSetTevSwapMode(GX_TEVSTAGE2, GX_TEV_SWAP0, GX_TEV_SWAP0);
}

/**
 * @note Address: 0x801E4F30
 * @note Size: 0xC
 */
void psSetupTevInvalidState(void)
{
	lbl_8065BAB8 = -1;
}

/**
 * @note Address: 0x801E4F3C
 * @note Size: 0x6D8
 */
void psSetupTev(HSD_Particle* pp)
{
	s32 kind;

	kind = pp->kind & 0x80100480;
	if (kind != lbl_8065BAB8) {
		lbl_8065BAB8 = kind;
		switch (kind) {
		case 0x80000080:
			pp->kind &= ~0x80;
			lbl_8065BAB8 &= ~0x80;
			// fallthrough
		case 0x80000000:
			GXSetNumTevStages(2);
			GXSetNumTexGens(0);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_C2, GX_CC_RASC, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A2, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
			GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_C0, GX_CC_CPREV, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_A0, GX_CA_APREV, GX_CA_ZERO);
			return;
		case 0x80:
			pp->kind &= ~0x80;
			lbl_8065BAB8 &= ~0x80;
			// fallthrough
		case 0x0:
			GXSetNumTevStages(1);
			GXSetNumTexGens(0);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
			return;
		case 0x80000480:
			GXSetNumTevStages(3);
			GXSetNumTexGens(1);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_C0, GX_CC_TEXC, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A1, GX_CA_A0, GX_CA_TEXA, GX_CA_ZERO);
			GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_C2, GX_CC_CPREV, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_A2, GX_CA_APREV, GX_CA_ZERO);
			GXSetTevColorIn(GX_TEVSTAGE2, GX_CC_ZERO, GX_CC_RASC, GX_CC_CPREV, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE2, GX_CA_APREV, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
			return;
		case 0x80000400:
			GXSetNumTevStages(2);
			GXSetNumTexGens(1);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_C2, GX_CC_RASC, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A2, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
			GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_TEXC, GX_CC_CPREV, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_TEXA, GX_CA_APREV, GX_CA_ZERO);
			return;
		case 0x400:
		case 0x480:
			GXSetNumTevStages(1);
			GXSetNumTexGens(1);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_C0, GX_CC_TEXC, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A1, GX_CA_A0, GX_CA_TEXA, GX_CA_ZERO);
			return;
		case 0x100400:
			GXSetNumTevStages(1);
			GXSetNumTexGens(1);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
			GXSetTevOp(GX_TEVSTAGE0, GX_MODULATE);
			return;
		case 0x100480:
			GXSetNumTevStages(2);
			GXSetNumTexGens(1);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_C0, GX_CC_TEXC, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A1, GX_CA_A0, GX_CA_TEXA, GX_CA_ZERO);
			GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_CPREV, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_RASA, GX_CA_APREV, GX_CA_ZERO);
			return;
		case 0x80100400:
			GXSetNumTevStages(2);
			GXSetNumTexGens(1);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_C2, GX_CC_RASC, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_A2, GX_CA_RASA, GX_CA_ZERO);
			GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_TEXC, GX_CC_CPREV, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_TEXA, GX_CA_APREV, GX_CA_ZERO);
			return;
		case 0x80100480:
			GXSetNumTevStages(3);
			GXSetNumTexGens(1);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_C0, GX_CC_TEXC, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A1, GX_CA_A0, GX_CA_TEXA, GX_CA_ZERO);
			GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_C2, GX_CC_CPREV, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_A2, GX_CA_APREV, GX_CA_ZERO);
			GXSetTevColorIn(GX_TEVSTAGE2, GX_CC_ZERO, GX_CC_RASC, GX_CC_CPREV, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE2, GX_CA_ZERO, GX_CA_RASA, GX_CA_APREV, GX_CA_ZERO);
			return;
		case 0x100080:
			pp->kind &= ~0x80;
			lbl_8065BAB8 &= ~0x80;
			// fallthrough
		case 0x100000:
			GXSetNumTevStages(1);
			GXSetNumTexGens(0);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_RASA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
			return;
		case 0x80100080:
			pp->kind &= ~0x80;
			lbl_8065BAB8 &= ~0x80;
			// fallthrough
		case 0x80100000:
			GXSetNumTevStages(2);
			GXSetNumTexGens(0);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_C2, GX_CC_RASC, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_A2, GX_CA_RASA, GX_CA_ZERO);
			GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_C0, GX_CC_CPREV, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_A0, GX_CA_APREV, GX_CA_ZERO);
			return;
		}
	}
}
