#pragma once

#include "AbstractDisassembler.h"

class Disassemblers
{
public:
    static bool disassemble(Disassembly& disasm, const Elf* elf, Elf32_Addr startAddress, Elf32_Addr endAddress);
    static AbstractDisassembler* disassembler(Elf32_Half machine);
};
