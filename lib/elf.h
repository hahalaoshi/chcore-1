/*
 * Copyright (c) 2020 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * OS-Lab-2020 (i.e., ChCore) is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *   http://license.coscl.org.cn/MulanPSL
 *   THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 *   PURPOSE.
 *   See the Mulan PSL v1 for more details.
 */

#pragma once

#include <common/types.h>
#include <lib/endianness.h>
#include <lib/errno.h>

/*
 * ELF format according to
 * https://en.wikipedia.org/wiki/Executable_and_Linkable_Format
 */

#define EI_MAG_SIZE 4

#define PT_NULL        0x00000000
#define PT_LOAD        0x00000001
#define PT_DYNAMIC    0x00000002
#define PT_INTERP    0x00000003
#define PT_NOTE        0x00000004
#define PT_SHLIB    0x00000005
#define PT_PHDR        0x00000006
#define PT_LOOS        0x60000000
#define PT_HIOS        0x6fffffff
#define PT_LOPROC    0x70000000
#define PT_HIRPOC    0x7fffffff

#define PF_ALL    0x7
#define PF_X    0x1
#define PF_W    0x2
#define PF_R    0x4

/*
 * This part of ELF header is endianness-independent.
 */
// 16 bytes
struct elf_indent {
    // not important, 0x7f followed by ELF
    u8 ei_magic[4];
    // show 32 bit(1) or 64 bit(2)
    u8 ei_class;
    // show endianness, little(1), big(2)
    u8 ei_data;
    // Set to 1 for the original and current version of ELF.
    u8 ei_version;
    // target operating system
    u8 ei_osabi;
    // Further specifies the ABI version
    u8 ei_abiversion;
    // unused
    u8 ei_pad[7];
};

/*
 * ELF header format. One should check the `e_indent` to decide the endianness.
 */
struct elf_header {
    // 16 bytes
    struct elf_indent e_indent;
    // 48 bytes
    // 	Identifies object file type.
    u16 e_type;
    // Specifies target instruction set architecture
    u16 e_machine;
    // Set to 1 for the original version of ELF.
    u32 e_version;
    // This is the memory address of the entry point from where the process starts executing.
    u64 e_entry;
    // Points to the start of the program header table.
    u64 e_phoff;
    // Points to the start of the section header table.
    u64 e_shoff;
    // Interpretation of this field depends on the target architecture.
    u32 e_flags;
    // Contains the size of this header, normally 64 ByteS.
    u16 e_ehsize;
    u16 e_phentsize; /* The size of a program header table entry */
    u16 e_phnum;     /* The number of entries in the program header table */
    u16 e_shentsize; /* The size of a section header table entry */
    u16 e_shnum;     /* The number of entries in the section header table */
    u16 e_shstrndx;  /* Index of the section header table entry that
			    contains the section names. */
};

/*
 * 32-Bit of the elf_header. Check the `e_indent` first to decide.
 */
struct elf_header_32 {
    struct elf_indent e_indent;
    u16 e_type;
    u16 e_machine;
    u32 e_version;
    u32 e_entry;
    u32 e_phoff;
    u32 e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;
    u16 e_shentsize;
    u16 e_shnum;
    u16 e_shstrndx;
};

struct elf_program_header {
    // Identifies the type of the segment. PT_***
    u32 p_type;
    // Segment-dependent flags
    u32 p_flags;
    // Offset of the segment in the file image.
    u64 p_offset;
    // Virtual address of the segment in memory.
    u64 p_vaddr;
    // On systems where physical address is relevant, reserved for segment's physical address.
    u64 p_paddr;
    // Size in bytes of the segment in the file image.
    u64 p_filesz;
    // Size in bytes of the segment in memory.
    u64 p_memsz;
    // 0 and 1 specify no alignment. Otherwise should be a positive, integral power of 2, with p_vaddr equating p_offset modulus p_align.
    u64 p_align;
};

struct elf_program_header_32 {
    u32 p_type;
    u32 p_offset;
    u32 p_vaddr;
    u32 p_paddr;
    u32 p_filesz;
    u32 p_memsz;
    u32 p_flags;
    u32 p_align;
};

struct elf_section_header {
    // An offset to a string in the .shstrtab section that represents the name of this section
    u32 sh_name;
    // Identifies the type of this header. 	SHT_STRTAB ...
    u32 sh_type;
    // 	Identifies the attributes of the section. write, alloc, executable, ...
    u64 sh_flags;
    // Virtual address of the section in memory, for sections that are loaded.
    u64 sh_addr;
    // 	Offset of the section in the file image.
    u64 sh_offset;
    // Size in bytes of the section in the file image.
    u64 sh_size;
    // Contains the section index of an associated section.
    u32 sh_link;
    // Contains extra information about the section.
    u32 sh_info;
    // Contains the required alignment of the section.
    u64 sh_addralign;
    // Contains the size, in bytes, of each entry, for sections that contain fixed-size entries.
    u64 sh_entsize;
};

struct elf_section_header_32 {
    u32 sh_name;
    u32 sh_type;
    u32 sh_flags;
    u32 sh_addr;
    u32 sh_offset;
    u32 sh_size;
    u32 sh_link;
    u32 sh_info;
    u32 sh_addralign;
    u32 sh_entsize;
};

struct elf_file {
    // 64 bytes
    struct elf_header header;
    // Program header table, describing zero or more memory segments. Continuous memory of program header?
    struct elf_program_header *p_headers;
    // Section header table, describing zero or more sections
    struct elf_section_header *s_headers;
    // The segments contain information that is needed for run time execution of the file,
    // while sections contain important data for linking and relocation.
};

struct elf_file *elf_parse_file(const char *code);

void elf_free(struct elf_file *elf);
