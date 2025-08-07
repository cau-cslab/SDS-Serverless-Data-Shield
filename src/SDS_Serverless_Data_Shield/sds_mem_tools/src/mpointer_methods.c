#include "include/mpointer.h"

static PyObject* MPointer_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int MPointer_init(MPointer *self, PyObject *args, PyObject *kwds);
static void MPointer_dealloc(MPointer *self);
static int MPointer_traverse(MPointer *self, visitproc visit, void *arg);
static int MPointer_clear(MPointer *self);
static PyObject* MPointer_value(MPointer *self, PyObject* Py_UNUSED(ignored));

static PyMemberDef MPointer_members[] = {
    {NULL}
};

static PyMethodDef MPointer_methods[] = {
    {"value", (PyCFunction)MPointer_value, METH_NOARGS, "Get memory value as string"},
    {NULL}
};

/**
 * @brief Creates a new, uninitialized MPointer object.
 * @details This function serves as the constructor for the MPointer type.
 * @param type The Python type object for MPointer.
 * @param args Unused positional arguments.
 * @param kwds Unused keyword arguments.
 * @return A new, empty MPointer object, or NULL if memory allocation fails.
 */
static PyObject* MPointer_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    MPointer *self = (MPointer *)PyObject_GC_New(MPointer, type);
    if (self != NULL)
    {
        self->pointer = NULL;
        self->size = 0;
        self->owner = NULL;
        PyObject_GC_Track(self);
    }
    return (PyObject *)self;
}

/**
 * @brief Initializes an MPointer object.
 * @details This function is called after a new MPointer object is created.
 * @param self A pointer to the MPointer object to initialize.
 * @param args Unused positional arguments.
 * @param kwds Unused keyword arguments.
 * @return 0 on successful initialization, -1 on failure.
 */
static int MPointer_init(MPointer *self, PyObject *args, PyObject *kwds)
{
    self->pointer = NULL;
    self->size = 0;
    self->owner = NULL;
    return 0;
}

/**
 * @brief Deallocates an MPointer object.
 * @details This function is the destructor for the MPointer type. It decrements the
 * reference count of the owner object.
 * @param self A pointer to the MPointer object to be deallocated.
 */
static void MPointer_dealloc(MPointer *self)
{
    PyObject_GC_UnTrack(self);

    if (self->pointer != NULL)
    {
        self->pointer = NULL;
        self->size = 0;
    }

    if (self->owner != NULL)
    {
        Py_CLEAR(self->owner);
    }

    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int
MPointer_traverse(MPointer *self, visitproc visit, void *arg)
{
    Py_VISIT(self->owner);
    return 0;
}

static int
MPointer_clear(MPointer *self)
{
    Py_CLEAR(self->owner);
    return 0;
}

/**
 * @brief Retrieves the value of the memory pointed to by the MPointer object.
 * @details This function returns the content of the memory as a new Python
 * unicode string. 
 * @warning This function is intended for debugging purposes only, to verify that memory is cleared properly.
 * Using this in production may lead to security vulnerabilities.
 *
 * @usage
 * val = mpointer.value()
 */
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
