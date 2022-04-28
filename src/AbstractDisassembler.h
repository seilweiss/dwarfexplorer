#pragma once

#include <qobject.h>

#include "Disassembly.h"

class AbstractDisassembler : public QObject
{
    Q_OBJECT

public:
    AbstractDisassembler(QObject* parent = nullptr);

    virtual bool disassemble(Disassembly& disasm, const Elf* elf, Elf32_Addr startAddress, Elf32_Addr endAddress) = 0;
};
