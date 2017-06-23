//
//  OCTraceLogger.cpp
//  IPAPatch
//
//  Created by wadahana on 01/06/2017.
//  Copyright © 2017. All rights reserved.
//

#include "OCTraceLogger.h"

OCTraceLogger::OCTraceLogger() {
    pthread_mutex_init(&this->m_map_mutex, NULL);
}

OCTraceLogger::~OCTraceLogger() {
    pthread_mutex_destroy(&this->m_map_mutex);
}

OCTraceLoggerCallee OCTraceLogger::makeCallee(intptr_t obj_ptr, intptr_t op_ptr) {
    OCTraceLoggerCallee callee;
    callee.m_obj_ptr = obj_ptr;
    callee.m_op_ptr = op_ptr;
    callee.m_thread_id = getCurrentThreadID();
    callee.m_process_id = getProcessID();
    callee.m_class_name = this->getClassName(obj_ptr);
    callee.m_op_name = this->getSelectorName(op_ptr);
    return callee;
}

void OCTraceLogger::logBeforeCallee(intptr_t obj_ptr, intptr_t op_ptr) {
    OCTraceLoggerCallee callee = this->makeCallee(obj_ptr, op_ptr);
    if (callee.m_thread_id) {

        pthread_mutex_lock(&this->m_map_mutex);
        OCTraceLoggerMap::iterator itor = this->m_thread_map.find(callee.m_thread_id);
        if (itor == this->m_thread_map.end()) {
            OCTraceLoggerStack * stack = new OCTraceLoggerStack();
            callee.m_depth = (int)stack->size();
            stack->push(callee);
            this->m_thread_map.insert(OCTraceLoggerPair(callee.m_thread_id, stack));
//            fprintf(stderr, "before 0 [%llu] %p,%p, class(%s),op(%s),size(%lu)\n",
//                    callee.m_thread_id, (void*)obj_ptr, (void*)op_ptr,
//                    callee.m_class_name.c_str(), callee.m_op_name.c_str(), stack->size());
        } else {
            OCTraceLoggerStack * stack = itor->second;
            callee.m_depth = (int)stack->size();
            stack->push(callee);
//            fprintf(stderr, "before 1 [%llu] %p,%p, class(%s),op(%s),size(%lu)\n",
//                    callee.m_thread_id, (void*)obj_ptr, (void*)op_ptr,
//                    callee.m_class_name.c_str(), callee.m_op_name.c_str(), stack->size());
        }
        pthread_mutex_unlock(&this->m_map_mutex);
    }
    return;
}

void OCTraceLogger::logAfterCallee(intptr_t obj_ptr, intptr_t op_ptr) {
    __uint64_t thread_id = this->getCurrentThreadID();
    if (thread_id) {
        pthread_mutex_lock(&this->m_map_mutex);
        OCTraceLoggerMap::iterator itor = this->m_thread_map.find(thread_id);
        if (itor != this->m_thread_map.end()) {
            OCTraceLoggerStack * stack = itor->second;
            if (!stack->empty()) {
                OCTraceLoggerCallee callee = stack->top();
                if (callee.m_obj_ptr == obj_ptr && callee.m_op_ptr == op_ptr) {
                    for (int i = stack->get_current_depth(); i < (int)stack->size(); i++) {
                        OCTraceLoggerCallee c = stack->at(i);
                        this->trace(c);
                    }
                    stack->pop();
                    stack->set_current_depth((int)stack->size());
                    
                }
            }
        }
     //   fprintf(stderr, "after [%llu] %p,%p size(%d)\n", thread_id, (void*)obj_ptr, (void*)op_ptr, stack_size);
        pthread_mutex_unlock(&this->m_map_mutex);
    }
    return;
}
