//
//  OCTraceLocalLogger.cpp
//  IPAPatch
//
//  Created by wadahana on 01/06/2017.
//  Copyright © 2017. All rights reserved.
//

#include "OCTraceLocalLogger.h"
#include "OCTraceUtils.h"

OCTraceLocalLogger::OCTraceLocalLogger() {

}

OCTraceLocalLogger::~OCTraceLocalLogger() {

}

void OCTraceLocalLogger::trace(OCTraceLoggerCallee & callee) {
//    fprintf(stderr, "trace [%llu:%llu] [%s %s] -> depth(%d)\n",
//            callee.m_process_id, callee.m_thread_id,
//            callee.m_class_name.c_str(), callee.m_op_name.c_str(), callee.m_depth);
    
    std::string line = string_sprintf("octrace [%llu:%llu] |", callee.m_process_id, callee.m_thread_id);
    for (int i = 0; i < callee.m_depth; i++) {
        line.append("- ");
    }
    string_appendf(&line, "- [%s %s]", callee.m_class_name.c_str(), callee.m_op_name.c_str());

    fprintf(stderr, "%s\n", line.c_str());
    fflush(stderr);
}
