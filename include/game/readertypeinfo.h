#pragma once

struct ReaderTypeInfo {
	const char* name;
	const void* baseTypes;
};

struct ReaderBaseEntry {
	const ReaderTypeInfo* type;
	int offset;
};
template <int Count>
struct ReaderBaseList {
	ReaderBaseEntry entries[Count];
	const ReaderTypeInfo* end;
};
