extern "C" {
#include <sysdolphin/mobj.h>
}

struct ArchiveMethodName {
	char name[10];
	char padding[2];
};
#if defined(VERSION_GGTP01)
#define NAME_PADDING { 0, 0 }
#else
#define NAME_PADDING { '0', '0' }
#endif
ArchiveMethodName sObjectReadAsyncName = { "ReadAsync", NAME_PADDING };
ArchiveMethodName sObjectReadCheckName = { "ReadCheck", NAME_PADDING };

void XCObjInfoInit();
HSD_MObjInfo xCObj = { XCObjInfoInit };
