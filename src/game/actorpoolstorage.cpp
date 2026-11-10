#include "game/actorpoolstorage.h"

// Each pool is constructed on first use and followed by its destructor-chain record.
HBoxPoolStorage gHBoxPool ATTRIBUTE_ALIGN(8);
DestructorChain sHBoxDestructor;
FreePoolStorage gFreePool ATTRIBUTE_ALIGN(8);
DestructorChain sFreeDestructor;
