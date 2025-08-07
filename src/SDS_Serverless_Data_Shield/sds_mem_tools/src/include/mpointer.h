#ifndef MPOINTER_H
#define MPOINTER_H
#include "Python.h"

typedef struct
{
    PyObject_HEAD
    void *pointer;
    size_t size;
    PyObject* owner;
} MPointer;

extern PyTypeObject MPointerType;

#endif //MPOINTER_H
