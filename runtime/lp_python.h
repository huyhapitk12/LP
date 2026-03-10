#ifndef LP_PYTHON_H
#define LP_PYTHON_H

/*
 * LP Python Interoperability Bridge
 * Uses the Python/C API to call Python libraries from LP programs.
 *
 * This header provides:
 *   - Python interpreter lifecycle management
 *   - Module importing
 *   - Function/method calling with type conversion
 *   - LP <-> PyObject type marshalling
 */

#ifdef _WIN32
  /* Prevent linking issues on Windows */
  #define Py_NO_ENABLE_SHARED
#endif

#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

/* ================================================================
 * Error Handling
 * ================================================================ */

static inline void lp_py_check_error(const char *context) {
    if (PyErr_Occurred()) {
        fprintf(stderr, "[LP Python Error] %s: ", context);
        PyErr_Print();
    }
}

/* ================================================================
 * Interpreter Lifecycle
 * ================================================================ */

static int _lp_py_initialized = 0;

static inline void lp_py_init(void) {
    if (!_lp_py_initialized) {
        Py_Initialize();
        _lp_py_initialized = 1;
    }
}

static inline void lp_py_finalize(void) {
    if (_lp_py_initialized) {
        Py_FinalizeEx();
        _lp_py_initialized = 0;
    }
}

/* ================================================================
 * Module Import
 * ================================================================ */

static inline PyObject *lp_py_import(const char *module_name) {
    PyObject *mod = PyImport_ImportModule(module_name);
    if (!mod) {
        fprintf(stderr, "[LP] Error: cannot import Python module '%s'\n", module_name);
        lp_py_check_error("import");
        return NULL;
    }
    return mod;
}

/* ================================================================
 * Attribute Access (module.attr or obj.attr)
 * ================================================================ */

static inline PyObject *lp_py_getattr(PyObject *obj, const char *attr) {
    if (!obj) return NULL;
    PyObject *val = PyObject_GetAttrString(obj, attr);
    if (!val) {
        fprintf(stderr, "[LP] Error: attribute '%s' not found\n", attr);
        lp_py_check_error("getattr");
    }
    return val;
}

/* ================================================================
 * Function / Method Calling
 * ================================================================ */

/* Call with no arguments */
static inline PyObject *lp_py_call0(PyObject *callable) {
    if (!callable) return NULL;
    PyObject *result = PyObject_CallNoArgs(callable);
    if (!result) lp_py_check_error("call0");
    return result;
}

/* Call with a pre-built tuple of arguments */
static inline PyObject *lp_py_call(PyObject *callable, PyObject *args) {
    if (!callable) return NULL;
    PyObject *result = PyObject_CallObject(callable, args);
    if (!result) lp_py_check_error("call");
    Py_XDECREF(args);
    return result;
}

/* Call a named function on a module/object with a tuple of args */
static inline PyObject *lp_py_call_method(PyObject *obj, const char *method_name, PyObject *args) {
    if (!obj) return NULL;
    PyObject *func = PyObject_GetAttrString(obj, method_name);
    if (!func) {
        fprintf(stderr, "[LP] Error: method '%s' not found\n", method_name);
        lp_py_check_error("call_method");
        return NULL;
    }
    PyObject *result = PyObject_CallObject(func, args);
    if (!result) lp_py_check_error("call_method");
    Py_DECREF(func);
    Py_XDECREF(args);
    return result;
}

/* ================================================================
 * Argument Tuple Builders
 * ================================================================ */

static inline PyObject *lp_py_args0(void) {
    return PyTuple_New(0);
}

static inline PyObject *lp_py_args1(PyObject *a) {
    PyObject *t = PyTuple_New(1);
    PyTuple_SET_ITEM(t, 0, a);  /* steals reference */
    return t;
}

static inline PyObject *lp_py_args2(PyObject *a, PyObject *b) {
    PyObject *t = PyTuple_New(2);
    PyTuple_SET_ITEM(t, 0, a);
    PyTuple_SET_ITEM(t, 1, b);
    return t;
}

static inline PyObject *lp_py_args3(PyObject *a, PyObject *b, PyObject *c) {
    PyObject *t = PyTuple_New(3);
    PyTuple_SET_ITEM(t, 0, a);
    PyTuple_SET_ITEM(t, 1, b);
    PyTuple_SET_ITEM(t, 2, c);
    return t;
}

/* Variable-arg builder: pass count + PyObject* args */
static inline PyObject *lp_py_argsN(int count, ...) {
    PyObject *t = PyTuple_New(count);
    va_list ap;
    va_start(ap, count);
    for (int i = 0; i < count; i++) {
        PyObject *obj = va_arg(ap, PyObject *);
        PyTuple_SET_ITEM(t, i, obj);
    }
    va_end(ap);
    return t;
}

/* ================================================================
 * LP -> PyObject Conversion
 * ================================================================ */

static inline PyObject *lp_py_from_int(int64_t v) {
    return PyLong_FromLongLong((long long)v);
}

static inline PyObject *lp_py_from_float(double v) {
    return PyFloat_FromDouble(v);
}

static inline PyObject *lp_py_from_str(const char *v) {
    return PyUnicode_FromString(v);
}

static inline PyObject *lp_py_from_bool(int v) {
    return PyBool_FromLong(v);
}

/* ================================================================
 * PyObject -> LP Type Conversion
 * ================================================================ */

static inline int64_t lp_py_to_int(PyObject *obj) {
    if (!obj) return 0;
    if (PyLong_Check(obj)) {
        int64_t v = (int64_t)PyLong_AsLongLong(obj);
        return v;
    }
    if (PyFloat_Check(obj)) {
        return (int64_t)PyFloat_AsDouble(obj);
    }
    /* Try generic conversion */
    PyObject *as_int = PyNumber_Long(obj);
    if (as_int) {
        int64_t v = (int64_t)PyLong_AsLongLong(as_int);
        Py_DECREF(as_int);
        return v;
    }
    lp_py_check_error("to_int");
    return 0;
}

static inline double lp_py_to_float(PyObject *obj) {
    if (!obj) return 0.0;
    if (PyFloat_Check(obj)) {
        return PyFloat_AsDouble(obj);
    }
    if (PyLong_Check(obj)) {
        return (double)PyLong_AsLongLong(obj);
    }
    /* Try generic conversion */
    PyObject *as_float = PyNumber_Float(obj);
    if (as_float) {
        double v = PyFloat_AsDouble(as_float);
        Py_DECREF(as_float);
        return v;
    }
    lp_py_check_error("to_float");
    return 0.0;
}

static inline const char *lp_py_to_str(PyObject *obj) {
    if (!obj) return "";
    if (PyUnicode_Check(obj)) {
        return PyUnicode_AsUTF8(obj);
    }
    /* Convert to string via str() */
    PyObject *str_obj = PyObject_Str(obj);
    if (str_obj) {
        const char *s = PyUnicode_AsUTF8(str_obj);
        /* Note: caller should not free, lifetime tied to str_obj */
        return s;
    }
    lp_py_check_error("to_str");
    return "";
}

static inline int lp_py_to_bool(PyObject *obj) {
    if (!obj) return 0;
    return PyObject_IsTrue(obj);
}

/* ================================================================
 * List / Array Helpers
 * ================================================================ */

static inline PyObject *lp_py_list_new(void) {
    return PyList_New(0);
}

static inline void lp_py_list_append(PyObject *list, PyObject *item) {
    if (list && item) {
        PyList_Append(list, item);
    }
}

static inline PyObject *lp_py_list_get(PyObject *list, Py_ssize_t index) {
    if (!list) return NULL;
    PyObject *item = PyList_GetItem(list, index);  /* borrowed ref */
    Py_XINCREF(item);
    return item;
}

static inline Py_ssize_t lp_py_list_len(PyObject *list) {
    if (!list) return 0;
    return PyList_Size(list);
}

/* Build a Python list from LP int array */
static inline PyObject *lp_py_list_from_ints(int64_t *arr, int count) {
    PyObject *list = PyList_New(count);
    for (int i = 0; i < count; i++) {
        PyList_SET_ITEM(list, i, PyLong_FromLongLong((long long)arr[i]));
    }
    return list;
}

/* Build a Python list from LP float array */
static inline PyObject *lp_py_list_from_floats(double *arr, int count) {
    PyObject *list = PyList_New(count);
    for (int i = 0; i < count; i++) {
        PyList_SET_ITEM(list, i, PyFloat_FromDouble(arr[i]));
    }
    return list;
}

/* ================================================================
 * Print Helpers (for PyObject)
 * ================================================================ */

static inline void lp_py_print(PyObject *obj) {
    if (!obj) {
        printf("None\n");
        return;
    }
    PyObject *str_obj = PyObject_Str(obj);
    if (str_obj) {
        const char *s = PyUnicode_AsUTF8(str_obj);
        if (s) printf("%s\n", s);
        Py_DECREF(str_obj);
    } else {
        printf("<unprintable PyObject>\n");
        lp_py_check_error("print");
    }
}

/* ================================================================
 * Reference Management Helpers
 * ================================================================ */

static inline void lp_py_decref(PyObject *obj) {
    Py_XDECREF(obj);
}

static inline PyObject *lp_py_incref(PyObject *obj) {
    Py_XINCREF(obj);
    return obj;
}

#endif /* LP_PYTHON_H */
