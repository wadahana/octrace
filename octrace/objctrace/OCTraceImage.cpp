//
//  OCTraceImage.cpp
//  IPAPatch
//
//  Created by wadahana on 01/06/2017.
//  Copyright © 2017. All rights reserved.
//




#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dlfcn.h>
#include <assert.h>
#include <string.h>
#include <uuid/uuid.h>


#import <dlfcn.h>
#import <mach-o/loader.h>
#import <mach-o/dyld.h>
#import <objc/objc.h>
#import <objc/runtime.h>

#include "OCTraceImage.h"

#include <string>
#include <map>

static bool s_dump_image_loading = true;

static std::map<std::string, OCTraceImage> s_image_map;

const static OCTraceImage s_null_image;

static const uuid_t *__octrace_image_retrieve_uuid(const struct mach_header *mh);

//static uint64_t __octrace_image_text_segment_size(const struct mach_header *mh);

static uint64_t __octrace_image_total_size(const struct mach_header *mh);

static std::string __octrace_truncate_image_name(const char * image_path);

static void __octrace_print_image(const struct mach_header *mh, bool added);

static void __octrace_add_image_to_list(const struct mach_header *mh);

static void __octrace_remove_image_from_list(const struct mach_header *mh);


static void __octrace_image_added(const struct mach_header *mh, intptr_t slide)
{
    if (s_dump_image_loading) {
        __octrace_print_image(mh, true);
    }
    __octrace_add_image_to_list(mh);
}

static void __octrace_image_removed(const struct mach_header *mh, intptr_t slide)
{
    if (s_dump_image_loading) {
        __octrace_print_image(mh, false);
    }
    __octrace_remove_image_from_list(mh);
}

static std::string __octrace_truncate_image_name(const char * image_path) {
    std::string path = image_path;
    std::string filename;
    size_t index =  path.rfind('/');
    if (index != std::string::npos && index < path.length()) {
        if (index < path.length()) {
            filename = path.substr(index + 1);
        }
    }
    return filename;
}


static void __octrace_add_image_to_list(const struct mach_header *mh) {
    Dl_info image_info;
    
    
    int result = dladdr(mh, &image_info);
    
    if (result == 0) {
        printf("Could not print info for mach_header: %p\n\n", mh);
        return;
    }
    const char *image_name = image_info.dli_fname;
    
    const intptr_t image_base_address = (intptr_t)image_info.dli_fbase;
    const uint64_t image_total_size = __octrace_image_total_size(mh);
    
    char image_uuid[37];
    const uuid_t *image_uuid_bytes = __octrace_image_retrieve_uuid(mh);
    uuid_unparse(*image_uuid_bytes, image_uuid);
    
    std::string filename = __octrace_truncate_image_name(image_name);
    
    OCTraceImage image(image_name,
                        filename.c_str(),
                        image_uuid,
                        image_base_address,
                        (intptr_t)(image_base_address + image_total_size));
    
    s_image_map.insert(std::pair<std::string, OCTraceImage>(filename, image));
    return;
    
}

static void __octrace_remove_image_from_list(const struct mach_header *mh) {
    Dl_info image_info;
    int result = dladdr(mh, &image_info);
    
    if (result == 0) {
        printf("Could not print info for mach_header: %p\n\n", mh);
        return;
    }
    const char *image_name = image_info.dli_fname;
    std::string filename = __octrace_truncate_image_name(image_name);
    
    std::map<std::string, OCTraceImage>::iterator itor = s_image_map.find(filename);
    if (itor != s_image_map.end()) {
        s_image_map.erase(itor);
    }
    return;
}

static void __octrace_print_image(const struct mach_header *mh, bool added)
{
    
    Dl_info image_info;
    int result = dladdr(mh, &image_info);
    
    if (result == 0) {
        printf("Could not print info for mach_header: %p\n\n", mh);
        return;
    }
    
    const char *image_name = image_info.dli_fname;
    
    const intptr_t image_base_address = (intptr_t)image_info.dli_fbase;
    const uint64_t image_total_size = __octrace_image_total_size(mh);
    
    char image_uuid[37];
    const uuid_t *image_uuid_bytes = __octrace_image_retrieve_uuid(mh);
    uuid_unparse(*image_uuid_bytes, image_uuid);
    
    const char *log = added ? "Added" : "Removed";
    printf("%s: 0x%02lx (0x%02llx) %s <%s>\n", log, image_base_address, image_total_size, image_name, image_uuid);
    
    return;
}


static uint32_t __octrace_image_header_size(const struct mach_header *mh)
{
    bool is_header_64_bit = (mh->magic == MH_MAGIC_64 || mh->magic == MH_CIGAM_64);
    return (is_header_64_bit ? sizeof(struct mach_header_64) : sizeof(struct mach_header));
}

static void __octrace_image_visit_load_commands(const struct mach_header *mh, void (^visitor)(struct load_command *lc, bool *stop))
{
    assert(visitor != NULL);
    
    uintptr_t lc_cursor = (uintptr_t)mh + __octrace_image_header_size(mh);
    
    for (uint32_t idx = 0; idx < mh->ncmds; idx++) {
        struct load_command *lc = (struct load_command *)lc_cursor;
        
        bool stop = false;
        visitor(lc, &stop);
        
        if (stop) {
            return;
        }
        
        lc_cursor += lc->cmdsize;
    }
}

static const uuid_t *__octrace_image_retrieve_uuid(const struct mach_header *mh)
{
    __block const struct uuid_command *uuid_cmd = NULL;
    __octrace_image_visit_load_commands(mh, ^ (struct load_command *lc, bool *stop) {
        if (lc->cmdsize == 0) {
            return;
        }
        if (lc->cmd == LC_UUID) {
            uuid_cmd = (const struct uuid_command *)lc;
            *stop = true;
        }
    });
    
    if (uuid_cmd == NULL) {
        return NULL;
    }
    
    return &uuid_cmd->uuid;
}

static uint64_t __octrace_image_total_size(const struct mach_header *mh) {
    
    __block uint64_t total_size = 0;
    
    __octrace_image_visit_load_commands(mh, ^ (struct load_command *lc, bool *stop) {
        if (lc->cmdsize == 0) {
            return;
        }
        if (lc->cmd == LC_SEGMENT) {
            struct segment_command *seg_cmd = (struct segment_command *)lc;
            if (strcmp(seg_cmd->segname, "__PAGEZERO") != 0) {
                total_size += seg_cmd->vmsize;
            }
        }
        if (lc->cmd == LC_SEGMENT_64) {
            struct segment_command_64 *seg_cmd = (struct segment_command_64 *)lc;
            if (strcmp(seg_cmd->segname, "__PAGEZERO") != 0) {
                total_size += seg_cmd->vmsize;
            }
        }
    });
    return total_size;
}

#if 0
static uint64_t __octrace_image_text_segment_size(const struct mach_header *mh)
{
    static const char *text_segment_name = "__TEXT";
    
    __block uint64_t text_size = 0;
    
    __octrace_image_visit_load_commands(mh, ^ (struct load_command *lc, bool *stop) {
        if (lc->cmdsize == 0) {
            return;
        }
        if (lc->cmd == LC_SEGMENT) {
            struct segment_command *seg_cmd = (struct segment_command *)lc;
            if (strcmp(seg_cmd->segname, text_segment_name) == 0) {
                text_size = seg_cmd->vmsize;
                *stop = true;
                return;
            }
        }
        if (lc->cmd == LC_SEGMENT_64) {
            struct segment_command_64 *seg_cmd = (struct segment_command_64 *)lc;
            if (strcmp(seg_cmd->segname, text_segment_name) == 0) {
                text_size = seg_cmd->vmsize;
                *stop = true;
                return;
            }
        }
    });
    
    return text_size;
}
#endif

void OCTraceImageListInit() {
    
    s_image_map.clear();
    
    _dyld_register_func_for_add_image(&__octrace_image_added);
    _dyld_register_func_for_remove_image(&__octrace_image_removed);
    
    return;
}

const OCTraceImage & OCTraceGetImageWithName(const char* name) {
    
    std::string image_name = name;
    std::map<std::string, OCTraceImage>::iterator itor = s_image_map.find(image_name);
    if (itor != s_image_map.end()) {
        return itor->second;
    }
    return s_null_image;
}
