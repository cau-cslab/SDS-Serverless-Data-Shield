/*
 * memview_type.c
 *
 * This file defines the `MemViewType` object, which represents the type definition
 * for the MemView custom Python type. It configures the basic properties of the
 * type, such as its name, size, and flags, and links to the methods that
 * implement its behavior, including creation, initialization, and deallocation.
 */

#include "include/memview.h"
#include "memview_methods.c"

PyTypeObject MemViewType = {
    PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "MemView",
        .tp_basicsize = sizeof(MemView),
        .tp_itemsize = 0,
        .tp_flags = Py_TPFLAGS_DEFAULT,
        .tp_new = MemView_new,
        .tp_init = (initproc)MemView_init,
        .tp_dealloc = (destructor)MemView_dealloc,
        .tp_members = MemView_members,
        .tp_methods = MemView_methods
    };