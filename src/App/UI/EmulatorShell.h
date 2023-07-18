
#ifndef EMULATOR_SHELL_H
#define EMULATOR_SHELL_H

#include <stdint.h>

#define DEFAUL_COL_Y_SPACING 8

void EmuShell_Init();
void EmuShell_UpdateFrame(uint32_t * pixels);
void EmuShell_KeyPressed(const char code);
uint8_t EmuShell_Shown();

#endif