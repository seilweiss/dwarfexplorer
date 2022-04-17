#include "Dwarf.h"

#include <memory>
#include <qdebug.h>

static void countAttribute(const Elf* elf, char*& data, int& attributeCount)
{
    Elf32_Half name = elf->read<Elf32_Half>(data);

    switch (name & 0xf)
    {
    case DW_FORM_ADDR:
    {
        data += sizeof(Elf32_Addr);
        break;
    }
    case DW_FORM_REF:
    {
        data += sizeof(Elf32_Off);
        break;
    }
    case DW_FORM_BLOCK2:
    {
        Elf32_Half length = elf->read<Elf32_Half>(data);
        data += length;
        break;
    }
    case DW_FORM_BLOCK4:
    {
        Elf32_Word length = elf->read<Elf32_Word>(data);
        data += length;
        break;
    }
    case DW_FORM_DATA2:
    {
        data += sizeof(Elf32_Half);
        break;
    }
    case DW_FORM_DATA4:
    {
        data += sizeof(Elf32_Word);
        break;
    }
    case DW_FORM_DATA8:
    {
        data += sizeof(Elf32_Xword);
        break;
    }
    case DW_FORM_STRING:
    {
        while (*data != '\0')
        {
            data++;
        }

        data++;

        break;
    }
    }

    attributeCount++;
}

static void countEntry(const Elf* elf, char*& data, int& entryCount, int& attributeCount)
{
    char* start = data;
    Elf32_Word length = elf->read<Elf32_Word>(data);
    Elf32_Half tag = elf->read<Elf32_Half>(data);

    char* end = start + length;

    if (tag == DW_TAG_padding || length < 8) // padding or null entry
    {
        data = end;
    }
    else
    {
        while (data < end)
        {
            countAttribute(elf, data, attributeCount);
        }
    }

    entryCount++;
}

static void readAttribute(const Elf* elf, char*& data, DwarfAttribute*& attribute)
{
    char* start = data;

    attribute->name = elf->read<Elf32_Half>(data);

    switch (attribute->name & 0xf)
    {
    case DW_FORM_ADDR:
    {
        attribute->addr = elf->read<Elf32_Addr>(data);
        break;
    }
    case DW_FORM_REF:
    {
        attribute->ref = elf->read<Elf32_Off>(data);
        break;
    }
    case DW_FORM_BLOCK2:
    {
        attribute->blockLength = elf->read<Elf32_Half>(data);
        attribute->block = data;
        data += attribute->blockLength;
        break;
    }
    case DW_FORM_BLOCK4:
    {
        attribute->blockLength = elf->read<Elf32_Word>(data);
        attribute->block = data;
        data += attribute->blockLength;
        break;
    }
    case DW_FORM_DATA2:
    {
        attribute->data2 = elf->read<Elf32_Half>(data);
        break;
    }
    case DW_FORM_DATA4:
    {
        attribute->data4 = elf->read<Elf32_Word>(data);
        break;
    }
    case DW_FORM_DATA8:
    {
        attribute->data8 = elf->read<Elf32_Xword>(data);
        break;
    }
    case DW_FORM_STRING:
    {
        attribute->string = data;

        while (*data != '\0')
        {
            data++;
        }

        data++;

        break;
    }
    }

    attribute->length = (Elf32_Off)(data - start);

    attribute++;
}

static void readEntry(const Elf* elf, char*& data, DwarfEntry*& entry, DwarfAttribute*& attribute)
{
    char* start = data;

    entry->length = elf->read<Elf32_Off>(data);
    entry->tag = elf->read<Elf32_Half>(data);

    char* end = start + entry->length;

    if (entry->tag == DW_TAG_padding || entry->length < 8) // padding or null entry
    {
        entry->attributes = nullptr;
        entry->attributeCount = 0;
        data = end;
    }
    else
    {
        entry->attributes = attribute;
        entry->attributeCount = 0;

        while (data < end)
        {
            attribute->offset = entry->offset + (Elf32_Off)(data - start);
            readAttribute(elf, data, attribute);
            entry->attributeCount++;
        }
    }

    entry++;
}

static void countSourceStatementEntry(const Elf* elf, char*& data, int& entryCount)
{
    // skip line number + line character + address delta
    data += sizeof(Elf32_Word) + sizeof(Elf32_Half) + sizeof(Elf32_Word);
    entryCount++;
}

static void countSourceStatementTable(const Elf* elf, char*& data, int& tableCount, int& entryCount)
{
    char* start = data;
    Elf32_Word length = elf->read<Elf32_Word>(data);
    char* end = start + length;
    
    // skip start address
    data += sizeof(Elf32_Addr);

    while (data < end)
    {
        countSourceStatementEntry(elf, data, entryCount);
    }

    tableCount++;
}

static void readSourceStatementEntry(const Elf* elf, char*& data, DwarfSourceStatementEntry*& entry)
{
    entry->lineNumber = elf->read<Elf32_Word>(data);
    entry->lineCharacter = elf->read<Elf32_Half>(data);
    entry->address = elf->read<Elf32_Addr>(data);
    entry++;
}

static void readSourceStatementTable(const Elf* elf, char*& data, DwarfSourceStatementTable*& table, DwarfSourceStatementEntry*& entry)
{
    char* start = data;
    Elf32_Word length = elf->read<Elf32_Word>(data);
    char* end = start + length;

    table->startAddress = elf->read<Elf32_Addr>(data);
    table->entries = nullptr;
    table->entryCount = 0;

    if (data < end)
    {
        table->entries = entry;

        while (data < end)
        {
            readSourceStatementEntry(elf, data, entry);
            table->entryCount++;
        }

        for (int i = 0; i < table->entryCount; i++)
        {
            table->entries[i].address += table->startAddress;
        }
    }

    table++;
}

Dwarf::ReadResult Dwarf::read(const Elf* elf)
{
    destroy();

    this->elf = elf;

    Elf32_Half debugSectionIndex = elf->getSectionIndex(".debug");

    if (debugSectionIndex == SHN_UNDEF)
    {
        return ReadSectionNotFound;
    }

    char* debugData = (char*)elf->getSectionData(debugSectionIndex);

    if (!debugData)
    {
        return ReadSectionNotFound;
    }

    char* debugDataStart = debugData;
    char* debugDataEnd = debugData + elf->sectionHeaderTable[debugSectionIndex].sh_size;

    while (debugData < debugDataEnd)
    {
        countEntry(elf, debugData, entryCount, attributeCount);
    }

    Elf32_Half lineNumberTableSectionIndex = elf->getSectionIndex(".line");
    char* lineNumberTableData = nullptr;

    if (lineNumberTableSectionIndex != SHN_UNDEF)
    {
        lineNumberTableData = (char*)elf->getSectionData(lineNumberTableSectionIndex);
    }

    char* lineNumberTableDataStart = nullptr;
    char* lineNumberTableDataEnd = nullptr; 

    if (lineNumberTableData)
    {
        lineNumberTableDataStart = lineNumberTableData;
        lineNumberTableDataEnd = lineNumberTableData + elf->sectionHeaderTable[lineNumberTableSectionIndex].sh_size;

        while (lineNumberTableData < lineNumberTableDataEnd)
        {
            countSourceStatementTable(elf, lineNumberTableData, sourceStatementTableCount, sourceStatementEntryCount);
        }
    }

    if (entryCount == 0
        && attributeCount == 0
        && sourceStatementTableCount == 0
        && sourceStatementEntryCount == 0)
    {
        return ReadSuccess;
    }

    internalData = malloc(
        entryCount * sizeof(DwarfEntry)
        + attributeCount * sizeof(DwarfAttribute)
        + sourceStatementTableCount * sizeof(DwarfSourceStatementTable)
        + sourceStatementEntryCount * sizeof(DwarfSourceStatementEntry));

    entries = (DwarfEntry*)internalData;
    attributes = (DwarfAttribute*)(entries + entryCount);
    sourceStatementTables = (DwarfSourceStatementTable*)(attributes + attributeCount);
    sourceStatementEntries = (DwarfSourceStatementEntry*)(sourceStatementTables + sourceStatementTableCount);

    if (entryCount == 0)
    {
        entries = nullptr;
    }

    if (attributeCount == 0)
    {
        attributes = nullptr;
    }

    if (sourceStatementTableCount == 0)
    {
        sourceStatementTables = nullptr;
    }

    if (sourceStatementEntryCount == 0)
    {
        sourceStatementEntries = nullptr;
    }

    if (entries)
    {
        debugData = debugDataStart;

        DwarfEntry* entry = entries;
        DwarfAttribute* attribute = attributes;

        while (debugData < debugDataEnd)
        {
            entry->offset = (Elf32_Off)(debugData - debugDataStart);

            readEntry(elf, debugData, entry, attribute);
        }

        for (int i = 0; i < entryCount - 1; i++)
        {
            entry = &entries[i];
            entry->sibling = nullptr;
            entry->firstChild = nullptr;

            if (!entry->isNull() && entry->tag != DW_TAG_padding)
            {
                DwarfAttribute* siblingAttribute = entry->findAttribute(DW_AT_sibling);

                if (siblingAttribute)
                {
                    DwarfEntry* nextEntry = entry + 1;

                    if (siblingAttribute->ref == nextEntry->offset)
                    {
                        entry->sibling = nextEntry;
                    }
                    else
                    {
                        entry->firstChild = nextEntry;

                        for (int j = i + 2; j < entryCount; j++)
                        {
                            DwarfEntry* sibling = &entries[j];

                            if (sibling->offset == siblingAttribute->ref)
                            {
                                entry->sibling = sibling;
                                break;
                            }
                        }
                    }
                }
            }
        }

        entries[entryCount - 1].sibling = nullptr;
        entries[entryCount - 1].siblingCount = 0;
        entries[entryCount - 1].firstChild = nullptr;
        entries[entryCount - 1].childCount = 0;

        for (int i = 0; i < entryCount - 1; i++)
        {
            entry = &entries[i];

            DwarfEntry* child = entry->firstChild;

            while (child)
            {
                if (!child->sibling && child->tag == DW_TAG_padding)
                {
                    DwarfEntry* nextEntry = child + 1;

                    if (nextEntry < entries + entryCount && nextEntry != entry->sibling)
                    {
                        child->sibling = nextEntry;
                    }
                }

                child = child->sibling;
            }
        }

        /* TODO: optimize or get rid of this, it's slow */
        for (int i = 0; i < entryCount - 1; i++)
        {
            entry = &entries[i];
            entry->siblingCount = 0;
            entry->childCount = 0;

            DwarfEntry* sibling = entry->sibling;

            while (sibling)
            {
                entry->siblingCount++;
                sibling = sibling->sibling;
            }

            DwarfEntry* child = entry->firstChild;

            while (child)
            {
                entry->childCount++;
                child = child->sibling;
            }
        }
    }

    if (sourceStatementTables)
    {
        lineNumberTableData = lineNumberTableDataStart;

        DwarfSourceStatementTable* table = sourceStatementTables;
        DwarfSourceStatementEntry* entry = sourceStatementEntries;

        while (lineNumberTableData < lineNumberTableDataEnd)
        {
            table->offset = (Elf32_Off)(lineNumberTableData - lineNumberTableDataStart);

            readSourceStatementTable(elf, lineNumberTableData, table, entry);
        }
    }

    return ReadSuccess;
}

void Dwarf::destroy()
{
    if (internalData)
    {
        free(internalData);
    }

    internalData = nullptr;
    elf = nullptr;
    entries = nullptr;
    entryCount = 0;
    attributes = nullptr;
    attributeCount = 0;
    sourceStatementTables = nullptr;
    sourceStatementTableCount = 0;
}

void Dwarf::readAttribute(char*& data, DwarfAttribute* attribute)
{
    ::readAttribute(elf, data, attribute);
}

const char* Dwarf::tagToString(Elf32_Half tag)
{
    static char defaultString[32];

    switch (tag)
    {
    case DW_TAG_padding: return "TAG_padding";
    case DW_TAG_array_type: return "TAG_array_type";
    case DW_TAG_class_type: return "TAG_class_type";
    case DW_TAG_entry_point: return "TAG_entry_point";
    case DW_TAG_enumeration_type: return "TAG_enumeration_type";
    case DW_TAG_formal_parameter: return "TAG_formal_parameter";
    case DW_TAG_global_subroutine: return "TAG_global_subroutine";
    case DW_TAG_global_variable: return "TAG_global_variable";
    case DW_TAG_label: return "TAG_label";
    case DW_TAG_lexical_block: return "TAG_lexical_block";
    case DW_TAG_local_variable: return "TAG_local_variable";
    case DW_TAG_member: return "TAG_member";
    case DW_TAG_pointer_type: return "TAG_pointer_type";
    case DW_TAG_reference_type: return "TAG_reference_type";
    case DW_TAG_compile_unit: return "TAG_compile_unit";
    case DW_TAG_string_type: return "TAG_string_type";
    case DW_TAG_structure_type: return "TAG_structure_type";
    case DW_TAG_subroutine: return "TAG_subroutine";
    case DW_TAG_subroutine_type: return "TAG_subroutine_type";
    case DW_TAG_typedef: return "TAG_typedef";
    case DW_TAG_union_type: return "TAG_union_type";
    case DW_TAG_unspecified_parameters: return "TAG_unspecified_parameters";
    case DW_TAG_variant: return "TAG_variant";
    case DW_TAG_common_block: return "TAG_common_block";
    case DW_TAG_common_inclusion: return "TAG_common_inclusion";
    case DW_TAG_inheritance: return "TAG_inheritance";
    case DW_TAG_inlined_subroutine: return "TAG_inlined_subroutine";
    case DW_TAG_module: return "TAG_module";
    case DW_TAG_ptr_to_member_type: return "TAG_ptr_to_member_type";
    case DW_TAG_set_type: return "TAG_set_type";
    case DW_TAG_subrange_type: return "TAG_subrange_type";
    case DW_TAG_with_stmt: return "TAG_with_stmt";
    }

    sprintf(defaultString, "TAG_<unknown 0x%x>", tag);
    return defaultString;
}

const char* Dwarf::formToString(Elf32_Half form)
{
    static char defaultString[32];

    switch (form)
    {
    case DW_FORM_ADDR: return "FORM_ADDR";
    case DW_FORM_REF: return "FORM_REF";
    case DW_FORM_BLOCK2: return "FORM_BLOCK2";
    case DW_FORM_BLOCK4: return "FORM_BLOCK4";
    case DW_FORM_DATA2: return "FORM_DATA2";
    case DW_FORM_DATA4: return "FORM_DATA4";
    case DW_FORM_DATA8: return "FORM_DATA8";
    case DW_FORM_STRING: return "FORM_STRING";
    }

    sprintf(defaultString, "FORM_<unknown 0x%x>", form);
    return defaultString;
}

const char* Dwarf::attrNameToString(Elf32_Half name)
{
    static char defaultString[32];

    switch (name)
    {
    case DW_AT_sibling: return "AT_sibling";
    case DW_AT_location: return "AT_location";
    case DW_AT_name: return "AT_name";
    case DW_AT_fund_type: return "AT_fund_type";
    case DW_AT_mod_fund_type: return "AT_mod_fund_type";
    case DW_AT_user_def_type: return "AT_user_def_type";
    case DW_AT_mod_u_d_type: return "AT_mod_u_d_type";
    case DW_AT_ordering: return "AT_ordering";
    case DW_AT_subscr_data: return "AT_subscr_data";
    case DW_AT_byte_size: return "AT_byte_size";
    case DW_AT_bit_offset: return "AT_bit_offset";
    case DW_AT_bit_size: return "AT_bit_size";
    case DW_AT_element_list: return "AT_element_list";
    case DW_AT_stmt_list: return "AT_stmt_list";
    case DW_AT_low_pc: return "AT_low_pc";
    case DW_AT_high_pc: return "AT_high_pc";
    case DW_AT_language: return "AT_language";
    case DW_AT_member: return "AT_member";
    case DW_AT_discr: return "AT_discr";
    case DW_AT_discr_value: return "AT_discr_value";
    case DW_AT_string_length: return "AT_string_length";
    case DW_AT_common_reference: return "AT_common_reference";
    case DW_AT_comp_dir: return "AT_comp_dir";
    case DW_AT_const_value_string: return "AT_const_value_string";
    case DW_AT_const_value_data2: return "AT_const_value_data2";
    case DW_AT_const_value_data4: return "AT_const_value_data4";
    case DW_AT_const_value_data8: return "AT_const_value_data8";
    case DW_AT_const_value_block2: return "AT_const_value_block2";
    case DW_AT_const_value_block4: return "AT_const_value_block4";
    case DW_AT_containing_type: return "AT_containing_type";
    case DW_AT_default_value_addr: return "AT_default_value_addr";
    case DW_AT_default_value_data2: return "AT_default_value_data2";
    case DW_AT_default_value_data4: return "AT_default_value_data4";
    case DW_AT_default_value_data8: return "AT_default_value_data8";
    case DW_AT_default_value_string: return "AT_default_value_string";
    case DW_AT_friends: return "AT_friends";
    case DW_AT_inline: return "AT_inline";
    case DW_AT_is_optional: return "AT_is_optional";
    case DW_AT_lower_bound_ref: return "AT_lower_bound_ref";
    case DW_AT_lower_bound_data2: return "AT_lower_bound_data2";
    case DW_AT_lower_bound_data4: return "AT_lower_bound_data4";
    case DW_AT_lower_bound_data8: return "AT_lower_bound_data8";
    case DW_AT_program: return "AT_program";
    case DW_AT_private: return "AT_private";
    case DW_AT_producer: return "AT_producer";
    case DW_AT_protected: return "AT_protected";
    case DW_AT_prototyped: return "AT_prototyped";
    case DW_AT_public: return "AT_public";
    case DW_AT_pure_virtual: return "AT_pure_virtual";
    case DW_AT_return_addr: return "AT_return_addr";
    case DW_AT_specification: return "AT_specification";
    case DW_AT_start_scope: return "AT_start_scope";
    case DW_AT_stride_size: return "AT_stride_size";
    case DW_AT_upper_bound_ref: return "AT_upper_bound_ref";
    case DW_AT_upper_bound_data2: return "AT_upper_bound_data2";
    case DW_AT_upper_bound_data4: return "AT_upper_bound_data4";
    case DW_AT_upper_bound_data8: return "AT_upper_bound_data8";
    case DW_AT_virtual: return "AT_virtual";
    case DW_AT_MW_mangled: return "AT_mangled";
    case DW_AT_MW_restore_SP: return "AT_restore_SP";
    case DW_AT_MW_global_ref: return "AT_global_ref";
    case DW_AT_MW_global_ref_by_name: return "AT_global_ref_by_name";
    case DW_AT_MW_DWARF2_location: return "AT_DWARF2_location";
    case DW_AT_MW_source_file_names: return "AT_source_file_names";
    case DW_AT_MW_src_info: return "AT_src_info";
    case DW_AT_MW_mac_info: return "AT_mac_info";
    case DW_AT_MW_src_coords: return "AT_src_coords";
    case DW_AT_MW_body_begin: return "AT_body_begin";
    case DW_AT_MW_body_end: return "AT_body_end";
    case DW_AT_MW_source_info: return "AT_source_info";
    }

    sprintf(defaultString, "AT_<unknown 0x%x>", name);
    return defaultString;
}

DwarfAttribute* DwarfEntry::findAttribute(Elf32_Half name) const
{
    for (int i = 0; i < attributeCount; i++)
    {
        if (attributes[i].name == name)
        {
            return &attributes[i];
        }
    }

    return nullptr;
}

const char* DwarfEntry::getName() const
{
    DwarfAttribute* nameAttribute = findAttribute(DW_AT_name);

    if (nameAttribute)
    {
        return nameAttribute->string;
    }

    return "";
}
