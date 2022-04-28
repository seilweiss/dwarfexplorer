#include "Elf.h"

#include <cstdio>
#include <memory>

Elf::ReadResult Elf::read(const char* path)
{
    header = nullptr;
    programHeaderTable = nullptr;
    sectionHeaderTable = nullptr;
    sectionNameTable = nullptr;
    stringTable = nullptr;
    symbolTable = nullptr;

    FILE* file = fopen(path, "rb");

    if (!file)
    {
        return ReadOpenFailed;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* data = (char*)malloc(size);

    header = (Elf32_Ehdr*)data;

    size_t bytesRead = fread(data, 1, size, file);

    fclose(file);

    if (bytesRead != size)
    {
        return ReadFailed;
    }

    if (header->e_ident[EI_MAG0] != ELFMAG0 ||
        header->e_ident[EI_MAG1] != ELFMAG1 ||
        header->e_ident[EI_MAG2] != ELFMAG2 ||
        header->e_ident[EI_MAG3] != ELFMAG3)
    {
        return ReadInvalidHeader;
    }

    toNativeEndian(&header->e_type);
    toNativeEndian(&header->e_machine);
    toNativeEndian(&header->e_version);
    toNativeEndian(&header->e_entry);
    toNativeEndian(&header->e_phoff);
    toNativeEndian(&header->e_shoff);
    toNativeEndian(&header->e_flags);
    toNativeEndian(&header->e_ehsize);
    toNativeEndian(&header->e_phentsize);
    toNativeEndian(&header->e_phnum);
    toNativeEndian(&header->e_shentsize);
    toNativeEndian(&header->e_shnum);
    toNativeEndian(&header->e_shstrndx);

    if (header->e_phoff != 0)
    {
        programHeaderTable = (Elf32_Phdr*)(data + header->e_phoff);

        for (Elf32_Half i = 0; i < header->e_phnum; i++)
        {
            toNativeEndian(&programHeaderTable[i].p_type);
            toNativeEndian(&programHeaderTable[i].p_offset);
            toNativeEndian(&programHeaderTable[i].p_vaddr);
            toNativeEndian(&programHeaderTable[i].p_paddr);
            toNativeEndian(&programHeaderTable[i].p_filesz);
            toNativeEndian(&programHeaderTable[i].p_memsz);
            toNativeEndian(&programHeaderTable[i].p_flags);
            toNativeEndian(&programHeaderTable[i].p_align);
        }
    }

    if (header->e_shoff != 0)
    {
        sectionHeaderTable = (Elf32_Shdr*)(data + header->e_shoff);

        for (Elf32_Half i = 0; i < header->e_shnum; i++)
        {
            toNativeEndian(&sectionHeaderTable[i].sh_name);
            toNativeEndian(&sectionHeaderTable[i].sh_type);
            toNativeEndian(&sectionHeaderTable[i].sh_flags);
            toNativeEndian(&sectionHeaderTable[i].sh_addr);
            toNativeEndian(&sectionHeaderTable[i].sh_offset);
            toNativeEndian(&sectionHeaderTable[i].sh_size);
            toNativeEndian(&sectionHeaderTable[i].sh_link);
            toNativeEndian(&sectionHeaderTable[i].sh_info);
            toNativeEndian(&sectionHeaderTable[i].sh_addralign);
            toNativeEndian(&sectionHeaderTable[i].sh_entsize);
        }
    }

    if (sectionHeaderTable)
    {
        if (header->e_shstrndx != SHN_UNDEF)
        {
            sectionNameTable = (char*)getSectionData(header->e_shstrndx);
        }

        stringTable = (char*)getSectionData(".strtab");

        for (Elf32_Half i = 0; i < header->e_shnum; i++)
        {
            if (sectionHeaderTable[i].sh_type == SHT_SYMTAB)
            {
                symbolTable = (Elf32_Sym*)getSectionData(i);

                if (symbolTable)
                {
                    for (Elf32_Half j = 0; j < sectionHeaderTable[i].sh_size / sizeof(Elf32_Sym); j++)
                    {
                        toNativeEndian(&symbolTable[j].st_name);
                        toNativeEndian(&symbolTable[j].st_value);
                        toNativeEndian(&symbolTable[j].st_size);
                        toNativeEndian(&symbolTable[j].st_info);
                        toNativeEndian(&symbolTable[j].st_other);
                        toNativeEndian(&symbolTable[j].st_shndx);
                    }
                }

                break;
            }
        }
    }

    return ReadSuccess;
}

void Elf::destroy()
{
    if (header)
    {
        free(header);
    }

    header = nullptr;
    programHeaderTable = nullptr;
    sectionHeaderTable = nullptr;
    sectionNameTable = nullptr;
    stringTable = nullptr;
    symbolTable = nullptr;
}

void* Elf::offsetToPointer(Elf32_Off offset) const
{
    return (char*)header + offset;
}

char* Elf::getSectionName(Elf32_Half index) const
{
    if (sectionNameTable)
    {
        return sectionNameTable + sectionHeaderTable[index].sh_name;
    }

    return nullptr;
}

void* Elf::getSectionData(Elf32_Half index) const
{
    if (sectionHeaderTable[index].sh_offset != 0)
    {
        return (char*)offsetToPointer(sectionHeaderTable[index].sh_offset);
    }

    return nullptr;
}

void* Elf::getSectionData(const char* name) const
{
    Elf32_Half index = getSectionIndex(name);

    if (index != -1)
    {
        return getSectionData(index);
    }

    return nullptr;
}

Elf32_Half Elf::getSectionIndex(const char* name) const
{
    for (Elf32_Half i = 0; i < header->e_shnum; i++)
    {
        char* curName = getSectionName(i);

        if (curName && strcmp(curName, name) == 0)
        {
            return i;
        }
    }

    return SHN_UNDEF;
}


void* Elf::getAddressData(Elf32_Addr addr) const
{
    for (Elf32_Half i = 0; i < header->e_phnum; i++)
    {
        Elf32_Phdr* phdr = &programHeaderTable[i];

        if (phdr->p_vaddr <= addr && phdr->p_vaddr + phdr->p_memsz > addr)
        {
            return offsetToPointer(phdr->p_offset + (addr - phdr->p_vaddr));
        }
    }

    return nullptr;
}
