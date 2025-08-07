/*
 * mem_tools.h
 *
 * This header file defines the core data structure for MemView, a Python extension type
 * designed for secure and efficient memory management. It provides a low-level memory view
 * that can be used to safely handle sensitive data within Python applications.
 */

#ifndef MEM_TOOLS_H
#define MEM_TOOLS_H
#include <stdbool.h>

#include "Python.h"

typedef enum _memtype
{
    STR_MEM_TYPE
} memview_type;

typedef struct
{
    PyObject_HEAD
    memview_type type;
    void* data;
    size_t size;
    bool _retain_memory;
} MemView;

extern PyTypeObject MemViewType;

#endif //MEM_TOOLS_H
