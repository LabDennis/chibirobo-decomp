inline void XSound::registerName(const char* name, s32 bank)
{
	if (name == NULL) {
		return;
	}
	for (u32 i = 0; i < 16; ++i) {
		if (mStageRequests.entries[i].mName != NULL && strcmp(mStageRequests.entries[i].mName, name) == 0) {
			return;
		}
	}
	for (u32 i = 0; i < 16; ++i) {
		if (mStageRequests.entries[i].mName == NULL) {
			++mLoadRequestCount;
			mStageRequests.entries[i].mName = name;
			mStageRequests.entries[i].mBank = bank;
#if !defined(VERSION_PAL)
			OSReport(sSoundRegisterFormat, name);
#endif
			return;
		}
	}
#if !defined(VERSION_PAL)
	OSReport(sSoundLoadQueueFull);
#endif
}
