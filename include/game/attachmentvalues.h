#ifndef _GAME_ATTACHMENTVALUES_H
#define _GAME_ATTACHMENTVALUES_H

#include "types.h"

struct AttachmentValue {
	s32 mId;
	s32 mValue;
};

struct AttachmentValues {
	AttachmentValue mEntries[9];

	s32 Find(s32 id) const
	{
		for (s32 i = 0; i < ARRAY_SIZE(mEntries); ++i) {
			if (mEntries[i].mId == id)
				return i;
		}
		return -1;
	}

	void SetCount(s32 id, s32 value)
	{
		s32 index = Find(id);
		if (index >= 0)
			mEntries[index].mValue = value;
	}
};
STATIC_ASSERT(sizeof(AttachmentValues) == 0x48);

extern "C" void SetAttachmentCountById(AttachmentValues* values, s32 id, s32 value);

#endif
