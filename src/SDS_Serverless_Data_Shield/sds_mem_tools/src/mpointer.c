/**
 * mpointer.c
 * This file defines the MPointer Python type, which is used to point to the memory of a MemView object.
 * @warning This module is intended for debugging purposes only, to verify that memory is cleared properly.
 * Using this in production may lead to security vulnerabilities.
*/

#include "include/mpointer.h"
#include "mpointer_methods.c"

PyTypeObject MPointerType = {
PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "MPointer",
    .tp_basicsize = sizeof(MPointer),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_new = MPointer_new,
    .tp_init = (initproc)MPointer_init,
    .tp_dealloc = (destructor)MPointer_dealloc,
    .tp_traverse = (traverseproc)MPointer_traverse,
    .tp_clear = (inquiry)MPointer_clear,
    .tp_members = MPointer_members,
    .tp_methods = MPointer_methods
};