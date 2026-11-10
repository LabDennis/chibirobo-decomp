#include "Version.h"
#include "types.h"

extern "C" {
extern const int sCarJointPath[2]      = { 0, 0 };
extern const char sPlayerCarTypeName[] = "Car::PlayerCarRoot";
extern const char sCarRootTypeName[]   = "Car::CarRoot";
extern const char sCarJObjHeader[]     = "jobj.h";
extern const char sCarJObjExpression[] = "jobj";
#if !defined(VERSION_PAL)
extern const char sCarTerminateMessage[] = "Terminate\n";
extern const char sCarInitMessage[]      = "Car::Init\n";
#endif
extern const float sCarZero                  = 0.0f;
extern const float sCarFrameHundredths       = 100.0f;
extern const char sCarAObjHeader[]           = "aobj.h";
extern const char sCarAObjExpression[]       = "aobj";
extern const float sCarTrailHeight           = 20.0f;
extern const float sCarOne                   = 1.0f;
extern const float sCarAnimBlendRate         = 10.0f;
extern const double sCarSignedConversionBias = 4503601774854144.0;
extern const float sCarTargetHeight          = 30.0f;
extern const float sCarTargetDepth           = -120.0f;
extern const float sCarCameraDepth           = 60.0f;
#if !defined(VERSION_PAL)
extern const char sCarRegisterMessage[] = "Car::Register\n";
#endif
extern const float sCarModelScale              = 0.5f;
extern const char sCarMtxExpression[]          = "mtx";
extern const float sCarAttachmentX             = 3.75f;
extern const float sCarAttachmentY             = 5.0f;
extern const float sCarAttachmentZ             = -1.0f;
extern const float sCarBrakeRateDivisor        = 1000.0f;
extern const double sCarUnsignedConversionBias = 4503599627370496.0;
extern const float sCarCameraFov               = 40.0f;
extern const char sCarCameraTypeName[]         = "Car::CCameraActionCar";
extern const char sCarCameraBaseTypeName[]     = "CInterfaceCameraAction";
extern const float sCarCameraEaseDivisor       = 0.1f;
}

namespace Car {
class CarRoot;
class PlayerCarRoot;
class CCameraActionCar;
} // namespace Car
class CCamera;
class CInterfaceCameraAction;
struct Vec;

extern "C" {
const void* sCarRootTypeInfo[]    = { sCarRootTypeName, 0 };
const void* sPlayerCarBaseTypes[] = { sCarRootTypeInfo, 0, 0 };
const void* sPlayerCarTypeInfo[]  = { sPlayerCarTypeName, sPlayerCarBaseTypes };

void Update__Q23Car13PlayerCarRootFUl(Car::PlayerCarRoot*, u32);
void Brake__Q23Car7CarRootFv(Car::CarRoot*);
void Start__Q23Car13PlayerCarRootFv(Car::PlayerCarRoot*);
void Clear__Q23Car13PlayerCarRootFv(Car::PlayerCarRoot*);

struct PlayerCarRootVTable {
	const void* typeInfo;
	long offset;
	void (*update)(Car::PlayerCarRoot*, u32);
	void (*brake)(Car::CarRoot*);
	void (*start)(Car::PlayerCarRoot*);
	void (*clear)(Car::PlayerCarRoot*);
};

PlayerCarRootVTable __vt__Q23Car13PlayerCarRoot = {
	sPlayerCarTypeInfo,
	0,
	Update__Q23Car13PlayerCarRootFUl,
	Brake__Q23Car7CarRootFv,
	Start__Q23Car13PlayerCarRootFv,
	Clear__Q23Car13PlayerCarRootFv,
};

void Update__Q23Car7CarRootFUl(Car::CarRoot*, u32);
void Start__Q23Car7CarRootFv(Car::CarRoot*);

struct CarRootVTable {
	const void* typeInfo;
	long offset;
	void (*update)(Car::CarRoot*, u32);
	void (*brake)(Car::CarRoot*);
	void (*start)(Car::CarRoot*);
};

CarRootVTable __vt__Q23Car7CarRoot = {
	sCarRootTypeInfo, 0, Update__Q23Car7CarRootFUl, Brake__Q23Car7CarRootFv, Start__Q23Car7CarRootFv,
};

const void* sCarCameraBaseTypeInfo[] = { sCarCameraBaseTypeName, 0 };
const void* sCarCameraBaseTypes[]    = { sCarCameraBaseTypeInfo, 0, 0 };
const void* sCarCameraTypeInfo[]     = { sCarCameraTypeName, sCarCameraBaseTypes };

void* __dt__Q23Car16CCameraActionCarFv(Car::CCameraActionCar*, short);
void PreviousUpdate__22CInterfaceCameraActionFv(CInterfaceCameraAction*);
void Update__Q23Car16CCameraActionCarFv(Car::CCameraActionCar*);
void vf14__22CInterfaceCameraActionFv(CInterfaceCameraAction*);
void SetCamera__22CInterfaceCameraActionFP7CCamera(CInterfaceCameraAction*, CCamera*);
void Init__22CInterfaceCameraActionFv(CInterfaceCameraAction*);
void vf20__22CInterfaceCameraActionFv(CInterfaceCameraAction*);
void vf24__22CInterfaceCameraActionFv(CInterfaceCameraAction*);
void vf28__22CInterfaceCameraActionFv(CInterfaceCameraAction*);
void SetEye__22CInterfaceCameraActionFfff(CInterfaceCameraAction*, float, float, float);
void SetInterest__22CInterfaceCameraActionFfff(CInterfaceCameraAction*, float, float, float);
void SetAngle__22CInterfaceCameraActionFfff(CInterfaceCameraAction*, float, float, float);
void SetEye__22CInterfaceCameraActionFRC3Vec(CInterfaceCameraAction*, const Vec&);
void SetInterest__22CInterfaceCameraActionFRC3Vec(CInterfaceCameraAction*, const Vec&);
void SetAngle__22CInterfaceCameraActionFRC3Vec(CInterfaceCameraAction*, const Vec&);
void SetDistance__22CInterfaceCameraActionFf(CInterfaceCameraAction*, float);
void SetFov__22CInterfaceCameraActionFf(CInterfaceCameraAction*, float);
void SetAngleZ__22CInterfaceCameraActionFf(CInterfaceCameraAction*, float);
s32 GetMode__Q23Car16CCameraActionCarFv(Car::CCameraActionCar*);
s32 GetFlags__Q23Car16CCameraActionCarFv(Car::CCameraActionCar*);
float GetEaseDivisor__Q23Car16CCameraActionCarFv(Car::CCameraActionCar*);
s32 vf5C__22CInterfaceCameraActionFv(CInterfaceCameraAction*);
s32 GetTarget__22CInterfaceCameraActionFv(CInterfaceCameraAction*);

struct CarCameraVTable {
	const void* typeInfo;
	long offset;
	void* (*destroy)(Car::CCameraActionCar*, short);
	void (*previousUpdate)(CInterfaceCameraAction*);
	void (*update)(Car::CCameraActionCar*);
	void (*vf14)(CInterfaceCameraAction*);
	void (*setCamera)(CInterfaceCameraAction*, CCamera*);
	void (*init)(CInterfaceCameraAction*);
	void (*vf20)(CInterfaceCameraAction*);
	void (*vf24)(CInterfaceCameraAction*);
	void (*vf28)(CInterfaceCameraAction*);
	void (*setEye)(CInterfaceCameraAction*, float, float, float);
	void (*setInterest)(CInterfaceCameraAction*, float, float, float);
	void (*setAngle)(CInterfaceCameraAction*, float, float, float);
	void (*setEyeVec)(CInterfaceCameraAction*, const Vec&);
	void (*setInterestVec)(CInterfaceCameraAction*, const Vec&);
	void (*setAngleVec)(CInterfaceCameraAction*, const Vec&);
	void (*setDistance)(CInterfaceCameraAction*, float);
	void (*setFov)(CInterfaceCameraAction*, float);
	void (*setAngleZ)(CInterfaceCameraAction*, float);
	s32 (*getMode)(Car::CCameraActionCar*);
	s32 (*getFlags)(Car::CCameraActionCar*);
	float (*getEaseDivisor)(Car::CCameraActionCar*);
	s32 (*vf5C)(CInterfaceCameraAction*);
	s32 (*getTarget)(CInterfaceCameraAction*);
};

CarCameraVTable __vt__Q23Car16CCameraActionCar = {
	sCarCameraTypeInfo,
	0,
	__dt__Q23Car16CCameraActionCarFv,
	PreviousUpdate__22CInterfaceCameraActionFv,
	Update__Q23Car16CCameraActionCarFv,
	vf14__22CInterfaceCameraActionFv,
	SetCamera__22CInterfaceCameraActionFP7CCamera,
	Init__22CInterfaceCameraActionFv,
	vf20__22CInterfaceCameraActionFv,
	vf24__22CInterfaceCameraActionFv,
	vf28__22CInterfaceCameraActionFv,
	SetEye__22CInterfaceCameraActionFfff,
	SetInterest__22CInterfaceCameraActionFfff,
	SetAngle__22CInterfaceCameraActionFfff,
	SetEye__22CInterfaceCameraActionFRC3Vec,
	SetInterest__22CInterfaceCameraActionFRC3Vec,
	SetAngle__22CInterfaceCameraActionFRC3Vec,
	SetDistance__22CInterfaceCameraActionFf,
	SetFov__22CInterfaceCameraActionFf,
	SetAngleZ__22CInterfaceCameraActionFf,
	GetMode__Q23Car16CCameraActionCarFv,
	GetFlags__Q23Car16CCameraActionCarFv,
	GetEaseDivisor__Q23Car16CCameraActionCarFv,
	vf5C__22CInterfaceCameraActionFv,
	GetTarget__22CInterfaceCameraActionFv,
};
}
