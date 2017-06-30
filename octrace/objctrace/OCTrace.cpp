//
//  OCTrace.cpp
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
#include <pthread.h>
#include <uuid/uuid.h>



#include <objc/objc.h>
#include <objc/runtime.h>
#include <mach/mach.h>
#include <mach-o/dyld_images.h>
#include <mach/vm_map.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/fat.h>
#include <mach-o/loader.h>
#include <mach-o/dyld.h>

#import "fishhook.h"

#include "OCTrace.h"
#include "OCTraceImage.h"
#include "OCTraceLogger.h"
#include "OCTraceLocalLogger.h"
#include "OCTraceRemoteLogger.h"
#include <vector>
#include <string>

static OCTraceLogger * s_logger = NULL;
static const char * s_skip_image_names[] = {
    "WeChat",
    "demo",
    "IPAPatch",
    NULL,
};


static const char * s_skip_class_names[] = {
    "NewStrategyItem",
    "StrategyInterval",
    NULL,
};


static std::vector<OCTraceImage> s_image_list;


typedef id (*fn_objc_msgSend)(id self, SEL op);

static fn_objc_msgSend s_origin_objc_msgSend = NULL;

static bool __skip_image_addr(intptr_t addr) {
    bool retval = true;
    if (addr != (intptr_t)NULL) {
        for (std::vector<OCTraceImage>::iterator itor = s_image_list.begin();
             itor != s_image_list.end(); itor += 1) {
            OCTraceImage image = *itor;
            if (addr >= image.start_addr && addr <= image.end_addr) {
                retval = false;
                break;
            }
        }
    }
    return retval;
}


static bool __skip_class_name(const char * name) {
    bool retval = false;
    if (name != NULL) {
        for (int i = 0; s_skip_class_names[i] != NULL; i++) {
            if (strcasecmp(name, s_skip_class_names[i]) == 0) {
                retval = true;
            }
        }
    }
    return retval;
}


extern "C"
void * __hook_callback_pre(id self, SEL op, intptr_t arg0, intptr_t arg1) {
    const char* class_name = (char*) object_getClassName( self );
    Class clazz = objc_lookUpClass(class_name);
    IMP imp = method_getImplementation(class_getInstanceMethod(clazz, op));
    if (!imp) {
        imp = method_getImplementation(class_getClassMethod(clazz, op));
    }
    
    if (!__skip_image_addr((intptr_t)imp) && !__skip_class_name(class_name)) {
#if 0
        const char* op_name = (const char*) op;
        op_name = !op_name ? "null" : op_name;

        __uint64_t threadId = 0;
        if (pthread_threadid_np(0, &threadId)) {
            threadId = pthread_mach_thread_np(pthread_self());
        }
        
        fprintf(stderr, "[%ld:%llu] [%s %s] -> \n", (long)getpid(), threadId, class_name, op_name);
#endif
        if (s_logger) {
            s_logger->logBeforeCallee((intptr_t)self, (intptr_t)op);
        }
    }
    return (void *)s_origin_objc_msgSend;
}

extern "C"
void  __hook_callback_post(id self, SEL op) {
//    fprintf(stderr, "post self :%p, op:%s\n", self, (const char *)op);
    if (s_logger) {
        s_logger->logAfterCallee((intptr_t)self, (intptr_t)op);
    }
    return;
}


#if  __LP64__
__attribute__((naked))
static id new_objc_msgSend(id self, SEL op) {
    
    __asm__
    __volatile__ (
                  "stp fp, lr, [sp, #-16]!;\n"
                  "mov fp, sp;\n"

                  // store x10-x13,
                  "sub    sp, sp, #(8*16+14*8);\n"
                  "stp    x0, x1, [sp, #(0*8)];\n"
                  "stp    x2, x3, [sp, #(2*8)];\n"
                  "stp    x4, x5, [sp, #(4*8)];\n"
                  "stp    x6, x7, [sp, #(6*8)];\n"
                  "stp    x8, x9, [sp, #(8*8)];\n"
                  "stp    x10, x11, [sp, #(10*8)];\n"
                  "stp    x12, x13, [sp, #(12*8)];\n"
                  "stp    q0, q1, [sp, #(14*8+0*16)];\n"
                  "stp    q2, q3, [sp, #(14*8+2*16)];\n"
                  "stp    q4, q5, [sp, #(14*8+4*16)];\n"
                  "stp    q6, q7, [sp, #(14*8+6*16)];\n"
                  
                  "ldr x9, [fp];\n"
                  "add x10, fp, #16;\n"
                  
                  "adr lr, Llocal_return;\n"
                  "stp fp, lr, [sp, #-16]!;\n"
                  "mov fp, sp;\n"
                  
                  // reconstruct arguments stack
                  "mov x11, x9;\n"
                  "mov x12, sp;\n"
                  "cmp x10, x11;\n"
                  "bhs Lskip_copy;\n"
                  "Lcopy_stack:\n"
                  "ldr x13, [x11,#-8]!;\n"
                  "str x13, [x12,#-8]!;\n"
                  "cmp x10, x11;\n"
                  "bne Lcopy_stack;\n"
                  "mov sp, x12;\n"
                  "Lskip_copy:\n"
                  
                  "adr lr, Llocal_return;\n"
                  
                  "BL ___hook_callback_pre;\n"
                  "mov x9, x0;\n"
                  
                  "add    x10, fp, #16;\n"
                  "ldp    x0, x1, [x10, #(0*8)];\n"
                  "ldp    x2, x3, [x10, #(2*8)];\n"
                  "ldp    x4, x5, [x10, #(4*8)];\n"
                  "ldp    x6, x7, [x10, #(6*8)];\n"
                  "ldr    x8,     [x10, #(8*8)];\n"
                  "ldp    q0, q1, [x10, #(14*8+0*16)];\n"
                  "ldp    q2, q3, [x10, #(14*8+2*16)];\n"
                  "ldp    q4, q5, [x10, #(14*8+4*16)];\n"
                  "ldp    q6, q7, [x10, #(14*8+6*16)];\n"
                  
                  "BLR    x9;\n"
                  "Llocal_return:\n"
                  
                  // store x0-x8 / q0-q7 after call objc_sendMsg.
                  "add    x9, fp, #16;\n"
                  "ldp    x10, x11, [x9, #(0*8)];\n"
                  "stp    x0, x1, [x9, #(0*8)];\n"
                  "stp    x2, x3, [x9, #(2*8)];\n"
                  "stp    x4, x5, [x9, #(4*8)];\n"
                  "stp    x6, x7, [x9, #(6*8)];\n"
                  "str    x8,     [x9, #(8*8)];\n"
                  "stp    q0, q1, [x9, #(14*8+0*16)];\n"
                  "stp    q2, q3, [x9, #(14*8+2*16)];\n"
                  "stp    q4, q5, [x9, #(14*8+4*16)];\n"
                  "stp    q6, q7, [x9, #(14*8+6*16)];\n"
                  
                  // restore self and op before call __hook_callback_post
                  "mov    x0, x10;\n"
                  "mov    x1, x11;\n"
                  "BL ___hook_callback_post;\n"
                  
                  "mov    sp, fp;\n"
                  "ldp    fp, lr, [sp], #16;\n"
                  
                  // restore x0-x13 and q0-q7 to objc_sendMsg's return values
                  "ldp    x0, x1, [sp, #(0*8)];\n"
                  "ldp    x2, x3, [sp, #(2*8)];\n"
                  "ldp    x4, x5, [sp, #(4*8)];\n"
                  "ldp    x6, x7, [sp, #(6*8)];\n"
                  "ldp    x8, x9, [sp, #(8*8)];\n"
                  "ldp    x10, x11, [sp, #(10*8)];\n"
                  "ldp    x12, x13, [sp, #(12*8)];\n"
                  "ldp    q0, q1, [sp, #(14*8+0*16)];\n"
                  "ldp    q2, q3, [sp, #(14*8+2*16)];\n"
                  "ldp    q4, q5, [sp, #(14*8+4*16)];\n"
                  "ldp    q6, q7, [sp, #(14*8+6*16)];\n"
                  
                  "mov    sp, fp;\n"
                  "ldp    fp, lr, [sp], #16;\n"
                  "ret;\n"
                  );
}

#else

__attribute__((naked))
static id new_objc_msgSend(id self, SEL op) {

}

#endif

int OCTraceInit(OCTraceLogger * logger) {
    
#if 1
    OCTraceImageListInit();
    s_image_list.clear();
    for(int i = 0; s_skip_image_names[i] != NULL; i++) {
        const char * image_name = s_skip_image_names[i];
        OCTraceImage image = OCTraceGetImageWithName(image_name);
        if (image.start_addr != (intptr_t)NULL) {
            s_image_list.insert(s_image_list.end(), image);
        }
    }
    
#endif
    s_logger = new OCTraceLocalLogger();
    //s_logger = logger ;//new OCTraceRemoteLogger();
    
    s_origin_objc_msgSend = (fn_objc_msgSend)dlsym(RTLD_DEFAULT, "objc_msgSend");
    return rebind_symbols((struct rebinding[1]){{"objc_msgSend", (void *)new_objc_msgSend}}, 1);

}

