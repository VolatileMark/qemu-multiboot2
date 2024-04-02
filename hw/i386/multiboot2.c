/*
 * QEMU PC System Emulator
 *
 * Copyright (c) 2003-2004 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "qemu/option.h"
#include "cpu.h"
#include "hw/nvram/fw_cfg.h"
#include "multiboot2.h"
#include "hw/loader.h"
#include "elf.h"
#include "sysemu/sysemu.h"
#include "qemu/error-report.h"

/* Show multiboot debug output */
#define DEBUG_MULTIBOOT

#ifdef DEBUG_MULTIBOOT
#define mb_debug(a...) fprintf(stderr, ## a)
#else
#define mb_debug(a...)
#endif

#define MULTIBOOT_MEM 0x8000

#if MULTIBOOT_MEM > 0xf0000
#error multiboot struct needs to fit in 16 bit real mode
#endif

/**********************************************************************
 * multiboot2.h                                                       *
 **********************************************************************/
/*  multiboot2.h - Multiboot 2 header file.  */
/*  Copyright (C) 1999,2003,2007,2008,2009,2010  Free Software Foundation, Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL ANY
 *  DEVELOPER OR DISTRIBUTOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef MULTIBOOT_HEADER
#define MULTIBOOT_HEADER 1

/* How many bytes from the start of the file we search for the header.  */
#define MULTIBOOT_SEARCH			32768
#define MULTIBOOT_HEADER_ALIGN			8

/* The magic field should contain this.  */
#define MULTIBOOT2_HEADER_MAGIC			0xe85250d6

/* This should be in %eax.  */
#define MULTIBOOT2_BOOTLOADER_MAGIC		0x36d76289

/* Alignment of multiboot modules.  */
#define MULTIBOOT_MOD_ALIGN			0x00001000

/* Alignment of the multiboot info structure.  */
#define MULTIBOOT_INFO_ALIGN			0x00000008

/* Flags set in the 'flags' member of the multiboot header.  */

#define MULTIBOOT_TAG_ALIGN                  8
#define MULTIBOOT_TAG_TYPE_END               0
#define MULTIBOOT_TAG_TYPE_CMDLINE           1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME  2
#define MULTIBOOT_TAG_TYPE_MODULE            3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO     4
#define MULTIBOOT_TAG_TYPE_BOOTDEV           5
#define MULTIBOOT_TAG_TYPE_MMAP              6
#define MULTIBOOT_TAG_TYPE_VBE               7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER       8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS      9
#define MULTIBOOT_TAG_TYPE_APM               10
#define MULTIBOOT_TAG_TYPE_EFI32             11
#define MULTIBOOT_TAG_TYPE_EFI64             12
#define MULTIBOOT_TAG_TYPE_SMBIOS            13
#define MULTIBOOT_TAG_TYPE_ACPI_OLD          14
#define MULTIBOOT_TAG_TYPE_ACPI_NEW          15
#define MULTIBOOT_TAG_TYPE_NETWORK           16

#define MULTIBOOT_HEADER_TAG_END  0
#define MULTIBOOT_HEADER_TAG_INFORMATION_REQUEST  1
#define MULTIBOOT_HEADER_TAG_ADDRESS  2
#define MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS  3
#define MULTIBOOT_HEADER_TAG_CONSOLE_FLAGS  4
#define MULTIBOOT_HEADER_TAG_FRAMEBUFFER  5
#define MULTIBOOT_HEADER_TAG_MODULE_ALIGN  6

#define MULTIBOOT_ARCHITECTURE_I386  0
#define MULTIBOOT_ARCHITECTURE_X86_64 1
#define MULTIBOOT_ARCHITECTURE_MIPS32  4
#define MULTIBOOT_HEADER_TAG_OPTIONAL 1

#define MULTIBOOT_CONSOLE_FLAGS_CONSOLE_REQUIRED 1
#define MULTIBOOT_CONSOLE_FLAGS_EGA_TEXT_SUPPORTED 2

#ifndef ASM_FILE

typedef unsigned char		multiboot_uint8_t;
typedef unsigned short		multiboot_uint16_t;
typedef unsigned int		multiboot_uint32_t;
typedef unsigned long long	multiboot_uint64_t;

struct multiboot_header
{
  /* Must be MULTIBOOT_MAGIC - see above.  */
  multiboot_uint32_t magic;

  /* ISA */
  multiboot_uint32_t architecture;

  /* Total header length.  */
  multiboot_uint32_t header_length;

  /* The above fields plus this one must equal 0 mod 2^32. */
  multiboot_uint32_t checksum;
};

struct multiboot_header_tag
{
  multiboot_uint16_t type;
  multiboot_uint16_t flags;
  multiboot_uint32_t size;
};

struct multiboot_header_tag_information_request
{
  multiboot_uint16_t type;
  multiboot_uint16_t flags;
  multiboot_uint32_t size;
  multiboot_uint32_t requests[0];
};

struct multiboot_header_tag_address
{
  multiboot_uint16_t type;
  multiboot_uint16_t flags;
  multiboot_uint32_t size;
  multiboot_uint32_t header_addr;
  multiboot_uint32_t load_addr;
  multiboot_uint32_t load_end_addr;
  multiboot_uint32_t bss_end_addr;
};

struct multiboot_header_tag_entry_address
{
  multiboot_uint16_t type;
  multiboot_uint16_t flags;
  multiboot_uint32_t size;
  multiboot_uint32_t entry_addr;
};

struct multiboot_header_tag_console_flags
{
  multiboot_uint16_t type;
  multiboot_uint16_t flags;
  multiboot_uint32_t size;
  multiboot_uint32_t console_flags;
};

struct multiboot_header_tag_framebuffer
{
  multiboot_uint16_t type;
  multiboot_uint16_t flags;
  multiboot_uint32_t size;
  multiboot_uint32_t width;
  multiboot_uint32_t height;
  multiboot_uint32_t depth;
};

struct multiboot_header_tag_module_align
{
  multiboot_uint16_t type;
  multiboot_uint16_t flags;
  multiboot_uint32_t size;
  multiboot_uint32_t width;
  multiboot_uint32_t height;
  multiboot_uint32_t depth;
};

struct multiboot_color
{
  multiboot_uint8_t red;
  multiboot_uint8_t green;
  multiboot_uint8_t blue;
};

struct multiboot_mmap_entry
{
  multiboot_uint64_t addr;
  multiboot_uint64_t len;
#define MULTIBOOT_MEMORY_AVAILABLE		1
#define MULTIBOOT_MEMORY_RESERVED		2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE       3
#define MULTIBOOT_MEMORY_NVS                    4
#define MULTIBOOT_MEMORY_BADRAM                 5
  multiboot_uint32_t type;
  multiboot_uint32_t zero;
} __attribute__((packed));
typedef struct multiboot_mmap_entry multiboot_memory_map_t;

struct multiboot_tag
{
  multiboot_uint32_t type;
  multiboot_uint32_t size;
};

struct multiboot_tag_string
{
  multiboot_uint32_t type;
  multiboot_uint32_t size;
  char string[0];
};

struct multiboot_tag_module
{
  multiboot_uint32_t type;
  multiboot_uint32_t size;
  multiboot_uint32_t mod_start;
  multiboot_uint32_t mod_end;
  char cmdline[0];
};

struct multiboot_tag_basic_meminfo
{
  multiboot_uint32_t type;
  multiboot_uint32_t size;
  multiboot_uint32_t mem_lower;
  multiboot_uint32_t mem_upper;
};

struct multiboot_tag_bootdev
{
  multiboot_uint32_t type;
  multiboot_uint32_t size;
  multiboot_uint32_t biosdev;
  multiboot_uint32_t slice;
  multiboot_uint32_t part;
};

struct multiboot_tag_mmap
{
  multiboot_uint32_t type;
  multiboot_uint32_t size;
  multiboot_uint32_t entry_size;
  multiboot_uint32_t entry_version;
  struct multiboot_mmap_entry entries[0];
};

struct multiboot_vbe_info_block
{
  multiboot_uint8_t external_specification[512];
};

struct multiboot_vbe_mode_info_block
{
  multiboot_uint8_t external_specification[256];
};

struct multiboot_tag_vbe
{
  multiboot_uint32_t type;
  multiboot_uint32_t size;

  multiboot_uint16_t vbe_mode;
  multiboot_uint16_t vbe_interface_seg;
  multiboot_uint16_t vbe_interface_off;
  multiboot_uint16_t vbe_interface_len;

  struct multiboot_vbe_info_block vbe_control_info;
  struct multiboot_vbe_mode_info_block vbe_mode_info;
};

struct multiboot_tag_framebuffer_common
{
  multiboot_uint32_t type;
  multiboot_uint32_t size;

  multiboot_uint64_t framebuffer_addr;
  multiboot_uint32_t framebuffer_pitch;
  multiboot_uint32_t framebuffer_width;
  multiboot_uint32_t framebuffer_height;
  multiboot_uint8_t framebuffer_bpp;
#define MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED 0
#define MULTIBOOT_FRAMEBUFFER_TYPE_RGB     1
#define MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT	2
  multiboot_uint8_t framebuffer_type;
  multiboot_uint16_t reserved;
};

struct multiboot_tag_framebuffer
{
  struct multiboot_tag_framebuffer_common common;

  union
  {
    struct
    {
      multiboot_uint16_t framebuffer_palette_num_colors;
      struct multiboot_color framebuffer_palette[0];
    };
    struct
    {
      multiboot_uint8_t framebuffer_red_field_position;
      multiboot_uint8_t framebuffer_red_mask_size;
      multiboot_uint8_t framebuffer_green_field_position;
      multiboot_uint8_t framebuffer_green_mask_size;
      multiboot_uint8_t framebuffer_blue_field_position;
      multiboot_uint8_t framebuffer_blue_mask_size;
    };
  };
};

struct multiboot_tag_elf_sections
{
  multiboot_uint32_t type;
  multiboot_uint32_t size;
  multiboot_uint32_t num;
  multiboot_uint32_t entsize;
  multiboot_uint32_t shndx;
  char sections[0];
};

struct multiboot_tag_apm
{
  multiboot_uint32_t type;
  multiboot_uint32_t size;
  multiboot_uint16_t version;
  multiboot_uint16_t cseg;
  multiboot_uint32_t offset;
  multiboot_uint16_t cseg_16;
  multiboot_uint16_t dseg;
  multiboot_uint16_t flags;
  multiboot_uint16_t cseg_len;
  multiboot_uint16_t cseg_16_len;
  multiboot_uint16_t dseg_len;
};

struct multiboot_tag_efi32
{
  multiboot_uint32_t type;
  multiboot_uint32_t size;
  multiboot_uint32_t pointer;
};

struct multiboot_tag_efi64
{
  multiboot_uint32_t type;
  multiboot_uint32_t size;
  multiboot_uint64_t pointer;
};

struct multiboot_tag_smbios
{
  multiboot_uint32_t type;
  multiboot_uint32_t size;
  multiboot_uint8_t major;
  multiboot_uint8_t minor;
  multiboot_uint8_t reserved[6];
  multiboot_uint8_t tables[0];
};

struct multiboot_tag_old_acpi
{
  multiboot_uint32_t type;
  multiboot_uint32_t size;
  multiboot_uint8_t rsdp[0];
};

struct multiboot_tag_new_acpi
{
  multiboot_uint32_t type;
  multiboot_uint32_t size;
  multiboot_uint8_t rsdp[0];
};

struct multiboot_tag_network
{
  multiboot_uint32_t type;
  multiboot_uint32_t size;
  multiboot_uint8_t dhcpack[0];
};

#endif /* ! ASM_FILE */

#endif /* ! MULTIBOOT_HEADER */


/**********************************************************************
 * multiboot2 loader based on multiboot.c                             *
 * Copyright 2011 Goswin von Brederlow <goswin-v-b@web.de>            *
 **********************************************************************/

typedef struct {
    /* buffers holding kernel and boot info tags */
    void *mb_buf;
    void *mb_tags;
    /* address in target */
    hwaddr mb_buf_phys;
    /* size of mb_buf in bytes */
    unsigned mb_buf_size;
    /* size of tags in bytes */
    unsigned mb_tags_size;
    /* modules offset */
    unsigned offset_mods;
    /* number of modules */
    unsigned mb_mods_avail;
} MultibootState;

static void mb_add_cmdline(MultibootState *s, const char *cmdline)
{
    int len = strlen(cmdline) + 1;
    struct multiboot_tag_string *tag;
    unsigned new_size = s->mb_tags_size;

    mb_debug("mb_add_cmdline: len = %d '%s'\n", len, cmdline);

    new_size += sizeof(struct multiboot_tag_string) + len;
    new_size = (new_size + 7) & ~7;

    /* allocate space for cmdline tag */
    s->mb_tags = g_realloc(s->mb_tags, new_size);
    tag = (struct multiboot_tag_string *)(s->mb_tags + s->mb_tags_size);
    s->mb_tags_size = new_size;

    /* Fill tag */
    tag->type = MULTIBOOT_TAG_TYPE_CMDLINE;
    tag->size = sizeof(struct multiboot_tag_string) + len;
    memcpy(tag->string, cmdline, len);
}

static void mb_add_basic_meminfo(MultibootState *s, uint32_t mem_lower, uint32_t mem_upper)
{
    struct multiboot_tag_basic_meminfo *tag;
    unsigned new_size = s->mb_tags_size;

    new_size += sizeof(struct multiboot_tag_basic_meminfo);
    new_size = (new_size + 7) & ~7;

    /* allocate space for basic_meminfo tag */
    s->mb_tags = g_realloc(s->mb_tags, new_size);
    tag = (struct multiboot_tag_basic_meminfo *)(s->mb_tags + s->mb_tags_size);
    s->mb_tags_size = new_size;

    /* Fill tag */
    tag->type = MULTIBOOT_TAG_TYPE_BASIC_MEMINFO;
    tag->size = sizeof(struct multiboot_tag_basic_meminfo);
    tag->mem_lower = mem_lower;
    tag->mem_upper = mem_upper;
}

static void mb_add_mod(MultibootState *s, hwaddr start, hwaddr end, char *cmdline) {
    struct multiboot_tag_module *tag;
    unsigned new_size = s->mb_tags_size;
    
    unsigned cmdline_len = cmdline ? strlen(cmdline) + 1 : 0;
    new_size += sizeof(struct multiboot_tag_module) + cmdline_len;
    new_size = (new_size + 7) & ~7;

    s->mb_tags = g_realloc(s->mb_tags, new_size);
    tag = (struct multiboot_tag_module *) (s->mb_tags + s->mb_tags_size);
    s->mb_tags_size = new_size;

    tag->type = MULTIBOOT_TAG_TYPE_MODULE;
    tag->size = sizeof(struct multiboot_tag_module);
    tag->mod_start = start;
    tag->mod_end = end;
    if (cmdline) {
        tag->size += strlen(cmdline) + 1;
        strcpy(tag->cmdline, cmdline);
    }
}

int load_multiboot2(X86MachineState *x86ms,
                   FWCfgState *fw_cfg,
                   FILE *f,
                   const char *kernel_filename,
                   const char *initrd_filename,
                   const char *kernel_cmdline,
                   int kernel_file_size,
                   uint8_t *header)
{
    MultibootState mbs;
    int i, is_multiboot = 0, align_modules = 0;
    uint32_t architecture = 0;
    uint32_t header_length = 0;
    struct multiboot_header_tag *current_tag, *last_tag;
    uint32_t mh_entry_addr;
    uint32_t mh_load_addr;
    uint32_t mb_kernel_size;
    uint32_t ram_size;
    uint32_t cmdline_len;
    GList *mods = NULL;

    /* Ok, let's see if it is a multiboot image.
       The header is in the first 32k. */
    for (i = 0; (i < 32768 - 15) && (i < kernel_file_size - 15) ; i += 4) {
        if (ldl_p(header+i) == MULTIBOOT2_HEADER_MAGIC) {
            uint32_t checksum = ldl_p(header+i+12);
            architecture = ldl_p(header+i+4);
            header_length = ldl_p(header+i+8);
            checksum += MULTIBOOT2_HEADER_MAGIC;
            checksum += architecture;
            checksum += header_length;
            if (!checksum) {
                is_multiboot = 1;
                break;
            }
        }
    }

    if (!is_multiboot)
        return 0; /* no multiboot */

    switch(architecture) {
        case MULTIBOOT_ARCHITECTURE_I386:
            // Start in 32bit mode
            mb_debug("qemu: architecture i386\n");
            fw_cfg_add_i32(fw_cfg, FW_CFG_KERNEL_64BIT, 0);
            break;
        case MULTIBOOT_ARCHITECTURE_X86_64:
            // Start in 64bit mode
            mb_debug("qemu: architecture x86_64\n");
            fw_cfg_add_i32(fw_cfg, FW_CFG_KERNEL_64BIT, 1);
            break;
        default:
            fprintf(stderr, "qemu: multiboot2 architecture must be i386 or x86_64.\n");
            exit(1);
    }

    mb_debug("qemu: I believe we found a multiboot2 image!\n");
    mb_debug("header_length = %#x\n", header_length);

    /* Zero out multiboot infos */
    memset(&mbs, 0, sizeof(mbs));

    current_tag = (struct multiboot_header_tag *)(header + i + sizeof(struct multiboot_header));
    last_tag = (struct multiboot_header_tag *)(header + i + header_length);
    for(; current_tag != last_tag; current_tag = (struct multiboot_header_tag *)((uint8_t*)current_tag + current_tag->size)) {
        switch (current_tag->type) {
            case MULTIBOOT_HEADER_TAG_END:
                // Ignore ending tag.
                break;
            case MULTIBOOT_HEADER_TAG_MODULE_ALIGN:
                align_modules = 1;
                break;
            default:
                mb_debug("multiboot2 loader does not support tags of type %u, yet.\n", current_tag->type);
                if (!(current_tag->flags & MULTIBOOT_HEADER_TAG_OPTIONAL)) {
                    fprintf(stderr, "qemu: unsupported multiboot2 tag is not optional.\n");
                    exit(1);
                }
                break;
        }
    }

    /* Add size field to multiboot info */
    mbs.mb_tags = g_malloc(8);
    mbs.mb_tags_size = 8;

    /* Commandline support */
    cmdline_len = strlen(kernel_filename) + 1;
    cmdline_len += strlen(kernel_cmdline) + 1;
    char *kcmdline = g_malloc(cmdline_len);
    snprintf(kcmdline, cmdline_len, "%s %s", kernel_filename, kernel_cmdline);
    if (initrd_filename) {
        char *lastchar;
        const char *r = initrd_filename;
        uint32_t initrd_len = strlen(initrd_filename) + 1;
        kcmdline = g_realloc(kcmdline, cmdline_len + initrd_len);
        memset(kcmdline + cmdline_len, 0, initrd_len);
        lastchar = &kcmdline[cmdline_len-1];
        while (*r) {
            char *value;
            r = get_opt_value(r, &value);
            *lastchar = ' ';
            lastchar = stpcpy(kcmdline + cmdline_len, value);
            cmdline_len += strlen(value);
            mbs.mb_mods_avail++;
            mods = g_list_append(mods, value);
            if (*r) {
                r++;
            }
        }
    }
    mb_add_cmdline(&mbs, kcmdline);
    
    /* Basic memory info */
    ram_size = x86ms->below_4g_mem_size + x86ms->above_4g_mem_size;
    mb_add_basic_meminfo(&mbs, 640, (ram_size / 1024) - 1024);

    /* Load kernel */
    /* FIXME: only elf support for now */
    {
        uint64_t elf_entry;
        uint64_t elf_low, elf_high;
        int kernel_size;
        fclose(f);

        if (((struct elf64_hdr*)header)->e_machine == EM_X86_64) {
            mb_debug("qemu: 64bit elf, I hope you know what you are doing\n");
        }

        kernel_size = load_elf(kernel_filename, NULL, NULL, NULL, &elf_entry,
                               &elf_low, &elf_high, NULL, 0, I386_ELF_MACHINE,
                               0, 0);
        if (kernel_size < 0) {
            fprintf(stderr, "Error while loading elf kernel\n");
            exit(1);
        }
        mh_load_addr = elf_low;
        mb_kernel_size = elf_high - elf_low;
        mh_entry_addr = elf_entry;

        mbs.mb_buf = g_malloc(mb_kernel_size);
        if (rom_copy(mbs.mb_buf, mh_load_addr, mb_kernel_size) != mb_kernel_size) {
            fprintf(stderr, "Error while fetching elf kernel from rom\n");
            exit(1);
        }

        mb_debug("qemu: loading multiboot-elf kernel (%#x bytes) with entry %#zx\n",
                  mb_kernel_size, (size_t)mh_entry_addr);
    }

    /* Align to next page */
    /* FIXME: load modules */
    mbs.mb_buf_size = TARGET_PAGE_ALIGN(mb_kernel_size);
    if (mods) {
        uint32_t offs;
        GList *tmpl = mods;
        mbs.offset_mods = offs = mb_kernel_size;
        while (tmpl) {
            int mb_mod_length;
            char *next_space, *one_file = tmpl->data;
            /* if a space comes after the module filename, treat everything
               after that as parameters */
            next_space = strchr(one_file, ' ');
            if (next_space) {
                *(next_space++) = '\0';
            }
            mb_debug("multiboot2 loading module: %s\n", one_file);
            mb_mod_length = get_image_size(one_file);
            if (mb_mod_length < 0) {
                error_report("Failed to open file '%s'\n", one_file);
                exit(1);
            }
            if (align_modules) {
                offs = TARGET_PAGE_ALIGN(offs);
            }
            mbs.mb_buf_size = offs + mb_mod_length;
            mbs.mb_buf_size = TARGET_PAGE_ALIGN(mbs.mb_buf_size);
            mbs.mb_buf = g_realloc(mbs.mb_buf, mbs.mb_buf_size);
            if (load_image_size(one_file, (char*)mbs.mb_buf + offs, mb_mod_length) < 0) {
                error_report("Error loading file: '%s', %s", one_file, strerror(errno));
                exit(1);
            }
            mb_add_mod(&mbs, mh_load_addr + offs,
                       mh_load_addr + offs + mb_mod_length, tmpl->data);
            offs += mb_mod_length;
            g_free(tmpl->data);
            tmpl = tmpl->next;
        }
    }

    /* FIXME: add other tags */

    /* The multiboot2 bootrom will add the mmap and end tags. */

    /* Set size of multiboot infos */
    multiboot_uint64_t *size = mbs.mb_tags;
    *size = mbs.mb_tags_size;

    /* Free kcmdline */
    g_free(kcmdline);

    /* Display infos */
    mb_debug("qemu: kernel_entry = %#zx\n", (size_t)mh_entry_addr);
    mb_debug("      kernel_addr  = %#zx\n", (size_t)mh_load_addr);
    mb_debug("      kernel_size  = %#zx\n", (size_t)mbs.mb_buf_size);
    unsigned char *p = mbs.mb_buf;
    for(i = 0; i < 0xff; ++i, ++p) {
        if (i % 16 == 0) mb_debug("\n0x%02x:", i);
        mb_debug(" %02x", *p);
    }
    mb_debug("\n");
    mb_debug("      initrd_addr  = %#zx\n", (size_t)MULTIBOOT_MEM);
    mb_debug("      initrd_size  = %#zx\n", (size_t)mbs.mb_tags_size);
    p = mbs.mb_tags;
    for(i = 0; i < mbs.mb_tags_size; ++i, ++p) {
        if (i % 16 == 0) mb_debug("\n0x%02x:", i);
        mb_debug(" %02x", *p);
    }
    mb_debug("\n");

    /* Add extra space for dynamic tags */
    mbs.mb_tags_size += 4096;
    mbs.mb_tags = g_realloc(mbs.mb_tags, mbs.mb_tags_size);

    /* Pass variables to option rom */
    fw_cfg_add_i32(fw_cfg, FW_CFG_KERNEL_ENTRY, mh_entry_addr);
    fw_cfg_add_i32(fw_cfg, FW_CFG_KERNEL_ADDR, mh_load_addr);
    fw_cfg_add_i32(fw_cfg, FW_CFG_KERNEL_SIZE, mbs.mb_buf_size);
    fw_cfg_add_bytes(fw_cfg, FW_CFG_KERNEL_DATA,
                     mbs.mb_buf, mbs.mb_buf_size);

    fw_cfg_add_i32(fw_cfg, FW_CFG_INITRD_ADDR, MULTIBOOT_MEM);
    fw_cfg_add_i32(fw_cfg, FW_CFG_INITRD_SIZE, mbs.mb_tags_size);
    fw_cfg_add_bytes(fw_cfg, FW_CFG_INITRD_DATA, mbs.mb_tags,
                     mbs.mb_tags_size);

    option_rom[nb_option_roms].name = "multiboot2.bin";
    option_rom[nb_option_roms].bootindex = 0;
    nb_option_roms++;

    return 1; /* yes, we are multiboot */
}
