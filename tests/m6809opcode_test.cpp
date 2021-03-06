#include <m6809.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdint.h>

using ::testing::Return;
using ::testing::_;

class MockMemory
{
public:
    MOCK_METHOD1(Read, uint8_t(uint16_t addr));
    MOCK_METHOD2(Write, void(uint16_t addr, uint8_t value));
};

static uint8_t read_mem(intptr_t ref, uint16_t addr)
{
    return reinterpret_cast<MockMemory*>(ref)->Read(addr);
}

static void write_mem(intptr_t ref, uint16_t addr, uint8_t data)
{
    reinterpret_cast<MockMemory*>(ref)->Write(addr, data);
}

M6809 OpCodeTestHelper(MockMemory &mem)
{
    EXPECT_CALL(mem, Read(_))
            .WillOnce(Return(0x00))
            .WillOnce(Return(0x00));

    M6809 cpu;
    cpu.SetReadCallback(&read_mem, reinterpret_cast<intptr_t>(&mem));
    cpu.SetWriteCallback(&write_mem, reinterpret_cast<intptr_t>(&mem));

    cpu.Reset();

    return cpu;
}

TEST(M6809OpCodes, ABXInherent)
{

    MockMemory mem;
    M6809 cpu = OpCodeTestHelper(mem);
    auto &registers = cpu.getRegisters();

    EXPECT_CALL(mem, Read(_))
            .WillOnce(Return(0x3a));

    uint64_t cycles;

    registers.B = 0x10;

    EXPECT_EQ(E_SUCCESS, cpu.Execute(cycles));

    EXPECT_EQ(0x10, registers.X);
}

/*
 * ADDA
 */

TEST(M6809OpCodes, ADDAImmediate)
{
    MockMemory mem;
    uint64_t cycles;
    M6809 cpu = OpCodeTestHelper(mem);
    auto &registers = cpu.getRegisters();

    EXPECT_CALL(mem, Read(_))
            .WillOnce(Return(0x8B))
            .WillOnce(Return(0x10));

    registers.A = 0x10;

    EXPECT_EQ(E_SUCCESS, cpu.Execute(cycles));
    EXPECT_EQ(0x20, registers.A);
}

TEST(M6809OpCodes, ADDADirect)
{
    MockMemory mem;
    uint64_t cycles;
    M6809 cpu = OpCodeTestHelper(mem);
    auto &registers = cpu.getRegisters();

    EXPECT_CALL(mem, Read(_))
            .WillOnce(Return(0x9B))  // Direct ADDA
            .WillOnce(Return(0x10)); // Page offset

    // direct page offset will be 0x10, and the direct page will be 0x00
    EXPECT_CALL(mem, Read(0x0010))
            .WillOnce(Return(0x15));

    registers.A = 0x10;

    EXPECT_EQ(E_SUCCESS, cpu.Execute(cycles));

    // the result (0x25) should be stored in A
    EXPECT_EQ(0x25, registers.A);

}

TEST(M6809OpCodes, ADDAExtended)
{
    MockMemory mem;
    uint64_t cycles;
    M6809 cpu = OpCodeTestHelper(mem);
    auto &registers = cpu.getRegisters();

    EXPECT_CALL(mem, Read(_))
            .WillOnce(Return(0xBB))  // Direct ADDA
            .WillOnce(Return(0x01))  // 2 byte address (0x1001)
            .WillOnce(Return(0x10));

    // expect a read at the extended offset 0x1001
    EXPECT_CALL(mem, Read(0x1001))
            .WillOnce(Return(0x15));

    registers.A = 0x10;

    EXPECT_EQ(E_SUCCESS, cpu.Execute(cycles));

    // the result (0x25) should be stored in A
    EXPECT_EQ(0x25, registers.A);

}

/*
 * ADDD
 */
TEST(M6809OpCodes, ADDDImmediate)
{
    MockMemory mem;
    uint64_t cycles;
    M6809 cpu = OpCodeTestHelper(mem);
    auto &registers = cpu.getRegisters();

    EXPECT_CALL(mem, Read(_))
            .WillOnce(Return(0xC3))
            .WillOnce(Return(0x20)) // 0x1020
            .WillOnce(Return(0x10));

    registers.D = 0x2010;

    EXPECT_EQ(E_SUCCESS, cpu.Execute(cycles));

    EXPECT_EQ(registers.PC, 3);

    EXPECT_EQ(0x3030, registers.D);
}

/*
 * SUBA
 */

TEST(M6809OpCodes, SUBAImmediate)
{
    MockMemory mem;
    uint64_t cycles;
    M6809 cpu = OpCodeTestHelper(mem);
    auto &registers = cpu.getRegisters();

    EXPECT_CALL(mem, Read(_))
            .WillOnce(Return(0x80))
            .WillOnce(Return(0x10));

    registers.A = 0x10;

    EXPECT_EQ(E_SUCCESS, cpu.Execute(cycles));
    EXPECT_EQ(0x00, registers.A);
}

TEST(M6809OpCodes, SUBAImmediate2)
{
    MockMemory mem;
    uint64_t cycles;
    M6809 cpu = OpCodeTestHelper(mem);
    auto &registers = cpu.getRegisters();

    EXPECT_CALL(mem, Read(_))
            .WillOnce(Return(0x80))
            .WillOnce(Return(0x12));

    registers.A = 0x10;

    EXPECT_EQ(E_SUCCESS, cpu.Execute(cycles));
    EXPECT_EQ(0xfe, registers.A);
}

/*
 * BIT - bitwise and, the register should not be updated
 */
TEST(M6809OpCodes, BITAImmediate)
{
    MockMemory mem;
    uint64_t cycles;
    M6809 cpu = OpCodeTestHelper(mem);
    auto &registers = cpu.getRegisters();

    EXPECT_CALL(mem, Read(_))
            .WillOnce(Return(0x85))
            .WillOnce(Return(0x12));

    registers.A = 0xFF;

    EXPECT_EQ(E_SUCCESS, cpu.Execute(cycles));
    EXPECT_EQ(0xFF, registers.A);
    // no flags should be set
    EXPECT_EQ(0, registers.CC & (FLAG_Z|FLAG_N|FLAG_V));
}

/*
 * Illegal Opcode
 */
TEST(M6809OpCodes, IllegalOp)
{
    MockMemory mem;
    uint64_t cycles;
    M6809 cpu = OpCodeTestHelper(mem);
    auto &registers = cpu.getRegisters();

    EXPECT_CALL(mem, Read(_))
            .WillOnce(Return(0x05));

    EXPECT_EQ(E_UNKNOWN_OPCODE, cpu.Execute(cycles));

}
