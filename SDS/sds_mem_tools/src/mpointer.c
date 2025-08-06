#include "include/mpointer.h"
#include "mpointer_methods.c"

PyTypeObject MPointerType = {
PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "MPointer",
    .tp_basicsize = sizeof(MPointer),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = MPointer_new,
    .tp_init = (initproc)MPointer_init,
    .tp_dealloc = (destructor)MPointer_dealloc,
    .tp_members = MPointer_members,
    .tp_methods = MPointer_methods
};