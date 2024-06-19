#include <Emulation/GB_Emulation.h>
#include <Emulation/GB_Instruction.h>

#include <CPU/GB_Instructions.h>
#include <Memory/GB_MemoryMap.h>

#include <string.h>
#include <minemu.h>

static EmulationState * s_systemContext;
static uint32_t s_instructionLenght = 0;

static GameBoyInstruction s_gb_instruction_set[GB_INSTRUCTION_SET_LENGHT] =
    {
        // TODO: INSTRUCTIONS WITH R PREFIX HAVE MISSING CASES..
        // 8-BIT LOAD INSTRUCTIONS
        //-------------MASK----OPCODE--HANDLER
        GB_INSTRUCTION(0xFF, 0x0000, GB_NOP),

        // LD_R_R
        // GB_INSTRUCTION(0xFF, 0x0046, GB_LD_R_R),
        // GB_INSTRUCTION(0xFF, 0x004E, GB_LD_R_R),
        // GB_INSTRUCTION(0xFF, 0x0056, GB_LD_R_R),
        // GB_INSTRUCTION(0xFF, 0x005E, GB_LD_R_R),
        // GB_INSTRUCTION(0xFF, 0x0066, GB_LD_R_R),
        // GB_INSTRUCTION(0xFF, 0x006E, GB_LD_R_R),
        // GB_INSTRUCTION(0xFF, 0x0076, GB_LD_R_R),
        // GB_INSTRUCTION(0xFF, 0x0078, GB_LD_R_R),
        GB_INSTRUCTION(0XC0, 0x40, GB_LD_R_R),

        // LD_R_N
        GB_INSTRUCTION(0xC7, 0x0006, GB_LD_R_N),

        // LD_R_HL
        GB_INSTRUCTION(0xC7, 0x46, GB_LD_R_HL),

        // 16 BIT LOAD INSTRUCTIONS
        GB_INSTRUCTION(0xCF, 0x01, GB_LD_RR_NN),
        GB_INSTRUCTION(0xFF, 0x08, GB_LD_NN_SP),
        GB_INSTRUCTION(0xFF, 0xF9, GB_LD_SP_HL),
        GB_INSTRUCTION(0xCF, 0xC5, GB_PUSH_RR),
        GB_INSTRUCTION(0xCF, 0xC1, GB_POP_RR),

        // 8 BIT ALU
        GB_INSTRUCTION(0xF8, 0x80, GB_ADD_A_R),
        GB_INSTRUCTION(0xFF, 0xC6, GB_ADD_A_N),
        GB_INSTRUCTION(0xFF, 0x86, GB_ADD_A_HL),
        GB_INSTRUCTION(0xF8, 0x88, GB_ADC_A_R),
        GB_INSTRUCTION(0xFF, 0x08, GB_ADC_A_N),
        GB_INSTRUCTION(0xFF, 0x8E, GB_ADC_A_HL),
        GB_INSTRUCTION(0xF8, 0x90, GB_SUB_R),
        GB_INSTRUCTION(0xFF, 0x10, GB_SUB_N),
        GB_INSTRUCTION(0xFF, 0x96, GB_SUB_HL),
        GB_INSTRUCTION(0xF8, 0x98, GB_SBC_A_R),
        GB_INSTRUCTION(0xFF, 0x18, GB_SBC_A_N),
        GB_INSTRUCTION(0xFF, 0x9E, GB_SBC_A_HL),
        GB_INSTRUCTION(0xF8, 0xA0, GB_AND_R),
        GB_INSTRUCTION(0xFF, 0x20, GB_AND_N),
        GB_INSTRUCTION(0xFF, 0xA6, GB_AND_HL),
        GB_INSTRUCTION(0xF8, 0xA8, GB_XOR_R),
        GB_INSTRUCTION(0xFF, 0x28, GB_XOR_N),
        GB_INSTRUCTION(0xFF, 0xAE, GB_XOR_HL),
        GB_INSTRUCTION(0xF8, 0xB0, GB_OR_R),
        GB_INSTRUCTION(0xFF, 0x30, GB_OR_N),
        GB_INSTRUCTION(0xFF, 0xB6, GB_OR_HL),
        GB_INSTRUCTION(0xF8, 0xB8, GB_CP_R),
        GB_INSTRUCTION(0xFF, 0x38, GB_CP_N),
        GB_INSTRUCTION(0xFF, 0xBE, GB_CP_HL),
        GB_INSTRUCTION(0xF8, 0x04, GB_INC_R),
        GB_INSTRUCTION(0xFF, 0x34, GB_INC_HL),
        GB_INSTRUCTION(0xF8, 0x05, GB_DEC_R),
        GB_INSTRUCTION(0xFF, 0x35, GB_DEC_HL),
        GB_INSTRUCTION(0xFF, 0x27, GB_DAA),
        GB_INSTRUCTION(0xFF, 0x2F, GB_CPL),

        // 16 BIT ALU INSTRUCTIONS
        GB_INSTRUCTION(0xCF, 0x09, GB_ADD_HL_RR),
        GB_INSTRUCTION(0xCF, 0x03, GB_INC_RR),
        GB_INSTRUCTION(0XCB, 0x0B, GB_DEC_RR),
        GB_INSTRUCTION(0xFF, 0xE8, GB_ADD_SP_DD),
        GB_INSTRUCTION(0xFF, 0xF8, GB_LD_HL_SP_PLUS_DD),

        // CPU CONTROL INSTRUCTIONS
        GB_INSTRUCTION(0xFF, 0x00, GB_NOP), // THIS MF SHOULD BE AT THE START
        GB_INSTRUCTION(0xFF, 0x3F, GB_CCF),
        GB_INSTRUCTION(0xFF, 0x37, GB_SCF),
        GB_INSTRUCTION(0xFF, 0x76, GB_HALT),  
        GB_INSTRUCTION(0xFF, 0x10, GB_STOP),
        GB_INSTRUCTION(0xFF, 0xF3, GB_DI),
        GB_INSTRUCTION(0xFF, 0xFB, GB_EI),

        // JUMP INSTRUCTIONS
        // GB_INSTRUCTION(0xFFFF, 0xC3, GB_JP_NN),
        // GB_INSTRUCTION(0xFFFF, 0xE9, GB_JP_HL),
        // GB_INSTRUCTION(0xFFFF, 0xC2, GB_JP_CC_NN),
        // GB_INSTRUCTION(0x00F8, 0x18, GB_JR_E),
        // GB_INSTRUCTION(0x00F8, 0x20, GB_JR_CC_E),
        // GB_INSTRUCTION(0xFFFF, 0xCD, GB_CALL_NN),
        // GB_INSTRUCTION(0xFFFF, 0xC4, GB_CALL_CC_NN),
        // GB_INSTRUCTION(0xFFFF, 0xC9, GB_RET),
        // GB_INSTRUCTION(0xFFFF, 0xC0, GB_RET_CC),
        // GB_INSTRUCTION(0xFFFF, 0xD9, GB_RETI),
        // GB_INSTRUCTION(0xFFFF, 0xC7, GB_RST_N)
    };

uint8_t GB_Initialize(int argc, const char ** argv)
{
    MNE_New(s_systemContext->registers, 1, GB_Registers);
    MNE_New(s_systemContext->memory, GB_MEMORY_SIZE, uint8_t);

    // Default memory values on power on/reset 

    // TODO: WHEN READING FROMN CARTIGES UNCOMMENT THE LINE BELOW
    // s_systemContext->registers->PC = 0X0100; 

    s_systemContext->registers->SP = 0xFFFE;
    
    s_systemContext->memory[GB_IE_REGISTER] = 0x00;
    s_systemContext->memory[GB_IF_REGISTER] = 0xE0;

    s_systemContext->memory[GB_LCDC_REGISTER] = 0x91;
    s_systemContext->memory[GB_STAT_REGISTER] = 0x81;

    s_systemContext->memory[GB_BGP_REGISTER] = 0xFC;
    s_systemContext->memory[GB_OBP0_REGISTER] = 0XFF;
    s_systemContext->memory[GB_OBP1_REGISTER] = 0XFF;

    // s_systemContext->IF = 0x00;
    return 0;
}

long GB_LoadProgram(const char *filePath)
{
    return MNE_ReadFile(filePath, 0, GB_PopulateMemory);
}

void GB_ParseRom(const uint8_t *buffer, size_t size)
{   
    MNE_New(s_systemContext->header, 1, GB_Header);
    GB_Header * header = s_systemContext->header;

    header->entry_point = buffer[0x100] | (buffer[0x101] << 8);

    // Extract and assign the title (null-terminated string)
    strncpy(header->title, (const char *)(buffer + 0x134), sizeof(header->title));
    header->title[sizeof(header->title) - 1] = '\0'; // Ensure null-termination

    // extracting and assigning the Game Boy Color Flag
    header->gbc_flag = buffer[0x143];

    // extracting and assigning the Manufacturer Code
    strncpy(header->manufacturer_code, (const char *)(buffer + 0x13F), sizeof(header->manufacturer_code));
    header->manufacturer_code[sizeof(header->manufacturer_code) - 1] = '\0'; // Ensure null-termination

    // extracting and assigning the CGB Flag
    header->cgb_flag = buffer[0x143];

    // extracting and assigning the New Licensee Code
    strncpy(header->new_licensee_code, (const char *)(buffer + 0x144), sizeof(header->new_licensee_code));
    header->new_licensee_code[sizeof(header->new_licensee_code) - 1] = '\0'; // Ensure null-termination

    // extracting and assigning the SGB Flag
    header->sgb_flag = buffer[0x146];

    // extracting and assigning the Cartridge Type
    header->cartridge_type = buffer[0x147];

    // extracting and assigning the ROM Size
    header->rom_size = buffer[0x148];

    // extracting and assigning the RAM Size
    header->ram_size = buffer[0x149];

    // extracting and assigning the Destination Code
    header->destination_code = buffer[0x14A];

    // extracting and assigning the Old Licensee Code
    header->old_licensee_code = buffer[0x14B];

    // extracting and assigning the Mask ROM Version Number
    header->mask_rom_version = buffer[0x14C];

    // extracting and assigning the Header Checksum
    header->header_checksum = buffer[0x14D];

    // extracting and assigning the Global Checksum
    header->global_checksum = (buffer[0x14E] << 8) | buffer[0x14F];

    // TODO: FINALIZE EXTRACTING FIELDS
#ifdef GB_DEBUG
    GB_PrintRomInfo(header);
#endif
}

void GB_PrintRomInfo(const GB_Header * header)
{
    MNE_Log("[GAME BOY HEADER DUMP] ---------------------------");
    MNE_Log("Game boy title: %s\n", header->title);
    MNE_Log("Game Boy Color Flag: 0x%02X\n", header->gbc_flag);
    MNE_Log("Manufacturer Code: %s\n", header->manufacturer_code);
    MNE_Log("CGB Flag: 0x%02X\n", header->cgb_flag);
    MNE_Log("New Licensee Code: %s\n", header->new_licensee_code);
    MNE_Log("SGB Flag: 0x%02X\n", header->sgb_flag);
    MNE_Log("Cartridge Type: 0x%02X\n", header->cartridge_type);
    MNE_Log("ROM Size: 0x%02X\n", header->rom_size);
    MNE_Log("RAM Size: 0x%02X\n", header->ram_size);
    MNE_Log("Destination Code: 0x%02X\n", header->destination_code);
    MNE_Log("Old Licensee Code: 0x%02X\n", header->old_licensee_code);
    MNE_Log("Mask ROM Version Number: 0x%02X\n", header->mask_rom_version);
    MNE_Log("Header Checksum: 0x%02X\n", header->header_checksum);
    MNE_Log("Global Checksum: 0x%04X\n", header->global_checksum);
    MNE_Log("--------------------------------------------------");
}

void GB_PopulateMemory(const uint8_t *buffer, size_t bytesRead)
{
    //For development program is stored at 0x0000, when using the boot rom (bios) program should start at  0x1000
    uint16_t ramIndex = 0;// replace with 0x1000...
    uint16_t bufferIndex = 0;

    // TODO: add memory offset cond: (ramindex + 0x1000) < bytesRead
    for (; ramIndex < bytesRead; ramIndex++, bufferIndex++)
    {
        s_systemContext->memory[ramIndex] = buffer[bufferIndex];
    }
    //TODO: ADD RETURN TO CHECK 
    GB_ParseRom(buffer,bytesRead);
}

void GB_QuitProgram()
{
    if (s_systemContext == NULL) 
    {
        return;
    }

    MNE_Delete(s_systemContext->registers);
    MNE_Delete(s_systemContext->memory);
    MNE_Delete(s_systemContext->header);
}

void GB_TickTimers()
{

}

int GB_TickEmulation()
{
    //TODO: Implement CPU step function that take into account prefetch cycle (before executing an instruction fetch another one then execute both in order)
    
    if (s_systemContext == NULL) return 0;
    // Fetch
    const uint8_t instr = s_systemContext->memory[s_systemContext->registers->PC++]; //TODO: CHANGE THIS FOR A BUS READ!!!
    
    const GameBoyInstruction* fetchedInstruction = GB_FetchInstruction(instr);
    uint8_t clockCycles = 0;

    // Instruction execution
    if (fetchedInstruction->handler != NULL) 
    {
#ifdef GB_DEBUG
            MNE_Log(fetchedInstruction->handlerName, instr, s_systemContext->registers->PC);
#endif
            s_systemContext->registers->INSTRUCTION = instr;
            clockCycles = fetchedInstruction->handler(s_systemContext);
           
         return clockCycles;
    }
    else
    {
        s_systemContext->registers->INSTRUCTION = GB_INVALID_INSTRUCTION; // Invalidate last instruction entry
        s_systemContext->memory[GB_HALT_REGISTER] = 0x01; // TODO: IT MAY B NEEDED USING BUS WRITE TO TRIGGER REGISTER BEHEAVIOURS

        MNE_Log("[INVALID INSTRUCTION]:[%02X][HALTED]\n", instr);
        return 0;
    }
}

GameBoyInstruction* GB_FetchInstruction(const uint8_t opcode)
{
    for (int  i = 0x00; i <= GB_INSTRUCTION_SET_LENGHT; i++)
    {
        uint16_t opmask = (opcode & s_gb_instruction_set[i].maskl);

        if (opmask == s_gb_instruction_set[i].opcode)
        {
            return &s_gb_instruction_set[i];
        }
    }

    return NULL;
}
// TODO: FIX USAGE WHEN EMPTY INITIALIZED(EMULATOR FIRST RUN ON THE MENU...)
void GB_SetEmulationContext(const void *context)
{
    s_systemContext = (EmulationState*) context;
}

EmulationInfo GB_GetInfo()
{
    EmulationInfo info;
    strcpy(info.name, "MINEMU-GB");

    // Real display resolution
    info.displayWidth = GB_DISPLAY_WIDHT;
    info.displayHeight = GB_DISPLAY_HEIGHT;
    info.displayScaleFactor = 2;
    // Window resolution
    info.UIConfig.frameWidth = GB_DISPLAY_WIDHT * 2;
    info.UIConfig.frameHeight = GB_DISPLAY_HEIGHT * 2;

    return info;
}

// TODO: THIS FUNCTION ONLY DOES MINIMAL TILE RENDERING AND IS A TEST... (LCD CONTROLLER GOES HERE ALONG WITH THE PPU.. PROCESING)
void GB_OnRender(uint32_t* pixels, const int64_t w, const int64_t h)
{
    const uint8_t gameboy_tile[] = {0x3C, 0x7E, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x7E, 0x5E, 0x7E, 0x0A, 0x7C, 0x56, 0x38, 0x7C};
    const uint32_t pallete[] = {0x9BBC0FFF, 0x8BAC0FFF, 0x306230FF, 0x306230FF};

    // if (s_systemContext == NULL) return;

    for (int i = 0; i < GB_DISPLAY_HEIGHT; i++)
    {
        for (int j = 0; j < GB_DISPLAY_WIDHT; j++)
        {
            const uint64_t pixelIndex = i * GB_DISPLAY_WIDHT + j;
        
                pixels[pixelIndex] = pallete[(j + i)% 4];
        }
    }
}
