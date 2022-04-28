#pragma once

#include "AbstractDisassembler.h"

class PPCDisassembler : public AbstractDisassembler
{
    Q_OBJECT

public:
    PPCDisassembler(QObject* parent = nullptr);

    bool disassemble(Disassembly& disasm, const Elf* elf, Elf32_Addr startAddress, Elf32_Addr endAddress) override;
};
