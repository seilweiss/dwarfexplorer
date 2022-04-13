#pragma once

#include "Dwarf.h"

#include <vector>

struct DwarfLocationAtom
{
    char op;

    union
    {
        Elf32_Word number;
        Elf32_Addr addr;
    };
};

struct DwarfLocation
{
    std::vector<DwarfLocationAtom> atoms;

    void read(Dwarf* dwarf, DwarfAttribute* attribute);
};

struct DwarfType
{
    bool isFundamental;

    union
    {
        Elf32_Half fundType;
        Elf32_Off udTypeOffset;
    };
    
    std::vector<char> modifiers;

    void read(Dwarf* dwarf, DwarfAttribute* attribute);
};

struct DwarfSubscriptDataBound
{
    bool isConstant;
    Elf32_Word constant;
    DwarfLocation location;
};

struct DwarfSubscriptDataItem
{
    DwarfType indexType;
    DwarfSubscriptDataBound lowBound;
    DwarfSubscriptDataBound highBound;
};

struct DwarfSubscriptData
{
    DwarfType elementType;
    std::vector<DwarfSubscriptDataItem> items;

    void read(Dwarf* dwarf, DwarfAttribute* attribute);
};

struct DwarfElementListItem
{
    const char* name;
    int value;
};

struct DwarfElementList
{
    std::vector<DwarfElementListItem> items;

    void read(Dwarf* dwarf, DwarfAttribute* attribute);
};