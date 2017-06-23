//
//  OCTraceLogger.h
//  IPAPatch
//
//  Created by wadahana on 01/06/2017.
//  Copyright © 2017. All rights reserved.
//

#ifndef OCTraceLogger_h
#define OCTraceLogger_h

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

#include <string>
#include <stack>
#include <map>


class OCTraceLoggerCallee {
public:
    OCTraceLoggerCallee() {
        this->m_obj_ptr = NULL;
        this->m_op_ptr = NULL;
        this->m_thread_id = 0;
        this->m_process_id = 0;
        this->m_depth = 0;
    }
    std::string makeKey() {
        char buff[64];
        // 0xaabbccdd
        snprintf(buff, 64, "%16lx_%16lx", this->m_obj_ptr, this->m_op_ptr);
        std::string key = buff;
        return key;
    };
public:
    intptr_t m_obj_ptr;
    intptr_t m_op_ptr;
    __uint64_t m_thread_id;
    __uint64_t m_process_id;
    std::string m_class_name;
    std::string m_op_name;
    int m_depth;
};

typedef std::deque<OCTraceLoggerCallee> OCTraceLoggerQueue;

class OCTraceLoggerStack {

public:
    OCTraceLoggerStack() {
        this->m_current_depth = 0;
    }
    ~OCTraceLoggerStack();
    
    OCTraceLoggerCallee & at(int i) {
        int size = (int)this->m_callee_queue.size();
        return this->m_callee_queue.at(size-i-1);
    };
    
    OCTraceLoggerCallee & top() {
        return this->m_callee_queue.front();
    };
    
    void push(OCTraceLoggerCallee & callee) {
        this->m_callee_queue.push_front(callee);
    };
    
    void pop() {
        this->m_callee_queue.pop_front();
    };
    
    size_t size(){
        return this->m_callee_queue.size();
    };
    
    bool empty(){
        return this->m_callee_queue.empty();
    };
    
    void set_current_depth(int i) {
        this->m_current_depth = i;
    };
    
    int get_current_depth() {
        return this->m_current_depth;
    }
protected:
    int m_current_depth;
    OCTraceLoggerQueue m_callee_queue;
};

typedef std::map<__uint64_t, OCTraceLoggerStack*> OCTraceLoggerMap;
typedef std::pair<__uint64_t, OCTraceLoggerStack*> OCTraceLoggerPair;

class OCTraceLogger {
    
public:
    OCTraceLogger();
    ~OCTraceLogger();
    
  
public:
    
    void logBeforeCallee(intptr_t obj_ptr, intptr_t op_ptr);
    void logAfterCallee(intptr_t obj_ptr, intptr_t op_ptr) ;
    
    OCTraceLoggerCallee makeCallee(intptr_t obj_ptr, intptr_t op_ptr);
    
    const char * getClassName(intptr_t obj_ptr) {
        const char* class_name = (char*) object_getClassName((id)obj_ptr);
        if (!class_name) {
            class_name = "null";
        }
        return class_name;
    };
    const char * getSelectorName(intptr_t op_ptr) {
        const char* op_name = (const char*) op_ptr;
        op_name = !op_name ? "null" : op_name;
        return op_name;
    };
    __uint64_t getCurrentThreadID() {
        __uint64_t threadId = 0;
        if (pthread_threadid_np(0, &threadId)) {
            threadId = pthread_mach_thread_np(pthread_self());
        }
        return threadId;
    };
    __uint64_t getProcessID() {
        return (__uint64_t)getpid();
    };
    
protected:
    
    virtual void trace(OCTraceLoggerCallee & callee) = 0;
    
    pthread_mutex_t m_map_mutex;
    OCTraceLoggerMap m_thread_map;
};
#endif /* OCTraceLogger_h */
