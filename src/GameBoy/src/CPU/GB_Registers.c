#include <CPU/GB_Registers.h>

void GB_SetReg8(GB_Registers *registers, uint8_t r, uint8_t value)
{
    uint8_t * halfregs = ((uint8_t * )registers->CPU); 
    halfregs[r] = value;
}

uint8_t GB_GetReg8(const GB_Registers * registers, uint8_t r)
{
    const uint8_t * halfregs = ((uint8_t * )registers->CPU); 
    return halfregs[r];
}

void GB_SetFlag(GB_Registers *registers, uint8_t flag, uint8_t value)
{
    const uint8_t flagValue = flag << value;
    const uint8_t curentflags = GB_GetReg8(registers, GB_F_OFFSET);
    GB_SetReg8(registers, GB_F_OFFSET, flagValue | curentflags);
}

uint8_t GB_GetFlag(GB_Registers *registers, uint8_t flag)
{
    return GB_GetReg8(registers, GB_F_OFFSET) >> flag;
}