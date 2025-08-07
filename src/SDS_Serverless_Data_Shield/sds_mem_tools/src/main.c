/*
 * main.c
 *
 * This file serves as the entry point for the 'sdsmemtools' Python extension module.
 * It defines the module structure and initializes the MemView custom type, making it
 * accessible from Python.
 */

#include "Python.h"
#include "include/memview.h"
#include "include/mpointer.h"

static PyModuleDef sdsmemtools_module = {
    PyModuleDef_HEAD_INIT,
    "sdsmemtools",
    "",
    -1,
    NULL,NULL,NULL,NULL,NULL
};

PyMODINIT_FUNC
PyInit_sdsmemtools(void)
{
    PyObject *module;

    if (PyType_Ready(&MemViewType) < 0)
        return NULL;

    if (PyType_Ready(&MPointerType) < 0)
        return NULL;

    module = PyModule_Create(&sdsmemtools_module);
    if (module == NULL)
        return NULL;

    Py_INCREF(&MemViewType);
    if (PyModule_AddObject(module, "MemView", (PyObject *)&MemViewType) < 0) {
        Py_DECREF(&MemViewType);
        Py_DECREF(module);
        return NULL;
    }

    Py_INCREF(&MPointerType);
    if (PyModule_AddObject(module, "MPointer", (PyObject *)&MPointerType) < 0)
    {
        Py_DECREF(&MPointerType);
        Py_DECREF(module);
        return NULL;
    }

    return module;
}