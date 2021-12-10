#pragma once
#include "Python.h"
#include "pyhelper.hpp"
#include <string>
#include <iostream>
#include <vector>

using namespace std;

class PCWrapper
{
    public:
        void set(string pFile, string pFun)
        {
            cout << "[PCWrapper] flag 1" << endl;
            pName = PyString_FromString(pFile.c_str()); 

            cout << "[PCWrapper] flag 2" << endl;
            //pModule = PyImport_Import(pName); 
            pModule = PyImport_ImportModule(pFile.c_str()); 
            if(!pModule)
            {
                cout << "[error!]" << endl;
                PyErr_Print();
            }
            cout << "[PCWrapper] flag 3" << endl;
            pFunc = PyObject_GetAttrString(pModule,pFun.c_str());
            cout << "[PCWrapper] flag 4" << endl;
        }

        int callPythonDSTree(string valTuple)
        {
            if(pFunc && PyCallable_Check(pFunc))
            {
                //PyObject* arg_str_s = PyTuple_New(1);
                //PyObject* arg_str = Py_BuildValue("s",valTuple.c_str());
                //PyTuple_SetItem(arg_str_s, 0, arg_str);
                //PyObject* pRet = PyObject_CallObject(pFunc, arg_str_s);
                 arg_str_s = PyTuple_New(1);
                 arg_str = Py_BuildValue("s",valTuple.c_str());
                 PyTuple_SetItem(arg_str_s, 0, arg_str);
                 pRet = PyObject_CallObject(pFunc, arg_str_s);
                if(pRet)
                {
                    int result = PyInt_AsLong(pRet);
                    return result;
                }
                else
                {
                    cout << "ERROR:: pRet" << endl;
                }

            }
            else
            {
                cout << "EORROR::pFunc" << endl;
            }
        }

    private:
//        CPyInstance PInstance;
        CPyObject pName;
        CPyObject pModule;
        CPyObject pFunc;
        CPyObject arg_str_s;
        CPyObject arg_str;
        CPyObject pRet;
        
};
