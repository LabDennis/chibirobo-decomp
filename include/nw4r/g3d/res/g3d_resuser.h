#ifndef NW4R_G3D_RES_RES_USER_H
#define NW4R_G3D_RES_RES_USER_H
#include <nw4r/types_nw4r.h>

#include <nw4r/g3d/res/g3d_rescommon.h>
#include <nw4r/g3d/res/g3d_resdict.h>
#include <nw4r/db/db_assert.h>

namespace nw4r {
namespace g3d {

struct ResUserDataItemData {
	enum DataType { S32 };

	u32 nameID;    // at 0x0
	s32 ofsData;   // at 0x4
	u32 numData;   // at 0x8
	u32 valueType; // at 0xC
};

class ResUserDataItem : public ResCommon<ResUserDataItemData> {
public:
	explicit ResUserDataItem(void* pData = NULL)
	    : ResCommon<ResUserDataItemData>(pData)
	{
		if ((u32)pData & 0x3) {
			db::Panic("g3d_resuser_ac.h", 38, "NW4R:Failed assertion !((u32)p & 0x3)");
		}
	}

	ResUserDataItemData& ref()
	{
		if (!IsValid()) {
			db::Panic("g3d_resuser_ac.h", 38, "%s::%s: Object not valid.", "ResUserDataItem", "ref");
		}
		return *ptr();
	}

	const ResUserDataItemData& ref() const
	{
		if (!IsValid()) {
			db::Panic("g3d_resuser_ac.h", 38, "%s::%s: Object not valid.", "ResUserDataItem", "ref");
		}
		return *ptr();
	}

	bool operator==(const ResUserDataItem& other) const { return ptr() == other.ptr(); }
	bool operator!=(const ResUserDataItem& other) const { return ptr() != other.ptr(); }

	u32 GetNumData() const { return ref().numData; }

	u32 GetValueType() const { return ref().valueType; }

	s32 GetS32(u32 idx) const
	{
		if (GetValueType() != ResUserDataItemData::S32) {
			db::Panic("g3d_resuser_ac.h", 54, "NW4R:Failed assertion GetValueType() == ResUserDataItemData::S32");
		}
		return ofs_to_ptr<s32>(ref().ofsData)[idx];
	}
};

struct ResUserDataData {
	u32 size;       // at 0x0
	ResDicData dic; // at 0x4
};

class ResUserData : public ResCommon<ResUserDataData> {
public:
	explicit ResUserData(void* pData = NULL)
	    : ResCommon<ResUserDataData>(pData)
	{
	}

	ResUserDataData& ref()
	{
		if (!IsValid()) {
			db::Panic("g3d_resuser_ac.h", 87, "%s::%s: Object not valid.", "ResUserData", "ref");
		}
		return *ptr();
	}

	const ResUserDataData& ref() const
	{
		if (!IsValid()) {
			db::Panic("g3d_resuser_ac.h", 87, "%s::%s: Object not valid.", "ResUserData", "ref");
		}
		return *ptr();
	}

	bool operator==(const ResUserData& other) const { return ptr() == other.ptr(); }
	bool operator!=(const ResUserData& other) const { return ptr() != other.ptr(); }

	ResDic GetResDic() { return ResDic(&ref().dic); }
};

} // namespace g3d
} // namespace nw4r

#endif
