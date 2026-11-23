#ifndef NW4R_G3D_RES_RES_USER_H
#define NW4R_G3D_RES_RES_USER_H
#include <nw4r/types_nw4r.h>

#include <nw4r/g3d/res/g3d_rescommon.h>
#include <nw4r/g3d/res/g3d_resdict.h>

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
    NW4R_G3D_RESOURCE_FUNC_DEF(ResUserDataItem);

    u32 GetNumData() const {
        return ref().numData;
    }

    u32 GetValueType() const {
        return ref().valueType;
    }

    s32 GetS32(u32 idx) const {
        return ofs_to_ptr<s32>(ref().ofsData)[idx];
    }
};

struct ResUserDataData {
    u32 size;       // at 0x0
    ResDicData dic; // at 0x4
};

class ResUserData : public ResCommon<ResUserDataData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DEF(ResUserData);

    ResDic GetResDic() {
        return ResDic(&ref().dic);
    }
};

} // namespace g3d
} // namespace nw4r

#endif
