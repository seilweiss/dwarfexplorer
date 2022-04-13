#include "TypesModel.h"

#include "Util.h"

#include <qdir.h>

TypesModel::TypesModel(QObject* parent)
	: QAbstractItemModel(parent)
	, m_dwarf(nullptr)
	, m_typeItemMap()
{
}

Dwarf* TypesModel::dwarf() const
{
	return m_dwarf;
}

void TypesModel::setDwarf(Dwarf* dwarf)
{
	m_dwarf = dwarf;

	beginResetModel();
	refresh();
	endResetModel();
}

void TypesModel::clearItems()
{
	m_typeItemMap.clear();
}

static void recurseAssignParentItems(TypesModelItem* item)
{
	int childIndex = 0;

	for (TypesModelItem& child : item->subitems)
	{
		child.parentItem = item;
		child.indexInParent = childIndex;

		recurseAssignParentItems(&child);

		childIndex++;
	}
}

void TypesModel::refresh()
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

	for (DwarfEntry* entry = &m_dwarf->entries[0]; entry != nullptr; entry = entry->sibling)
	{
		if (entry->tag == DW_TAG_compile_unit)
		{
			QString compileUnitPath = entry->getName();

			for (DwarfEntry* child = entry->firstChild; child != nullptr; child = child->sibling)
			{
				bool isTypeEntry = true;
				QString typeKeyword;
				
				switch (child->tag)
				{
				case DW_TAG_class_type:
					typeKeyword = "class";
					break;
				case DW_TAG_structure_type:
					typeKeyword = "struct";
					break;
				case DW_TAG_enumeration_type:
					typeKeyword = "enum";
					break;
				case DW_TAG_array_type:
					typeKeyword = "array";
					break;
				case DW_TAG_subroutine_type:
					typeKeyword = "function";
					break;
				default:
					isTypeEntry = false;
					break;
				}

				if (isTypeEntry)
				{
					QString typeName = child->getName();
					int typeSize = 0;

					if (DwarfAttribute* attr = child->findAttribute(DW_AT_byte_size))
					{
						typeSize = attr->data4;
					}

					bool isNewItem = !m_typeItemMap.contains(typeName);
					TypesModelItem& typeItem = m_typeItemMap[typeName];

					if (isNewItem)
					{
						typeItem.type = TypesModelItem::TypeItem;
						typeItem.typeNameOrCompileUnit = typeName;
					}

					TypesModelItem defItem;
					defItem.type = TypesModelItem::DefinitionItem;
					defItem.typeKeyword = typeKeyword;
					defItem.typeNameOrCompileUnit = compileUnitPath;
					defItem.typeSize = typeSize;
					defItem.typeDwarfOffset = child->offset;

					typeItem.subitems.append(defItem);
				}
			}
		}
	}

	for (TypesModelItem& item : m_typeItemMap)
	{
		recurseAssignParentItems(&item);
	}
}

QModelIndex TypesModel::index(int row, int column, const QModelIndex& parent) const
{
	if (!hasIndex(row, column, parent))
	{
		return QModelIndex();
	}

	if (!parent.isValid())
	{
		return createIndex(row, column, &*(m_typeItemMap.begin() + row));
	}

	const TypesModelItem* parentItem = (TypesModelItem*)parent.internalPointer();
	const TypesModelItem* childItem = &parentItem->subitems[row];

	return createIndex(row, column, childItem);
}

QModelIndex TypesModel::parent(const QModelIndex& child) const
{
	if (!child.isValid())
	{
		return QModelIndex();
	}

	const TypesModelItem* childItem = (TypesModelItem*)child.internalPointer();

	if (!childItem->parentItem)
	{
		return QModelIndex();
	}

	return createIndex(childItem->parentItem->indexInParent, 0, childItem->parentItem);
}

int TypesModel::rowCount(const QModelIndex& parent) const
{
	if (!parent.isValid())
	{
		return m_typeItemMap.size();
	}

	const TypesModelItem* parentItem = (TypesModelItem*)parent.internalPointer();

	return parentItem->subitems.size();
}

int TypesModel::columnCount(const QModelIndex& parent) const
{
	return 4;
}

QVariant TypesModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
	{
		return QVariant();
	}

	if (role != Qt::DisplayRole)
	{
		return QVariant();
	}

	const TypesModelItem* item = (TypesModelItem*)index.internalPointer();

	if (item->type == TypesModelItem::TypeItem)
	{
		switch (index.column())
		{
		case 0:
			return item->typeNameOrCompileUnit;
		}
	}
	else if (item->type == TypesModelItem::DefinitionItem)
	{
		switch (index.column())
		{
		case 0:
			return item->typeKeyword;
		case 1:
			return item->typeNameOrCompileUnit;
		case 2:
			return Util::hexToString(item->typeSize);
		case 3:
			return Util::hexToString(item->typeDwarfOffset);
		}
	}

	return QVariant();
}

Qt::ItemFlags TypesModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
	{
		return Qt::NoItemFlags;
	}

	return QAbstractItemModel::flags(index);
}

QVariant TypesModel::headerData(int section, Qt::Orientation orientation, int role) const
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
	case 0:
		return tr("Type");
	case 1:
		return tr("Compile Unit");
	case 2:
		return tr("Size");
	case 3:
		return tr("DWARF Offset");
	}

	return QVariant();
}

QString TypesModel::typeName(const QModelIndex& index) const
{
	if (!index.isValid())
	{
		return QString();
	}

	QStringList parts;
	const TypesModelItem* item = (TypesModelItem*)index.internalPointer();

	if (item->type == TypesModelItem::TypeItem)
	{
		return item->typeNameOrCompileUnit;
	}
	else if (item->type == TypesModelItem::DefinitionItem)
	{
		return item->parentItem->typeNameOrCompileUnit;
	}
}

QString TypesModel::compileUnit(const QModelIndex& index) const
{
	if (!index.isValid())
	{
		return QString();
	}

	QStringList parts;
	const TypesModelItem* item = (TypesModelItem*)index.internalPointer();

	if (item->type == TypesModelItem::TypeItem)
	{
		return QString();
	}
	else if (item->type == TypesModelItem::DefinitionItem)
	{
		return item->typeNameOrCompileUnit;
	}
}

Elf32_Off TypesModel::dwarfOffset(const QModelIndex& index) const
{
	if (!index.isValid())
	{
		return 0;
	}

	const TypesModelItem* item = (TypesModelItem*)index.internalPointer();

	if (item->type == TypesModelItem::TypeItem)
	{
		return 0;
	}
	else if (item->type == TypesModelItem::DefinitionItem)
	{
		return item->typeDwarfOffset;
	}
}

bool TypesModel::isType(const QModelIndex& index) const
{
	if (!index.isValid())
	{
		return false;
	}

	const TypesModelItem* item = (TypesModelItem*)index.internalPointer();

	return item->type == TypesModelItem::TypeItem;
}

bool TypesModel::isDefinition(const QModelIndex& index) const
{
	if (!index.isValid())
	{
		return false;
	}

	const TypesModelItem* item = (TypesModelItem*)index.internalPointer();

	return item->type == TypesModelItem::DefinitionItem;
}
