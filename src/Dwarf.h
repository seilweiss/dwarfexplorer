#pragma once

#include "Elf.h"

#include <vector>

#define DW_TAG_padding 0x0000
#define DW_TAG_array_type 0x0001
#define DW_TAG_class_type 0x0002
#define DW_TAG_entry_point 0x0003
#define DW_TAG_enumeration_type 0x0004
#define DW_TAG_formal_parameter 0x0005
#define DW_TAG_global_subroutine 0x0006
#define DW_TAG_global_variable 0x0007
#define DW_TAG_label 0x000a
#define DW_TAG_lexical_block 0x000b
#define DW_TAG_local_variable 0x000c
#define DW_TAG_member 0x000d
#define DW_TAG_pointer_type 0x000f
#define DW_TAG_reference_type 0x0010
#define DW_TAG_compile_unit 0x0011
#define DW_TAG_source_file 0x0011
#define DW_TAG_string_type 0x0012
#define DW_TAG_structure_type 0x0013
#define DW_TAG_subroutine 0x0014
#define DW_TAG_subroutine_type 0x0015
#define DW_TAG_typedef 0x0016
#define DW_TAG_union_type 0x0017
#define DW_TAG_unspecified_parameters 0x0018
#define DW_TAG_variant 0x0019
#define DW_TAG_common_block 0x001a
#define DW_TAG_common_inclusion 0x001b
#define DW_TAG_inheritance 0x001c
#define DW_TAG_inlined_subroutine 0x001d
#define DW_TAG_module 0x001e
#define DW_TAG_ptr_to_member_type 0x001f
#define DW_TAG_set_type 0x0020
#define DW_TAG_subrange_type 0x0021
#define DW_TAG_with_stmt 0x0022
#define DW_TAG_lo_user 0x4080
#define DW_TAG_hi_user 0xffff

#define DW_FORM_ADDR 0x1
#define DW_FORM_REF 0x2
#define DW_FORM_BLOCK2 0x3
#define DW_FORM_BLOCK4 0x4
#define DW_FORM_DATA2 0x5
#define DW_FORM_DATA4 0x6
#define DW_FORM_DATA8 0x7
#define DW_FORM_STRING 0x8

#define DW_AT_sibling (0x0010|DW_FORM_REF)
#define DW_AT_location (0x0020|DW_FORM_BLOCK2)
#define DW_AT_name (0x0030|DW_FORM_STRING)
#define DW_AT_fund_type (0x0050|DW_FORM_DATA2)
#define DW_AT_mod_fund_type (0x0060|DW_FORM_BLOCK2)
#define DW_AT_user_def_type (0x0070|DW_FORM_REF)
#define DW_AT_mod_u_d_type (0x0080|DW_FORM_BLOCK2)
#define DW_AT_ordering (0x0090|DW_FORM_DATA2)
#define DW_AT_subscr_data (0x00a0|DW_FORM_BLOCK2)
#define DW_AT_byte_size (0x00b0|DW_FORM_DATA4)
#define DW_AT_bit_offset (0x00c0|DW_FORM_DATA2)
#define DW_AT_bit_size (0x00d0|DW_FORM_DATA4)
#define DW_AT_element_list (0x00f0|DW_FORM_BLOCK4)
#define DW_AT_stmt_list (0x0100|DW_FORM_DATA4)
#define DW_AT_low_pc (0x0110|DW_FORM_ADDR)
#define DW_AT_high_pc (0x0120|DW_FORM_ADDR)
#define DW_AT_language (0x0130|DW_FORM_DATA4)
#define DW_AT_member (0x0140|DW_FORM_REF)
#define DW_AT_discr (0x0150|DW_FORM_REF)
#define DW_AT_discr_value (0x0160|DW_FORM_BLOCK2)
#define DW_AT_string_length (0x0190|DW_FORM_BLOCK2)
#define DW_AT_common_reference (0x01a0|DW_FORM_REF)
#define DW_AT_comp_dir (0x01b0|DW_FORM_STRING)
#define DW_AT_const_value_string (0x01c0|DW_FORM_STRING)
#define DW_AT_const_value_data2 (0x01c0|DW_FORM_DATA2)
#define DW_AT_const_value_data4 (0x01c0|DW_FORM_DATA4)
#define DW_AT_const_value_data8 (0x01c0|DW_FORM_DATA8)
#define DW_AT_const_value_block2 (0x01c0|DW_FORM_BLOCK2)
#define DW_AT_const_value_block4 (0x01c0|DW_FORM_BLOCK4)
#define DW_AT_containing_type (0x01d0|DW_FORM_REF)
#define DW_AT_default_value_addr (0x01e0|DW_FORM_ADDR)
#define DW_AT_default_value_data2 (0x01e0|DW_FORM_DATA2)
#define DW_AT_default_value_data4 (0x01e0|DW_FORM_DATA4)
#define DW_AT_default_value_data8 (0x01e0|DW_FORM_DATA8)
#define DW_AT_default_value_string (0x01e0|DW_FORM_STRING)
#define DW_AT_friends (0x01f0|DW_FORM_BLOCK2)
#define DW_AT_inline (0x0200|DW_FORM_STRING)
#define DW_AT_is_optional (0x0210|DW_FORM_STRING)
#define DW_AT_lower_bound_ref (0x0220|DW_FORM_REF)
#define DW_AT_lower_bound_data2 (0x0220|DW_FORM_DATA2)
#define DW_AT_lower_bound_data4 (0x0220|DW_FORM_DATA4)
#define DW_AT_lower_bound_data8 (0x0220|DW_FORM_DATA8)
#define DW_AT_program (0x0230|DW_FORM_STRING)
#define DW_AT_private (0x0240|DW_FORM_STRING)
#define DW_AT_producer (0x0250|DW_FORM_STRING)
#define DW_AT_protected (0x0260|DW_FORM_STRING)
#define DW_AT_prototyped (0x0270|DW_FORM_STRING)
#define DW_AT_public (0x0280|DW_FORM_STRING)
#define DW_AT_pure_virtual (0x0290|DW_FORM_STRING)
#define DW_AT_return_addr (0x02a0|DW_FORM_BLOCK2)
#define DW_AT_specification (0x02b0|DW_FORM_REF)
#define DW_AT_start_scope (0x02c0|DW_FORM_DATA4)
#define DW_AT_stride_size (0x02e0|DW_FORM_DATA4)
#define DW_AT_upper_bound_ref (0x02f0|DW_FORM_REF)
#define DW_AT_upper_bound_data2 (0x02f0|DW_FORM_DATA2)
#define DW_AT_upper_bound_data4 (0x02f0|DW_FORM_DATA4)
#define DW_AT_upper_bound_data8 (0x02f0|DW_FORM_DATA8)
#define DW_AT_virtual (0x0300|DW_FORM_STRING)
#define DW_AT_lo_user 0x2000
#define DW_AT_hi_user 0x3ff0

// Metrowerks
#define DW_AT_mangled (0x2000|DW_FORM_STRING)
#define DW_AT_source_info (0x2020|DW_FORM_REF)

#define DW_OP_REG 0x01
#define DW_OP_BASEREG 0x02
#define DW_OP_ADDR 0x03
#define DW_OP_CONST 0x04
#define DW_OP_DEREF2 0x05
#define DW_OP_DEREF 0x06
#define DW_OP_DEREF4 0x06
#define DW_OP_ADD 0x07
#define DW_OP_lo_user 0xe0
#define DW_OP_hi_user 0xff

#define DW_FT_char 0x0001
#define DW_FT_signed_char 0x0002
#define DW_FT_unsigned_char 0x0003
#define DW_FT_short 0x0004
#define DW_FT_signed_short 0x0005
#define DW_FT_unsigned_short 0x0006
#define DW_FT_integer 0x0007
#define DW_FT_signed_integer 0x0008
#define DW_FT_unsigned_integer 0x0009
#define DW_FT_long 0x000a
#define DW_FT_signed_long 0x000b
#define DW_FT_unsigned_long 0x000c
#define DW_FT_pointer 0x000d
#define DW_FT_float 0x000e
#define DW_FT_dbl_prec_float 0x000f
#define DW_FT_ext_prec_float 0x0010
#define DW_FT_complex 0x0011
#define DW_FT_dbl_prec_complex 0x0012
#define DW_FT_void 0x0014
#define DW_FT_boolean 0x0015
#define DW_FT_ext_prec_complex 0x0016
#define DW_FT_label 0x0017
#define DW_FT_lo_user 0x8000
#define DW_FT_hi_user 0xffff

// Metrowerks
#define DW_FT_long_long (DW_FT_lo_user | 0x0008)

#define DW_MOD_pointer_to 0x01
#define DW_MOD_reference_to 0x02
#define DW_MOD_const 0x03
#define DW_MOD_volatile 0x04
#define DW_MOD_lo_user 0x80
#define DW_MOD_hi_user 0xff

#define DW_LANG_C89 0x00000001
#define DW_LANG_C 0x00000002
#define DW_LANG_ADA83 0x00000003
#define DW_LANG_C_PLUS_PLUS 0x00000004
#define DW_LANG_COBOL74 0x00000005
#define DW_LANG_COBOL85 0x00000006
#define DW_LANG_FORTRAN77 0x00000007
#define DW_LANG_FORTRAN90 0x00000008
#define DW_LANG_PASCAL83 0x00000009
#define DW_LANG_MODULA2 0x0000000a
#define DW_LANG_lo_user 0x00008000
#define DW_LANG_hi_user 0x0000ffff

#define DW_ORD_row_major 0
#define DW_ORD_col_major 1

#define DW_FMT_FT_C_C 0x0
#define DW_FMT_FT_C_X 0x1
#define DW_FMT_FT_X_C 0x2
#define DW_FMT_FT_X_X 0x3
#define DW_FMT_UT_C_C 0x4
#define DW_FMT_UT_C_X 0x5
#define DW_FMT_UT_X_C 0x6
#define DW_FMT_UT_X_X 0x7
#define DW_FMT_ET 0x8

struct DwarfEntry;

struct DwarfAttribute
{
    Elf32_Off offset;
    Elf32_Off length;
    Elf32_Half name;

    union
    {
        Elf32_Addr addr;
        Elf32_Off ref;

        struct
        {
            Elf32_Word blockLength;
            char* block;
        };

        Elf32_Half data2;
        Elf32_Word data4;
        Elf32_Xword data8;
        char* string;
    };

    Elf32_Half getForm() const { return name & 0xf; }
};

struct DwarfEntry
{
    Elf32_Off offset;
    Elf32_Off length;
    Elf32_Half tag;
    DwarfAttribute* attributes;
    int attributeCount;
    DwarfEntry* sibling;
    int siblingCount;
    DwarfEntry* firstChild;
    int childCount;

    bool isNull() const { return length < 8; }
    DwarfAttribute* findAttribute(Elf32_Half name) const;
    const char* getName() const;
};

struct Dwarf
{
    const Elf* elf;
    DwarfEntry* entries;
    int entryCount;
    DwarfAttribute* attributes;
    int attributeCount;

    enum ReadResult
    {
        ReadSuccess,
        ReadSectionNotFound
    };

    ReadResult read(const Elf* elf);
    void destroy();

    void readAttribute(char*& data, DwarfAttribute* attribute);

    static const char* tagToString(Elf32_Half tag);
    static const char* formToString(Elf32_Half form);
    static const char* attrNameToString(Elf32_Half name);
};
