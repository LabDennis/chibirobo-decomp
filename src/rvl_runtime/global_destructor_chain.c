#include "PowerPC_EABI_Support/Runtime/global_destructor_chain.h"
#include "PowerPC_EABI_Support/Runtime/MWCPlusLib.h"

// the RELs link this too, built without small data, addresses below are the dol ones

DestructorChain* __global_destructor_chain;

/**
 * @note Address: 0x80150E60
 * @note Size: 0x18
 */
void* __register_global_object(void* object, void* destructor, void* regmem)
{
	((DestructorChain*)regmem)->next       = __global_destructor_chain;
	((DestructorChain*)regmem)->destructor = destructor;
	((DestructorChain*)regmem)->object     = object;
	__global_destructor_chain              = (DestructorChain*)regmem;

	return object;
}

/**
 * @note Address: 0x80150E18
 * @note Size: 0x48
 */
void __destroy_global_chain(void)
{
	DestructorChain* iter;

	while ((iter = __global_destructor_chain) != 0) {
		__global_destructor_chain = iter->next;
		DTORCALL_COMPLETE(iter->destructor, iter->object);
	}
}

