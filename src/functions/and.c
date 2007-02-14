/* @header@ */
#include <stdint.h>

#include "emu/emu_cpu.h"
#include "emu/emu_cpu_data.h"
#include "emu/emu_cpu_functions.h"
#include "emu/emu_memory.h"


#ifdef INSTR_CALC_AND_SET_FLAGS
#undef INSTR_CALC_AND_SET_FLAGS
#endif // INSTR_CALC_AND_SET_FLAGS

#define INSTR_CALC_AND_SET_FLAGS(inttype, cpu, a, b, c, operation)	\
INSTR_CALC(inttype, a, b, c, operation)								\
CPU_FLAG_UNSET(cpu, f_of);										\
CPU_FLAG_UNSET(cpu, f_cf);										\
INSTR_SET_FLAG_ZF(cpu)											\
INSTR_SET_FLAG_PF(cpu)											\
INSTR_SET_FLAG_SF(cpu)											

int32_t instr_and_20(struct emu_cpu *c, struct instruction *i)
{
	/* 20 /r
	 * r/m8 AND r8
	 * AND r/m8,r8  
	 */

	if ( i->modrm.mod != 3 )
	{
		uint8_t dst;
		MEM_BYTE_READ(c, i->modrm.ea, &dst);
		INSTR_CALC_AND_SET_FLAGS(8, 
								 c, 
								 dst, 
								 *c->reg8[i->modrm.opc], 
								 dst, 
								 &)
		MEM_BYTE_WRITE(c, i->modrm.ea, dst);
	} else
	{
		INSTR_CALC_AND_SET_FLAGS(8, 
								 c, 
								 *c->reg8[i->modrm.rm], 
								 *c->reg8[i->modrm.opc], 
								 *c->reg8[i->modrm.rm], 
								 &)
	}

	return 0;
}

int32_t instr_and_21(struct emu_cpu *c, struct instruction *i)
{

	if ( i->modrm.mod != 3 )
	{
		if ( i->prefixes & PREFIX_OPSIZE )
		{
			/* 21 /r
			 * AND r/m16,r16
			 * r/m16 AND r16
			 */

			uint16_t dst;
			MEM_WORD_READ(c, i->modrm.ea, &dst);
			INSTR_CALC_AND_SET_FLAGS(16, 
									 c, 
									 dst, 
									 *c->reg16[i->modrm.opc], 
									 dst, 
									 &)
			MEM_WORD_WRITE(c, i->modrm.ea, dst);
		} else
		{
			/* 21 /r
			 * r/m32 AND r32
			 * AND r/m32,r32   
			 */
			uint32_t dst;
			MEM_DWORD_READ(c, i->modrm.ea, &dst);
			INSTR_CALC_AND_SET_FLAGS(32, 
									 c, 
									 dst, 
									 c->reg[i->modrm.opc], 
									 dst, 
									 &)
			MEM_DWORD_WRITE(c, i->modrm.ea, dst);
		}
	} else
	{
		if ( i->prefixes & PREFIX_OPSIZE )
		{
			/* 21 /r
			 * AND r/m16,r16
			 * r/m16 AND r16
			 */
			INSTR_CALC_AND_SET_FLAGS(16, 
									 c, 
									 *c->reg16[i->modrm.rm], 
									 *c->reg16[i->modrm.opc], 
									 *c->reg16[i->modrm.rm], 
									 &)
		} else
		{
			/* 21 /r
			 * r/m32 AND r32
			 * AND r/m32,r32   
			 */
			INSTR_CALC_AND_SET_FLAGS(32, 
									 c, 
									 c->reg[i->modrm.rm], 
									 c->reg[i->modrm.opc], 
									 c->reg[i->modrm.rm], 
									 &)
		}
	}

	return 0;
}

int32_t instr_and_22(struct emu_cpu *c, struct instruction *i)
{
	/* 22 /r
	 * r8 AND r/m8
	 * AND r8,r/m8     
	 */

	if ( i->modrm.mod != 3 )
	{
		uint8_t op;
		MEM_BYTE_READ(c, i->modrm.ea, &op);

		INSTR_CALC_AND_SET_FLAGS(8, 
								 c, 
								 op, 
								 *c->reg8[i->modrm.opc], 
								 *c->reg8[i->modrm.opc], 
								 &)
	} else
	{
		INSTR_CALC_AND_SET_FLAGS(8, 
								 c, 
								 *c->reg8[i->modrm.opc], 
								 *c->reg8[i->modrm.rm], 
								 *c->reg8[i->modrm.opc], 
								 &)
	}

	return 0;
}

int32_t instr_and_23(struct emu_cpu *c, struct instruction *i)
{


	if ( i->modrm.mod != 3 )
	{
		if ( i->prefixes & PREFIX_OPSIZE )
		{
			/* 23 /r
			 * r16 AND r/m16
			 * AND r16,r/m16   
			 */

			uint16_t op;
			MEM_WORD_READ(c, i->modrm.ea, &op);

			INSTR_CALC_AND_SET_FLAGS(16, 
									 c, 
									 op,
									 *c->reg16[i->modrm.opc], 
									 *c->reg16[i->modrm.opc], 
									 &)
		} else
		{
			/* 23 /r
			 * r32 AND r/m32
			 * AND r32,r/m32   
			 */

			uint32_t op;
			MEM_DWORD_READ(c, i->modrm.ea, &op);

			INSTR_CALC_AND_SET_FLAGS(32, 
									 c, 
									 op,
									 c->reg[i->modrm.opc], 
									 c->reg[i->modrm.opc], 
									 &)
		}
	} else
	{
		if ( i->prefixes & PREFIX_OPSIZE )
		{
			/* 23 /r
			 * r16 AND r/m16
			 * AND r16,r/m16   
			 */
			INSTR_CALC_AND_SET_FLAGS(16, 
									 c, 
									 *c->reg16[i->modrm.rm], 
									 *c->reg16[i->modrm.opc], 
									 *c->reg16[i->modrm.opc], 
									 &)
		} else
		{
			/* 23 /r
			 * r32 AND r/m32
			 * AND r32,r/m32   
			 */
			INSTR_CALC_AND_SET_FLAGS(32, 
									 c, 
									 c->reg[i->modrm.rm], 
									 c->reg[i->modrm.opc], 
									 c->reg[i->modrm.opc], 
									 &)
		}
	}

	return 0;
}

int32_t instr_and_24(struct emu_cpu *c, struct instruction *i)
{
	/* 24 ib
	 * AL AND imm8
	 * AND AL,imm8
	 */

	INSTR_CALC_AND_SET_FLAGS(8, 
							 c, 
							 *c->reg8[eax], 
							 *i->imm8, 
							 *c->reg8[eax], 
							 &)
	return 0;
}

int32_t instr_and_25(struct emu_cpu *c, struct instruction *i)
{


	if ( i->prefixes & PREFIX_OPSIZE )
	{
		/* 25 iw    
		 * AX AND imm16
		 * AND AX,imm16    
		 */
		INSTR_CALC_AND_SET_FLAGS(16, 
								 c, 
								 *c->reg16[ax], 
								 *i->imm16, 
								 *c->reg16[ax], 
								 &)
	} else
	{
		/* 25 id
		 * EAX AND imm32
		 * AND EAX,imm32
		 */
		INSTR_CALC_AND_SET_FLAGS(32, 
								 c, 
								 c->reg[eax], 
								 i->imm, 
								 c->reg[eax], 
								 &)
	}

	return 0;
}


int32_t instr_group_1_80_and(struct emu_cpu *cpu, uint8_t a, uint8_t b, uint8_t *result)
{
	INSTR_CALC_AND_SET_FLAGS(8, 
							 cpu, 
							 a, 
							 b, 
							 *result, 
							 &)
	return 0;
}