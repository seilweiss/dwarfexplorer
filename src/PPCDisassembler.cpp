#include "PPCDisassembler.h"

#include "Common/GekkoDisassembler.h"

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

    for (Elf32_Addr address = startAddress; address < endAddress; address += 4)
    {
        u32 opcode = *(u32*)(data + (address - startAddress));
        bool bigEndian = Q_BYTE_ORDER == Q_BIG_ENDIAN;
        QString text = QString::fromStdString(Common::GekkoDisassembler::Disassemble(opcode, address, bigEndian));
        QString leftText = text.section('\t', 0, 0);
        QString rightText = text.section('\t', 1);

        disasm.addLine(address, leftText, rightText);
    }

    return true;
}
