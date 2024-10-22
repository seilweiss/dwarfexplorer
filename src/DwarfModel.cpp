#include "DwarfModel.h"

#include "Util.h"
#include "DwarfAttributes.h"

static QString locationToString(Dwarf* dwarf, DwarfAttribute* attribute)
{
    DwarfLocation location;
    location.read(dwarf, attribute);

    QStringList text;

    for (DwarfLocationAtom& atom : location.atoms)
    {
        switch (atom.op)
        {
        case DW_OP_REG:
            text += QString("OP_REG(%1)").arg(Util::hexToString(atom.number));
            break;
        case DW_OP_BASEREG:
            text += QString("OP_BASEREG(%1)").arg(Util::hexToString(atom.number));
            break;
        case DW_OP_ADDR:
            text += QString("OP_ADDR(%1)").arg(Util::hexToString(atom.addr));
            break;
        case DW_OP_CONST:
            text += QString("OP_CONST(%1)").arg(Util::hexToString(atom.number));
            break;
        case DW_OP_DEREF2:
            text += QString("OP_DEREF2");
            break;
        case DW_OP_DEREF:
            text += QString("OP_DEREF");
            break;
        case DW_OP_ADD:
            text += QString("OP_ADD");
            break;
        default:
            text += QString("OP_<unknown %1>").arg(atom.op);
            break;
        }
    }

    return text.join(" ");
}

static DwarfModelItem* getSiblingItem(DwarfModelItem* item, int index)
{
    for (int i = 0; i < index; i++)
    {
        item = item->e.siblingItem;
    }

    return item;
}

static int getSiblingCount(DwarfEntry* entry)
{
    int count = 0;

    while (entry)
    {
        count++;
        entry = entry->sibling;
    }

    return count;
}

DwarfModel::DwarfModel(QObject* parent)
    : QAbstractItemModel(parent)
    , m_dwarf(nullptr)
    , m_items(nullptr)
{
}

DwarfModel::~DwarfModel()
{
    clearItems();
}

Dwarf* DwarfModel::dwarf() const
{
    return m_dwarf;
}

void DwarfModel::setDwarf(Dwarf* dwarf)
{
    m_dwarf = dwarf;

    beginResetModel();
    refresh();
    endResetModel();
}

void DwarfModel::clearItems()
{
    if (m_items)
    {
        delete[] m_items;
        m_items = nullptr;
    }
}

void DwarfModel::refresh()
{
    clearItems();

    if (!m_dwarf)
    {
        return;
    }

    if (m_dwarf->entryCount == 0)
    {
        return;
    }

    m_items = new DwarfModelItem[m_dwarf->entryCount + m_dwarf->attributeCount];

    DwarfModelItem* entryItems = m_items;
    DwarfModelItem* attributeItems = m_items + m_dwarf->entryCount;

    for (int i = 0; i < m_dwarf->entryCount; i++)
    {
        DwarfEntry* entry = &m_dwarf->entries[i];
        DwarfModelItem* entryItem = &entryItems[i];

        entryItem->type = DwarfModelItem::EntryItem;
        entryItem->e.entry = entry;
        entryItem->e.attributeItems = entry->attributes ? &attributeItems[entry->attributes - m_dwarf->attributes] : nullptr;
        entryItem->e.siblingItem = entry->sibling ? &entryItems[entry->sibling - m_dwarf->entries] : nullptr;
        entryItem->e.childItem = entry->firstChild ? &entryItems[entry->firstChild - m_dwarf->entries] : nullptr;
    }

    for (int i = 0; i < m_dwarf->attributeCount; i++)
    {
        DwarfAttribute* attribute = &m_dwarf->attributes[i];
        DwarfModelItem* attributeItem = &attributeItems[i];

        attributeItem->type = DwarfModelItem::AttributeItem;
        attributeItem->a.attribute = attribute;
    }

    for (int i = 0; i < m_dwarf->entryCount; i++)
    {
        DwarfEntry* entry = &m_dwarf->entries[i];
        DwarfModelItem* entryItem = &entryItems[i];

        for (int j = 0; j < entry->attributeCount; j++)
        {
            DwarfModelItem* attributeItem = &entryItem->e.attributeItems[j];

            attributeItem->parentItem = entryItem;
            attributeItem->indexInParent = j;
        }

        int childIndex = 0;

        for (DwarfModelItem* childItem = entryItem->e.childItem; childItem != nullptr; childItem = childItem->e.siblingItem)
        {
            childItem->parentItem = entryItem;
            childItem->indexInParent = childIndex;

            childIndex++;
        }
    }

    DwarfModelItem* topLevelItem = m_items;
    int topLevelIndex = 0;

    while (topLevelItem)
    {
        topLevelItem->parentItem = nullptr;
        topLevelItem->indexInParent = topLevelIndex;

        topLevelItem = topLevelItem->e.siblingItem;
        topLevelIndex++;
    }
}

QModelIndex DwarfModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!m_dwarf)
    {
        return QModelIndex();
    }

    if (!hasIndex(row, column, parent))
    {
        return QModelIndex();
    }

    if (!parent.isValid())
    {
        return createIndex(row, column, getSiblingItem(m_items, row));
    }

    DwarfModelItem* parentItem = (DwarfModelItem*)parent.internalPointer();

    if (parentItem->type == DwarfModelItem::EntryItem)
    {
        DwarfEntry* entry = parentItem->e.entry;

        if (row < entry->attributeCount)
        {
            return createIndex(row, column, &parentItem->e.attributeItems[row]);
        }
        else
        {
            return createIndex(row, column, getSiblingItem(parentItem->e.childItem, row - entry->attributeCount));
        }
    }

    return QModelIndex();
}

QModelIndex DwarfModel::parent(const QModelIndex& child) const
{
    if (!m_dwarf)
    {
        return QModelIndex();
    }

    if (!child.isValid())
    {
        return QModelIndex();
    }

    DwarfModelItem* childItem = (DwarfModelItem*)child.internalPointer();

    if (!childItem->parentItem)
    {
        return QModelIndex();
    }

    return createIndex(childItem->parentItem->indexInParent, 0, childItem->parentItem);
}

int DwarfModel::rowCount(const QModelIndex& parent) const
{
    if (!m_dwarf)
    {
        return 0;
    }

    if (parent.column() > 0)
    {
        return 0;
    }

    if (!parent.isValid())
    {
        return getSiblingCount(m_items[0].e.entry);
    }

    DwarfModelItem* parentItem = (DwarfModelItem*)parent.internalPointer();

    if (parentItem->type == DwarfModelItem::EntryItem)
    {
        return parentItem->e.entry->attributeCount + getSiblingCount(parentItem->e.entry->firstChild);
    }

    return 0;
}

int DwarfModel::columnCount(const QModelIndex& parent) const
{
    return ColumnCount;
}

QVariant DwarfModel::data(const QModelIndex& index, int role) const
{
    if (!m_dwarf)
    {
        return 0;
    }

    if (!index.isValid())
    {
        return QVariant();
    }

    if (role != Qt::DisplayRole)
    {
        return QVariant();
    }

    DwarfModelItem* item = (DwarfModelItem*)index.internalPointer();

    if (item->type == DwarfModelItem::EntryItem)
    {
        DwarfEntry* entry = item->e.entry;

        switch (index.column())
        {
        case OffsetColumn:
            return Util::hexToString(entry->offset);
        case TagColumn:
            return entry->isNull() ? tr("<null entry>") : Dwarf::tagToString(entry->tag);
        case NameDataColumn:
            return entry->getName();
        }
    }
    else if (item->type == DwarfModelItem::AttributeItem)
    {
        DwarfAttribute* attribute = item->a.attribute;

        switch (index.column())
        {
        case OffsetColumn:
            return Util::hexToString(attribute->offset);
        case TagColumn:
            return Dwarf::attrNameToString(attribute->name);
        case NameDataColumn:
            switch (attribute->name)
            {
            case DW_AT_location:
                return locationToString(m_dwarf, attribute);
            }

            switch (attribute->getForm())
            {
            case DW_FORM_ADDR:
                return QString("0x%1").arg(attribute->addr, 8, 16, QLatin1Char('0'));
            case DW_FORM_REF:
                return QString("0x%1").arg(attribute->ref, 0, 16);
            case DW_FORM_DATA2:
                return QString("%1").arg(attribute->data2);
            case DW_FORM_DATA4:
                return QString("%1").arg(attribute->data4);
            case DW_FORM_DATA8:
                return QString("%1").arg(attribute->data8);
            case DW_FORM_STRING:
                return attribute->string;
            default:
                return QString();
            }
        }
    }

    return QString();
}

Qt::ItemFlags DwarfModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return Qt::NoItemFlags;
    }

    return QAbstractItemModel::flags(index);
}

QVariant DwarfModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
    {
        return QVariant();
    }

    if (orientation != Qt::Horizontal)
    {
        return QVariant();
    }

    switch (section)
    {
    case OffsetColumn:
        return tr("Offset");
    case TagColumn:
        return tr("Tag");
    case NameDataColumn:
        return tr("Name/Data");
    }

    return QVariant();
}

DwarfEntry* DwarfModel::entry(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return nullptr;
    }

    const DwarfModelItem* item = (DwarfModelItem*)index.internalPointer();

    if (item->type == DwarfModelItem::EntryItem)
    {
        return item->e.entry;
    }

    return nullptr;
}

DwarfAttribute* DwarfModel::attribute(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return nullptr;
    }

    const DwarfModelItem* item = (DwarfModelItem*)index.internalPointer();

    if (item->type == DwarfModelItem::AttributeItem)
    {
        return item->a.attribute;
    }

    return nullptr;
}

bool DwarfModel::isEntry(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return false;
    }

    const DwarfModelItem* item = (DwarfModelItem*)index.internalPointer();

    return item->type == DwarfModelItem::EntryItem;
}

bool DwarfModel::isAttribute(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return false;
    }

    const DwarfModelItem* item = (DwarfModelItem*)index.internalPointer();

    return item->type == DwarfModelItem::AttributeItem;
}
