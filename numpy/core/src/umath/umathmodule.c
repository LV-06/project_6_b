/* -*- c -*- */

/*
 * vim:syntax=c
 */

/*
 *****************************************************************************
 **                            INCLUDES                                     **
 *****************************************************************************
 */

/*
 * _UMATHMODULE IS needed in __ufunc_api.h, included from numpy/ufuncobject.h.
 * This is a mess and it would be nice to fix it. It has nothing to do with
 * __ufunc_api.c
 */
#define _UMATHMODULE
#define NPY_NO_DEPRECATED_API NPY_API_VERSION

#include "Python.h"

#include "npy_config.h"
#define PY_ARRAY_UNIQUE_SYMBOL _npy_umathmodule_ARRAY_API

#include "numpy/arrayobject.h"
#include "numpy/ufuncobject.h"
#include "numpy/npy_3kcompat.h"
#include "abstract.h"

#include "numpy/npy_math.h"

/*
 *****************************************************************************
 **                    INCLUDE GENERATED CODE                               **
 *****************************************************************************
 */
#include "funcs.inc"
#include "loops.h"
#include "ufunc_object.h"
#include "ufunc_type_resolution.h"
#include "__umath_generated.c"
#include "__ufunc_api.c"

NPY_NO_EXPORT int initscalarmath(PyObject *);

static PyUFuncGenericFunction pyfunc_functions[] = {PyUFunc_On_Om};

static int
object_ufunc_type_resolver(PyUFuncObject *ufunc,
                                NPY_CASTING casting,
                                PyArrayObject **operands,
                                PyObject *type_tup,
                                PyArray_Descr **out_dtypes)
{
    int i, nop = ufunc->nin + ufunc->nout;

    out_dtypes[0] = PyArray_DescrFromType(NPY_OBJECT);
    if (out_dtypes[0] == NULL) {
        return -1;
    }

    for (i = 1; i < nop; ++i) {
        Py_INCREF(out_dtypes[0]);
        out_dtypes[i] = out_dtypes[0];
    }

    return 0;
}

static int
object_ufunc_loop_selector(PyUFuncObject *ufunc,
                            PyArray_Descr **NPY_UNUSED(dtypes),
                            PyUFuncGenericFunction *out_innerloop,
                            void **out_innerloopdata,
                            int *out_needs_api)
{
    *out_innerloop = ufunc->functions[0];
    *out_innerloopdata = ufunc->data[0];
    *out_needs_api = 1;

    return 0;
}

static PyObject *
ufunc_frompyfunc(PyObject *NPY_UNUSED(dummy), PyObject *args, PyObject *NPY_UNUSED(kwds)) {
    /* Keywords are ignored for now */

    PyObject *function, *pyname = NULL;
    int nin, nout, i, nargs;
    PyUFunc_PyFuncData *fdata;
    PyUFuncObject *self;
    char *fname, *str, *types, *doc;
    Py_ssize_t fname_len = -1;
    void * ptr, **data;
    int offset[2];

    if (!PyArg_ParseTuple(args, "Oii:frompyfunc", &function, &nin, &nout)) {
        return NULL;
    }
    if (!PyCallable_Check(function)) {
        PyErr_SetString(PyExc_TypeError, "function must be callable");
        return NULL;
    }
    nargs = nin + nout;

    pyname = PyObject_GetAttrString(function, "__name__");
    if (pyname) {
        (void) PyString_AsStringAndSize(pyname, &fname, &fname_len);
    }
    if (PyErr_Occurred()) {
        fname = "?";
        fname_len = 1;
        PyErr_Clear();
    }

    /*
     * ptr will be assigned to self->ptr, holds a pointer for enough memory for
     * self->data[0] (fdata)
     * self->data
     * self->name
     * self->types
     *
     * To be safest, all of these need their memory aligned on void * pointers
     * Therefore, we may need to allocate extra space.
     */
    offset[0] = sizeof(PyUFunc_PyFuncData);
    i = (sizeof(PyUFunc_PyFuncData) % sizeof(void *));
    if (i) {
        offset[0] += (sizeof(void *) - i);
    }
    offset[1] = nargs;
    i = (nargs % sizeof(void *));
    if (i) {
        offset[1] += (sizeof(void *)-i);
    }
    ptr = PyArray_malloc(offset[0] + offset[1] + sizeof(void *) +
                            (fname_len + 14));
    if (ptr == NULL) {
        Py_XDECREF(pyname);
        return PyErr_NoMemory();
    }
    fdata = (PyUFunc_PyFuncData *)(ptr);
    fdata->callable = function;
    fdata->nin = nin;
    fdata->nout = nout;

    data = (void **)(((char *)ptr) + offset[0]);
    data[0] = (void *)fdata;
    types = (char *)data + sizeof(void *);
    for (i = 0; i < nargs; i++) {
        types[i] = NPY_OBJECT;
    }
    str = types + offset[1];
    memcpy(str, fname, fname_len);
    memcpy(str+fname_len, " (vectorized)", 14);
    Py_XDECREF(pyname);

    /* Do a better job someday */
    doc = "dynamic ufunc based on a python function";

    self = (PyUFuncObject *)PyUFunc_FromFuncAndData(
            (PyUFuncGenericFunction *)pyfunc_functions, data,
            types, /* ntypes */ 1, nin, nout, PyUFunc_None,
            str, doc, /* unused */ 0);

    if (self == NULL) {
        PyArray_free(ptr);
        return NULL;
    }
    Py_INCREF(function);
    self->obj = function;
    self->ptr = ptr;

    self->type_resolver = &object_ufunc_type_resolver;
    self->legacy_inner_loop_selector = &object_ufunc_loop_selector;

    return (PyObject *)self;
}

/* docstring in numpy.add_newdocs.py */
static PyObject *
add_newdoc_ufunc(PyObject *NPY_UNUSED(dummy), PyObject *args)
{
    PyUFuncObject *ufunc;
    PyObject *str;
    char *docstr, *newdocstr;

#if defined(NPY_PY3K)
    if (!PyArg_ParseTuple(args, "O!O!:_add_newdoc_ufunc", &PyUFunc_Type, &ufunc,
                                        &PyUnicode_Type, &str)) {
        return NULL;
    }
    docstr = PyBytes_AS_STRING(PyUnicode_AsUTF8String(str));
#else
    if (!PyArg_ParseTuple(args, "O!O!:_add_newdoc_ufunc", &PyUFunc_Type, &ufunc,
                                         &PyString_Type, &str)) {
         return NULL;
    }
    docstr = PyString_AS_STRING(str);
#endif

    if (NULL != ufunc->doc) {
        PyErr_SetString(PyExc_ValueError,
                "Cannot change docstring of ufunc with non-NULL docstring");
        return NULL;
    }

    /*
     * This introduces a memory leak, as the memory allocated for the doc
     * will not be freed even if the ufunc itself is deleted. In practice
     * this should not be a problem since the user would have to
     * repeatedly create, document, and throw away ufuncs.
     */
    newdocstr = malloc(strlen(docstr) + 1);
    strcpy(newdocstr, docstr);
    ufunc->doc = newdocstr;

    Py_RETURN_NONE;
}


/*
 *****************************************************************************
 **                            SETUP UFUNCS                                 **
 *****************************************************************************
 */

NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_out = NULL;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_where = NULL;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_axes = NULL;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_axis = NULL;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_keepdims = NULL;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_casting = NULL;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_order = NULL;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_dtype = NULL;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_subok = NULL;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_signature = NULL;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_sig = NULL;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_extobj = NULL;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_array = NULL;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_initial = NULL;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_indices = NULL;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_array_prepare = NULL;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_array_wrap = NULL;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_array_finalize = NULL;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_ufunc = NULL;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_pyvals_name = NULL;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str___call__;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_reduce;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_accumulate;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_reduceat;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_outer;
NPY_VISIBILITY_HIDDEN PyObject *npy_um_str_at;

/* intern some strings used in ufuncs */
static int
intern_strings(void)
{
    npy_um_str_out = PyUString_InternFromString("out");
    npy_um_str_where = PyUString_InternFromString("where");
    npy_um_str_axes = PyUString_InternFromString("axes");
    npy_um_str_axis = PyUString_InternFromString("axis");
    npy_um_str_keepdims = PyUString_InternFromString("keepdims");
    npy_um_str_casting = PyUString_InternFromString("casting");
    npy_um_str_order = PyUString_InternFromString("order");
    npy_um_str_dtype = PyUString_InternFromString("dtype");
    npy_um_str_subok = PyUString_InternFromString("subok");
    npy_um_str_signature = PyUString_InternFromString("signature");
    npy_um_str_sig = PyUString_InternFromString("sig");
    npy_um_str_extobj = PyUString_InternFromString("extobj");
    npy_um_str_array = PyUString_InternFromString("array");
    npy_um_str_initial = PyUString_InternFromString("initial");
    npy_um_str_indices = PyUString_InternFromString("indices");
    npy_um_str_array_prepare = PyUString_InternFromString("__array_prepare__");
    npy_um_str_array_wrap = PyUString_InternFromString("__array_wrap__");
    npy_um_str_array_finalize = PyUString_InternFromString("__array_finalize__");
    npy_um_str_ufunc = PyUString_InternFromString("__array_ufunc__");
    npy_um_str_pyvals_name = PyUString_InternFromString(UFUNC_PYVALS_NAME);
    npy_um_str___call__ = PyUString_InternFromString("__call__");
    npy_um_str_reduce = PyUString_InternFromString("reduce");
    npy_um_str_accumulate = PyUString_InternFromString("accumulate");
    npy_um_str_reduceat = PyUString_InternFromString("reduceat");
    npy_um_str_outer = PyUString_InternFromString("outer");
    npy_um_str_at = PyUString_InternFromString("at");
    return (npy_um_str_out && npy_um_str_where &&
            npy_um_str_axes && npy_um_str_axis && npy_um_str_keepdims &&
            npy_um_str_casting && npy_um_str_order && npy_um_str_dtype &&
            npy_um_str_subok && npy_um_str_signature && npy_um_str_sig &&
            npy_um_str_extobj && npy_um_str_array && npy_um_str_initial &&
            npy_um_str_indices && npy_um_str_array_prepare &&
            npy_um_str_array_wrap && npy_um_str_array_finalize &&
            npy_um_str_ufunc && npy_um_str_pyvals_name &&
            npy_um_str___call__ && npy_um_str_reduce &&
            npy_um_str_accumulate && npy_um_str_reduceat &&
            npy_um_str_outer && npy_um_str_at);
}

/* Setup the umath module */
/* Remove for time being, it is declared in __ufunc_api.h */
/*static PyTypeObject PyUFunc_Type;*/

static struct PyMethodDef methods[] = {
    {"frompyfunc",
        (PyCFunction) ufunc_frompyfunc,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"seterrobj",
        (PyCFunction) ufunc_seterr,
        METH_VARARGS, NULL},
    {"geterrobj",
        (PyCFunction) ufunc_geterr,
        METH_VARARGS, NULL},
    {"_add_newdoc_ufunc", (PyCFunction)add_newdoc_ufunc,
        METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL}                /* sentinel */
};


#if defined(NPY_PY3K)
static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "umath",
        NULL,
        -1,
        methods,
        NULL,
        NULL,
        NULL,
        NULL
};
#endif

#include <stdio.h>

#if defined(NPY_PY3K)
#define RETVAL(x) x
PyMODINIT_FUNC PyInit_umath(void)
#else
#define RETVAL(x)
PyMODINIT_FUNC initumath(void)
#endif
{
    PyObject *m, *d, *s, *s2, *c_api;
    int UFUNC_FLOATING_POINT_SUPPORT = 1;

#ifdef NO_UFUNC_FLOATING_POINT_SUPPORT
    UFUNC_FLOATING_POINT_SUPPORT = 0;
#endif
    /* Create the module and add the functions */
#if defined(NPY_PY3K)
    m = PyModule_Create(&moduledef);
#else
    m = Py_InitModule("umath", methods);
#endif
    if (!m) {
        goto err;
    }

    /* Import the array */
    if (_import_array() < 0) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_ImportError,
                            "umath failed: Could not import array core.");
        }
        goto err;
    }

    /* Initialize the types */
    if (PyType_Ready(&PyUFunc_Type) < 0)
        goto err;

    /* Add some symbolic constants to the module */
    d = PyModule_GetDict(m);

    c_api = NpyCapsule_FromVoidPtr((void *)PyUFunc_API, NULL);
    if (PyErr_Occurred()) {
        goto err;
    }
    PyDict_SetItemString(d, "_UFUNC_API", c_api);
    Py_DECREF(c_api);
    if (PyErr_Occurred()) {
        goto err;
    }

    /* Load the ufunc operators into the array module's namespace */
    if (InitOperators(d) < 0) {
        goto err;
    }

    PyDict_SetItemString(d, "pi", s = PyFloat_FromDouble(NPY_PI));
    Py_DECREF(s);
    PyDict_SetItemString(d, "e", s = PyFloat_FromDouble(NPY_E));
    Py_DECREF(s);
    PyDict_SetItemString(d, "euler_gamma", s = PyFloat_FromDouble(NPY_EULER));
    Py_DECREF(s);

#define ADDCONST(str) PyModule_AddIntConstant(m, #str, UFUNC_##str)
#define ADDSCONST(str) PyModule_AddStringConstant(m, "UFUNC_" #str, UFUNC_##str)

    ADDCONST(ERR_IGNORE);
    ADDCONST(ERR_WARN);
    ADDCONST(ERR_CALL);
    ADDCONST(ERR_RAISE);
    ADDCONST(ERR_PRINT);
    ADDCONST(ERR_LOG);
    ADDCONST(ERR_DEFAULT);

    ADDCONST(SHIFT_DIVIDEBYZERO);
    ADDCONST(SHIFT_OVERFLOW);
    ADDCONST(SHIFT_UNDERFLOW);
    ADDCONST(SHIFT_INVALID);

    ADDCONST(FPE_DIVIDEBYZERO);
    ADDCONST(FPE_OVERFLOW);
    ADDCONST(FPE_UNDERFLOW);
    ADDCONST(FPE_INVALID);

    ADDCONST(FLOATING_POINT_SUPPORT);

    ADDSCONST(PYVALS_NAME);

#undef ADDCONST
#undef ADDSCONST
    PyModule_AddIntConstant(m, "UFUNC_BUFSIZE_DEFAULT", (long)NPY_BUFSIZE);

    PyModule_AddObject(m, "PINF", PyFloat_FromDouble(NPY_INFINITY));
    PyModule_AddObject(m, "NINF", PyFloat_FromDouble(-NPY_INFINITY));
    PyModule_AddObject(m, "PZERO", PyFloat_FromDouble(NPY_PZERO));
    PyModule_AddObject(m, "NZERO", PyFloat_FromDouble(NPY_NZERO));
    PyModule_AddObject(m, "NAN", PyFloat_FromDouble(NPY_NAN));

#if defined(NPY_PY3K)
    s = PyDict_GetItemString(d, "true_divide");
    PyDict_SetItemString(d, "divide", s);
#endif

    s = PyDict_GetItemString(d, "conjugate");
    s2 = PyDict_GetItemString(d, "remainder");
    /* Setup the array object's numerical structures with appropriate
       ufuncs in d*/
    PyArray_SetNumericOps(d);

    PyDict_SetItemString(d, "conj", s);
    PyDict_SetItemString(d, "mod", s2);

    initscalarmath(m);

    if (!intern_strings()) {
        goto err;
    }

    return RETVAL(m);

 err:
    /* Check for errors */
    if (!PyErr_Occurred()) {
        PyErr_SetString(PyExc_RuntimeError,
                        "cannot load umath module.");
    }
    return RETVAL(NULL);
}
