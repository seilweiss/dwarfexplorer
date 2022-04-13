#include "DwarfAttributes.h"

#include <qdebug.h>

static const char* readString(char*& data)
{
    const char* str = data;

    while (*data != '\0')
    {
        data++;
    }

    data++;

    return str;
}

void DwarfLocation::read(Dwarf* dwarf, DwarfAttribute* attribute)
{
    Q_ASSERT(attribute->name == DW_AT_location);

    char* data = attribute->block;
    char* end = data + attribute->blockLength;

    while (data < end)
    {
        DwarfLocationAtom atom;
        atom.op = dwarf->elf->read<char>(data);

        switch (atom.op)
        {
        case DW_OP_REG:
        case DW_OP_BASEREG:
        case DW_OP_CONST:
            atom.number = dwarf->elf->read<Elf32_Word>(data);
            break;
        case DW_OP_ADDR:
            atom.addr = dwarf->elf->read<Elf32_Addr>(data);
            break;
        }

        atoms.push_back(atom);
    }
}

void DwarfType::read(Dwarf* dwarf, DwarfAttribute* attribute)
{
    Q_ASSERT(attribute->name == DW_AT_fund_type
        || attribute->name == DW_AT_user_def_type
        || attribute->name == DW_AT_mod_fund_type
        || attribute->name == DW_AT_mod_u_d_type);

    if (attribute->name == DW_AT_fund_type)
    {
        isFundamental = true;
        fundType = attribute->data2;
    }
    else if (attribute->name == DW_AT_user_def_type)
    {
        isFundamental = false;
        udTypeOffset = attribute->ref;
    }
    else if (attribute->name == DW_AT_mod_fund_type)
    {
        isFundamental = true;

        Q_ASSERT(attribute->blockLength >= sizeof(Elf32_Half));

        char* data = attribute->block;
        char* typeData = data + attribute->blockLength - sizeof(Elf32_Half);

        while (data < typeData)
        {
            char modifier = dwarf->elf->read<char>(data);
            modifiers.push_back(modifier);
        }

        fundType = dwarf->elf->read<Elf32_Half>(data);
    }
    else if (attribute->name == DW_AT_mod_u_d_type)
    {
        isFundamental = false;

        Q_ASSERT(attribute->blockLength >= sizeof(Elf32_Word));

        char* data = attribute->block;
        char* typeData = data + attribute->blockLength - sizeof(Elf32_Word);

        while (data < typeData)
        {
            char modifier = dwarf->elf->read<char>(data);
            modifiers.push_back(modifier);
        }

        udTypeOffset = dwarf->elf->read<Elf32_Off>(data);
    }
}

void DwarfSubscriptData::read(Dwarf* dwarf, DwarfAttribute* attribute)
{
    Q_ASSERT(attribute->name == DW_AT_subscr_data);

    char* data = attribute->block;
    char* end = data + attribute->blockLength;

    while (data < end)
    {
        char format = dwarf->elf->read<char>(data);

        if (format == DW_FMT_ET)
        {
            // Read type attribute
            DwarfAttribute attribute;
            dwarf->readAttribute(data, &attribute);
            elementType.read(dwarf, &attribute);
        }
        else
        {
            DwarfSubscriptDataItem item;

            // Read format specifier
            if (format & 0x4) // User-defined type
            {
                item.indexType.isFundamental = false;
                item.indexType.udTypeOffset = dwarf->elf->read<Elf32_Off>(data);
            }
            else // Fundamental type
            {
                item.indexType.isFundamental = true;
                item.indexType.fundType = dwarf->elf->read<Elf32_Half>(data);
            }

            // Read low bound
            if (format & 0x2) // Location
            {
                item.lowBound.isConstant = false;

                DwarfAttribute attribute;
                attribute.name = DW_AT_location;
                attribute.blockLength = dwarf->elf->read<Elf32_Half>(data);
                attribute.block = data;

                item.lowBound.location.read(dwarf, &attribute);
            }
            else // Constant
            {
                item.lowBound.isConstant = true;
                item.lowBound.constant = dwarf->elf->read<Elf32_Word>(data);
            }

            // Read high bound
            if (format & 0x1) // Location
            {
                item.highBound.isConstant = false;

                DwarfAttribute attribute;
                attribute.name = DW_AT_location;
                attribute.blockLength = dwarf->elf->read<Elf32_Half>(data);
                attribute.block = data;

                item.highBound.location.read(dwarf, &attribute);
            }
            else // Constant
            {
                item.highBound.isConstant = true;
                item.highBound.constant = dwarf->elf->read<Elf32_Word>(data);
            }

            items.push_back(item);
        }
    }
}

void DwarfElementList::read(Dwarf* dwarf, DwarfAttribute* attribute)
{
    Q_ASSERT(attribute->name == DW_AT_element_list);

    char* data = attribute->block;
    char* end = data + attribute->blockLength;

    while (data < end)
    {
        DwarfElementListItem item;
        item.value = dwarf->elf->read<Elf32_Word>(data);
        item.name = readString(data);

        items.push_back(item);
    }
}
