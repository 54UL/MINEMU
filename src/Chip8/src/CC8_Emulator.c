#include <stdlib.h>
#include <stdio.h>
#include <CC8_Emulator.h>
#include <CC8_Instructions.h>

KHASH_MAP_INIT_INT(instrHashTable, instructionFnPtr)

#define CC8_FONT_ADDR_START 0x000
#define CC8_BOOT_ADDR_START 0x200
#define CC8_FILE_PROGRAM_BUFFER_SIZE 4096

static CC8_Memory * s_currentChipCtx = NULL;
static khash_t(instrHashTable)* instructionTable;

static instructionFnPtr s_instructions[CC8_INSTRUCTION_SET_LENGHT];
static uint16_t s_instructionMasks[CC8_INSTRUCTION_SET_LENGHT];
uint16_t s_instrIndex = 0;

const uint8_t CC8_FONT[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // "0"
    0x20, 0x60, 0x20, 0x20, 0x70, // "1"
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // "2"
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // "3"
    0x90, 0x90, 0xF0, 0x10, 0x10, // "4"
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // "5"
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // "6"
    0xF0, 0x10, 0x20, 0x40, 0x40, // "7"
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // "8"
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // "9"
    0xF0, 0x90, 0xF0, 0x90, 0x90, // "A"
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // "B"
    0xF0, 0x80, 0x80, 0x80, 0xF0, // "C"
    0xE0, 0x90, 0x90, 0x90, 0xE0, // "D"
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // "E"
    0xF0, 0x80, 0xF0, 0x80, 0x80  // "F"
};

void AddInstruction(uint16_t mask, instructionFnPtr fn)
{
    s_instructionMasks[s_instrIndex] = mask;
    s_instructions[s_instrIndex] = fn;
    s_instrIndex++;
}

static int columCount =0; // TODO: remove this debug var
instructionFnPtr FetchInstruction(uint16_t opcode)
{
    // Implement hashing!!!
    for (int  i = 0; i < 33 ; i++)
    {
        uint16_t opmask = (opcode & s_instructionMasks[i]);

        if ((opmask == s_instructionMasks[i]) && ((opcode < s_instructionMasks[i + 1]) || opmask == 0x8000 || opmask == 0XE000 || opmask == 0XF000))
        {
          
            return s_instructions[i];
        }
    }

    return NULL;
}

void CC8_BuildInstructionLUT()
{
    //Allocate hash table
    instructionTable = kh_init(instrHashTable);
    // Compute instructions LUT

    AddInstruction(0x00E0, CC8_CLS);
    AddInstruction(0x00EE, CC8_RET);
    AddInstruction(0x1000, CC8_JMP);
    AddInstruction(0x2000, CC8_CALL);
    AddInstruction(0x3000, CC8_SE_VX_BYTE);
    AddInstruction(0x4000, CC8_SNE_VX_BYTE); 
    AddInstruction(0x5000, CC8_SE_VX_VY); 
    AddInstruction(0x6000, CC8_LD_VX_BYTE); 
    AddInstruction(0x7000, CC8_ADD_VX_BYTE); 
    AddInstruction(0x8000, CC8_LD_VX_VY); 
    AddInstruction(0x8001, CC8_OR_VX_VY); 
    AddInstruction(0x8002, CC8_AND_VX_VY); 
    AddInstruction(0x8003, CC8_XOR_VX_VY); 
    AddInstruction(0x8004, CC8_ADD_VX_VY); 
    AddInstruction(0x8005, CC8_SUB_VX_VY); 
    AddInstruction(0x8006, CC8_SHR_VX_VY); 
    AddInstruction(0x8007, CC8_SUBN_VX_VY); 
    AddInstruction(0x800E, CC8_SHL_VX_VY); 
    AddInstruction(0x9000, CC8_SNE_VX_VY); 
    AddInstruction(0xA000, CC8_LD_I_ADDR); 
    AddInstruction(0xB000, CC8_JP_V0_ADDR); 
    AddInstruction(0xC000, CC8_RND_VX_BYTE); 
    AddInstruction(0xD000, CC8_DRW_VX_VY_NIBBLE);
    AddInstruction(0xE09E, CC8_SKP_VX);
    AddInstruction(0xE0A1, CC8_SKNP_VX);
    AddInstruction(0xF007, CC8_LD_VX_DT);
    AddInstruction(0xF00A, CC8_LD_VX_K);
    AddInstruction(0xF015, CC8_LD_DT_VX);
    AddInstruction(0xF018, CC8_LD_ST_VX);
    AddInstruction(0xF01E, CC8_ADD_I_VX);
    AddInstruction(0xF029, CC8_LD_F_VX);
    AddInstruction(0xF033, CC8_LD_B_VX);
    AddInstruction(0xF055, CC8_LD_I_VX);
    AddInstruction(0xF065, CC8_LD_VX_I);

    // TODO: ADD S-CHIP8 instructions below...
}

// TODO:MOVE TO A PROPPER HEADER FILE called utils or such
void HexDump(uint8_t *buffer, size_t size)
{
    // Print header
    printf("Hex dump:\n");
    printf("---------------------------------------------------------------\n");
    printf("Offset |                         Hexadecimal                   \n");
    printf("-------|-------------------------------------------------------\n");

    // Print buffer contents in hexadecimal format with ASCII representation
    int i = 0;
    int j = 0;

    for (i = 0; i < size; i += 16)
    {
        // Print offset
        printf("%06X | ", i);

        // Print hexadecimal values
        for (j = i; j < i + 16 && j < size; j++)
        {
            printf("%02X ", buffer[j]);
        }
        printf("\n");
    }

    // Print footer
    printf("-----------------------------------------------------------------\n");
}

long CC8_LoadProgram(const char *filePath)
{
    CC8_BuildInstructionLUT();
    FILE *file = fopen(filePath, "rb");
    if (file == NULL)
    {
        printf("Unable to open file %s\n", filePath);
        return 0;
    }

    // Get the size of the file
    fseek(file, 0L, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    printf("Loaded file, %s: %i bytes\n", filePath, file_size);

    // Allocate a buffer for reading the file
    uint8_t *buffer = (uint8_t *)calloc(1, file_size + 1);
    size_t bytes_read = fread(buffer, 1, file_size, file);
    s_currentChipCtx->PC = CC8_BOOT_ADDR_START;

    // LOAD PROGRAM
    uint16_t addr = 0;
    uint16_t loop_index = 0;
    for (addr = CC8_BOOT_ADDR_START; (addr < CC8_BOOT_ADDR_START + bytes_read); addr++)
    {
        s_currentChipCtx->RAM[addr] = buffer[loop_index++];
    }

    // LOAD FONT
    loop_index = 0;
    printf("Loaded font size: %i\n", sizeof(CC8_FONT));
    for (addr = CC8_FONT_ADDR_START; (addr < CC8_FONT_ADDR_START + sizeof(CC8_FONT)); addr++)
    {
        s_currentChipCtx->RAM[CC8_FONT_ADDR_START + addr] = CC8_FONT[loop_index++];
    }

    // TODO: ADD DEBUG FLAG for dumping hex
    HexDump(buffer, bytes_read);

    // Clean up resources
    free(buffer);
    fclose(file);
    return file_size;
}

void CC8_QuitProgram()
{
    if (s_currentChipCtx != NULL)
    {
        free(s_currentChipCtx);
        s_currentChipCtx = NULL;
    }
}

void CC8_Step(uint16_t opcode)
{
    if (opcode == 0x0000) // NOP
    { 
        s_currentChipCtx->INSTRUCTION = 0x0000;
        return; 
    }

    InstructionContext ctx;
    
    // Instruction decoding
    ctx.x = (opcode >> 8) & 0x0F;
    ctx.y = (opcode >> 4) & 0x0F;
    ctx.nnn = opcode & 0x0FFF;
    ctx.kk = opcode & 0x00FF;
    ctx.n = opcode & 0x000F;
    ctx.memory = s_currentChipCtx;

    // Instruction fetching
    instructionFnPtr fetchedInstruction = FetchInstruction(opcode);
    
    // Instruction execution
    if (fetchedInstruction != NULL)
    {
        printf("[%04X] ", opcode);
        if (columCount++ >= 16)
        {
            printf("\n");
            columCount = 0;
        }

        fetchedInstruction(&ctx);
        s_currentChipCtx->INSTRUCTION = opcode; // Stores executed opcode to check later if was running fine
    }
    else
    {
        printf("[Invalid opcode: %04X]\n", opcode);
        columCount = 0;
        s_currentChipCtx->INSTRUCTION = CC8_INVALID_INSTRUCTION; // Invalidate last instruction entry
    }
}

void CC8_TickDelayTimer()
{
    if (s_currentChipCtx->DELAY != 0)
        s_currentChipCtx->DELAY--;
}

int CC8_TickEmulation()
{
    if (s_currentChipCtx == NULL) return 0;

    uint8_t higherByte = s_currentChipCtx->RAM[s_currentChipCtx->PC];
    uint8_t lowerByte = s_currentChipCtx->RAM[s_currentChipCtx->PC + 1];
    uint16_t value16 = (higherByte << 8) | lowerByte;
    
    CC8_Step(value16);

    s_currentChipCtx->PC += 2;
    return 1;
}

void CC8_SetKeyboardValue(uint8_t key)
{
    s_currentChipCtx->KEYBOARD = key;
}

void CC8_SetEmulationContext(const void *context)
{
    s_currentChipCtx = (CC8_Memory *) context;
}