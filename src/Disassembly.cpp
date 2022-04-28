#include "Disassembly.h"

Disassembly::Disassembly() :
    m_lines()
{
}

void Disassembly::clear()
{
    m_lines.clear();
}

void Disassembly::addLine(Elf32_Addr address, const QString& leftText, const QString& rightText)
{
    DisassemblyLine line;
    line.address = address;
    line.leftText = leftText;
    line.rightText = rightText;
    m_lines.append(line);
}

int Disassembly::lineCount() const
{
    return m_lines.size();
}

Elf32_Addr Disassembly::address(int line) const
{
    Q_ASSERT(line >= 0 && line < m_lines.size());
    return m_lines[line].address;
}

QString Disassembly::leftText(int line) const
{
    Q_ASSERT(line >= 0 && line < m_lines.size());
    return m_lines[line].leftText;
}

QString Disassembly::rightText(int line) const
{
    Q_ASSERT(line >= 0 && line < m_lines.size());
    return m_lines[line].rightText;
}
