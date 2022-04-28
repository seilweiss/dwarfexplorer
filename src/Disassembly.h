#pragma once

#include "Elf.h"

#include <qstring.h>
#include <qlist.h>

struct DisassemblyLine
{
    Elf32_Addr address;
    QString leftText;
    QString rightText;
};

class Disassembly
{
public:
    Disassembly();

    void clear();
    void addLine(Elf32_Addr address, const QString& leftText, const QString& rightText);

    int lineCount() const;
    Elf32_Addr address(int line) const;
    QString leftText(int line) const;
    QString rightText(int line) const;

private:
    QList<DisassemblyLine> m_lines;
};
