/*
 * memview_methods.c
 *
 * This file implements the methods for the MemView Python type.
 * It includes functions for creating, initializing, deallocating,
 * and manipulating memory views. The methods defined here are
 * designed to safely handle sensitive data within Python applications.
 */

#include "include/mem_tools.h"

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
    {NULL} 
};

#define MEMVIEWTYPE_CHECK(object) \
    (Py_TYPE(object) == (&MemViewType))

/**
 * @brief Creates a new MemView object.
 *
 * This function is the constructor for the MemView type. It allocates memory for a new
 * MemView object and initializes its data and size to NULL and 0, respectively.
 *
 * @param type The Python type object for MemView.
 * @param args Unused.
 * @param kwds Unused.
 * @return A new MemView object, or NULL on failure.
 */
static PyObject*
MemView_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    MemView *self = (MemView *)type->tp_alloc(type, 0);
    if (self != NULL)
    {
        self->data = NULL;
        self->size = 0;
    }
    return (PyObject *)self;
}

/**
 * @brief Initializes a MemView object.
 *
 * This function initializes a MemView object with a value provided as a Python object.
 * It parses the input arguments and calls MemView_assign to set the initial value.
 *
 * @param self A pointer to the MemView object.
 * @param args The positional arguments.
 * @param kwargs The keyword arguments.
 * @return 0 on success, -1 on failure.
 */
static int
MemView_init(MemView* self, PyObject* args, PyObject* kwargs)
{
    PyObject* input = NULL;
    static char *kwlist[] = {"value", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &input))
        return -1;

    if (self->data != NULL)
        MemView_dealloc(self);

    MemView_assign(self, input);

    return 0;
}

/**
 * @brief Deallocates a MemView object.
 *
 * This function is the destructor for the MemView type. It frees the memory
 * allocated for the data and sets the data pointer to NULL and size to 0.
 *
 * @param self A pointer to the MemView object.
 */
static void
MemView_dealloc(MemView* self)
{
    if (self->data != NULL)
    {
        PyMem_RawFree(self->data);
        self->data = NULL;
        self->size = 0;
    }
}

/**
 * @brief Assigns a new value to a MemView object.
 *
 * This function assigns a new value to the MemView object from a Python unicode string.
 * It handles memory allocation and reallocation as needed.
 *
 * @param self A pointer to the MemView object.
 * @param other The Python object to assign from (must be a unicode string).
 * @return Py_None on success, NULL on failure.
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
 * @brief Clears the memory content of a MemView object.
 *
 * This function securely clears the memory held by the MemView object by setting it to zero.
 *
 * @param self A pointer to the MemView object.
 * @param ignored Unused.
 * @return Py_None on success.
 */
static PyObject*
MemView_clear(MemView* self, PyObject* Py_UNUSED(ignored))
{
    if (self->data != NULL && self->size > 0)
        memset(self->data, 0, self->size);
    Py_RETURN_NONE;
}

/**
 * @brief Returns the value of a MemView object as a Python string.
 *
 * This function returns the content of the MemView object as a new Python unicode string.
 *
 * @param self A pointer to the MemView object.
 * @param ignored Unused.
 * @return A new Python unicode string object on success, NULL on failure.
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
 * @brief Performs a bitwise XOR operation.
 *
 * This function performs a bitwise XOR operation between two MemView objects.
 *
 * @param self A pointer to the first MemView object.
 * @param other_obj A pointer to the second MemView object.
 * @return A new MemView object containing the result of the XOR operation, or NULL on failure.
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
 * @brief Performs a bitwise left shift.
 *
 * This function performs a bitwise left shift on the MemView object's data.
 *
 * @param self A pointer to the MemView object.
 * @param args The number of bits to shift by.
 * @return A new MemView object with the shifted data, or NULL on failure.
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

    // if Total bits are larger than total size, fill all zero
    size_t total_bits = self->size * 8;
    if ((size_t)shift >= total_bits)
    {
        MemView* result = PyObject_New(MemView, &MemViewType);
        if (result == NULL)
        {
            PyErr_NoMemory();
            return NULL;
        }
        result->size = self->size;
        result->data = PyMem_RawCalloc(self->size, 1);
        result->type = STR_MEM_TYPE;
        if (result->data == NULL)
        {
            Py_DECREF(result);
            PyErr_NoMemory();
            return NULL;
        }
        return (PyObject*)result;
    }

    MemView* result = PyObject_New(MemView, &MemViewType);
    if (result == NULL)
    {
        PyErr_NoMemory();
        return NULL;
    }
    result->size = self->size;
    result->data = PyMem_RawCalloc(self->size, 1);
    result->type = STR_MEM_TYPE;
    if (result->data == NULL) {
        Py_DECREF(result);
        PyErr_NoMemory();
        return NULL;
    }

    // Shift
    const unsigned char* src = (const unsigned char*)self->data;
    unsigned char* dst = (unsigned char*)result->data;

    for (size_t i = 0; i < total_bits - shift; ++i) {
        size_t src_idx = i + shift;
        size_t src_byte = src_idx >> 3;
        size_t src_bit = 7 - (src_idx & 7);
        int bit_val = (src[src_byte] >> src_bit) & 1;

        size_t dst_byte = i >> 3;
        size_t dst_bit = 7 - (i & 7);

        if (bit_val)
            dst[dst_byte] |= (1u << dst_bit);
    }

    return (PyObject*)result;
}

/**
 * @brief Concatenates two MemView objects.
 *
 * This function concatenates the data of two MemView objects.
 *
 * @param self A pointer to the first MemView object.
 * @param other_obj A pointer to the second MemView object.
 * @return A new MemView object containing the concatenated data, or NULL on failure.
 */
static PyObject*
MemView_concat(MemView* self, PyObject* other_obj)
{
    if (!MEMVIEWTYPE_CHECK(other_obj))
    {
        PyErr_SetString(PyExc_TypeError, "Only MemView type available");
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
    if (result->data == NULL)
    {
        Py_DECREF(result);
        PyErr_NoMemory();
        return NULL;
    }
    memcpy(result->data, other->data, self->size);
    memcpy(result->data+self->size, other->data, (size_t) other->size);
    return (PyObject*) result;
}

/**
 * @brief Slices the data of a MemView object.
 *
 * This function extracts a slice of the data from a MemView object.
 *
 * @param self A pointer to the MemView object.
 * @param args The start and end positions for the slice.
 * @return A new MemView object containing the sliced data, or NULL on failure.
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
        Py_DECREF(result);
        PyErr_NoMemory();
        return NULL;
    }
    result->size = out_bytes;
    result->type = STR_MEM_TYPE;

    // Copy bit
    const unsigned char *src = (const unsigned char*) self->data + origin;;
    unsigned char *dst = (unsigned char*) result->data;
    for (size_t i = 0; i < out_bytes; i++)
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
 * @brief Returns the byte size of the MemView object's data.
 *
 * @param self A pointer to the MemView object.
 * @param ignored Unused.
 * @return A Python integer representing the size of the data in bytes.
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