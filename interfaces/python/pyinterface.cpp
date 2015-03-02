/*  Python Interface for MiniZinc constraint modelling
 *  Author:
 *     Tai Tran <tai.tran@student.adelaide.edu.au>
 *  Supervisor:
 *     Guido Tack <guido.tack@monash.edu>
 */

#include "pyinterface.h";

using namespace MiniZinc;
using namespace std;


/* 
 * Description: Converts a minizinc call
 * Note: Need an outer GCLock for this to work
 */
static PyObject*
Mzn_Call(MznModel* self, PyObject* args)
{
  const char* name;
  PyObject* variableTuple;
  PyTypeObject* returnType;
  if (!PyArg_ParseTuple(args, "sOO", &name, &variableTuple, &returnType)) {
    PyErr_SetString(PyExc_TypeError, "Mzn_Call: Accepts two values: a string and a tuple of minizinc variable");
    PyErr_Print();
    return NULL;
  }

  if (!PyList_Check(variableTuple)) {
    PyErr_SetString(PyExc_TypeError, "Mzn_Call: Second argument must be a list");
    return NULL;
  }

  if (!PyType_Check(returnType)) {
    PyErr_SetString(PyExc_TypeError, "Mzn_Call: Third argument must be a type");
    return NULL;
  }

  long len = PyList_GET_SIZE(variableTuple);
  vector<Expression*> expressionList(len);
  for (long i = 0; i!=len; ++i) {
    PyObject* pyval = PyList_GET_ITEM(variableTuple, i);
    if (PyObject_TypeCheck(pyval, &MznObject_Type)) {
      expressionList[i] = MznObject_get_e(reinterpret_cast<MznObject*>(pyval));
    } else {
      Type type;
      vector<pair<int, int> > dimList;
      expressionList[i] = python_to_minizinc(pyval, type, dimList);
      if (expressionList[i] == NULL) {
        stringstream ss;
        ss << "MznCall: Item at position " << i << "must be a MiniZinc Object or Python int/float/string/list/tuple";
        string errorLog(ss.str());
        PyErr_SetString(PyExc_RuntimeError, errorLog.c_str());
        return NULL;
      }
    }
  }

  PyObject* ret;
  //ret = MznExpression_new(&MznExpression_Type, NULL, NULL);
  
  // WARNING: Don't use returnType as Mzn*_new first argument
  if (returnType == &MznAnnotation_Type)
    ret = MznAnnotation_new(&MznAnnotation_Type, NULL, NULL);
  else
    ret = MznExpression_new(&MznExpression_Type, NULL, NULL);
  /*if (returnType == &MznExpression_Type)
    ret = MznExpression_new(returnType, NULL, NULL);
  else if (returnType == &MznAnnotation_Type)
    ret = MznAnnotation_new(returnType, NULL, NULL);
  else if (returnType == &MznVarSet_Type)
    ret = MznVarSet_new(returnType, NULL, NULL);
  else if (returnType == &MznSet_Type)
    throw logic_error("Mzn_Call: unhandled set type");
  else if (returnType == &PyBool_Type || returnType == &PyInt_Type || returnType == &PyFloat_Type || returnType == &PyString_Type)
    ret = MznVariable_new(returnType, NULL, NULL);
  else
    throw runtime_error("Mzn_Call: unexpected type");*/
  reinterpret_cast<MznExpression*>(ret)->e = new Call(Location(), string(name), expressionList);

  return ret;
}

// Need an outer GCLock
static PyObject*
Mzn_Id(MznModel* self, PyObject* args)
{
  const char* name;
  if (!PyArg_ParseTuple(args, "s", &name)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a string");
    return NULL;
  }
  PyObject* ret = MznAnnotation_new(&MznAnnotation_Type, NULL, NULL);
  reinterpret_cast<MznAnnotation*>(ret)->e = new Id(Location(), name, NULL);
  return ret;
}

/* 
 * Description: Creates a minizinc UnOp expression
 * Note: Need an outer GCLock for this to work
 */
static PyObject*
Mzn_UnOp(MznModel* self, PyObject* args)
{
  /*
  enum UnOpType {
    UOT_NOT,          // 0
    UOT_PLUS,         // 1
    UOT_MINUS         // 2
  };*/
  PyObject* r;
  unsigned int op;
  if (!PyArg_ParseTuple(args, "IO", &op, &r)) {
    PyErr_SetString(PyExc_TypeError, "Mzn_UnOp: Requires a MiniZinc object and an integer");
    return NULL;
  }
  Expression *rhs;

  if (PyObject_TypeCheck(r, &MznObject_Type)) {
    //XXX: Maybe ignore MznSet_Type here
    rhs = MznObject_get_e(reinterpret_cast<MznObject*>(r));
    //rhs = (reinterpret_cast<MznObject*>(r))->e();
  } else if (PyBool_Check(r)) {
    rhs = new BoolLit(Location(), PyInt_AS_LONG(r));
  } else if (PyInt_Check(r)) {
    rhs = new IntLit(Location(), IntVal(PyInt_AS_LONG(r)));
  } else if (PyFloat_Check(r)) {
    rhs = new FloatLit(Location(), PyFloat_AS_DOUBLE(r));
  } else if (PyString_Check(r)) {
    rhs = new StringLit(Location(), string(PyString_AS_STRING(r)));
  } else {
    PyErr_SetString(PyExc_TypeError, "Object must be a Python value or a MiniZinc object");
    return NULL;
  }


  GCLock Lock;

  PyObject* ret = MznExpression_new(&MznExpression_Type, NULL, NULL);
  reinterpret_cast<MznExpression*>(ret)->e = new UnOp(Location(), static_cast<UnOpType>(op), rhs);
  return ret;
}


/* 
 * Description: Creates a minizinc BinOp expression
 * Note: Need an outer GCLock for this to work
 */
static PyObject* 
Mzn_BinOp(MznModel* self, PyObject* args)
{
  /*
  enum BinOpType {
    BOT_PLUS,         // 0
    BOT_MINUS,        // 1
    BOT_MULT,         // 2
    BOT_DIV,          // 3
    BOT_IDIV,         // 4
    BOT_MOD,          // 5
    BOT_LE,           // 6
    BOT_LQ,           // 7
    BOT_GR,           // 8
    BOT_GQ,           // 9
    BOT_EQ,           //10
    BOT_NQ,           //11
    BOT_IN,           //12
    BOT_SUBSET,       //13
    BOT_SUPERSET,     //14
    BOT_UNION,        //15
    BOT_DIFF,         //16
    BOT_SYMDIFF,      //17
    BOT_INTERSECT,    //18
    BOT_PLUSPLUS,     //19
    BOT_EQUIV,        //20
    BOT_IMPL,         //21
    BOT_RIMPL,        //22
    BOT_OR,           //23
    BOT_AND,          //24
    BOT_XOR,          //25
    BOT_DOTDOT        //26
  };*/
  PyObject* PyPre[2];
  unsigned int op;
  if (!PyArg_ParseTuple(args, "OIO", &PyPre[0], &op, &PyPre[1])) {
    PyErr_SetString(PyExc_TypeError, "Mzn_BinOp: Requires two MiniZinc objects and an integer");
    return NULL;
  }
  Expression *pre[2];
  // pre[0]: lhs;
  // pre[1]: rhs;
  for (int i=0; i!=2; ++i) {
    if (PyObject_TypeCheck(PyPre[i], &MznObject_Type)) {
      // XXX: Maybe ignore Set Type here
      pre[i] = MznObject_get_e(reinterpret_cast<MznObject*>(PyPre[i]));
    } else if (PyBool_Check(PyPre[i])) {
      pre[i] = new BoolLit(Location(), PyInt_AS_LONG(PyPre[i]));
    } else if (PyInt_Check(PyPre[i])) {
      pre[i] = new IntLit(Location(), IntVal(PyInt_AS_LONG(PyPre[i])));
    } else if (PyFloat_Check(PyPre[i])) {
      pre[i] = new FloatLit(Location(), PyFloat_AS_DOUBLE(PyPre[i]));
    } else if (PyString_Check(PyPre[i])) {
      pre[i] = new StringLit(Location(), string(PyString_AS_STRING(PyPre[i])));
    } else {
      PyErr_SetString(PyExc_TypeError, "Object must be a Python value or MiniZinc object");
      return NULL;
    }
  }


  GCLock Lock;

  PyObject* ret = MznExpression_new(&MznExpression_Type, NULL, NULL);
  reinterpret_cast<MznExpression*>(ret)->e = (new BinOp(Location(), pre[0], static_cast<BinOpType>(op), pre[1]));
  return ret;
}


static PyObject* Mzn_load(PyObject* self, PyObject* args, PyObject* keywds) {
  PyObject* model = MznModel_new(&MznModelType, NULL, NULL);
  if (model == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "Model cannot be created");
    return NULL;
  }
  if (MznModel_init(reinterpret_cast<MznModel*>(model), NULL) < 0) 
    return NULL;
  if (MznModel_load(reinterpret_cast<MznModel*>(model), args, keywds)==NULL)
    return NULL;
  return model;
}

static PyObject* Mzn_loadFromString(PyObject* self, PyObject* args, PyObject* keywds) {
  PyObject* model = MznModel_new(&MznModelType, NULL, NULL);
  if (model == NULL)
    return NULL;
  if (MznModel_init(reinterpret_cast<MznModel*>(model), NULL) < 0)
    return NULL;
  if (MznModel_loadFromString(reinterpret_cast<MznModel*>(model), args, keywds)==NULL)
    return NULL;
  return model;
}

static PyObject* 
Mzn_retrieveNames(MznModel* self, PyObject* args) {
  PyObject* boolfuncs = PyDict_New();
  PyObject* annfuncs = PyDict_New();
  PyObject* annvars = PyList_New(0);
  PyObject* libName = NULL;

  {
    Py_ssize_t n = PyTuple_GET_SIZE(args);
    if (n > 1) {
      PyErr_SetString(PyExc_TypeError, "Mzn_retrieveNames: accepts at most 1 argument");
      return NULL;
    } else if (n == 1) {
      libName = PyTuple_GET_ITEM(args, 0);
      if (PyObject_IsTrue(libName)) {
        if (!PyString_Check(libName)) {
          PyErr_SetString(PyExc_TypeError, "Mzn_retrieveNames: first argument must be a string");
          return NULL;
        }
      } else
        libName = NULL;
    }
  }

  // If a library name is specified here, it means that this function is called at least once already.
  // If this function is called, functions in globals.mzn and stdlib.mzn is already defined, so we dont want to reinclude it
  bool include_global_mzn = (libName == NULL);

  MznModel* tempModel = reinterpret_cast<MznModel*>(MznModel_new(&MznModelType, NULL, NULL));

  if (MznModel_init(tempModel,libName) != 0) {
    return NULL;
  }
  CollectBoolFuncNames bool_fv(boolfuncs, include_global_mzn);
  CollectAnnNames ann_fv(annfuncs, annvars, include_global_mzn);
  iterItems(bool_fv, tempModel->_m);
  iterItems(ann_fv, tempModel->_m);
  MznModel_dealloc(tempModel);

  PyObject* dict = PyDict_New();
  PyDict_SetItemString(dict, "boolfuncs", boolfuncs);
  PyDict_SetItemString(dict, "annfuncs", annfuncs);
  PyDict_SetItemString(dict, "annvars", annvars);

  Py_DECREF(boolfuncs);
  Py_DECREF(annfuncs);
  Py_DECREF(annvars);
  return dict;
}


PyMODINIT_FUNC
initminizinc_internal(void) {
  PyObject* model = Py_InitModule3("minizinc_internal", Mzn_methods, "A python interface for minizinc constraint modeling");

  if (model == NULL)
    return;

  if (PyType_Ready(&MznObject_Type) < 0)
    return;
  Py_INCREF(&MznObject_Type);
  PyModule_AddObject(model, "Object", reinterpret_cast<PyObject*>(&MznObject_Type));

  if (PyType_Ready(&MznSetIter_Type) < 0)
    return;
  Py_INCREF(&MznSetIter_Type);
  PyModule_AddObject(model, "Set_Iter", reinterpret_cast<PyObject*>(&MznSetIter_Type));

  if (PyType_Ready(&MznExpression_Type) < 0)
    return;
  Py_INCREF(&MznExpression_Type);
  PyModule_AddObject(model, "Expression", reinterpret_cast<PyObject*>(&MznExpression_Type));

  if (PyType_Ready(&MznAnnotation_Type) < 0)
    return;
  Py_INCREF(&MznAnnotation_Type);
  PyModule_AddObject(model, "Annotation", reinterpret_cast<PyObject*>(&MznAnnotation_Type));

  if (PyType_Ready(&MznDeclaration_Type) < 0)
    return;
  Py_INCREF(&MznDeclaration_Type);
  PyModule_AddObject(model, "Declaration", reinterpret_cast<PyObject*>(&MznDeclaration_Type));

  if (PyType_Ready(&MznVariable_Type) < 0)
    return;
  Py_INCREF(&MznVariable_Type);
  PyModule_AddObject(model, "Variable", reinterpret_cast<PyObject*>(&MznVariable_Type));

  if (PyType_Ready(&MznArray_Type) < 0)
    return;
  Py_INCREF(&MznArray_Type);
  PyModule_AddObject(model, "Array", reinterpret_cast<PyObject*>(&MznArray_Type));

  if (PyType_Ready(&MznSet_Type) < 0)
    return;
  Py_INCREF(&MznSet_Type);
  PyModule_AddObject(model, "Set", reinterpret_cast<PyObject*>(&MznSet_Type));

  if (PyType_Ready(&MznVarSet_Type) < 0)
    return;
  Py_INCREF(&MznVarSet_Type);
  PyModule_AddObject(model, "VarSet", reinterpret_cast<PyObject*>(&MznVarSet_Type));

  if (PyType_Ready(&MznModelType) < 0)
    return;
  Py_INCREF(&MznModelType);
  PyModule_AddObject(model, "Model", reinterpret_cast<PyObject*>(&MznModelType));

  /*PyMethodDef temp;

  temp.ml_name = "next";
  temp.ml_meth = (PyCFunction)MznSolver_next;
  temp.ml_flags = METH_NOARGS;
  temp.ml_doc = "Next Solution";
  MznSolver_methods[0] = temp;
  temp.ml_name = "getValue";
  temp.ml_meth = (PyCFunction)MznSolver_getValue;
  temp.ml_flags = METH_VARARGS;
  temp.ml_doc = "Get value";
  MznSolver_methods[1] = temp;
  temp.ml_name = NULL;
  temp.ml_meth = NULL;
  temp.ml_flags = NULL;
  temp.ml_doc = NULL;
  MznSolver_methods[2] = temp;*/

  if (PyType_Ready(&MznSolverType) < 0)
    return;
  Py_INCREF(&MznSolverType);
  PyModule_AddObject(model, "Solver", reinterpret_cast<PyObject*>(&MznSolverType));
}