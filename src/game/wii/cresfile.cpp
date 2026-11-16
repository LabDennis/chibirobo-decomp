// the Wii resource file unit, cresfile.cpp by the file name its OSPanic calls
// carry: ResourceFile's refcounted file handle, the CFileRequest an owner
// drives an asynchronous read with and CResFile's brres accessor init.
#include "game/wii/cresfile.h"
#include "Dolphin/os.h"

extern "C" {
// the nw4r resource walkers CResFile's methods call: they still live in the
// unnamed library unit around 0x8025B000
void* fn_8025BA34(CResFile* file, s32 index);
void fn_8025C76C(CResFile* file, void** ref);
void fn_8025CBA0(CResFile* file);
void fn_8025D9A4(void** dic);
// the nw4r panic (file, line, message, ...); ends in PPCHalt
void fn_80236070(const char* file, int line, const char* message, ...);
}

// the g3d assert strings live in .data in retail
static char sG3dResfileAcAssert[] = "NW4R:Failed assertion !((u32)p & 0x1f)";
static char sG3dResfileAcFile[]   = "g3d_resfile_ac.h";

s32 ResourceFile::GetResult()
{
	s32 result = CFile::GetResult();
	if (result == 0) {
		mRefCount++;
		CloseHandle();
	}
	return result;
}

BOOL ResourceFile::Close()
{
	if (--mRefCount == 0) {
		CFile::Close();
		return TRUE;
	}
	return FALSE;
}

void CFileRequest::Reset()
{
	mFile   = NULL;
	mState  = 0;
	mResult = 0;
}

BOOL CFileRequest::Request(ResourceFile* file, const char* path, u32 flags, u32 offset, u32 size)
{
	mFile   = file;
	mFlags  = flags;
	mResult = 0;
	mOffset = offset;
	mSize   = size;
	if (mState != 0) {
		return FALSE;
	}
	if (file->mBuffer != NULL) {
		mState = 10;
		return TRUE;
	}
	s32 result;
	if (size != 0) {
		result = file->ReadAsync(path, offset, size, TRUE);
	} else {
		result = file->ReadAsync(path, 0, 0, FALSE);
	}
	if (result >= 0) {
		mState++;
	} else {
		mResult = result;
		if ((mFlags & 0x01000000) != 0) {
			mFile->Close();
			return FALSE;
		}
		OSPanic(__FILE__, 0x6f, "%s(%d)\n", path, result);
	}
	return TRUE;
}

BOOL CFileRequest::Update()
{
	switch (mState) {
	case 1: {
		s32 result = mFile->GetResult();
		if (result == 0) {
			mState++;
		} else if (result >= 0) {
			break;
		} else {
			mResult = result;
			if (result == -2) {
				mState = 0;
				break;
			}
			if ((mFlags & 0x01000000) != 0) {
				mFile->Close();
				mState = 0;
				break;
			}
			OSPanic(__FILE__, 0x8a, "%s(%d)\n", mFile->mPath, result);
		}
	}
	// fallthrough
	case 2:
		mState = 0;
		break;
	case 10: {
		s32 useCount = mFile != NULL ? mFile->mRefCount : 0;
		if (useCount > 0) {
			mFile->mRefCount++;
			mState = 0;
		} else if (mFile->mBuffer == NULL) {
			mState = 0;
			Request(mFile, mFile->mPath, mFlags, mOffset, mSize);
		}
		break;
	}
	}
	return mState == 0;
}

// NONMATCHING: epilogue restores r31 before the saved lr; retail does the
// opposite. Body is identical, only the restore order is scheduled differently.
void CFileRequest::Release()
{
	if (mFile != NULL && mFile->mBuffer != NULL && mFile->Close()) {
		mFile = NULL;
	}
	mState = 0;
}

void CResFile::Init(void* data, BOOL bindCommon, BOOL bindAll)
{
	if (((u32)data & 0x1f) != 0) {
		fn_80236070(sG3dResfileAcFile, 0x3c, sG3dResfileAcAssert);
	}
	mRef = data;
	if (bindCommon) {
		fn_8025CBA0(this);
	}
	if (bindAll) {
		void* ref = mRef;
		fn_8025C76C(this, &ref);
	}
	mDics[0] = NULL;
	mDics[1] = NULL;
	mDics[2] = NULL;
	mDics[3] = NULL;
	mDics[4] = NULL;
	mDics[5] = NULL;
	mDics[6] = NULL;
	mDics[8] = NULL;
	mDics[7] = NULL;
}

void CResFile::BindTop(s32 index, BOOL bind)
{
	mTop = fn_8025BA34(this, index);
	if (bind) {
		fn_8025D9A4(&mTop);
	}
}
