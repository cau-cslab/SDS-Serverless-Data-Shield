#include "include/mpointer.h"

static PyObject* MPointer_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int MPointer_init(MPointer *self, PyObject *args, PyObject *kwds);
static void MPointer_dealloc(MPointer *self);
static PyObject* MPointer_value(MPointer *self, PyObject* Py_UNUSED(ignored));

static PyMemberDef MPointer_members[] = {
    {NULL}
};

static PyMethodDef MPointer_methods[] = {
    {"value", (PyCFunction)MPointer_value, METH_NOARGS, "Get memory value as string"},
    {NULL}
};

static PyObject* MPointer_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    MPointer *self = (MPointer *)type->tp_alloc(type, 0);
    if (self != NULL)
    {
        self->pointer = NULL;
        self->size = 0;
        self->owner = NULL;
    }
    return (PyObject *)self;
}

static int MPointer_init(MPointer *self, PyObject *args, PyObject *kwds)
{
    self->pointer = NULL;
    self->size = 0;
    self->owner = NULL;
    return 0;
}

static void MPointer_dealloc(MPointer *self)
{
    if (self->pointer != NULL)
    {
        self->pointer = NULL;
        self->size = 0;
    }

    if (self->owner != NULL)
        Py_XDECREF(self->owner);
}

static PyObject* MPointer_value(MPointer *self, PyObject* Py_UNUSED(ignored))
{
    if (self->pointer == NULL)
    {
        PyErr_NoMemory();
        return NULL;
    }

    if (self->size > (size_t) PY_SSIZE_T_MAX)
    {
        PyErr_SetString(PyExc_ValueError, "Memory size is too large");
        return NULL;
    }

    PyObject* str = PyUnicode_FromStringAndSize(self->pointer, (Py_ssize_t) self->size);
    if (str == NULL)
    {
        PyErr_SetString(PyExc_MemoryError, "Cannot convert to string");
        return NULL;
    }

    return str;
}
