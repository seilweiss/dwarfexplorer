#include "PPCDisassembler.h"

#include "capstone/capstone.h"

#include <qendian.h>

PPCDisassembler::PPCDisassembler(QObject* parent) :
    AbstractDisassembler(parent)
{
}

bool PPCDisassembler::disassemble(Disassembly& disasm, const Elf* elf, Elf32_Addr startAddress, Elf32_Addr endAddress)
{
    char* data = (char*)elf->getAddressData(startAddress);

    if (!data)
    {
        return false;
    }

    csh handle;
    cs_insn* insn;
    size_t count;

    if (cs_open(CS_ARCH_PPC, (cs_mode)(CS_MODE_32 + CS_MODE_BIG_ENDIAN), &handle) != CS_ERR_OK)
    {
        return false;
    }

    cs_option(handle, CS_OPT_SKIPDATA, CS_OPT_ON);

    bool success = true;

    count = cs_disasm(handle, (uint8_t*)data, endAddress - startAddress, startAddress, 0, &insn);

    if (count == 0)
    {
        success = false;
    }
    else
    {
        Elf32_Addr address = startAddress;

        for (size_t i = 0; i < count; i++)
        {
            disasm.addLine(address, insn[i].mnemonic, insn[i].op_str);
            address += 4;
        }

        cs_free(insn, count);
    }

    cs_close(&handle);

    return success;
}
