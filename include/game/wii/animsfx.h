#ifndef GAME_WII_ANIMSFX_H
#define GAME_WII_ANIMSFX_H

#include "types.h"
#include "game/wii/csound.h"
#include "game/wii/csound3dactor.h"
#include "nw4r/snd/snd_SoundArchivePlayer.h"
#include "nw4r/snd/snd_Sound3DManager.h"

struct WiiPlayerState;

// one sound an anim plays between two frames, a negative mAnim ends an object's table. The
// GameCube's AnimSfxEvent with the actor that plays it added
struct AnimSfxEvent {
	s32 mAnim;  // _00
	f32 mStart; // _04
	f32 mEnd;   // _08
	s32 mKind;  // _0C, 1 plays when the frame passes mStart or mEnd
	s32 mActor; // _10, which of the object's six actors plays it
	s32 mSfx;   // _14
	u8 _18;     // _18
	u8 mCount;  // _19, mSfx and the next ones are played in turn
	u8 mIndex;  // _1A
	u8 _1B;     // _1B
};
STATIC_ASSERT(sizeof(AnimSfxEvent) == 0x1C);

// one of an object's sound sources. The class has no RTTI, the name is ours
class AnimSfxActor {
public:
	WiiSound3D* mManager;  // _00
	CSound3DActor* mActor; // _04
	// _08 vtable

	AnimSfxActor();
	virtual ~AnimSfxActor();

	void Reset(); // 0x8003C3C4

	// stops and frees the actor
	void Release()
	{
		if (mManager != NULL) {
			CSound3DActor* actor = mActor;
			if (actor != NULL) {
				actor->mHandle.Stop(0);
				delete actor;
				mActor = NULL;
			}
		}
	}
	// makes the actor if there is none
	void Create()
	{
		if (mActor == NULL) {
			WiiSound3D* manager = mManager;
			mActor              = new CSound3DActor(*(nw4r::snd::SoundArchivePlayer*)manager->mPlayer, manager->mManager);
		}
	}
	// moves a playing actor to pos and steps its fades
	void Update(const Vec* pos, f32 step = 2.0f)
	{
		CSound3DActor* actor = mActor;
		if (actor != NULL && actor->mHandle.IsAttachedSound()) {
			actor->SetPosition(nw4r::math::VEC3(*pos));
			actor->UpdateFade(step);
		}
	}
	// the actor's handle while it plays a sound
	CSoundHandle* GetHandle()
	{
		if (mActor != NULL && mActor->mHandle.IsAttachedSound()) {
			return &mActor->mHandle;
		}
		return NULL;
	}
};
STATIC_ASSERT(sizeof(AnimSfxActor) == 0xC);

// plays an anim's sound events as its frames pass, from the object's place in the room; vtable at 0x802E797C
class AnimSfx {
public:
	AnimSfxEvent* mEvents; // _00, ends at an anim id below 0
	s32 mAnimId;           // _04
	f32 mFrame;            // _08, the last frame Update saw
	// the object the sounds come from. todo: any XObj; WiiPlayerState is the one declaration of its vtable so far
	WiiPlayerState* mOwner;  // _0C
	AnimSfxActor mActors[6]; // _10
	// _58 vtable

	virtual void vf08();                                            // _08
	virtual void Update(s32 id, f32 frame, f32 volume);             // _0C, the volume comes in f2
	virtual BOOL CheckStart(AnimSfxEvent* event, f32 from, f32 to); // _10, starts the event
	virtual BOOL CheckStop(AnimSfxEvent* event);                    // _14, stops it
	virtual s32 GetSfx(AnimSfxEvent* event);                        // _18, its sfx
	virtual void Pause(f32 fade);                                   // _1C
	virtual void Resume(f32 fade);                                  // _20

	void Init(WiiPlayerState* owner, AnimSfxEvent* events); // 0x8003C4D0
};
STATIC_ASSERT(sizeof(AnimSfx) == 0x5C);

#endif
