#include "AbstractCodeModel.h"

AbstractCodeModel::AbstractCodeModel(QObject* parent)
    : QObject(parent)
    , m_dwarf(nullptr)
{
}

Dwarf* AbstractCodeModel::dwarf() const
{
    return m_dwarf;
}

void AbstractCodeModel::setDwarf(Dwarf* dwarf)
{
    m_dwarf = dwarf;

    parseDwarf(dwarf);
}

void AbstractCodeModel::requestRewrite()
{
    emit rewriteRequested();
}
