#include <nw4hbm/lyt.h>

namespace nw4hbm {
namespace lyt {

ResourceAccessor::~ResourceAccessor() {}

ResourceAccessor::ResourceAccessor() {}

ut::Font* ResourceAccessor::GetFont(const char* pName) {
#pragma unused(pName)

    return NULL;
}

} // namespace lyt
} // namespace nw4hbm
