/*
TODO WALL:
    - IMPLEMENT ALL THE INSTRUCTIONS TESTS (ONLY PARTIAL TESTING IS DONE) AND CB PREFIX TESTS... (OH GOOD PLEASE HAVE MERCY ON ME)
    - RENAME THIS TEST FILE TO 'GameBoyCpuTest'

RESOURCES:
- TEST ROOMS EMULATION REFERENCES:
    - https://github.com/Hacktix/BullyGB/wiki
    - http://blargg.8bitalley.com/parodius/gb-tests/
*/

// #define TEST_ROOM_PATH "../../../ROMS/GameBoy/blargg-ld_r_r.gb"
#define BIOS_PATH "../../../roms/gameboy/bios.gb"

#define ExpectMessage(instr, message) "GB INSTRUCTION [" << #instr << "] EXPECTED: " << #message
#define GB_DEBUG

#include <gtest/gtest.h>
#include <stdlib.h>
#include <fstream>

extern "C"
{
#include <minemu.h>
#include <GameBoy.h>
}

// TODO: MOVE TEST FUNCTIONS INTO A HEADER FILE...
void RunProgram(const Emulation *emulator, EmulationState *emulationCtx, const uint8_t *programArray, const uint16_t programLength);

void Load_And_Store_Tests_8bit(const Emulation *emulator, EmulationState *emulationCtx);
void Load_And_Store_Tests_16bit(const Emulation *emulator, EmulationState *emulationCtx);

void CPU_ALU_Tests_8bit(const Emulation *emulator, EmulationState *emulationCtx);
void CPU_ALU_Tests_16bit(const Emulation *emulator, EmulationState *emulationCtx);

void CPU_Jumps_Tests(const Emulation *emulator, EmulationState *emulationCtx);
void CPU_BIOS_Test(const Emulation *emulator, EmulationState *emulationCtx);

class GameBoyFixture : public testing::Test
{
protected:
    Emulation *emulator;
    EmulationState *emulationCtx;

    void SetUp() override
    {
        // Initialize the emulation API
        MNE_New(emulationCtx, 1, EmulationState);

        emulator = &GameBoyEmulator;
        emulator->SetEmulationContext(static_cast<void *>(emulationCtx));
        emulator->Initialize(NULL, NULL);
    }

    void TearDown() override
    {
        // Dispose emulation resources here
        emulator->QuitProgram();
        MNE_Delete(emulationCtx);
    }
};

void RunProgram(const Emulation *emulator, EmulationState *emulationCtx, const uint8_t *programArray, const uint16_t programLength)
{
    MNE_Log("\n--------------PROGRAM START------------------\n");
    const uint8_t maxProgramSize = 0xFF;// Ok due to bus read and bus write logic, here we assume writting into the bank 00 0xFF bytes of data so no bus mux needed...
    
    // Copy program into memory
    for (int i = 0; i < maxProgramSize; i++)
    {
        if (i < programLength)
        {
            emulationCtx->bank_00[i] = programArray[i];
        }
        else
        {
            emulationCtx->bank_00[i] = 0;
        }
    }

    // Restart pc
    emulationCtx->registers.PC = 0;

    // Process instructions...
    // Iterations != program lenght so, for the  moment im iterating the emulation 4096 cycles to avoid any problems related to not executing all instructions.... (fix me)
    for (int i = 0; i < 4096; i++)
    {
        // Step emulation (if clock cycles == 0 is an error)
        if (emulator->TickEmulation() == 0) 
        {
            // Check if processed op code was executed fine
            EXPECT_TRUE(emulationCtx->registers.INSTRUCTION != GB_INVALID_INSTRUCTION);
            MNE_Log("-----------------PROGRAM ERROR-----------------\n");
            break;
        }
    }

    MNE_Log("----------------PROGRAM END------------------\n");
}

void Load_And_Store_Tests_8bit(const Emulation *emulator, EmulationState *emulationCtx)
{
    constexpr uint8_t testValue = 0x12;
 
    constexpr uint8_t loadA[] =       {0x3E, testValue}; // LD_R_N
    constexpr uint8_t copyAToB[] =    {0x78}; // LD_R_R
    constexpr uint8_t loadAFromHL[] = {0x7E}; // LD A,(HL) - 0x7E

    // Run the program loadA
    RunProgram(emulator, emulationCtx, loadA, sizeof(loadA));
    EXPECT_TRUE(emulationCtx->registers.A == testValue) << ExpectMessage(loadA, testValue);

    // Run the program copyAToB
    RunProgram(emulator, emulationCtx, copyAToB, sizeof(copyAToB));
    EXPECT_TRUE((emulationCtx->registers.A) == (emulationCtx->registers.BC >> 8)) << ExpectMessage(copyAToB, "B = A");

    // Run the program loadAFromHL
    GB_BusWrite(emulationCtx, GB_HRAM_START, testValue);
    emulationCtx->registers.HL = GB_HRAM_START;

    RunProgram(emulator, emulationCtx, loadAFromHL, sizeof(loadAFromHL));
    EXPECT_TRUE(emulationCtx->registers.A == testValue) << ExpectMessage(loadAFromHL, "A = (HL)");
}

void Load_And_Store_Tests_16bit(const Emulation *emulator, EmulationState *emulationCtx)
{
    // GB_HRAM_START = 0xFF80
    constexpr uint8_t ldRRNNInstruction[] = {0x01, 0xCD, 0xAB};   // LD BC, 0xABCD
    constexpr uint8_t ldNNSPInstruction[] = {0x08, 0xFF, 0x80};   // LD (0xFF80), SP
    constexpr uint8_t ldSPHLInstruction[] = {0xF9};               // LD SP, HL
    constexpr uint8_t pushRRInstruction[] = {0xC5};               // PUSH BC
    constexpr uint8_t popRRInstruction[] =  {0xD1};               // POP DE

    RunProgram(emulator, emulationCtx, ldRRNNInstruction, sizeof(ldRRNNInstruction));
    EXPECT_TRUE(emulationCtx->registers.BC == 0xABCD) << "LD BC, 0xABCD";

    {
        uint16_t stackPointerVal = emulationCtx->registers.SP;
        RunProgram(emulator, emulationCtx, ldNNSPInstruction, sizeof(ldNNSPInstruction));
        
        uint8_t lsb = GB_BusRead(emulationCtx, GB_HRAM_START);
        uint8_t msb = GB_BusRead(emulationCtx, GB_HRAM_START + 1);

        EXPECT_TRUE(lsb == (stackPointerVal & 0xFF) && msb == (stackPointerVal >> 8)) << "LD (0x1234), SP";
    }

    RunProgram(emulator, emulationCtx, ldSPHLInstruction, sizeof(ldSPHLInstruction));
    EXPECT_TRUE(emulationCtx->registers.SP == emulationCtx->registers.HL) << "LD SP, HL";

    {
        emulationCtx->registers.BC = 0xA0B0;
        uint16_t stackBeforePush = emulationCtx->registers.SP;
        RunProgram(emulator, emulationCtx, pushRRInstruction, sizeof(pushRRInstruction));

        uint8_t msb = GB_BusRead(emulationCtx, --stackBeforePush);
        uint8_t lsb = GB_BusRead(emulationCtx, --stackBeforePush);

        EXPECT_TRUE(msb== 0xA0 && lsb == 0xB0) << "PUSH BC";
    }

    RunProgram(emulator, emulationCtx, popRRInstruction, sizeof(popRRInstruction));
    EXPECT_TRUE(emulationCtx->registers.DE == emulationCtx->registers.BC) << "PUSH DE";
}

void CPU_Jumps_Tests(const Emulation *emulator, EmulationState *emulationCtx)
{
    constexpr uint8_t jumpInstruction[] = {0xC3, 0x10, 0x00};   // JP 0x0010
    constexpr uint8_t jumpRelativeInstruction[] = {0x18, 0x0F}; // JR 0x000F
    constexpr uint8_t callInstruction[] = {0xCD, 0x20, 0x00}; // CALL 0x0020
    constexpr uint8_t returnInstruction[] = {0xC9};

    constexpr uint8_t conditionalJumpInstruction[] = {0xC2, 0x30, 0x00};   // JP NZ, 0x0030
    constexpr uint8_t conditionalJumpRelativeInstruction[] = {0x20, 0x0F}; // JR NZ, 0x000F
    
    constexpr uint8_t conditionalCallInstruction[] = {0xC4, 0x40, 0x00}; // CALL NZ, 0x0040
    constexpr uint8_t conditionalReturnInstruction[] = {0xC0};           // RET NZ

    constexpr uint8_t restartInstruction[] = {0xC7}; // RST 00H

    // Jump instructions
    RunProgram(emulator, emulationCtx, jumpInstruction, sizeof(jumpInstruction));
    EXPECT_TRUE(emulationCtx->registers.PC == 0x0010) << "Jump instruction";

    RunProgram(emulator, emulationCtx, jumpRelativeInstruction, sizeof(jumpRelativeInstruction));
    EXPECT_TRUE(emulationCtx->registers.PC == 0x11) << "Jump Relative instruction";

    RunProgram(emulator, emulationCtx, callInstruction, sizeof(callInstruction));
    EXPECT_TRUE(emulationCtx->registers.PC == 0x0020) << "Call instruction";

    // TODO: to test this instructions is requiered to modify run program function to preserve pc state and program runnin from the current pc position
    
    // RunProgram(emulator, emulationCtx, returnInstruction, sizeof(returnInstruction));
    // EXPECT_TRUE(emulationCtx->registers.PC == 0x0013) << "Return instruction";
    // Conditional call and return instructions
    // RunProgram(emulator, emulationCtx, conditionalCallInstruction, sizeof(conditionalCallInstruction));
    // EXPECT_TRUE(emulationCtx->registers.PC == 0x0040) << "Conditional Call instruction";
    // RunProgram(emulator, emulationCtx, conditionalReturnInstruction, sizeof(conditionalReturnInstruction));
    // EXPECT_TRUE(emulationCtx->registers.PC == 0x0043) << "Conditional Return instruction";
    // Restart (RST) instructions
    // RunProgram(emulator, emulationCtx, restartInstruction, sizeof(restartInstruction));
    // EXPECT_TRUE(emulationCtx->registers.PC == 0x0000) << "Restart instruction";
}

void CPU_ALU_Tests_8bit(const Emulation *emulator, EmulationState *emulationCtx)
{
    // ALU instructions
    constexpr uint8_t testValue = 0x10; // 16 decimal

    constexpr uint8_t testOverflowValue = 0XFF;

    // 0x3E MEANS LD A,D8...
    constexpr uint8_t addInstruction[] = {0x3E, testOverflowValue, 0x87}; // ADD A, A
    constexpr uint8_t adcInstruction[] = {0x3E, testValue, 0x8F }; // ADC A, A
    constexpr uint8_t subInstruction[] = {0x3E, testValue, 0x97}; // SUB A, A
    constexpr uint8_t sbcInstruction[] = {0x3E, testValue, 0x9F}; // SBC A, A
    constexpr uint8_t andInstruction[] = {0x3E, testValue, 0xA7}; // AND A, A
    constexpr uint8_t xorInstruction[] = {0x3E, testValue, 0xAF}; // XOR A, A
    constexpr uint8_t orInstruction[] = {0x3E, testValue, 0xB7};  // OR A, A
    constexpr uint8_t cpInstruction[] = {0x3E, testValue, 0xBF};  // CP A, A

    // ALU instructions
    // ADD (testOverflowValue +testOverflowValue) = (510 & 0xFF) = 254, carry = 1
    RunProgram(emulator, emulationCtx, addInstruction, sizeof(addInstruction));
    uint8_t overflowedValue = ((testOverflowValue + testOverflowValue) & 0XFF);
    EXPECT_TRUE(emulationCtx->registers.A == overflowedValue) << "ADD A, A";

    // ADC (testvalue + testvalue) + carry
    RunProgram(emulator, emulationCtx, adcInstruction, sizeof(adcInstruction));
    EXPECT_TRUE(emulationCtx->registers.A == (testValue + testValue + 1)) << "ADC A, A";

    RunProgram(emulator, emulationCtx, subInstruction, sizeof(subInstruction));
    EXPECT_TRUE(emulationCtx->registers.A == 0x00) << "SUB A, A";

    emulationCtx->registers.CARRY_FLAG = 1; // force overflow...
    RunProgram(emulator, emulationCtx, sbcInstruction, sizeof(sbcInstruction));
    EXPECT_TRUE(emulationCtx->registers.A == 0xFF) << "SBC A, A";

    RunProgram(emulator, emulationCtx, andInstruction, sizeof(andInstruction));
    EXPECT_TRUE(emulationCtx->registers.A == testValue) << "AND A, A";

    RunProgram(emulator, emulationCtx, xorInstruction, sizeof(xorInstruction));
    EXPECT_TRUE(emulationCtx->registers.A == 0x00) << "XOR A, A";

    RunProgram(emulator, emulationCtx, orInstruction, sizeof(orInstruction));
    EXPECT_TRUE(emulationCtx->registers.A == testValue) << "OR A, A";

    RunProgram(emulator, emulationCtx, cpInstruction, sizeof(cpInstruction));

    EXPECT_TRUE(emulationCtx->registers.ZERO_FLAG) << "CP A, A";
}

void CPU_ALU_Tests_16bit(const Emulation *emulator, EmulationState *emulationCtx)
{
    constexpr uint8_t testValueLow = 0x12;
    constexpr uint8_t testValueHigh = 0x34;
    constexpr uint16_t testValuesSumResult = 0x1234 + 0xABCD; // 48641 or 0xBE01

    // 16-bit ALU instructions
    constexpr uint8_t ldHLInstruction[] = {0x21, testValueHigh, testValueLow}; // LD HL, nn 

    constexpr uint8_t addHLBCInstruction[] = {0x01, 0xCD, 0xAB, 0x09};         // LD BC, #0xABCD ; ADD HL, BC
    constexpr uint8_t incHLInstruction[] = {0x21, 0xFF, 0xFF, 0x23};           // LD HL, #0xFFFF ; INC HL
    constexpr uint8_t decHLInstruction[] = {0x21, 0x00, 0x01, 0x2B};           // LD HL, #0x0001 ; DEC HL

    constexpr uint8_t addHLSP_DDInstruction[] = {0xE8, 0x34}; // ADD HL, SP+34
    constexpr uint8_t ldHLSP_DDInstruction[] = {0xF8, 0x34};  // LD HL, SP+34

    // Populate hl with testing values
    RunProgram(emulator, emulationCtx, ldHLInstruction, sizeof(ldHLInstruction));
    
    // 16-bit ALU instructions tests
    RunProgram(emulator, emulationCtx, addHLBCInstruction, sizeof(addHLBCInstruction));
    EXPECT_TRUE(emulationCtx->registers.HL == testValuesSumResult) << "ADD HL, BC";
    
    RunProgram(emulator, emulationCtx, incHLInstruction, sizeof(incHLInstruction));
    EXPECT_TRUE(emulationCtx->registers.HL == 0X000) << "INC HL";

    RunProgram(emulator, emulationCtx, decHLInstruction, sizeof(decHLInstruction));
    EXPECT_TRUE(emulationCtx->registers.HL == 0xFF) << "DEC HL";

    // Additional tests for missing instructions
    // RunProgram(emulator, emulationCtx, addHLSP_DDInstruction, sizeof(addHLSP_DDInstruction));
    // EXPECT_TRUE(emulationCtx->registers.HL == 0x6B02) << "ADD HL, SP+34";

    // RunProgram(emulator, emulationCtx, ldHLSP_DDInstruction, sizeof(ldHLSP_DDInstruction));
    // EXPECT_TRUE(emulationCtx->registers.HL == 0x6B02) << "LD HL, SP+34";
}

const uint8_t not_the_nintendo_logo[] = {0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
                                         0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
                                         0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E};

void CPU_BIOS_Test(const Emulation *emulator, EmulationState *emulationCtx)
{
    std::ifstream biosFile (BIOS_PATH);
    
    EXPECT_TRUE(biosFile.is_open());

    // Determine the file size
    biosFile.seekg(0, std::ios::end);
    std::streamsize size = biosFile.tellg();
    biosFile.seekg(0, std::ios::beg);

    // Create a vector to hold the file data
    std::vector<uint8_t> buffer(size);

    EXPECT_TRUE(size == 256); // 256 BYTES, the last byte is supposed to be a nop
    EXPECT_TRUE(biosFile.read(reinterpret_cast<char *>(buffer.data()), size));
    MNE_HexDump(buffer.data(), buffer.size());

    // Put the logo into memory to validate the bios logo check...
    for (uint16_t addr = 0; addr < sizeof(not_the_nintendo_logo); addr++)
    {
        GB_BusWrite(emulationCtx, addr + 0x104, not_the_nintendo_logo[addr]);    
    }

    RunProgram(emulator, emulationCtx, buffer.data(), buffer.size());

    EXPECT_TRUE(emulationCtx->registers.PC == 0x00FE) << "PC SHOULD BE AT 0XFE WHEN FINISHING EXECUTING THE BIOS...";
    EXPECT_TRUE(emulationCtx->registers.INSTRUCTION == 0xE0) << "Last instruction must be (0xE0):	[LD (0xFF00+0x50),A	; 0x00fe;turn off DMG rom]";
}

// TEST_F(GameBoyFixture, Load_And_Store_8bit)
// {
//     Load_And_Store_Tests_8bit(emulator, emulationCtx);
// }

// TEST_F(GameBoyFixture, Load_And_Store_16bit)
// {
//     Load_And_Store_Tests_16bit(emulator, emulationCtx);
// }

// TEST_F(GameBoyFixture, ALU_8Bit)
// {
//     CPU_ALU_Tests_8bit(emulator, emulationCtx);
// }

// TEST_F(GameBoyFixture, ALU_16Bit)
// {
//     CPU_ALU_Tests_16bit(emulator, emulationCtx);
// }

// TEST_F(GameBoyFixture, CPU_JUMPS)
// {
//     CPU_Jumps_Tests(emulator, emulationCtx);
// }

TEST_F(GameBoyFixture, BIOS_TEST)
{
    CPU_BIOS_Test(emulator, emulationCtx);
}
