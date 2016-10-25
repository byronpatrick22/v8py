#include <Python.h>
#include <v8.h>
#include "v8py.h"
#include "template.h"
#include "jsobject.h"
#include "convert.h"

PyObject *py_from_js(Local<Value> value, Local<Context> context) {
    HandleScope hs(isolate);

    if (value->IsNull()) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    if (value->IsUndefined()) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    if (value->IsBoolean()) {
        Local<Boolean> bool_value = value.As<Boolean>();
        if (bool_value->Value()) {
            Py_INCREF(Py_True);
            return Py_True;
        } else {
            Py_INCREF(Py_False);
            return Py_False;
        }
    }
    
    if (value->IsObject()) {
        return (PyObject *) py_js_object_new(value.As<Object>(), context);
    }

    if (value->IsString()) {
        Local<String> str_value = value.As<String>();
        size_t bufsize = str_value->Length() * sizeof(uint16_t);
        uint16_t *buf = (uint16_t *) malloc(bufsize);
        str_value->Write(buf, 0, bufsize);
        PyObject *py_value = PyUnicode_DecodeUTF16((const char *) buf, bufsize, NULL, NULL);
        free(buf);
        return py_value;
    }
    if (value->IsUint32() || value->IsInt32()) {
        return PyLong_FromLongLong((PY_LONG_LONG) value.As<Integer>()->Value());
    }
    if (value->IsNumber()) {
        return PyFloat_FromDouble(value.As<Number>()->Value());
    }

    printf("cannot convert\n");
    Py_INCREF(Py_None);
    return Py_None;
}

Local<Value> js_from_py(PyObject *value, Local<Context> context) {
    EscapableHandleScope hs(isolate);

    if (PyUnicode_Check(value)) {
        PyObject *value_encoded = PyUnicode_EncodeUTF8(PyUnicode_AS_UNICODE(value), PyUnicode_GET_SIZE(value), NULL);
        Local<String> js_value = String::NewFromUtf8(isolate, PyString_AS_STRING(value_encoded), NewStringType::kNormal, PyString_GET_SIZE(value_encoded)).ToLocalChecked();
        Py_DECREF(value_encoded);
        return hs.Escape(js_value);
    } 
    if (PyString_Check(value)) {
        Local<String> js_value = String::NewFromUtf8(isolate, PyString_AS_STRING(value), NewStringType::kNormal, PyString_GET_SIZE(value)).ToLocalChecked();
        return hs.Escape(js_value);
    }

    if (value == Py_False) {
        return hs.Escape(False(isolate));
    }
    if (value == Py_True) {
        return hs.Escape(True(isolate));
    }

    if (PyNumber_Check(value)) {
        Local<Number> js_value;
        if (PyFloat_Check(value)) {
            js_value = Number::New(isolate, PyFloat_AS_DOUBLE(value));
        } else if (PyLong_Check(value)) {
            js_value = Integer::New(isolate, PyLong_AsLong(value));
        } else if (PyInt_Check(value)) {
            js_value = Integer::New(isolate, PyInt_AS_LONG(value));
        } else {
            printf("what the hell kind of number is this?!");
            return hs.Escape(Undefined(isolate));
        }
        return hs.Escape(js_value);
    }

    if (PyFunction_Check(value)) {
        py_template *templ = (py_template *) py_function_to_template(value);
        return hs.Escape(py_template_to_function(templ, context));
    }

    if (PyObject_TypeCheck(value, &py_js_object_type)) {
        py_js_object *py_value = (py_js_object *) value;
        return hs.Escape(py_value->object->Get(isolate));
    }

    if (value == Py_None) {
        return hs.Escape(Undefined(isolate));
    }

    printf("cannot convert to js\n");
    return hs.Escape(Undefined(isolate));
}

