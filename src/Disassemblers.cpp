#include "Disassemblers.h"

#include "PPCDisassembler.h"

static PPCDisassembler* ppc = new PPCDisassembler;

bool Disassemblers::disassemble(Disassembly& disasm, const Elf* elf, Elf32_Addr startAddress, Elf32_Addr endAddress)
{
    AbstractDisassembler* d = disassembler(elf->header->e_machine);

    if (!d)
    {
        return false;
    }

    return d->disassemble(disasm, elf, startAddress, endAddress);
}

AbstractDisassembler* Disassemblers::disassembler(Elf32_Half machine)
{
    switch (machine)
    {
    case EM_PPC: return ppc;
    }

    return nullptr;
}
