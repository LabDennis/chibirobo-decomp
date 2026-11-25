#include <global.h>
#include <math.h>
#include <string.h>
#include <sysdolphin/bytecode.h>
#include <sysdolphin/gobjproc.h>
#include <sysdolphin/list.h>
#include <sysdolphin/random.h>

#define DEG_TO_RAD 0.017453292519943295
#define RAD_TO_DEG 57.29577951308232

#define HSD_ASSERT(line, cond) ((cond) ? ((void)0) : __assert("bytecode.c", line, #cond))

enum {
	BC_NOP = 0x00,
	BC_RETURN = 0x01,
	BC_PUSH_ARGUMENT = 0x02,
	BC_JUMP_IF_TRUE = 0x03,
	BC_JUMP = 0x04,
	BC_DROP = 0x05,
	BC_PUSH_LITERAL = 0x06,
	BC_FLOAT_TO_INT = 0x07,
	BC_INT_TO_FLOAT = 0x08,
	BC_NEGATE_FLOAT = 0x09,
	BC_NEGATE_INT = 0x0A,
	BC_RANDOM_BIT = 0x0B,
	BC_RANDOM_FLOAT = 0x0C,
	BC_SIN = 0x0D,
	BC_COS = 0x0E,
	BC_TAN = 0x0F,
	BC_ASIN = 0x10,
	BC_ACOS = 0x11,
	BC_ATAN = 0x12,
	BC_LOG = 0x13,
	BC_EXP = 0x14,
	BC_ABS_FLOAT = 0x15,
	BC_SQRT = 0x16,
	BC_ADD_FLOAT = 0x17,
	BC_SUB_FLOAT = 0x18,
	BC_MUL_FLOAT = 0x19,
	BC_DIV_FLOAT = 0x1A,
	BC_MOD_FLOAT = 0x1B,
	BC_ADD_INT = 0x1C,
	BC_SUB_INT = 0x1D,
	BC_MUL_INT = 0x1E,
	BC_DIV_INT = 0x1F,
	BC_MOD_INT = 0x20,
	BC_POW = 0x21,
	BC_MIN_FLOAT = 0x22,
	BC_MAX_FLOAT = 0x23,
	BC_MIN_INT = 0x24,
	BC_MAX_INT = 0x25,
	BC_ATAN2 = 0x26,
	BC_RANDOM_INT_RANGE = 0x27,
	BC_ABS_INT = 0x28,
	BC_LESS_INT = 0x29,
	BC_GREATER_INT = 0x2A,
	BC_LESS_EQUAL_INT = 0x2B,
	BC_GREATER_EQUAL_INT = 0x2C,
	BC_EQUAL_INT = 0x2D,
	BC_NOT_EQUAL_INT = 0x2E,
	BC_LOGICAL_AND = 0x2F,
	BC_LOGICAL_OR = 0x30,
	BC_LOGICAL_NOT = 0x31,
	BC_LOGICAL_XOR = 0x32,
	BC_LESS_FLOAT = 0x33,
	BC_GREATER_FLOAT = 0x34,
	BC_LESS_EQUAL_FLOAT = 0x35,
	BC_GREATER_EQUAL_FLOAT = 0x36,
	BC_EQUAL_FLOAT = 0x37,
	BC_NOT_EQUAL_FLOAT = 0x38,
	BC_BITWISE_AND = 0x39,
	BC_BITWISE_OR = 0x3A,
	BC_BITWISE_XOR = 0x3B,
	BC_COPY_STACK_VALUE = 0x3C,
	BC_UNIMPLEMENTED = 0xFF,
};

static inline void RequireBinaryOperands(HSD_SList* stack, u32 line)
{
	HSD_ASSERT(line, stack);
	HSD_ASSERT(line, stack->next);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline f32 bytecode_atan2f(f32 y, f32 x)
{
	if (x == 0.0F) {
		return y >= 0.0F ? 1.5707963267948966F : -1.5707963267948966F;
	}
	return atan2(y, x);
}

/**
 * @note Address: 0x801D466C
 * @note Size: 0x15C8
 */
f32 HSD_ByteCodeEval(u8* bytecode, f32* args, s32 nb_args)
{
	HSD_SList* stack;
	int i;
	u8 opcode;
	int operand_bytes;
	unsigned int operand;
	HSD_SList* list;
	float top_float, next_float;
	int next_int, top_int;

	stack = NULL;
	operand_bytes = 0;

	if (bytecode == NULL) {
		return 0.0F;
	}

	for (;;) {
		if (operand_bytes > 0) {
			operand_bytes--;
			operand = (operand << 8) | *bytecode++;

			if (operand_bytes != 0) {
				continue;
			}

			switch (opcode) {
			case BC_PUSH_ARGUMENT: {
				u32 word;
				HSD_ASSERT(281, operand < nb_args);
				// Copy the bit pattern without converting the value.
				memcpy(&word, &args[operand], sizeof(word));
				stack = HSD_SListPrepend(stack, (void*)word);
				break;
			}
			case BC_DROP:
				for (i = 0; i < operand; i++) {
					stack = HSD_SListRemove(stack);
				}
				break;
			case BC_COPY_STACK_VALUE: {
				list = stack;
				i    = 0;
				while (list != NULL && i < operand) {
					list = list->next;
					i++;
				}
				if (list == NULL) {
					OSReport("specified stack doesn't exist (%d).\n", operand);
					HSD_Panic("bytecode.c", 299, "");
				} else {
					stack = HSD_SListPrepend(stack, list->data);
				}
				break;
			}
			case BC_JUMP_IF_TRUE:
				HSD_ASSERT(307, stack);
				if (stack->int_value != 0) {
					bytecode += operand;
				}
				stack = HSD_SListRemove(stack);
				break;
			case BC_JUMP:
				bytecode += operand;
				break;
			case BC_PUSH_LITERAL:
				stack = HSD_SListPrepend(stack, (void*)operand);
				break;
			case BC_UNIMPLEMENTED:
				HSD_Panic("bytecode.c", 323, "not yet implemented.\n");
				/* fallthrough */
			default:
				HSD_Panic("bytecode.c", 326, "unexpected byte code.\n");
				break;
			}
			continue;
		}

		opcode = *bytecode++;
		switch (opcode) {
		case BC_NOP:
			break;
		case BC_RETURN:
			HSD_ASSERT(339, stack);
			top_float = stack->float_value;
			while (stack != NULL) {
				stack = HSD_SListRemove(stack);
			}
			return top_float;
		case BC_DROP:
		case BC_COPY_STACK_VALUE:
		case BC_UNIMPLEMENTED:
			operand_bytes = 1;
			operand       = 0;
			break;
		case BC_PUSH_ARGUMENT:
		case BC_JUMP_IF_TRUE:
		case BC_JUMP:
			operand_bytes = 2;
			operand       = 0;
			break;
		case BC_PUSH_LITERAL:
			operand_bytes = 4;
			operand       = 0;
			break;
		case BC_FLOAT_TO_INT:
			HSD_ASSERT(376, stack);
			{
				stack->int_value = (int)stack->float_value;
			}
			break;
		case BC_INT_TO_FLOAT:
			HSD_ASSERT(381, stack);
			stack->float_value = (float)stack->int_value;
			break;
		case BC_NEGATE_FLOAT:
			HSD_ASSERT(387, stack);
			stack->float_value = -stack->float_value;
			break;
		case BC_NEGATE_INT:
			HSD_ASSERT(393, stack);
			stack->int_value = -stack->int_value;
			break;
		case BC_RANDOM_BIT:
			HSD_ASSERT(399, stack);
			stack->int_value = HSD_Randi(2);
			break;
		case BC_RANDOM_FLOAT:
			HSD_ASSERT(405, stack);
			stack->float_value = HSD_Randf();
			break;
		case BC_SIN:
			HSD_ASSERT(411, stack);
			stack->float_value = (float)sin((float)(DEG_TO_RAD * (double)stack->float_value));
			break;
		case BC_COS:
			HSD_ASSERT(417, stack);
			stack->float_value = (float)cos((float)(DEG_TO_RAD * (double)stack->float_value));
			break;
		case BC_TAN:
			HSD_ASSERT(423, stack);
			stack->float_value = (float)tan((float)(DEG_TO_RAD * (double)stack->float_value));
			break;
		case BC_ASIN:
			HSD_ASSERT(429, stack);
			stack->float_value = (float)(RAD_TO_DEG * (float)asin(stack->float_value));
			break;
		case BC_ACOS:
			HSD_ASSERT(435, stack);
			stack->float_value = (float)(RAD_TO_DEG * (float)acos(stack->float_value));
			break;
		case BC_ATAN:
			HSD_ASSERT(441, stack);
			stack->float_value = (float)(RAD_TO_DEG * (float)atan(stack->float_value));
			break;
		case BC_LOG:
			HSD_ASSERT(447, stack);
			stack->float_value = (float)log(stack->float_value);
			break;
		case BC_EXP:
			HSD_ASSERT(453, stack);
			stack->float_value = (float)exp(stack->float_value);
			break;
		case BC_ABS_FLOAT:
			HSD_ASSERT(459, stack);
			if (stack->float_value < 0.0F) {
				stack->float_value = -stack->float_value;
			}
			break;
		case BC_ABS_INT:
			HSD_ASSERT(467, stack);
			{
				next_int = stack->int_value;
				if (next_int < 0) {
					stack->int_value = -next_int;
				}
			}
			break;
		case BC_SQRT:
			HSD_ASSERT(474, stack);
			stack->float_value = dolsqrtf(stack->float_value);
			break;
		case BC_LOGICAL_NOT:
			HSD_ASSERT(480, stack);
			stack->int_value = !stack->int_value;
			break;
		case BC_ADD_FLOAT:
			RequireBinaryOperands(stack, 501);
			top_float = stack->float_value;
			stack = HSD_SListRemove(stack);
			stack->float_value = stack->float_value + top_float;
			break;
		case BC_SUB_FLOAT:
			RequireBinaryOperands(stack, 507);
			top_float = stack->float_value;
			stack = HSD_SListRemove(stack);
			stack->float_value = stack->float_value - top_float;
			break;
		case BC_MUL_FLOAT:
			RequireBinaryOperands(stack, 513);
			top_float = stack->float_value;
			stack = HSD_SListRemove(stack);
			stack->float_value = stack->float_value * top_float;
			break;
		case BC_DIV_FLOAT:
			RequireBinaryOperands(stack, 519);
			top_float = stack->float_value;
			stack = HSD_SListRemove(stack);
			stack->float_value = stack->float_value / top_float;
			break;
		case BC_MOD_FLOAT:
			RequireBinaryOperands(stack, 525);
			top_float = stack->float_value;
			stack = HSD_SListRemove(stack);
			next_float = stack->float_value;
			stack->float_value = (float)fmod(next_float, top_float);
			break;
		case BC_ADD_INT:
			RequireBinaryOperands(stack, 531);
			top_int = stack->int_value;
			stack = HSD_SListRemove(stack);
			stack->int_value = (stack->int_value + top_int);
			break;
		case BC_SUB_INT:
			RequireBinaryOperands(stack, 536);
			top_int = stack->int_value;
			stack = HSD_SListRemove(stack);
			stack->int_value = (stack->int_value - top_int);
			break;
		case BC_MUL_INT:
			RequireBinaryOperands(stack, 541);
			top_int = stack->int_value;
			stack = HSD_SListRemove(stack);
			stack->int_value = (stack->int_value * top_int);
			break;
		case BC_DIV_INT:
			RequireBinaryOperands(stack, 546);
			top_int = stack->int_value;
			stack = HSD_SListRemove(stack);
			stack->int_value = (stack->int_value / top_int);
			break;
		case BC_MOD_INT:
			RequireBinaryOperands(stack, 551);
			top_int = stack->int_value;
			stack = HSD_SListRemove(stack);
			next_int = stack->int_value;
			stack->int_value = (next_int % top_int);
			break;
		case BC_POW:
			RequireBinaryOperands(stack, 556);
			top_float = stack->float_value;
			stack = HSD_SListRemove(stack);
			stack->float_value = (float)pow(stack->float_value, top_float);
			break;
		case BC_MIN_FLOAT:
			RequireBinaryOperands(stack, 562);
			top_float = stack->float_value;
			stack = HSD_SListRemove(stack);
			if (stack->float_value > top_float) {
				stack->float_value = top_float;
			}
			break;
		case BC_MAX_FLOAT:
			RequireBinaryOperands(stack, 569);
			top_float = stack->float_value;
			stack = HSD_SListRemove(stack);
			if (stack->float_value < top_float) {
				stack->float_value = top_float;
			}
			break;
		case BC_MIN_INT:
			RequireBinaryOperands(stack, 576);
			top_int = stack->int_value;
			stack = HSD_SListRemove(stack);
			if (stack->int_value > top_int) {
				stack->int_value = top_int;
			}
			break;
		case BC_MAX_INT:
			RequireBinaryOperands(stack, 583);
			top_int = stack->int_value;
			stack = HSD_SListRemove(stack);
			if (stack->int_value < top_int) {
				stack->int_value = top_int;
			}
			break;
		case BC_ATAN2: {
			RequireBinaryOperands(stack, 590);
			top_float = stack->float_value;
			stack = HSD_SListRemove(stack);
			next_float = stack->float_value;
			stack->float_value = (float)(RAD_TO_DEG * bytecode_atan2f(next_float, top_float));
			break;
		}
		case BC_LESS_FLOAT:
			RequireBinaryOperands(stack, 596);
			top_float = stack->float_value;
			stack = HSD_SListRemove(stack);
			next_float = stack->float_value;
			stack->int_value = (next_float < top_float);
			break;
		case BC_GREATER_FLOAT:
			RequireBinaryOperands(stack, 601);
			top_float = stack->float_value;
			stack = HSD_SListRemove(stack);
			next_float = stack->float_value;
			stack->int_value = (next_float > top_float);
			break;
		case BC_LESS_EQUAL_FLOAT:
			RequireBinaryOperands(stack, 606);
			top_float = stack->float_value;
			stack = HSD_SListRemove(stack);
			stack->int_value = (stack->float_value <= top_float);
			break;
		case BC_GREATER_EQUAL_FLOAT:
			RequireBinaryOperands(stack, 611);
			top_float = stack->float_value;
			stack = HSD_SListRemove(stack);
			next_float = stack->float_value;
			stack->int_value = (next_float >= top_float);
			break;
		case BC_EQUAL_FLOAT:
			RequireBinaryOperands(stack, 616);
			top_float = stack->float_value;
			stack = HSD_SListRemove(stack);
			next_float = stack->float_value;
			stack->int_value = (next_float == top_float);
			break;
		case BC_NOT_EQUAL_FLOAT:
			RequireBinaryOperands(stack, 621);
			top_float = stack->float_value;
			stack = HSD_SListRemove(stack);
			next_float = stack->float_value;
			stack->int_value = (next_float != top_float);
			break;
		case BC_LESS_INT:
			RequireBinaryOperands(stack, 626);
			top_int = stack->int_value;
			stack = HSD_SListRemove(stack);
			stack->int_value = (stack->int_value < top_int);
			break;
		case BC_GREATER_INT:
			RequireBinaryOperands(stack, 631);
			top_int = stack->int_value;
			stack = HSD_SListRemove(stack);
			stack->int_value = (stack->int_value > top_int);
			break;
		case BC_LESS_EQUAL_INT:
			RequireBinaryOperands(stack, 636);
			top_int = stack->int_value;
			stack = HSD_SListRemove(stack);
			stack->int_value = (stack->int_value <= top_int);
			break;
		case BC_GREATER_EQUAL_INT:
			RequireBinaryOperands(stack, 641);
			top_int = stack->int_value;
			stack = HSD_SListRemove(stack);
			stack->int_value = (stack->int_value >= top_int);
			break;
		case BC_EQUAL_INT:
			RequireBinaryOperands(stack, 646);
			top_int = stack->int_value;
			stack = HSD_SListRemove(stack);
			stack->int_value = (stack->int_value == top_int);
			break;
		case BC_NOT_EQUAL_INT:
			RequireBinaryOperands(stack, 651);
			top_int = stack->int_value;
			stack = HSD_SListRemove(stack);
			stack->int_value = (stack->int_value != top_int);
			break;
		case BC_LOGICAL_AND:
			RequireBinaryOperands(stack, 656);
			top_int = stack->int_value;
			stack = HSD_SListRemove(stack);
			next_int = stack->int_value;
			stack->int_value = next_int && top_int;
			break;
		case BC_LOGICAL_OR:
			RequireBinaryOperands(stack, 661);
			top_int = stack->int_value;
			stack = HSD_SListRemove(stack);
			next_int = stack->int_value;
			stack->int_value = next_int || top_int;
			break;
		case BC_LOGICAL_XOR:
			RequireBinaryOperands(stack, 666);
			top_int = stack->int_value;
			stack = HSD_SListRemove(stack);
			next_int = stack->int_value;
			stack->int_value = (next_int == 0 && top_int != 0) || (next_int != 0 && top_int == 0);
			break;
		case BC_BITWISE_AND:
			RequireBinaryOperands(stack, 671);
			top_int = stack->int_value;
			stack = HSD_SListRemove(stack);
			stack->int_value = stack->int_value & top_int;
			break;
		case BC_BITWISE_OR:
			RequireBinaryOperands(stack, 676);
			top_int = stack->int_value;
			stack = HSD_SListRemove(stack);
			stack->int_value = stack->int_value | top_int;
			break;
		case BC_BITWISE_XOR:
			RequireBinaryOperands(stack, 681);
			top_int = stack->int_value;
			stack = HSD_SListRemove(stack);
			stack->int_value = stack->int_value ^ top_int;
			break;
		case BC_RANDOM_INT_RANGE:
			RequireBinaryOperands(stack, 687);
			top_int = stack->int_value;
			stack = HSD_SListRemove(stack);
			next_int = stack->int_value;
			stack->int_value = (next_int + HSD_Randi((top_int - next_int) + 1));
			break;
		default:
			OSReport("unexpected opcode 0x%x.\n", opcode);
			HSD_Panic("bytecode.c", 693, "");
			break;
		}
	}
}
