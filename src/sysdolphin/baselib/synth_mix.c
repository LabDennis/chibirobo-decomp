#include <dolphin/os.h>
#include <math.h>
#include <sysdolphin/synth.h>
#include <sysdolphin/synth_internal.h>

/**
 * @note Address: 0x801EDED4
 * @note Size: 0xA8
 */
void HSD_SynthSFXPause(s32 id)
{
	HSD_SynthSFXNode* node;
	BOOL enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL) {
		node->flags |= SYNTH_VOICE_PAUSED;
		HSD_SynthSFXUpdateVolume(node);
		if (node->flags & SYNTH_VOICE_STREAM_PENDING) {
			node->flags |= (SYNTH_VOICE_PAUSED | SYNTH_VOICE_PITCH_PAUSED);
		}
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EDF7C
 * @note Size: 0xA4
 */
void HSD_SynthSFXResume(s32 id)
{
	HSD_SynthSFXNode* node;
	BOOL enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL) {
		if (node->flags & SYNTH_VOICE_PAUSED) {
			node->flags &= ~(SYNTH_VOICE_PAUSED | SYNTH_VOICE_PITCH_PAUSED);
			HSD_SynthSFXUpdatePitch(node);
			HSD_SynthSFXUpdateVolume(node);
		}
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EE020
 * @note Size: 0xBC
 */
s32 HSD_SynthSFXCheck(s32 id)
{
	HSD_SynthSFXNode* node;
	s32 i;
	s32 result  = id;
	BOOL enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL) {
		if (node->flags & SYNTH_VOICE_KEY_OFF) {
			result = -1;
		} else if (!(node->flags & SYNTH_VOICE_STREAM_PENDING)) {
			for (i = 0; i < node->voice_count; i++) {
				if (node->voice[i]->pb.state == 0) {
					result = -1;
				}
			}
		}
	} else {
		result = -1;
	}
	OSRestoreInterrupts(enabled);
	return result;
}

/**
 * @note Address: 0x801EE0DC
 * @note Size: 0x220
 */
void HSD_SynthSetVolFader(s32 id, s32 vol, s32 fadeFrames, s32 flag, s32 dbMode)
{
	HSD_SynthSFXNode* node;
	s32 frames;
	s32 changed;
	BOOL enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL) {
		ASSERT_SYNTH(0x7EB, flag >= 0 && flag < USERVOL_NUM);

		frames  = fadeFrames;
		changed = 0;
		if (frames < 1) {
			frames = 1;
		}
		if (dbMode == 0) {
			node->u.user_vol[flag].target.f = (f32)(0.00392156862745098 * (f64)vol);
			if (node->u.user_vol[flag].volume != node->u.user_vol[flag].target.f) {
				node->u.user_vol[flag].duration = frames;
				changed                        = 1;
			}
		} else {
			node->u.user_vol[flag].target.i = HSD_SynthVolToDBx(node->u.user_vol[flag].volume);
			node->u.user_vol[flag].dbxRaw   = decibelsToDBx(vol);
			if (node->u.user_vol[flag].volume != HSD_SynthDBxToVol(node->u.user_vol[flag].dbxRaw)) {
				changed                        = 1;
				node->u.user_vol[flag].duration = frames;
			}
		}
		node->u.user_vol[flag].mode = dbMode;
		if (changed) {
			HSD_SynthSFXUpdateVolume(node);
		}
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EE2FC
 * @note Size: 0xD8
 */
void HSD_SynthSFXSetPan(s32 id, u8 pan, s32 flag)
{
	HSD_SynthSFXNode* node;
	BOOL enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL) {
		ASSERT_SYNTH(0x80F, flag >= 0 && flag < USERPAN_NUM);
		if (pan > 0xFE) {
			pan = 0xFE;
		}
		node->userPan[flag] = pan;
		HSD_SynthSFXUpdateMix(node, 1);
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EE3D4
 * @note Size: 0xC8
 */
void HSD_SynthSFXSetSPan(s32 id, u8 span, s32 flag)
{
	HSD_SynthSFXNode* node;
	BOOL enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL) {
		ASSERT_SYNTH(0x824, flag >= 0 && flag < USERPAN_NUM);
		node->userSPan[flag] = span;
		HSD_SynthSFXUpdateMix(node, 1);
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EE49C
 * @note Size: 0xB0
 */
void HSD_SynthSFXSetMix(s32 id, f32 mixMain, f32 mixAuxA, f32 mixAuxB)
{
	HSD_SynthSFXNode* node;
	BOOL enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL) {
		node->mixMain = mixMain;
		node->mixAuxA = mixAuxA;
		node->mixAuxB = mixAuxB;
		HSD_SynthSFXUpdateMix(node, 1);
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EE54C
 * @note Size: 0x70
 */
void HSD_SynthSFXLinkUpdateRearPitch(HSD_SynthSFXNode* rearEntry)
{
	HSD_SynthSFXNode* it;

	ASSERT_SYNTH(0x845, rearEntry->sfxID == -2);
	for (it = sSynthPendingRearPitchUpdates; it != NULL; it = it->link.rearNext) {
		if (it == rearEntry) {
			return;
		}
	}
	rearEntry->link.rearNext = sSynthPendingRearPitchUpdates;
	sSynthPendingRearPitchUpdates             = rearEntry;
}

/**
 * @note Address: 0x801EE5BC
 * @note Size: 0x1D0
 */
void HSD_SynthSFXUpdatePitch(HSD_SynthSFXNode* node)
{
	f32 pitch;
	s32 i;

	if (node->sfxID != -2) {
		if (node->flags & SYNTH_VOICE_PITCH_PAUSED) {
			pitch = 0.0f;
		} else {
			pitch = node->pitch[1] * (node->freqBase * node->pitch[0]);
		}
	} else {
		pitch = node->u.rear.pitch;
		if (node->voice[0]->pb.state == 0) {
			for (i = 0; i < node->voice_count; i++) {
				AXSetVoiceState(node->voice[i], 1);
			}
		}
	}
	if (!(node->flags & SYNTH_VOICE_STREAM_PENDING)) {
		for (i = 0; i < node->voice_count; i++) {
			s32 copysrc = 0;
			if (node->voice[i]->sync & AX_SYNC_FLAG_COPYSRC) {
				copysrc = 1;
			}
			AXSetVoiceSrcRatio(node->voice[i], pitch);
			// a full src copy already carries the ratio, keep just that flag
			if (copysrc) {
				node->voice[i]->sync &= ~AX_SYNC_FLAG_COPYRATIO;
				node->voice[i]->sync |= AX_SYNC_FLAG_COPYSRC;
			}
		}
	}
	if (node->sfxID != -2 && node->rearIndex != -1) {
		HSD_SynthSFXNode* rearEntry = &sSynthSfxNodes[node->rearIndex];
		ASSERT_SYNTH(0x86F, rearEntry->sfxID == -2);
		rearEntry->u.rear.pitch = pitch;
		HSD_SynthSFXLinkUpdateRearPitch(rearEntry);
	}
}

/**
 * @note Address: 0x801EE78C
 * @note Size: 0xF4
 */
void HSD_SynthSFXSetPitch(s32 id, s32 flag, f32 ratio)
{
	HSD_SynthSFXNode* node;
	BOOL enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL && !(node->flags & SYNTH_VOICE_KEY_OFF)) {
		ASSERT_SYNTH(0x88A, flag == 0 || flag == 1);
		node->pitch[flag] = ratio;
		HSD_SynthSFXUpdatePitch(node);
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EE880
 * @note Size: 0xB8
 */
void HSD_SynthSFXSetPriority(s32 id, s32 prio)
{
	HSD_SynthSFXNode* node;
	s32 i;
	BOOL enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL && !(node->flags & SYNTH_VOICE_KEY_OFF)) {
		for (i = 0; i < node->voice_count; i++) {
			AXSetVoicePriority(node->voice[i], prio);
		}
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline s32 tickFader(HSD_SynthVolFader* f)
{
	f32 vol;

	if (f->delay != 0) {
		f->delay -= 1;
		return 1;
	}
	if (f->duration != 0) {
		if (f->mode == 0) {
			f->volume = (f->volume * ((f32)f->duration - 1.0f)) / (f32)f->duration + f->target.f / (f32)f->duration;
		} else {
			// in db mode the target slot holds the current dbx and dbxRaw the goal
			f->target.i = ((f32)f->target.i * ((f32)f->duration - 1.0f)) / (f32)f->duration + (f32)f->dbxRaw / (f32)f->duration;
			if (f->target.i <= sSynthDecibelFloor) {
				vol = 0.0f;
			} else if (f->target.i >= 0) {
				vol = 1.0f;
			} else {
				vol = exp(1.75673301162265e-07 * f->target.i);
			}
			f->volume = vol;
		}
		f->duration -= 1;
		if (f->duration == 0) {
			return 0;
		}
		return 1;
	}
	return 0;
}

/**
 * @note Address: 0x801EE938
 * @note Size: 0x9B8
 */
void UpdateSynthVolumeFadersAndVoices(void)
{
	s32 i;
	BOOL enabled;
	HSD_SynthSFXNode* rearEntry;
	s32 masterActive;
	s32 group;
	s32 chan;
	HSD_SynthSFXNode** it;
	HSD_SynthSFXNode* entry;
	s32 changed;
	s32 vol;
	s32 delta;
	s32 lastVol;
	s32 lastDelta;
	AXPBVE rearVe;
	AXPBVE ve;

	enabled      = OSDisableInterrupts();
	masterActive = 0;
	if (sSynthMasterFader.duration != 0 && tickFader(&sSynthMasterFader)) {
		masterActive = 1;
	}

	for (group = 0; group < SYNTH_CHANNEL_COUNT / SYNTH_FADER_MASK_BITS; group++) {
		if (sSynthActiveChannelFaderMasks[group] != 0) {
			for (i = 0; i < SYNTH_FADER_MASK_BITS; i++) {
				chan = group * SYNTH_FADER_MASK_BITS + i;
				if (sSynthActiveChannelFaderMasks[group] & (1u << i)) {
					if (!tickFader(&sSynthChannelFaders[chan])) {
						sSynthActiveChannelFaderMasks[group] &= ~(1u << i);
					}
				}
			}
		}
	}

	it = &sSynthPendingVolumeUpdates;
	while (*it != NULL) {
		entry   = *it;
		changed = 0;
		if (entry->sfxID <= 0) {
			entry->volUpdatePending = 0;
			*it                     = entry->volUpdateNext;
			continue;
		}

		for (i = 0; i < USERVOL_NUM; i++) {
			if (entry->u.user_vol[i].duration != 0) {
				if (tickFader(&entry->u.user_vol[i])) {
					changed = 1;
				}
			}
		}
		if (sSynthChannelFaders[entry->channel].duration != 0) {
			changed = 1;
		}
		if (masterActive && entry->channel < SYNTH_MASTER_BYPASS_CHANNEL) {
			changed = 1;
		}

		if (!(entry->flags & (SYNTH_VOICE_KEY_OFF | SYNTH_VOICE_PAUSED))) {
			vol = 32767.0f
			    * (entry->u.user_vol[1].volume
			       * (entry->u.user_vol[0].volume
			          * (sSynthChannelFaders[entry->channel].volume
			             * (entry->channel < SYNTH_MASTER_BYPASS_CHANNEL ? sSynthStreamVolume * sSynthMasterFader.volume : sSynthStreamVolume))));
		} else {
			vol     = 0;
			changed = 0;
		}

		delta = (vol - entry->curVol) / SYNTH_SAMPLES_PER_FRAME;
		if (delta > 20) {
			delta = 20;
		} else if (delta < -20) {
			delta = -20;
		}
		for (i = 0; i < entry->voice_count; i++) {
			ASSERT_SYNTH(0x919, (entry->voice[i]->sync & AX_SYNC_USER_VE) == 0);
			AXSetVoiceVeDelta(entry->voice[i], delta);
		}
		entry->curVol += delta * SYNTH_SAMPLES_PER_FRAME;

		if (entry->rearIndex != -1) {
			rearEntry = &sSynthSfxNodes[entry->rearIndex];
			ASSERT_SYNTH(0x920, rearEntry->sfxID == -2);
			// the rearEntry voice plays last frame's ramp, phase flipped
			lastVol   = rearEntry->u.rear.lastVol;
			lastDelta = rearEntry->u.rear.lastDelta;
			if (rearEntry->voice_count == 1) {
				if (rearEntry->u.rear.side == 0) {
					rearVe.currentVolume = -lastVol;
					rearVe.currentDelta  = -lastDelta;
				} else {
					rearVe.currentVolume = lastVol;
					rearVe.currentDelta  = lastDelta;
				}
				AXSetVoiceVe(rearEntry->voice[0], &rearVe);
			} else {
				rearVe.currentVolume = -lastVol;
				rearVe.currentDelta  = -lastDelta;
				ASSERT_SYNTH(0x92F, (rearEntry->voice[0]->sync & AX_SYNC_USER_VE) == 0);
				AXSetVoiceVe(rearEntry->voice[0], &rearVe);
				rearVe.currentVolume = lastVol;
				rearVe.currentDelta  = lastDelta;
				ASSERT_SYNTH(0x933, (rearEntry->voice[1]->sync & AX_SYNC_USER_VE) == 0);
				AXSetVoiceVe(rearEntry->voice[1], &rearVe);
			}
			rearEntry->curVol += lastDelta * SYNTH_SAMPLES_PER_FRAME;
			if (rearEntry->u.rear.lastVol != vol || rearEntry->u.rear.lastDelta != delta) {
				changed = 1;
			}
			rearEntry->u.rear.lastVol   = vol;
			rearEntry->u.rear.lastDelta = delta;
		}

		if (delta == 0 && (!changed || 0.0f == (f32)vol)) {
			entry->curVol    = vol;
			ve.currentVolume = vol;
			ve.currentDelta  = 0;
			for (i = 0; i < entry->voice_count; i++) {
				entry->voice[i]->sync &= ~AX_SYNC_FLAG_SWAPVOL;
				AXSetVoiceVe(entry->voice[i], &ve);
			}
			if (entry->rearIndex != -1) {
				rearEntry->u.rear.lastVol   = vol;
				rearEntry->u.rear.lastDelta = 0;
			}
			if (!changed) {
				u8 flags;

				entry->volUpdatePending = 0;
				*it                     = entry->volUpdateNext;
				flags                   = entry->flags;
				if (flags & SYNTH_VOICE_KEY_OFF) {
					if (entry->rearIndex == -1 || lastVol == 0) {
						HSD_SynthSFXFreeVoice(entry);
					}
				} else if ((flags & (SYNTH_VOICE_PAUSED | SYNTH_VOICE_PITCH_PAUSED)) == SYNTH_VOICE_PAUSED) {
					entry->flags = flags | (SYNTH_VOICE_PAUSED | SYNTH_VOICE_PITCH_PAUSED);
					for (i = 0; i < entry->voice_count; i++) {
						HSD_SynthSFXUpdatePitch(entry);
					}
					if (sSynthDriverPauseCallback != NULL && entry->link.pstream == NULL) {
						sSynthDriverPauseCallback(entry->sfxID);
					}
				}
				continue;
			}
		}
		it = &entry->volUpdateNext;
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EF2F0
 * @note Size: 0x5C
 */
void HSD_SynthSFXUpdateVolume(HSD_SynthSFXNode* node)
{
	BOOL enabled = OSDisableInterrupts();
	if (!node->volUpdatePending && !(node->flags & SYNTH_VOICE_STREAM_PENDING)) {
		node->volUpdatePending = 1;
		node->volUpdateNext    = sSynthPendingVolumeUpdates;
		sSynthPendingVolumeUpdates           = node;
	}
	OSRestoreInterrupts(enabled);
}

#define VOICE_NODE(v) (&sSynthSfxNodes[(v)->index])

/**
 * @note Address: 0x801EF34C
 * @note Size: 0x250
 */
void AdvancePendingSynthMixUpdates(void)
{
	HSD_SynthSFXNode* node;
	s32 j;
	s32 i;
	s32 busy;
	s32 changed;
	BOOL enabled;
	HSD_SynthSFXNode** it;
	HSD_SynthSFXNode* base;
	AXVPB* voice;
	HSD_SynthAXMix mix;
	s16 delta;
	u16* current;
	const u16* target;
	AXPBVE* out;
	s32 bus;
	s32 moving;

	enabled = OSDisableInterrupts();
	it      = &sSynthPendingMixUpdates;
	while ((node = *it) != NULL) {
		busy = 0;
		for (i = 0; i < (node->rearIndex != -1) + 1; i++) {
			if (i == 0) {
				base = node;
			} else {
				base = &sSynthSfxNodes[node->rearIndex];
			}

			for (j = 0; j < node->voice_count; j++) {
				voice = base->voice[j];
				if (j == 1) {
					base = VOICE_NODE(voice);
				}
				current = base->mix.volume;
				target  = base->mixDst.volume;
				out     = mix.bus;
				changed = 0;
				for (bus = 0; bus < SYNTH_MIX_BUS_COUNT; bus++) {
					delta = (*target - *current) / SYNTH_SAMPLES_PER_FRAME;
					if (delta > 20) {
						delta = 20;
					} else if (delta < -20) {
						delta = -20;
					}
					out->currentDelta = delta;
					if (delta != 0) {
						out->currentVolume = *current;
						*current = *current + delta * SYNTH_SAMPLES_PER_FRAME;
						moving = 1;
					} else {
						out->currentVolume = *target;
						*current = *target;
						moving = 0;
					}
					changed |= moving;
					current++;
					target++;
					out++;
				}
				busy |= changed;
				AXSetVoiceMix(voice, &mix.params);
				if (i == 1 && node->voice_count == 1 && base->u.rear.side != base->u.rear.newSide && changed == 0) {
					base->mixDst      = base->u.rear.mix;
					base->u.rear.side = base->u.rear.newSide;
					HSD_SynthSFXUpdateVolume(*it);
					busy = 1;
					j--;
				}
			}
		}
		if (!busy) {
			*it                    = node->mixUpdateNext;
			node->mixUpdatePending = 0;
		} else {
			it = &node->mixUpdateNext;
		}
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: N/A
 * @note Size: 0x194
 */
void ApplySynthMixTargetImmediately(HSD_SynthSFXNode* node)
{
	HSD_SynthSFXNode** it = &sSynthPendingMixUpdates;

	while (*it != NULL) {
		if (node == *it) {
			HSD_SynthSFXNode* base;
			s32 j;
			s32 i;

			for (i = 0; i < (node->rearIndex != -1) + 1; i++) {
				if (i == 0) {
					base = node;
				} else {
					base = &sSynthSfxNodes[node->rearIndex];
				}

				for (j = 0; j < node->voice_count; j++) {
					AXVPB* voice = base->voice[j];
					HSD_SynthAXMix mix;
					const u16* target;
					AXPBVE* out;
					int bus;

					if (j == 1) {
						base = VOICE_NODE(voice);
					}
					// Apply the target volumes without a ramp.
					target = base->mixDst.volume;
					out    = mix.bus;
					for (bus = 0; bus < SYNTH_MIX_BUS_COUNT; bus++) {
						out->currentVolume = *target++;
						out->currentDelta = 0;
						out++;
					}
					base->mix = base->mixDst;
					AXSetVoiceMix(voice, &mix.params);
				}
			}
			*it                    = node->mixUpdateNext;
			node->mixUpdatePending = 0;
			return;
		}
		it = &(*it)->mixUpdateNext;
	}
}

/**
 * @note Address: 0x801EF730
 * @note Size: 0xCC
 */
void HSD_SynthSFXInitMix(HSD_SynthSFXNode* node)
{
	s32 i;

	for (i = 0; i < (node->rearIndex != -1) + 1; i++) {
		HSD_SynthSFXNode* base;

		if (i == 0) {
			base = node;
		} else {
			base = &sSynthSfxNodes[node->rearIndex];
		}

		base->mix.volume[SYNTH_MIX_MAIN_L] = 0;
		base->mix.volume[SYNTH_MIX_MAIN_R] = 0;
		base->mix.volume[SYNTH_MIX_AUX_A_L] = 0;
		base->mix.volume[SYNTH_MIX_AUX_A_R] = 0;
		base->mix.volume[SYNTH_MIX_AUX_B_L] = 0;
		base->mix.volume[SYNTH_MIX_AUX_B_R] = 0;
		base->mix.volume[SYNTH_MIX_AUX_B_S] = 0;
		base->mix.volume[SYNTH_MIX_MAIN_S] = 0;
		base->mix.volume[SYNTH_MIX_AUX_A_S] = 0;
		base->mixDst.volume[SYNTH_MIX_MAIN_L] = 0;
		base->mixDst.volume[SYNTH_MIX_MAIN_R] = 0;
		base->mixDst.volume[SYNTH_MIX_AUX_A_L] = 0;
		base->mixDst.volume[SYNTH_MIX_AUX_A_R] = 0;
		base->mixDst.volume[SYNTH_MIX_AUX_B_L] = 0;
		base->mixDst.volume[SYNTH_MIX_AUX_B_R] = 0;
		base->mixDst.volume[SYNTH_MIX_AUX_B_S] = 0;
		base->mixDst.volume[SYNTH_MIX_MAIN_S] = 0;
		base->mixDst.volume[SYNTH_MIX_AUX_A_S] = 0;
		if (i != 0) {
			base->u.rear.side = 0;
			base->u.rear.mix.volume[SYNTH_MIX_MAIN_L] = 0;
			base->u.rear.mix.volume[SYNTH_MIX_MAIN_R] = 0;
			base->u.rear.mix.volume[SYNTH_MIX_AUX_A_L] = 0;
			base->u.rear.mix.volume[SYNTH_MIX_AUX_A_R] = 0;
			base->u.rear.mix.volume[SYNTH_MIX_AUX_B_L] = 0;
			base->u.rear.mix.volume[SYNTH_MIX_AUX_B_R] = 0;
			base->u.rear.mix.volume[SYNTH_MIX_AUX_B_S] = 0;
			base->u.rear.mix.volume[SYNTH_MIX_MAIN_S] = 0;
			base->u.rear.mix.volume[SYNTH_MIX_AUX_A_S] = 0;
		}
	}
}

// itd shift by pan distance from center
static inline void clearFrontMix(HSD_SynthMix* m)
{
	m->volume[SYNTH_MIX_MAIN_L] = 0;
	m->volume[SYNTH_MIX_MAIN_R] = 0;
	m->volume[SYNTH_MIX_AUX_A_L] = 0;
	m->volume[SYNTH_MIX_AUX_A_R] = 0;
	m->volume[SYNTH_MIX_AUX_B_L] = 0;
	m->volume[SYNTH_MIX_AUX_B_R] = 0;
}

/**
 * @note Address: 0x801EF7FC
 * @note Size: 0xFC4
 */
void HSD_SynthSFXUpdateMix(HSD_SynthSFXNode* node, s32 interpolate)
{
	const f32 stereoSurroundGain = 0.7071678f;
	s32 pan;
	s32 span;
	s32 i;
	s32 shiftL;
	s32 shiftR;
	HSD_SynthSFXNode* rearEntry;
	HSD_SynthMix* m;
	f32 panL;
	f32 panR;
	f32 front;
	f32 sur;
	f32 rearL;
	f32 rearR;

	if (node->voice_count == 1) {
		pan = node->userPan[0] + node->userPan[1] - 0x7F;
		if (pan < 0) {
			pan = 0;
		}
		if (pan > 0xFE) {
			pan = 0xFE;
		}
		if (sSynthSoundMode == 0) {
			pan = 0x7F;
		}
		panL = sSynthSqrtLookup[0xFE - pan];
		panR = sSynthSqrtLookup[pan];
		if (sSynthSoundMode == 2 || sSynthSoundMode == 3 || sSynthSoundMode == 4) {
			span = node->userSPan[0] + node->userSPan[1];
			if (span > 0xFF) {
				span = 0xFF;
			}
			front = sSynthSurroundPanGain[span];
			sur   = sSynthSurroundPanGain[0xFF - span];
			if (span > 0) {
				s32 idx = 0.527559042f * pan + 60.0f;
				rearL   = sSynthSqrtLookup[0xFE - idx];
				rearR   = sSynthSqrtLookup[idx];
			} else {
				rearL = rearR = 0.0f;
			}
		} else {
			front = 1.0f;
			sur   = 0.0f;
		}

		if (node->rearIndex != -1) {
			rearEntry = &sSynthSfxNodes[node->rearIndex];
			ASSERT_SYNTH(0xA60, rearEntry->sfxID == -2);
			if (sSynthSoundMode == 4) {
				m = &rearEntry->mixDst;
				if (rearL >= rearR) {
					rearEntry->u.rear.newSide = 0;
					if (rearEntry->u.rear.side == 1) {
						clearFrontMix(m);
						m = &rearEntry->u.rear.mix;
					}
					m->volume[SYNTH_MIX_MAIN_L] = 32767.0f * (node->mixMain * (sur * (rearL - rearR)));
					m->volume[SYNTH_MIX_MAIN_R] = 0;
					m->volume[SYNTH_MIX_AUX_A_L] = 32767.0f * (node->mixAuxA * (sur * (rearL - rearR)));
					m->volume[SYNTH_MIX_AUX_A_R] = 0;
					m->volume[SYNTH_MIX_AUX_B_L] = 32767.0f * (node->mixAuxB * (sur * (rearL - rearR)));
					m->volume[SYNTH_MIX_AUX_B_R] = 0;
					sur *= rearR;
				} else {
					rearEntry->u.rear.newSide = 1;
					if (rearEntry->u.rear.side == 0) {
						clearFrontMix(m);
						m = &rearEntry->u.rear.mix;
					}
					m->volume[SYNTH_MIX_MAIN_L] = 0;
					m->volume[SYNTH_MIX_MAIN_R] = 32767.0f * (node->mixMain * (sur * (rearR - rearL)));
					m->volume[SYNTH_MIX_AUX_A_L] = 0;
					m->volume[SYNTH_MIX_AUX_A_R] = 32767.0f * (node->mixAuxA * (sur * (rearR - rearL)));
					m->volume[SYNTH_MIX_AUX_B_L] = 0;
					m->volume[SYNTH_MIX_AUX_B_R] = 32767.0f * (node->mixAuxB * (sur * (rearR - rearL)));
					sur *= rearL;
				}
			} else {
				clearFrontMix(&rearEntry->mixDst);
				if (sSynthSoundMode == 2) {
					sur *= stereoSurroundGain;
				}
			}
		} else if (sSynthSoundMode == 2) {
			sur *= stereoSurroundGain;
		}

		m = &node->mixDst;
		if (sSynthSoundMode != 3) {
			f32 l     = panL * front;
			f32 r     = panR * front;
			m->volume[SYNTH_MIX_MAIN_L] = 32767.0f * (node->mixMain * l);
			m->volume[SYNTH_MIX_MAIN_R] = 32767.0f * (node->mixMain * r);
			m->volume[SYNTH_MIX_MAIN_S] = 32767.0f * (sur * node->mixMain);
			m->volume[SYNTH_MIX_AUX_A_L] = 32767.0f * (node->mixAuxA * l);
			m->volume[SYNTH_MIX_AUX_A_R] = 32767.0f * (node->mixAuxA * r);
			m->volume[SYNTH_MIX_AUX_A_S] = 32767.0f * (sur * node->mixAuxA);
			m->volume[SYNTH_MIX_AUX_B_L] = 32767.0f * (node->mixAuxB * l);
			m->volume[SYNTH_MIX_AUX_B_R] = 32767.0f * (node->mixAuxB * r);
			m->volume[SYNTH_MIX_AUX_B_S] = 32767.0f * (sur * node->mixAuxB);
		} else {
			f32 l     = panL * front;
			f32 r     = panR * front;
			f32 sl    = rearL * sur;
			f32 sr    = rearR * sur;
			m->volume[SYNTH_MIX_MAIN_L] = 32767.0f * (node->mixMain * l);
			m->volume[SYNTH_MIX_MAIN_R] = 32767.0f * (node->mixMain * r);
			m->volume[SYNTH_MIX_AUX_B_L] = 32767.0f * (node->mixMain * sl);
			m->volume[SYNTH_MIX_AUX_B_R] = 32767.0f * (node->mixMain * sr);
			m->volume[SYNTH_MIX_AUX_A_L] = 32767.0f * (node->mixAuxA * l);
			m->volume[SYNTH_MIX_AUX_A_R] = 32767.0f * (node->mixAuxA * r);
			m->volume[SYNTH_MIX_AUX_A_S] = 32767.0f * (node->mixAuxA * sl);
			m->volume[SYNTH_MIX_AUX_B_S] = 32767.0f * (node->mixAuxA * sr);
			m->volume[SYNTH_MIX_MAIN_S] = 0;
		}
	} else {
		HSD_SynthMix* m2;

		pan = node->userPan[0] + node->userPan[1] - 0x7F;
		if (pan < 0) {
			pan = 0;
		}
		if (pan > 0xFE) {
			pan = 0xFE;
		}
		if (sSynthSoundMode == 0) {
			pan = 0x7F;
		}
		if (pan <= 0x7F) {
			panL = 1.0f;
		} else {
			panL = sSynthSqrtLookup[0xFE - (pan - 0x7F) * 2];
		}
		if (pan >= 0x7F) {
			panR = 1.0f;
		} else {
			panR = sSynthSqrtLookup[pan * 2];
		}
		if (sSynthSoundMode == 2 || sSynthSoundMode == 3 || sSynthSoundMode == 4) {
			span = node->userSPan[0] + node->userSPan[1];
			if (span > 0xFF) {
				span = 0xFF;
			}
			front = sSynthSurroundPanGain[span];
			sur   = sSynthSurroundPanGain[0xFF - span];
		} else {
			front = 1.0f;
			sur   = 0.0f;
		}

		if (node->rearIndex != -1) {
			rearEntry = &sSynthSfxNodes[node->rearIndex];
			ASSERT_SYNTH(0xB13, rearEntry->sfxID == -2);
			if (sSynthSoundMode == 4) {
				m         = &rearEntry->mixDst;
				m->volume[SYNTH_MIX_MAIN_L] = 32767.0 * (node->mixMain * (0.38 * panL * sur));
				m->volume[SYNTH_MIX_MAIN_R] = 0;
				m->volume[SYNTH_MIX_AUX_A_L] = 32767.0 * (node->mixAuxA * (0.38 * panL * sur));
				m->volume[SYNTH_MIX_AUX_A_R] = 0;
				m->volume[SYNTH_MIX_AUX_B_L] = 32767.0 * (node->mixAuxB * (0.38 * panL * sur));
				m->volume[SYNTH_MIX_AUX_B_R] = 0;
				m         = &(VOICE_NODE(rearEntry->voice[1]))->mixDst;
				m->volume[SYNTH_MIX_MAIN_L] = 0;
				m->volume[SYNTH_MIX_MAIN_R] = 32767.0 * (node->mixMain * (0.38 * panR * sur));
				m->volume[SYNTH_MIX_AUX_A_L] = 0;
				m->volume[SYNTH_MIX_AUX_A_R] = 32767.0 * (node->mixAuxA * (0.38 * panR * sur));
				m->volume[SYNTH_MIX_AUX_B_L] = 0;
				m->volume[SYNTH_MIX_AUX_B_R] = 32767.0 * (node->mixAuxB * (0.38 * panR * sur));
				sur *= 0.49;
			} else {
				clearFrontMix(&rearEntry->mixDst);
				clearFrontMix(&(VOICE_NODE(rearEntry->voice[1]))->mixDst);
				if (sSynthSoundMode == 2) {
					sur *= 0.5f;
				}
			}
		} else if (sSynthSoundMode == 2) {
			sur *= 0.5f;
		}

		m  = &node->mixDst;
		m2 = &(VOICE_NODE(node->voice[1]))->mixDst;
		if (sSynthSoundMode == 0) {
			f32 l      = panL / 2;
			f32 r      = panR / 2;
			m->volume[SYNTH_MIX_MAIN_L] = 32767.0f * (l * node->mixMain);
			m->volume[SYNTH_MIX_MAIN_R] = 32767.0f * (l * node->mixMain);
			m->volume[SYNTH_MIX_AUX_A_L] = 32767.0f * (l * node->mixAuxA);
			m->volume[SYNTH_MIX_AUX_A_R] = 32767.0f * (l * node->mixAuxA);
			m->volume[SYNTH_MIX_AUX_B_L] = 32767.0f * (l * node->mixAuxB);
			m->volume[SYNTH_MIX_AUX_B_R] = 32767.0f * (l * node->mixAuxB);
			m->volume[SYNTH_MIX_MAIN_S] = 0;
			m->volume[SYNTH_MIX_AUX_A_S] = 0;
			m->volume[SYNTH_MIX_AUX_B_S] = 0;
			m2->volume[SYNTH_MIX_MAIN_L] = 32767.0f * (r * node->mixMain);
			m2->volume[SYNTH_MIX_MAIN_R] = 32767.0f * (r * node->mixMain);
			m2->volume[SYNTH_MIX_AUX_A_L] = 32767.0f * (r * node->mixAuxA);
			m2->volume[SYNTH_MIX_AUX_A_R] = 32767.0f * (r * node->mixAuxA);
			m2->volume[SYNTH_MIX_AUX_B_L] = 32767.0f * (r * node->mixAuxB);
			m2->volume[SYNTH_MIX_AUX_B_R] = 32767.0f * (r * node->mixAuxB);
			m2->volume[SYNTH_MIX_MAIN_S] = 0;
			m2->volume[SYNTH_MIX_AUX_A_S] = 0;
			m2->volume[SYNTH_MIX_AUX_B_S] = 0;
		} else if (sSynthSoundMode != 3) {
			f32 l      = panL * front;
			f32 sl     = panL * sur;
			f32 r      = panR * front;
			f32 sr     = panR * sur;
			m->volume[SYNTH_MIX_MAIN_L] = 32767.0f * (node->mixMain * l);
			m->volume[SYNTH_MIX_AUX_A_L] = 32767.0f * (node->mixAuxA * l);
			m->volume[SYNTH_MIX_AUX_B_L] = 32767.0f * (node->mixAuxB * l);
			m->volume[SYNTH_MIX_MAIN_R] = 0;
			m->volume[SYNTH_MIX_AUX_A_R] = 0;
			m->volume[SYNTH_MIX_AUX_B_R] = 0;
			m->volume[SYNTH_MIX_MAIN_S] = 32767.0f * (node->mixMain * sl);
			m->volume[SYNTH_MIX_AUX_A_S] = 32767.0f * (node->mixAuxA * sl);
			m->volume[SYNTH_MIX_AUX_B_S] = 32767.0f * (node->mixAuxB * sl);
			m2->volume[SYNTH_MIX_MAIN_L] = 0;
			m2->volume[SYNTH_MIX_AUX_A_L] = 0;
			m2->volume[SYNTH_MIX_AUX_B_L] = 0;
			m2->volume[SYNTH_MIX_MAIN_R] = 32767.0f * (node->mixMain * r);
			m2->volume[SYNTH_MIX_AUX_A_R] = 32767.0f * (node->mixAuxA * r);
			m2->volume[SYNTH_MIX_AUX_B_R] = 32767.0f * (node->mixAuxB * r);
			m2->volume[SYNTH_MIX_MAIN_S] = 32767.0f * (node->mixMain * sr);
			m2->volume[SYNTH_MIX_AUX_A_S] = 32767.0f * (node->mixAuxA * sr);
			m2->volume[SYNTH_MIX_AUX_B_S] = 32767.0f * (node->mixAuxB * sr);
		} else {
			m->volume[SYNTH_MIX_MAIN_L] = 32767.0f * (node->mixMain * (panL * front));
			m->volume[SYNTH_MIX_MAIN_R] = 0;
			m->volume[SYNTH_MIX_AUX_B_L] = 32767.0 * (node->mixMain * (0.87 * panL * sur));
			m->volume[SYNTH_MIX_AUX_B_R] = 32767.0 * (node->mixMain * (0.49 * panL * sur));
			m->volume[SYNTH_MIX_AUX_A_L] = 32767.0f * (node->mixAuxA * (panL * front));
			m->volume[SYNTH_MIX_AUX_A_R] = 0;
			m->volume[SYNTH_MIX_AUX_A_S] = 32767.0 * (node->mixAuxA * (0.87 * panL * sur));
			m->volume[SYNTH_MIX_AUX_B_S] = 32767.0 * (node->mixAuxA * (0.49 * panL * sur));
			m->volume[SYNTH_MIX_MAIN_S] = 0;
			m2->volume[SYNTH_MIX_MAIN_L] = 0;
			m2->volume[SYNTH_MIX_MAIN_R] = 32767.0f * (node->mixMain * (panR * front));
			m2->volume[SYNTH_MIX_AUX_B_L] = 32767.0 * (node->mixMain * (0.49 * panR * sur));
			m2->volume[SYNTH_MIX_AUX_B_R] = 32767.0 * (node->mixMain * (0.87 * panR * sur));
			m2->volume[SYNTH_MIX_AUX_A_L] = 0;
			m2->volume[SYNTH_MIX_AUX_A_R] = 32767.0f * (node->mixAuxA * (panR * front));
			m2->volume[SYNTH_MIX_AUX_A_S] = 32767.0 * (node->mixAuxA * (0.49 * panR * sur));
			m2->volume[SYNTH_MIX_AUX_B_S] = 32767.0 * (node->mixAuxA * (0.87 * panR * sur));
			m2->volume[SYNTH_MIX_MAIN_S] = 0;
		}
	}

	if (interpolate == 0) {
		node->mix = node->mixDst;
		if (node->voice_count == 2) {
			HSD_SynthSFXNode* n = VOICE_NODE(node->voice[1]);
			n->mix              = n->mixDst;
		}
		if (node->rearIndex != -1) {
			rearEntry = &sSynthSfxNodes[node->rearIndex];
			ASSERT_SYNTH(0xBF5, rearEntry->sfxID == -2);
			if (rearEntry->u.rear.side != rearEntry->u.rear.newSide) {
				rearEntry->u.rear.side = rearEntry->u.rear.newSide;
				rearEntry->mixDst      = rearEntry->u.rear.mix;
			}
			rearEntry->mix = rearEntry->mixDst;
			if (rearEntry->voice_count == 2) {
				rearEntry = VOICE_NODE(rearEntry->voice[1]);
				ASSERT_SYNTH(0xBFD, rearEntry->sfxID == -3);
				rearEntry->mix = rearEntry->mixDst;
			}
		}
	}

	{
		BOOL enabled = OSDisableInterrupts();
		if (!node->mixUpdatePending && !(node->flags & SYNTH_VOICE_STREAM_PENDING)) {
			node->mixUpdatePending = 1;
			node->mixUpdateNext    = sSynthPendingMixUpdates;
			sSynthPendingMixUpdates           = node;
		}
		OSRestoreInterrupts(enabled);
	}

	shiftR = 0;
	shiftL = 0;
	if (sSynthSoundMode == 1 && node->voice_count == 1 && node->useItd != 0) {
		pan = node->userPan[0] + node->userPan[1] - 0x7F;
		if (pan < 0) {
			pan = 0;
		}
		if (pan > 0xFE) {
			pan = 0xFE;
		}
		if (pan < 0x7F) {
			shiftL = sSynthItdShift[0x7E - pan];
		} else {
			shiftR = sSynthItdShift[pan - 0x80];
		}
	}
	for (i = 0; i < node->voice_count; i++) {
		if (node->voice[i]->pb.itd.flag == 1) {
			if ((node->voice[i]->sync & AX_SYNC_FLAG_COPYITD) || interpolate == 0) {
				AXSetVoiceItdTarget(node->voice[i], shiftL, shiftR);
				node->voice[i]->pb.itd.shiftR = shiftR;
				node->voice[i]->pb.itd.shiftL = shiftL;
				node->voice[i]->sync &= ~AX_SYNC_FLAG_COPYTSHIFT;
				node->voice[i]->sync |= AX_SYNC_FLAG_COPYITD;
			} else {
				AXSetVoiceItdTarget(node->voice[i], shiftL, shiftR);
			}
		}
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x27C
 */
void HSD_SynthSFXUpdateAllVolume(s32 vol, u16 fadeFrames, s32 channel, s32 dbMode)
{
	HSD_SynthVolFader* f;
	s32 frames;
	s32 changed = 0;
	BOOL enabled;

	if (channel < 0 || channel >= SYNTH_CHANNEL_COUNT) {
		return;
	}
	f       = &sSynthChannelFaders[channel];
	enabled = OSDisableInterrupts();
	frames  = fadeFrames < 1 ? 1 : fadeFrames;
	if (dbMode == 0) {
		f32 target = (f32)(0.00392156862745098 * (f64)vol);
		if (f->volume != target) {
			f->duration = frames;
			changed     = 1;
		}
		f->target.f = target;
	} else {
		f->target.i = HSD_SynthVolToDBx(f->volume);
		f->dbxRaw   = decibelsToDBx(vol);
		if (f->volume != HSD_SynthDBxToVol(f->dbxRaw)) {
			changed     = 1;
			f->duration = frames;
		}
	}
	f->mode = dbMode;
	if (changed) {
		s32 i;
		BOOL enabled2;
		sSynthActiveChannelFaderMasks[channel / SYNTH_FADER_MASK_BITS] |= 1u << (channel & (SYNTH_FADER_MASK_BITS - 1));
		enabled2 = OSDisableInterrupts();
		for (i = 0; i < HSD_SYNTHSFXNODE_MAX; i++) {
			HSD_SynthSFXNode* node = &sSynthSfxNodes[i];
			if (node->sfxID > 0 && node->channel == channel) {
				HSD_SynthSFXUpdateVolume(node);
			}
		}
		OSRestoreInterrupts(enabled2);
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: N/A
 * @note Size: 0x210
 */
void HSD_SynthSFXSetMasterVolume(s32 vol, u16 fadeFrames, s32 dbMode)
{
	s32 frames;
	s32 changed = 0;
	BOOL enabled = OSDisableInterrupts();

	frames = fadeFrames == 0 ? 1 : fadeFrames;
	if (dbMode == 0) {
		f32 target            = (f32)(0.00392156862745098 * (f64)vol);
		sSynthMasterFader.target.f = target;
		if (sSynthMasterFader.volume != target) {
			sSynthMasterFader.duration = frames;
			changed               = 1;
		}
	} else {
		sSynthMasterFader.target.i = HSD_SynthVolToDBx(sSynthMasterFader.volume);
		sSynthMasterFader.dbxRaw   = decibelsToDBx(vol);
		if (sSynthMasterFader.volume != HSD_SynthDBxToVol(sSynthMasterFader.dbxRaw)) {
			sSynthMasterFader.duration = frames;
			changed               = 1;
		}
	}
	sSynthMasterFader.mode = dbMode;
	if (changed) {
		s32 i;
		BOOL enabled2 = OSDisableInterrupts();
		for (i = 0; i < HSD_SYNTHSFXNODE_MAX; i++) {
			HSD_SynthSFXNode* node = &sSynthSfxNodes[i];
			if (node->sfxID > 0) {
				HSD_SynthSFXUpdateVolume(node);
			}
		}
		OSRestoreInterrupts(enabled2);
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801F07C0
 * @note Size: 0x8
 */
void HSD_SynthSFXSetDriverInactivatedCallback(void (*callback)(s32))
{
	sSynthDriverInactivatedCallback = callback;
}

/**
 * @note Address: 0x801F07C8
 * @note Size: 0x8
 */
void HSD_SynthSFXSetDriverMasterClockCallback(void (*callback)(u32))
{
	sSynthDriverMasterClockCallback = callback;
}

/**
 * @note Address: 0x801F07D0
 * @note Size: 0x8
 */
void HSD_SynthSFXSetDriverPauseCallback(void (*callback)(s32))
{
	sSynthDriverPauseCallback = callback;
}

/**
 * @note Address: 0x801F07D8
 * @note Size: 0x1D0
 */
void HSD_SynthCallback(void)
{
	s32 i;
	BOOL enabled;
	s32 base;

	base = sSynthDriverFrame % 8 * 8;

	enabled = OSDisableInterrupts();
	if (sSynthPreFrameCallback != NULL) {
		sSynthPreFrameCallback();
	}

	while (sSynthPendingVoiceFreeCount-- > 0) {
		if (sSynthVoicesPendingFree[sSynthPendingVoiceFreeCount] != NULL) {
			AXFreeVoice(sSynthVoicesPendingFree[sSynthPendingVoiceFreeCount]);
		}
	}
	sSynthPendingVoiceFreeCount = 0;

	for (i = 0; i < 8; i++) {
		HSD_SynthSFXNode* node = &sSynthSfxNodes[base + i];

		if (node->sfxID > 0 && !(node->flags & SYNTH_VOICE_STREAM_PENDING) && node->voice[0]->pb.state == 0
		    && (node->voice_count == 1 || node->voice[1]->pb.state == 0)) {
			if (node->rearIndex != -1) {
				HSD_SynthSFXNode* rear = &sSynthSfxNodes[node->rearIndex];

				if (rear->voice[0]->pb.state != 0) {
					continue;
				}
				if (node->voice_count != 1 && rear->voice[1]->pb.state != 0) {
					continue;
				}
			}
			HSD_SynthSFXFreeVoice(node);
		}
	}

	while (sSynthPendingRearPitchUpdates != NULL) {
		HSD_SynthSFXUpdatePitch(sSynthPendingRearPitchUpdates);
		sSynthPendingRearPitchUpdates = sSynthPendingRearPitchUpdates->link.rearNext;
	}

	AdvancePendingSynthMixUpdates();
	UpdateSynthVolumeFadersAndVoices();
	if (sSynthDriverMasterClockCallback != NULL) {
		sSynthDriverMasterClockCallback(sSynthDriverFrame);
	}
	HSD_SynthPStreamMasterClockCallback();
	sSynthDriverFrame++;
	if (sSynthPostFrameCallback != NULL) {
		sSynthPostFrameCallback();
	}
	OSRestoreInterrupts(enabled);
}
