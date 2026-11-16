#include "game/wii/cgame.h"

// Retail 0x8003366C-0x800354DC holds no function anybody wrote. It is the tail of the file that defines the game
// object: its static initializer (0x800332D8, in the unit before this one) constructs gGame, and the compiler put
// the constructors it generated for CGame's bases and members here, followed by the destructor thunks of the
// nw4r::snd classes the sound members derive from. Each one comes out of its class definition, so the source of
// this range is the definition below plus the headers.
//
// 0x8003366C 0x408 CBase::CBase()
// 0x80033A74 0x0C8 CGsTevStage::CGsTevStage()
// 0x80033B3C 0x02C CGsChannel::CGsChannel()
// 0x80033B68 0x020 CLight::CLight()
// 0x80033B88 0x00C the 0x20 entries of CSoundArchivePlayer's table at _F4 (CSoundTrackSlot in csound.h)
// 0x80033B94 0x04C CTpl::CTpl()
// 0x80033BE0 0x028 CCamera::CCamera()
// 0x80033C08 0x010 CStopwatch::CStopwatch()
// 0x80033C18 0x044 CScrnFade::CScrnFade()
// 0x80033C5C 0x044 CScrnWipeCircle::CScrnWipeCircle()
// 0x80033CA0 0x05C CScrnWipeTex::CScrnWipeTex()
// 0x80033CFC 0x044 CScrnWipeCustom::CScrnWipeCustom()
// 0x80033D40 0x044 CScrnWipeTile::CScrnWipeTile()
// 0x80033D84 0x044 CScrnWipeCine::CScrnWipeCine()
// 0x80033DC8 0x044 CScrnRoboeyes::CScrnRoboeyes()
// 0x80033E0C 0x060 FbSubwin::FbSubwin()
// 0x80033E6C 0x05C ScrnTex::ScrnTex()
// 0x80033EC8 0x044 GameScrnColor::GameScrnColor()
// 0x80033F0C 0x044 GameScrnColor1::GameScrnColor1()
// 0x80033F50 0x07C Logo::Logo()
// 0x80033FCC 0x108 Title::Title()
// 0x800340D4 0x224 Cursor::Cursor(), the two 0x9B0 objects at Title _28
// 0x800342F8 0x060 CEvt::CEvt()
// 0x80034358 0x268 XObjDemo::XObjDemo(), the three 0xF30 objects at Title _1388
// 0x800345C0 0x030 CbPane::CbPane(), Title's eleven at _4DD8
// 0x800345F0 0x08C InfoScrn::InfoScrn()
// 0x8003467C 0x04C the two 0x268 entries at CGame _B7B0, each with two CFile at _84
// 0x800346C8 0x038 ResourceFile::ResourceFile()
// 0x80034700 0x108 CMsg::CMsg()
// 0x80034808 0x224 the two 0x9B0 objects at CMsgFont _120: the same members and vtable as Cursor, but a type of its own
// 0x80034A2C 0x028 MsgFontGlyph::MsgFontGlyph()
// 0x80034A54 0x078 CMotionControlCamera::CMotionControlCamera()
// 0x80034ACC 0x6B0 CCameraActions::CCameraActions()
// 0x8003517C 0x010 CInterpolationEaseInOut::CInterpolationEaseInOut()
// 0x8003518C 0x010 CInterpolationLinear::CInterpolationLinear()
// 0x8003519C 0x010 CInterpolationEaseOut::CInterpolationEaseOut()
// 0x800351AC 0x010 CInterpolationEaseOutDivide::CInterpolationEaseOutDivide()
// 0x800351BC 0x010 CInterpolationNone::CInterpolationNone()
// 0x800351CC 0x24C the object at CGame _EC1A8: a CTexObj, a Cursor at _128 and a 0x74 library object at _AD8
// 0x80035418 0x04C CGame::FbMirrorEx::FbMirrorEx()
// 0x80035464 0x040 the object at CGame _EE210: a CCamera and a CTexObj at _1FC
// 0x800354A4 0x008 @12@ thunk, nw4r::snd::SoundArchivePlayer::detail_ConvertLabelStringToSoundId
// 0x800354AC 0x008 @12@ thunk, nw4r::snd::SoundArchivePlayer::detail_SetupSound
// 0x800354B4 0x008 @12@ thunk, CSoundArchivePlayer::~CSoundArchivePlayer()
// 0x800354BC 0x008 @4@ thunk, nw4r::snd::Sound3DManager::detail_FreeAmbientArg
// 0x800354C4 0x008 @4@ thunk, nw4r::snd::Sound3DManager::detail_AllocAmbientArg
// 0x800354CC 0x008 @4@ thunk, CSound3DManager::~CSound3DManager()
// 0x800354D4 0x008 @8@ thunk, CSoundFxReverbHiDpl2's function at 0x800304B4
//
// NONMATCHING: the compiler only emits a member's constructor once CGame declares that member with its class.
// The members cgame.h still keeps as byte ranges or as single fields of a larger class (the CGs state objects, the
// sound classes, the wipe at _5010, FbSubwin, Logo, Title, the menu, InfoScrn, CEvt, the two CMsg, the mirrors and
// what follows them) give nothing here, and CBase, CMotionControlCamera and CCameraActions differ where their own
// members are still untyped.
CGame gGame;
