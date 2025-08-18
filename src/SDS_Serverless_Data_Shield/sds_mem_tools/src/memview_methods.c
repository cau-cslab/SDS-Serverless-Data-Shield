/**
 * @file memview_methods.c
 * @brief This file implements the methods for the MemView Python type.
 *
 * It includes functions for creating, initializing, deallocating,
 * and manipulating memory views. The methods defined here are
 * designed to safely handle sensitive data within Python applications.
 */

#include "include/memview.h"
#include "include/mpointer.h"

static PyObject* MemView_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int MemView_init(MemView* self, PyObject* args, PyObject* kwargs);
static void MemView_dealloc(MemView* self);
static PyObject* MemView_assign(MemView* self, PyObject* other);
static PyObject* MemView_clear(MemView* self, PyObject* Py_UNUSED(ignored));
static PyObject* MemView_value(MemView* self, PyObject* Py_UNUSED(ignored));
static PyObject* MemView_xor(MemView *self, PyObject *other_obj);
static PyObject* MemView_lshift(MemView *self, PyObject* args, PyObject* kwargs);
static PyObject* MemView_concat(MemView* self, PyObject* other_obj);
static PyObject* MemView_slicing(MemView *self, PyObject* args, PyObject* kwargs);
static PyObject* MemView_bsize(MemView* self, PyObject* Py_UNUSED(ignored));
static PyObject* MemView_badd(MemView* self, PyObject* other_obj);
static PyObject* MemView_pointer(MemView* self, PyObject* Py_UNUSED(ignored));

static PyMemberDef MemView_members[] = {
    {NULL}
};

static PyMethodDef MemView_methods[] = {
    {"assign", (PyCFunction)MemView_assign, METH_VARARGS | METH_KEYWORDS, "Assign a new value"},
    {"clear", (PyCFunction)MemView_clear, METH_NOARGS, "Clear the memory content."},
    {"value", (PyCFunction)MemView_value, METH_NOARGS, "Return the value as bytes."},
    {"xor", (PyCFunction)MemView_xor, METH_O, "Perform XOR operation with another MemView object."},
    {"lshift", (PyCFunction)MemView_lshift, METH_VARARGS | METH_KEYWORDS, "Bitwise left shift."},
    {"concat", (PyCFunction)MemView_concat, METH_O, "Concatenate with another MemView object."},
    {"slicing", (PyCFunction)MemView_slicing, METH_VARARGS | METH_KEYWORDS, "Slice bits from the origin."},
    {"bsize", (PyCFunction)MemView_bsize, METH_NOARGS, "Return the byte size of the memory."},
    {"badd", (PyCFunction)MemView_badd, METH_O, "Perform byte-wise addition with another MemView object"},
    {"pointer", (PyCFunction)MemView_pointer, METH_NOARGS, "Get pointer"},
    {NULL} 
};

#define MEMVIEWTYPE_CHECK(object) \
    (Py_TYPE(object) == (&MemViewType))

/**
 * @brief Creates a new, uninitialized MemView object.
 * @details This function serves as the constructor for the MemView type. It allocates
 * memory for a new MemView object and initializes its internal data pointer to NULL
 * and size to 0. It is called by the Python interpreter when a new instance of
 * MemView is created.
 * @param type The Python type object for MemView.
 * @param args Unused positional arguments.
 * @param kwds Unused keyword arguments.
 * @return A new, empty MemView object, or NULL if memory allocation fails.
 */
static PyObject*
MemView_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    MemView *self = (MemView *)type->tp_alloc(type, 0);
    if (self != NULL)
    {
        self->data = NULL;
        self->size = 0;
        self->_retain_memory = false;
    }
    return (PyObject *)self;
}

/**
 * @brief Initializes a MemView object with a given value.
 * @details This function sets the initial value of a MemView object. It expects a
 * Python unicode string as input. If the object is already initialized, its
 * previous content is deallocated before assignment.
 * @param self A pointer to the MemView object to initialize.
 * @param args Positional arguments passed during object creation (expects one unicode object).
 * @param kwargs Keyword arguments passed during object creation (expects 'value' keyword).
 * @return 0 on successful initialization, -1 on failure (e.g., type mismatch).
 */
static int
MemView_init(MemView* self, PyObject* args, PyObject* kwargs)
{
    PyObject* value = NULL;
    PyObject* retain_mem = NULL;
    static char *kwlist[] = {"value", "retain_mem", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", kwlist, &value, &retain_mem))
        return -1;

    if (self->data != NULL)
    {
        PyMem_RawFree(self->data);
        self->data = NULL;
        self->size = 0;
    }

    if (retain_mem != NULL)
        self->_retain_memory = PyObject_IsTrue(retain_mem);
    else
        self->_retain_memory = false;

    MemView_assign(self, value);

    return 0;
}

/**
 * @brief Deallocates a MemView object.
 * @details This function is the destructor for the MemView type. It frees the memory
 * allocated for the data and resets the size to 0.
 * @param self A pointer to the MemView object to be deallocated.
 */
static void
MemView_dealloc(MemView* self)
{
    if (self->data != NULL && !self->_retain_memory)
    {
        PyMem_RawFree(self->data);
        self->data = NULL;
        self->size = 0;
    }

    Py_TYPE(self)->tp_free((PyObject *)self);
}

/**
 * @brief Assigns a new value to a MemView object from a Python unicode string.
 * @details This function sets or updates the value of the MemView object. It takes a
 * Python unicode string, converts it to UTF-8, and copies it into the
 * object's internal buffer. It handles memory allocation and reallocation
 * automatically.
 * 
 * @usage
 * mv.assign("new_value")
 */
static PyObject*
MemView_assign(MemView* self, PyObject* other)
{
    if (!PyUnicode_Check(other))
    {
        PyErr_SetString(PyExc_TypeError, "Unsupported type for object value");
        return NULL;
    }

    Py_ssize_t src_size;
    const char* str = PyUnicode_AsUTF8AndSize(other, &src_size);
    if (str == NULL || src_size < 0)
    {
        PyErr_SetString(PyExc_ValueError, "Invalid object value");
        return NULL;
    }

    if (self->size != (size_t) src_size)
    {
        self->size = (size_t) src_size;
        if (self->data == NULL) self->data = PyMem_RawMalloc(self->size);
        else self->data = PyMem_RawRealloc(self->data, self->size);

        if (self->data == NULL)
        {
            PyErr_NoMemory();
            return NULL;
        }
    }
    self->type = STR_MEM_TYPE;
    memcpy(self->data, str, self->size);
    Py_RETURN_NONE;
}

/**
 * @brief Securely clears the memory content of a MemView object.
 * @details This function overwrites the memory buffer of the MemView object with zeros
 * to securely erase its content.
 *
 * @usage
 * mv.clear()
 */
static PyObject*
MemView_clear(MemView* self, PyObject* Py_UNUSED(ignored))
{
    if (self->data != NULL && self->size > 0)
        memset(self->data, 0, self->size);
    Py_RETURN_NONE;
}

/**
 * @brief Retrieves the value of a MemView object as a Python string.
 * @details This function returns the content of the MemView object as a new Python
 * unicode string. It fails if the internal data type is not string-based.
 *
 * @usage
 * val = mv.value()
 */
static PyObject*
MemView_value(MemView* self, PyObject* Py_UNUSED(ignored))
{
    if (self->type != STR_MEM_TYPE)
    {
        PyErr_SetString(PyExc_TypeError, "Only string type available");
        return NULL;
    }

    if (self->size > (size_t) PY_SSIZE_T_MAX)
    {
        PyErr_SetString(PyExc_ValueError, "Memory size is too large");
        return NULL;
    }

    PyObject* str = PyUnicode_FromStringAndSize(self->data, (Py_ssize_t) self->size);
    if (str == NULL)
    {
        PyErr_SetString(PyExc_MemoryError, "Cannot convert to string");
    }

    return str;
}

/******************************************************************************/
/*                             Bit Operation                                  */
/******************************************************************************/

/**
 * @brief Performs a byte-wise XOR operation between two MemView objects.
 * @details This function computes the XOR of two MemView objects of the same size.
 * It returns a new MemView object containing the result.
 *
 * @usage
 * result = mv1.xor(mv2)
 */
static PyObject *
MemView_xor(MemView *self, PyObject *other_obj)
{
    if (!MEMVIEWTYPE_CHECK(other_obj))
    {
        PyErr_SetString(PyExc_TypeError, "Only MemView type available");
        return NULL;
    }
    MemView* other = (MemView*)other_obj;
    if (other->type != STR_MEM_TYPE)
    {
        PyErr_SetString(PyExc_TypeError, "Only string type available");
        return NULL;
    }
    if (other->size != self->size)
    {
        PyErr_SetString(PyExc_ValueError, "Size mismatch");
        return NULL;
    }

    // Generate result object
    MemView* result = PyObject_New(MemView, &MemViewType);
    result->data = PyMem_RawMalloc(self->size);
    result->size = self->size;
    result->type = STR_MEM_TYPE;
    result->_retain_memory = self->_retain_memory && other->_retain_memory;
    if (result->data == NULL)
    {
        Py_DECREF(result);
        PyErr_NoMemory();
        return NULL;
    }

    // XOR operation
    for (int i = 0; i < self->size; i++)
        ((char*) result->data)[i] = ((char*)self->data)[i] ^ ((char*) other->data)[i];

    return (PyObject*) result;
}

/**
 * @brief Performs a bitwise left circular shift (rotation).
 * @details This function performs a bitwise left circular shift on the MemView
 * object's data. The bits shifted out from the left end are wrapped around
 * to the right end.
 *
 * @usage
 * result = mv.lshift(5)
 */
static PyObject *
MemView_lshift(MemView *self, PyObject* args, PyObject* kwargs)
{
    if (self->type != STR_MEM_TYPE)
    {
        PyErr_SetString(PyExc_TypeError, "Only string type available");
        return NULL;
    }

    int shift = 0;
    static char *kwlist[] = {"shift", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i", kwlist, &shift))
    {
        PyErr_SetString(PyExc_TypeError, "origin, offset are required.");
        return NULL;
    }
    if (shift <= 0)
    {
        PyErr_SetString(PyExc_ValueError, "shift must be positive.");
        return NULL;
    }

    const int total_bits = (int)self->size * 8;
    shift = shift % total_bits;

    if (shift == 0)
    {
        MemView* result = PyObject_New(MemView, &MemViewType);
        result->data = PyMem_RawMalloc(self->size);
        if (result->data == NULL)
        {
            Py_DECREF(result);
            PyErr_NoMemory();
            return NULL;
        }
        memcpy(result->data, self->data, self->size);
        result->size = self->size;
        result->type = STR_MEM_TYPE;
        return (PyObject*) result;
    }

    unsigned char* src = (unsigned char*) self->data;
    MemView* result = PyObject_New(MemView, &MemViewType);
    result->data = PyMem_RawMalloc(self->size);
    if (result->data == NULL)
    {
        Py_DECREF(result);
        PyErr_NoMemory();
        return NULL;
    }
    result->size = self->size;
    result->type = STR_MEM_TYPE;
    result->_retain_memory = self->_retain_memory;
    unsigned char* dst = (unsigned char*) result->data;
    memset(dst, 0, self->size);

    for (size_t i = 0; i < total_bits; i++)
    {
        size_t src_bit_pos = (i+shift) % total_bits;
        size_t src_byte_idx = src_bit_pos / 8;
        size_t src_bit_off = 7 - (src_bit_pos % 8);

        int bit_val = (src[src_byte_idx] >> src_bit_off) & 0x01;

        size_t dst_byte_idx = i / 8;
        size_t dst_bit_off = 7 - (i % 8);

        if (bit_val)
            dst[dst_byte_idx] |= (1u << dst_bit_off);
    }

    return (PyObject*)result;
}

/**
 * @brief Concatenates two MemView objects.
 * @details This function creates a new MemView object that is the result of
 * concatenating the data of the current object with another MemView object.
 *
 * @usage
 * result = mv1.concat(mv2)
 */
static PyObject*
MemView_concat(MemView* self, PyObject* other_obj)
{
    if (!MEMVIEWTYPE_CHECK(other_obj))
    {
        PyErr_SetString(PyExc_TypeError, "Only MemView type available");
        return NULL;
    }
    MemView* other = (MemView*)other_obj;
    if (other->type != STR_MEM_TYPE)
    {
        PyErr_SetString(PyExc_TypeError, "Only string type available");
        return NULL;
    }

    MemView* result = PyObject_New(MemView, &MemViewType);
    result->data = PyMem_RawMalloc(self->size+other->size);
    result->size = self->size+other->size;
    result->type = STR_MEM_TYPE;
    result->_retain_memory = self->_retain_memory && other->_retain_memory;
    if (result->data == NULL)
    {
        Py_DECREF(result);
        PyErr_NoMemory();
        return NULL;
    }
    memcpy(result->data, self->data, self->size);
    memcpy(result->data+self->size, other->data, (size_t) other->size);
    return (PyObject*) result;
}

/**
 * @brief Extracts a slice of bits from a MemView object.
 * @details This function extracts a specified number of bits from a given starting
 * position (origin) in the MemView object's data.
 *
 * @usage
 * slice = mv.slicing(8, 16)
 */
static PyObject*
MemView_slicing(MemView *self, PyObject* args, PyObject* kwargs)
{
    if (self->type != STR_MEM_TYPE)
    {
        PyErr_SetString(PyExc_TypeError, "Only string type available");
        return NULL;
    }

    int origin = 0, offset = 0;
    static char *kwlist[] = {"origin", "offset", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii", kwlist, &origin, &offset))
    {
        PyErr_SetString(PyExc_TypeError, "origin, offset are required.");
        return NULL;
    }
    if (origin < 0 || offset <= 0)
    {
        PyErr_SetString(PyExc_ValueError, "origin and offset must be positive.");
        return NULL;
    }
    size_t total_bits = self->size * 8;
    if ((size_t)origin + (size_t)offset > total_bits)
    {
        PyErr_SetString(PyExc_ValueError, "Slicing size is out of range");
        return NULL;
    }

    // Create New MemView
    size_t out_bits = (size_t) offset;
    size_t out_bytes = (size_t) (out_bits + 7) / 8;
    MemView* result = PyObject_New(MemView, &MemViewType);
    if (result == NULL)
    {
        PyErr_NoMemory();
        return NULL;
    }
    result->data = PyMem_RawMalloc(out_bytes);
    if (result->data == NULL)
    {
        PyErr_NoMemory();
        return NULL;
    }
    memset(result->data, 0, out_bytes);
    result->size = out_bytes;
    result->type = STR_MEM_TYPE;
    result->_retain_memory = self->_retain_memory;

    // Copy bit
    const unsigned char *src = (const unsigned char*) self->data;
    unsigned char *dst = (unsigned char*) result->data;
    for (size_t i = 0; i < out_bits; i++)
    {
        size_t src_idx_bit  = (size_t)origin + i;
        size_t src_byte_idx = src_idx_bit >> 3;
        size_t src_bit_off  = 7 - (src_idx_bit & 7);
        int bit_val = (src[src_byte_idx] >> src_bit_off) & 0x01;
        size_t dst_idx_bit  = i;
        size_t dst_byte_idx = dst_idx_bit >> 3;
        size_t dst_bit_off  = 7 - (dst_idx_bit & 7);

        if (bit_val)
            dst[dst_byte_idx] |= (1u << dst_bit_off);
    }

    return (PyObject*) result;
}

/**
 * @brief Returns the size of the MemView object's data in bytes.
 *
 * @usage
 * size = mv.bsize()
 */
static PyObject*
MemView_bsize(MemView* self, PyObject* Py_UNUSED(ignored))
{
    if (self->type != STR_MEM_TYPE)
    {
        PyErr_SetString(PyExc_TypeError, "Only string type available");
        return NULL;
    }
    size_t byte_size = self->size * sizeof(unsigned char);
    return (PyObject*) Py_BuildValue("i", byte_size);
}

/**
 * @brief Performs byte-wise addition of two MemView objects.
 * @details This function adds the byte values of two MemView objects of the same
 * size, handling carry-over between bytes. It simulates addition of two large
 * unsigned integers.
 *
 * @usage
 * result = mv1.badd(mv2)
 */
static PyObject*
    MemView_badd(MemView* self, PyObject* other_obj)
{
    if (self->type != STR_MEM_TYPE)
    {
        PyErr_SetString(PyExc_TypeError, "Only string type available");
        return NULL;
    }
    if (!MEMVIEWTYPE_CHECK(other_obj))
    {
        PyErr_SetString(PyExc_TypeError, "Only MemView type available");
        return NULL;
    }
    MemView* other = (MemView*) other_obj;
    if (other->type != STR_MEM_TYPE)
    {
        PyErr_SetString(PyExc_TypeError, "Only string type available");
        return NULL;
    }
    if (self->size != other->size)
    {
        PyErr_SetString(PyExc_ValueError, "Size mismatch");
        return NULL;
    }

    MemView* result = PyObject_New(MemView, &MemViewType);
    if (result == NULL)
    {
        PyErr_NoMemory();
        return NULL;
    }
    result->data = PyMem_RawMalloc(other->size);
    if (result->data == NULL)
    {
        PyErr_NoMemory();
        return NULL;
    }
    result->size = other->size;
    result->type = STR_MEM_TYPE;
    result->_retain_memory = self->_retain_memory && other->_retain_memory;

    int carry = 0;
    for (int i = (int)self->size-1; i >= 0; i--)
    {
        int sum = ((unsigned char*)self->data)[i] + ((unsigned char*)other->data)[i]+carry;
        ((unsigned char*)result->data)[i] = (unsigned char)(sum & 0xff);
        carry = sum > 0xff ? 1: 0;
    }

    return (PyObject*) result;
}

/**
 * @brief Returns a pointer to the MemView object's data.
 * @details This function is only available when the retain_mem flag is set to True.
 *
 * @usage
 * ptr = mv.pointer()
 */
static PyObject*
    MemView_pointer(MemView* self, PyObject* Py_UNUSED(ignored))
{
    if (!self->_retain_memory)
    {
        PyErr_SetString(PyExc_TypeError, "pointer method only available when retain_mem flag is activated");
        return NULL;
    }

    if (self->type != STR_MEM_TYPE)
    {
        PyErr_SetString(PyExc_TypeError, "Only string type available");
        return NULL;
    }

    MPointer* result = PyObject_GC_New(MPointer, &MPointerType);
    if (result == NULL)
    {
        PyErr_NoMemory();
        return NULL;
    }
    result->pointer = self->data;
    result->size = self->size;
    result->owner = (PyObject*) self;
    Py_INCREF(self);
    PyObject_GC_Track(result);

    return (PyObject*) result;
}