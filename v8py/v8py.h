#include <v8.h>

using namespace v8;

extern Isolate *isolate;

#define JSTR(str) String::NewFromUtf8(isolate, str, NewStringType::kNormal).ToLocalChecked()

PyObject *py_fake_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
