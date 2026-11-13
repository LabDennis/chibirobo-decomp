#include "game/attachmentvalues.h"

extern "C" void SetAttachmentCountById(AttachmentValues* values, s32 id, s32 value)
{
	values->SetCount(id, value);
}
