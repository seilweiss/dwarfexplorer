#include "FilesModel.h"

#include <qdir.h>

FilesModel::FilesModel(QObject* parent)
    : QAbstractItemModel(parent)
    , m_dwarf(nullptr)
    , m_rootItem()
{
    m_rootItem.type = FilesModelItem::DirectoryItem;
    m_rootItem.parentItem = nullptr;
    m_rootItem.indexInParent = 0;
}

Dwarf* FilesModel::dwarf() const
{
    return m_dwarf;
}

void FilesModel::setDwarf(Dwarf* dwarf)
{
    m_dwarf = dwarf;

    beginResetModel();
    refresh();
    endResetModel();
}

void FilesModel::clearItems()
{
    m_rootItem.subitems.clear();
}

static void recurseAssignParentItems(FilesModelItem* item)
{
    int childIndex = 0;

    for (FilesModelItem& child : item->subitems)
    {
        child.parentItem = item;
        child.indexInParent = childIndex;

        recurseAssignParentItems(&child);

        childIndex++;
    }
}

void FilesModel::refresh()
{
    clearItems();

    if (!m_dwarf)
    {
        return;
    }

    QStringList paths;

    for (DwarfEntry* entry = &m_dwarf->entries[0]; entry != nullptr; entry = entry->sibling)
    {
        if (entry->tag == DW_TAG_compile_unit)
        {
            QString path = entry->getName();

            if (!paths.contains(path))
            {
                paths << path;
            }
        }
    }

    for (const QString& path : paths)
    {
        QStringList parts = QDir::toNativeSeparators(path).split(QDir::separator(), Qt::SkipEmptyParts);
        FilesModelItem* item = &m_rootItem;

        for (int i = 0; i < parts.size(); i++)
        {
            QString part = parts[i];
            bool isNewItem = !item->subitems.contains(part);
            FilesModelItem* partItem = &item->subitems[part];

            if (isNewItem)
            {
                if (i == parts.size() - 1)
                {
                    partItem->type = FilesModelItem::FileItem;
                    partItem->path = path;
                }
                else
                {
                    partItem->type = FilesModelItem::DirectoryItem;
                    partItem->path = parts.first(i + 1).join(QDir::separator());
                }

                partItem->text = part;
            }

            item = partItem;
        }
    }

    recurseAssignParentItems(&m_rootItem);
}

QModelIndex FilesModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
    {
        return QModelIndex();
    }

    const FilesModelItem* parentItem = parent.isValid() ? (FilesModelItem*)parent.internalPointer() : &m_rootItem;
    const FilesModelItem* childItem = &*(parentItem->subitems.begin() + row);

    return createIndex(row, column, childItem);
}

QModelIndex FilesModel::parent(const QModelIndex& child) const
{
    if (!child.isValid())
    {
        return QModelIndex();
    }

    const FilesModelItem* childItem = (FilesModelItem*)child.internalPointer();

    if (!childItem->parentItem)
    {
        return QModelIndex();
    }

    return createIndex(childItem->parentItem->indexInParent, 0, childItem->parentItem);
}

int FilesModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0)
    {
        return 0;
    }

    const FilesModelItem* parentItem = parent.isValid() ? (FilesModelItem*)parent.internalPointer() : &m_rootItem;

    return parentItem->subitems.size();
}

int FilesModel::columnCount(const QModelIndex& parent) const
{
    return ColumnCount;
}

QVariant FilesModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    if (role != Qt::DisplayRole)
    {
        return QVariant();
    }

    const FilesModelItem* item = (FilesModelItem*)index.internalPointer();

    return item->text;
}

Qt::ItemFlags FilesModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return Qt::NoItemFlags;
    }

    return QAbstractItemModel::flags(index);
}

QVariant FilesModel::headerData(int section, Qt::Orientation orientation, int role) const
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
    case NameColumn:
        return tr("Name");
    }

    return QVariant();
}

QString FilesModel::path(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return QString();
    }

    QStringList parts;
    const FilesModelItem* item = (FilesModelItem*)index.internalPointer();

    return item->path;
}

bool FilesModel::isDirectory(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return false;
    }

    const FilesModelItem* item = (FilesModelItem*)index.internalPointer();

    return item->type == FilesModelItem::DirectoryItem;
}

bool FilesModel::isFile(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return false;
    }

    const FilesModelItem* item = (FilesModelItem*)index.internalPointer();

    return item->type == FilesModelItem::FileItem;
}
