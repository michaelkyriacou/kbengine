// Copyright 2008-2018 Yolo Technologies, Inc. All Rights Reserved. https://www.comblockengine.com

#include <stack>
#include <future>
#include <chrono>

#include "common.h"
#include "entitydef.h"
#include "py_entitydef.h"
#include "pyscript/py_platform.h"
#include "pyscript/pyobject_pointer.h"
#include "pyscript/script.h"
#include "pyscript/copy.h"
#include "resmgr/resmgr.h"

namespace KBEngine{ namespace script{ namespace entitydef {

struct CallContext
{
	PyObjectPtr pyArgs;
	PyObjectPtr pyKwargs;
	std::string optionName;
};

struct DefContext
{
	enum DCType
	{
		DC_TYPE_UNKNOWN = 0,
		DC_TYPE_ENTITY = 1,
		DC_TYPE_COMPONENT = 2,
		DC_TYPE_INTERFACE = 3,

		DC_TYPE_PROPERTY = 4,
		DC_TYPE_METHOD = 5,
		DC_TYPE_CLIENT_METHOD = 6,

		DC_TYPE_FIXED_DICT = 7,
		DC_TYPE_FIXED_ARRAY = 8,
		DC_TYPE_FIXED_ITEM = 9,

		DC_TYPE_RENAME = 10,
	};

	DefContext()
	{
		optionName = "";

		moduleName = "";
		attrName = "";
		methodArgs = "";
		returnType = "";

		isModuleScope = false;

		exposed = false;
		hasClient = false;
		persistent = -1;
		databaseLength = 0;

		propertyFlags = "";
		propertyIndex = "";
		propertyDefaultVal = "";

		implementedByModuleName = "";
		implementedByModuleFile = "";

		inheritEngineModuleType = DC_TYPE_UNKNOWN;
		type = DC_TYPE_UNKNOWN;

		componentType = UNKNOWN_COMPONENT_TYPE;
	}

	bool addToStream(MemoryStream* pMemoryStream)
	{
		(*pMemoryStream) << optionName;
		(*pMemoryStream) << moduleName;
		(*pMemoryStream) << attrName;
		(*pMemoryStream) << methodArgs;
		(*pMemoryStream) << returnType;

		(*pMemoryStream) << (int)argsvecs.size();
		std::vector< std::string >::iterator argsvecsIter = argsvecs.begin();
		for(; argsvecsIter != argsvecs.end(); ++argsvecsIter)
			(*pMemoryStream) << (*argsvecsIter);

		(*pMemoryStream) << (int)annotationsMaps.size();
		std::map< std::string, std::string >::iterator annotationsMapsIter = annotationsMaps.begin();
		for (; annotationsMapsIter != annotationsMaps.end(); ++annotationsMapsIter)
			(*pMemoryStream) << annotationsMapsIter->first << annotationsMapsIter->second;

		(*pMemoryStream) << isModuleScope;
		(*pMemoryStream) << exposed;
		(*pMemoryStream) << hasClient;

		(*pMemoryStream) << persistent;
		(*pMemoryStream) << databaseLength;

		(*pMemoryStream) << propertyFlags;
		(*pMemoryStream) << propertyIndex;
		(*pMemoryStream) << propertyDefaultVal;

		(*pMemoryStream) << implementedByModuleName;
		(*pMemoryStream) << implementedByModuleFile;

		(*pMemoryStream) << (int)baseClasses.size();
		std::vector< std::string >::iterator baseClassesIter = baseClasses.begin();
		for (; baseClassesIter != baseClasses.end(); ++baseClassesIter)
			(*pMemoryStream) << (*baseClassesIter);

		(*pMemoryStream) << (int)inheritEngineModuleType;
		(*pMemoryStream) << (int)type;

		(*pMemoryStream) << (int)methods.size();
		std::vector< DefContext >::iterator methodsIter = methods.begin();
		for (; methodsIter != methods.end(); ++methodsIter)
			(*methodsIter).addToStream(pMemoryStream);

		(*pMemoryStream) << (int)clienbt_methods.size();
		std::vector< DefContext >::iterator clienbt_methodsIter = clienbt_methods.begin();
		for (; clienbt_methodsIter != clienbt_methods.end(); ++clienbt_methodsIter)
			(*clienbt_methodsIter).addToStream(pMemoryStream);

		(*pMemoryStream) << (int)propertys.size();
		std::vector< DefContext >::iterator propertysIter = propertys.begin();
		for (; propertysIter != propertys.end(); ++propertysIter)
			(*propertysIter).addToStream(pMemoryStream);

		(*pMemoryStream) << (int)components.size();
		std::vector< std::string >::iterator componentsIter = components.begin();
		for (; componentsIter != components.end(); ++componentsIter)
			(*pMemoryStream) << (*componentsIter);

		return true;
	}

	bool createFromStream(MemoryStream* pMemoryStream)
	{
		(*pMemoryStream) >> optionName;
		(*pMemoryStream) >> moduleName;
		(*pMemoryStream) >> attrName;
		(*pMemoryStream) >> methodArgs;
		(*pMemoryStream) >> returnType;

		int size = 0;

		(*pMemoryStream) >> size;
		for (int i = 0; i < size; ++i)
		{
			std::string str;
			(*pMemoryStream) >> str;

			argsvecs.push_back(str);
		}

		(*pMemoryStream) >> size;
		for (int i = 0; i < size; ++i)
		{
			std::string key, val;
			(*pMemoryStream) >> key >> val;

			annotationsMaps[key] = val;
		}

		(*pMemoryStream) >> isModuleScope;
		(*pMemoryStream) >> exposed;
		(*pMemoryStream) >> hasClient;

		(*pMemoryStream) >> persistent;
		(*pMemoryStream) >> databaseLength;

		(*pMemoryStream) >> propertyFlags;
		(*pMemoryStream) >> propertyIndex;
		(*pMemoryStream) >> propertyDefaultVal;

		(*pMemoryStream) >> implementedByModuleName;
		(*pMemoryStream) >> implementedByModuleFile;

		(*pMemoryStream) >> size;
		for (int i = 0; i < size; ++i)
		{
			std::string str;
			(*pMemoryStream) >> str;

			baseClasses.push_back(str);
		}

		int t_inheritEngineModuleType;
		(*pMemoryStream) >> t_inheritEngineModuleType;
		inheritEngineModuleType = (DCType)t_inheritEngineModuleType;

		int t_type;
		(*pMemoryStream) >> t_type;
		type = (DCType)t_type;

		(*pMemoryStream) >> size;
		for (int i = 0; i < size; ++i)
		{
			DefContext dc;
			dc.createFromStream(pMemoryStream);

			methods.push_back(dc);
		}

		(*pMemoryStream) >> size;
		for (int i = 0; i < size; ++i)
		{
			DefContext dc;
			dc.createFromStream(pMemoryStream);

			clienbt_methods.push_back(dc);
		}

		(*pMemoryStream) >> size;
		for (int i = 0; i < size; ++i)
		{
			DefContext dc;
			dc.createFromStream(pMemoryStream);

			propertys.push_back(dc);
		}

		(*pMemoryStream) >> size;
		for (int i = 0; i < size; ++i)
		{
			std::string str;
			(*pMemoryStream) >> str;

			components.push_back(str);
		}

		return true;
	}

	bool addChildContext(DefContext& defContext)
	{
		std::vector< DefContext >* pContexts = NULL;

		if (defContext.type == DefContext::DC_TYPE_PROPERTY)
			pContexts = &propertys;
		else if (defContext.type == DefContext::DC_TYPE_METHOD)
			pContexts = &methods;
		else if (defContext.type == DefContext::DC_TYPE_CLIENT_METHOD)
			pContexts = &clienbt_methods;
		else if (defContext.type == DefContext::DC_TYPE_FIXED_ITEM)
			pContexts = &propertys;
		else
			KBE_ASSERT(false);

		std::vector< DefContext >::iterator iter = pContexts->begin();
		for (; iter != pContexts->end(); ++iter)
		{
			if ((*iter).attrName == defContext.attrName)
			{
				// �����assemblyContextsʱ���ܻᷢ�����������
				// ������������
				if (moduleName != defContext.moduleName)
					return true;

				return false;
			}
		}

		pContexts->push_back(defContext);
		return true;
	}

	std::string optionName;

	std::string moduleName;
	std::string attrName;
	std::string methodArgs;
	std::string returnType;

	std::vector< std::string > argsvecs;
	std::map< std::string, std::string > annotationsMaps;

	bool isModuleScope;

	bool exposed;
	bool hasClient;

	// -1��δ���ã� 0��false�� 1��true
	int persistent;

	int databaseLength;

	std::string propertyFlags;
	std::string propertyIndex;
	std::string propertyDefaultVal;

	PyObjectPtr implementedBy;
	std::string implementedByModuleName;
	std::string implementedByModuleFile;

	PyObjectPtr pyObjectPtr;

	std::vector< std::string > baseClasses;

	DCType inheritEngineModuleType;
	DCType type;

	std::vector< DefContext > methods;
	std::vector< DefContext > clienbt_methods;
	std::vector< DefContext > propertys;
	std::vector< std::string > components;

	COMPONENT_TYPE componentType;
};

static std::stack<CallContext> g_callContexts;
static std::string pyDefModuleName = "";

typedef std::map<std::string, DefContext> DEF_CONTEXT_MAP;
static DEF_CONTEXT_MAP g_allScriptDefContextMaps;

static bool g_inited = false;

//-------------------------------------------------------------------------------------
static bool assemblyContexts(bool notfoundModuleError = false)
{
	std::vector< std::string > dels;

	DEF_CONTEXT_MAP::iterator iter = g_allScriptDefContextMaps.begin();
	for (; iter != g_allScriptDefContextMaps.end(); ++iter)
	{
		DefContext& defContext = iter->second;

		if (defContext.type == DefContext::DC_TYPE_PROPERTY ||
			defContext.type == DefContext::DC_TYPE_METHOD ||
			defContext.type == DefContext::DC_TYPE_CLIENT_METHOD ||
			defContext.type == DefContext::DC_TYPE_FIXED_ITEM)
		{
			DEF_CONTEXT_MAP::iterator fiter = g_allScriptDefContextMaps.find(defContext.moduleName);
			if (fiter == g_allScriptDefContextMaps.end())
			{
				if (notfoundModuleError)
				{
					PyErr_Format(PyExc_AssertionError, "PyEntityDef::process(): No \'%s\' module defined!\n", defContext.moduleName.c_str());
					return false;
				}

				return true;
			}

			if (!fiter->second.addChildContext(defContext))
			{
				PyErr_Format(PyExc_AssertionError, "\'%s.%s\' already exists!\n",
					fiter->second.moduleName.c_str(), defContext.attrName.c_str());

				return false;
			}

			dels.push_back(iter->first);
		}
		else
		{
		}
	}

	std::vector< std::string >::iterator diter = dels.begin();
	for (; diter != dels.end(); ++diter)
	{
		g_allScriptDefContextMaps.erase((*diter));
	}

	// ���Խ�������Ϣ��䵽������
	iter = g_allScriptDefContextMaps.begin();
	for (; iter != g_allScriptDefContextMaps.end(); ++iter)
	{
		DefContext& defContext = iter->second;
		if (defContext.baseClasses.size() > 0)
		{
			for (size_t i = 0; i < defContext.baseClasses.size(); ++i)
			{
				std::string parentClass = defContext.baseClasses[i];

				DEF_CONTEXT_MAP::iterator fiter = g_allScriptDefContextMaps.find(parentClass);
				if (fiter == g_allScriptDefContextMaps.end())
				{
					//PyErr_Format(PyExc_AssertionError, "not found parentClass(\'%s\')!\n", parentClass.c_str());
					//return false;
					continue;
				}

				DefContext& parentDefContext = fiter->second;
				std::vector< DefContext > childContexts;

				childContexts.insert(childContexts.end(), parentDefContext.methods.begin(), parentDefContext.methods.end());
				childContexts.insert(childContexts.end(), parentDefContext.clienbt_methods.begin(), parentDefContext.clienbt_methods.end());
				childContexts.insert(childContexts.end(), parentDefContext.propertys.begin(), parentDefContext.propertys.end());

				std::vector< DefContext >::iterator itemIter = childContexts.begin();
				for (; itemIter != childContexts.end(); ++itemIter)
				{
					DefContext& parentDefContext = (*itemIter);
					if (!defContext.addChildContext(parentDefContext))
					{
						PyErr_Format(PyExc_AssertionError, "\'%s.%s\' already exists(%s.%s)!\n",
							defContext.moduleName.c_str(), parentDefContext.attrName.c_str(), parentDefContext.moduleName.c_str(), parentDefContext.attrName.c_str());

						return false;
					}
				}
			}
		}
	}

	return true;
}

//-------------------------------------------------------------------------------------
static bool registerDefContext(DefContext& defContext)
{
	std::string name = defContext.moduleName;

	if (defContext.type == DefContext::DC_TYPE_PROPERTY ||
		defContext.type == DefContext::DC_TYPE_METHOD ||
		defContext.type == DefContext::DC_TYPE_CLIENT_METHOD ||
		defContext.type == DefContext::DC_TYPE_FIXED_ITEM)
		name += "." + defContext.attrName;

	DEF_CONTEXT_MAP::iterator iter = g_allScriptDefContextMaps.find(name);
	if (iter != g_allScriptDefContextMaps.end())
	{
		PyErr_Format(PyExc_AssertionError, "Def.%s: \'%s\' already exists!\n", 
			defContext.optionName.c_str(), name.c_str());

		return false;
	}

	g_allScriptDefContextMaps[name] = defContext;
	return assemblyContexts();
}

//-------------------------------------------------------------------------------------
static bool onDefRename(DefContext& defContext)
{
	defContext.type = DefContext::DC_TYPE_RENAME;
	return registerDefContext(defContext);
}

//-------------------------------------------------------------------------------------
static bool onDefFixedDict(DefContext& defContext)
{
	defContext.type = DefContext::DC_TYPE_FIXED_DICT;
	return registerDefContext(defContext);
}

//-------------------------------------------------------------------------------------
static bool onDefFixedArray(DefContext& defContext)
{
	defContext.type = DefContext::DC_TYPE_FIXED_ARRAY;
	return registerDefContext(defContext);
}

//-------------------------------------------------------------------------------------
static bool onDefFixedItem(DefContext& defContext)
{
	defContext.type = DefContext::DC_TYPE_FIXED_ITEM;
	return registerDefContext(defContext);
}

//-------------------------------------------------------------------------------------
static bool onDefProperty(DefContext& defContext)
{
	defContext.type = DefContext::DC_TYPE_PROPERTY;
	return registerDefContext(defContext);
}

//-------------------------------------------------------------------------------------
static bool onDefMethod(DefContext& defContext)
{
	defContext.type = DefContext::DC_TYPE_METHOD;
	return registerDefContext(defContext);
}

//-------------------------------------------------------------------------------------
static bool onDefClientMethod(DefContext& defContext)
{
	defContext.type = DefContext::DC_TYPE_CLIENT_METHOD;
	return registerDefContext(defContext);
}

//-------------------------------------------------------------------------------------
static bool onDefEntity(DefContext& defContext)
{
	defContext.type = DefContext::DC_TYPE_ENTITY;
	return registerDefContext(defContext);
}

//-------------------------------------------------------------------------------------
static bool onDefInterface(DefContext& defContext)
{
	defContext.type = DefContext::DC_TYPE_INTERFACE;
	return registerDefContext(defContext);
}

//-------------------------------------------------------------------------------------
static bool onDefComponent(DefContext& defContext)
{
	defContext.type = DefContext::DC_TYPE_COMPONENT;
	return registerDefContext(defContext);
}

//-------------------------------------------------------------------------------------
static bool isRefEntityDefModule(PyObject *pyObj)
{
	if(!pyObj)
		return true;

	PyObject *entitydefModule = PyImport_AddModule(pyDefModuleName.c_str());
	PyObject* pydict = PyObject_GetAttrString(entitydefModule, "__dict__");

	PyObject *key, *value;
	Py_ssize_t pos = 0;

	while (PyDict_Next(pydict, &pos, &key, &value)) {
		if (value == pyObj)
		{
			Py_DECREF(pydict);
			return true;
		}
	}

	Py_DECREF(pydict);
	return false;
}

//-------------------------------------------------------------------------------------
#define PY_RETURN_ERROR { g_allScriptDefContextMaps.clear(); while(!g_callContexts.empty()) g_callContexts.pop(); return NULL; }

static PyObject* __py_def_parse(PyObject *self, PyObject* args)
{
	CallContext cc = g_callContexts.top();
	g_callContexts.pop();
	
	DefContext defContext;
	defContext.optionName = cc.optionName;

	PyObject* kbeModule = PyImport_AddModule("KBEngine");
	KBE_ASSERT(kbeModule);

	PyObject* pyComponentName = PyObject_GetAttrString(kbeModule, "component");
	if (!pyComponentName)
	{
		PyErr_Format(PyExc_AssertionError, "Def.__py_def_call(): get KBEngine.component error!\n");
		PY_RETURN_ERROR;
	}

	defContext.componentType = ComponentName2ComponentType(PyUnicode_AsUTF8AndSize(pyComponentName, NULL));
	Py_DECREF(pyComponentName);

	if (!args || PyTuple_Size(args) < 1)
	{
		PyErr_Format(PyExc_AssertionError, "Def.__py_def_call(Def.%s): error!\n", defContext.optionName.c_str());
		PY_RETURN_ERROR;
	}

	PyObject* pyFunc = PyTuple_GET_ITEM(args, 0);

	PyObject* pyModuleQualname = PyObject_GetAttrString(pyFunc, "__qualname__");
	if (!pyModuleQualname)
	{
		PY_RETURN_ERROR;
	}

	const char* moduleQualname = PyUnicode_AsUTF8AndSize(pyModuleQualname, NULL);
	Py_DECREF(pyModuleQualname);

	defContext.pyObjectPtr = PyObjectPtr(pyFunc);

	if (defContext.optionName == "method")
	{
		static char * keywords[] =
		{
			const_cast<char *> ("exposed"),
			NULL
		};

		PyObject* pyExposed = NULL;

		if (!PyArg_ParseTupleAndKeywords(cc.pyArgs.get(), cc.pyKwargs.get(), "|O",
			keywords, &pyExposed))
		{
			PY_RETURN_ERROR;
		}

		defContext.exposed = pyExposed == Py_True;
	}
	else if (defContext.optionName == "property" || defContext.optionName == "fixed_item")
	{
		if (defContext.optionName != "fixed_item")
		{
			static char * keywords[] =
			{
				const_cast<char *> ("flags"),
				const_cast<char *> ("persistent"),
				const_cast<char *> ("index"),
				const_cast<char *> ("databaseLength"),
				NULL
			};

			PyObject* pyFlags = NULL;
			PyObject* pyPersistent = NULL;
			PyObject* pyIndex = NULL;
			PyObject* pyDatabaseLength = NULL;

			if (!PyArg_ParseTupleAndKeywords(cc.pyArgs.get(), cc.pyKwargs.get(), "|OOOO",
				keywords, &pyFlags, &pyPersistent, &pyIndex, &pyDatabaseLength))
			{
				PY_RETURN_ERROR;
			}

			if (!pyFlags || !isRefEntityDefModule(pyFlags))
			{
				PyErr_Format(PyExc_AssertionError, "Def.%s: \'flags\' must be referenced from the [Def.ALL_CLIENTS, Def.*] module!\n", defContext.optionName.c_str());
				PY_RETURN_ERROR;
			}

			if (!isRefEntityDefModule(pyIndex))
			{
				PyErr_Format(PyExc_AssertionError, "Def.%s: \'index\' must be referenced from the [Def.UNIQUE, Def.INDEX] module!\n", defContext.optionName.c_str());
				PY_RETURN_ERROR;
			}

			if (pyDatabaseLength && !PyLong_Check(pyDatabaseLength))
			{
				PyErr_Format(PyExc_AssertionError, "Def.%s: \'databaseLength\' error! not a number type.\n", defContext.optionName.c_str());
				PY_RETURN_ERROR;
			}

			if (pyPersistent && !PyBool_Check(pyPersistent))
			{
				PyErr_Format(PyExc_AssertionError, "Def.%s: \'persistent\' error! not a bool type.\n", defContext.optionName.c_str());
				PY_RETURN_ERROR;
			}

			defContext.propertyFlags = PyUnicode_AsUTF8AndSize(pyFlags, NULL);

			if (pyPersistent)
				defContext.persistent = pyPersistent == Py_True;

			if (pyIndex)
				defContext.propertyIndex = PyUnicode_AsUTF8AndSize(pyIndex, NULL);

			if (pyDatabaseLength)
				defContext.databaseLength = (int)PyLong_AsLong(pyDatabaseLength);
		}

		// �������ԣ� ������Ҫ��÷���ֵ��ΪĬ��ֵ
		PyObject* pyRet = PyObject_CallFunction(pyFunc,
			const_cast<char*>("(O)"), Py_None);

		if (!pyRet)
			return NULL;

		if (pyRet != Py_None)
		{
			PyObject* pyStrResult = PyObject_Str(pyRet);
			defContext.propertyDefaultVal = PyUnicode_AsUTF8AndSize(pyStrResult, NULL);
			Py_DECREF(pyStrResult);

			// ��֤����ַ����Ƿ���Ի�ԭ�ɶ���
			if (defContext.propertyDefaultVal.size() > 0)
			{
				PyObject* module = PyImport_AddModule("__main__");
				if (module == NULL)
					return NULL;

				PyObject* mdict = PyModule_GetDict(module); // Borrowed reference.
				PyObject* result = PyRun_String(const_cast<char*>(defContext.propertyDefaultVal.c_str()),
					Py_eval_input, mdict, mdict);

				if (result == NULL)
					return NULL;

				Py_DECREF(result);
			}
		}

		Py_DECREF(pyRet);
	}
	else if (defContext.optionName == "entity")
	{
		defContext.isModuleScope = true;

		static char * keywords[] =
		{
			const_cast<char *> ("hasClient"),
			NULL
		};

		PyObject* pyHasClient = NULL;

		if (!PyArg_ParseTupleAndKeywords(cc.pyArgs.get(), cc.pyKwargs.get(), "|O",
			keywords, &pyHasClient))
		{
			PY_RETURN_ERROR;
		}

		defContext.hasClient = pyHasClient == Py_True;
	}
	else if (defContext.optionName == "interface")
	{
		defContext.isModuleScope = true;
		defContext.inheritEngineModuleType = DefContext::DC_TYPE_INTERFACE;
	}
	else if (defContext.optionName == "component")
	{
		defContext.isModuleScope = true;
	}
	else if (defContext.optionName == "fixed_dict")
	{
		defContext.isModuleScope = true;

		static char * keywords[] =
		{
			const_cast<char *> ("implementedBy"),
			NULL
		};

		PyObject* pImplementedBy = NULL;

		if (!PyArg_ParseTupleAndKeywords(cc.pyArgs.get(), cc.pyKwargs.get(), "|O",
			keywords, &pImplementedBy))
		{
			PY_RETURN_ERROR;
		}

		if (pImplementedBy)
		{
			if (isRefEntityDefModule(pImplementedBy))
			{
				if (std::string(PyUnicode_AsUTF8AndSize(pImplementedBy, NULL)) == "thisClass")
				{
					defContext.implementedBy = pyFunc;
				}
			}
			else
			{
				defContext.implementedBy = pImplementedBy;
			}

			PyObject* pyQualname = PyObject_GetAttrString(defContext.implementedBy.get(), "__qualname__");
			if (!pyQualname)
			{
				PY_RETURN_ERROR;
			}

			defContext.implementedByModuleName = PyUnicode_AsUTF8AndSize(pyQualname, NULL);
			Py_DECREF(pyQualname);

			PyObject* pyInspectModule =
				PyImport_ImportModule(const_cast<char*>("inspect"));

			PyObject* pyGetsourcefile = NULL;
			if (pyInspectModule)
			{
				pyGetsourcefile =
					PyObject_GetAttrString(pyInspectModule, const_cast<char *>("getsourcefile"));

				Py_DECREF(pyInspectModule);
			}
			else
			{
				PY_RETURN_ERROR;
			}

			if (pyGetsourcefile)
			{
				PyObject* pyFile = PyObject_CallFunction(pyGetsourcefile,
					const_cast<char*>("(O)"), defContext.implementedBy.get());

				Py_DECREF(pyGetsourcefile);

				if (!pyFile)
				{
					PY_RETURN_ERROR;
				}
				else
				{
					// ��ֹ��ͬϵͳ��ɵ�·����һ�£��޳�ϵͳ���·��
					defContext.implementedByModuleFile = PyUnicode_AsUTF8AndSize(pyFile, NULL);
					strutil::kbe_replace(defContext.implementedByModuleFile, "/", "");
					strutil::kbe_replace(defContext.implementedByModuleFile, "\\", "");
					std::string kbe_root = Resmgr::getSingleton().getPyUserScriptsPath();
					strutil::kbe_replace(kbe_root, "/", "");
					strutil::kbe_replace(kbe_root, "\\", "");
					strutil::kbe_replace(defContext.implementedByModuleFile, kbe_root, "");
					Py_DECREF(pyFile);
				}
			}
			else
			{
				PY_RETURN_ERROR;
			}
		}
	}
	else if (defContext.optionName == "fixed_array")
	{
		defContext.isModuleScope = true;
	}
	else if (defContext.optionName == "fixed_item")
	{
	}
	else if (defContext.optionName == "rename")
	{
	}
	else
	{
		PyErr_Format(PyExc_AssertionError, "Def.%s: not support!\n", defContext.optionName.c_str());
		PY_RETURN_ERROR;
	}

	if (!defContext.isModuleScope)
	{
		std::vector<std::string> outs;

		if (moduleQualname)
			strutil::kbe_splits(moduleQualname, ".", outs);

		if (defContext.optionName != "rename")
		{
			if (outs.size() != 2)
			{
				if(PyFunction_Check(pyFunc))
					PyErr_Format(PyExc_AssertionError, "Def.%s: \'%s\' must be defined in the entity module!\n", 
						defContext.optionName.c_str(), moduleQualname);
				else
					PyErr_Format(PyExc_AssertionError, "Def.%s: please check the command format is: Def.%s(..)\n", 
						defContext.optionName.c_str(), defContext.optionName.c_str());

				PY_RETURN_ERROR;
			}

			defContext.moduleName = outs[0];
			defContext.attrName = outs[1];
		}
		else
		{
			if (outs.size() != 1)
			{
				PyErr_Format(PyExc_AssertionError, "Def.%s: error! such as: @Def.rename()\n\tdef ENTITY_ID() -> int: pass\n", defContext.optionName.c_str());
				PY_RETURN_ERROR;
			}

			defContext.moduleName = outs[0];
		}

		PyObject* pyInspectModule =
			PyImport_ImportModule(const_cast<char*>("inspect"));

		PyObject* pyGetfullargspec = NULL;
		if (pyInspectModule)
		{
			pyGetfullargspec =
				PyObject_GetAttrString(pyInspectModule, const_cast<char *>("getfullargspec"));

			Py_DECREF(pyInspectModule);
		}
		else
		{
			PY_RETURN_ERROR;
		}

		if (pyGetfullargspec)
		{
			PyObject* pyGetMethodArgs = PyObject_CallFunction(pyGetfullargspec,
				const_cast<char*>("(O)"), pyFunc);

			if (!pyGetMethodArgs)
			{
				PY_RETURN_ERROR;
			}
			else
			{
				PyObject* pyGetMethodArgsResult = PyObject_GetAttrString(pyGetMethodArgs, const_cast<char *>("args"));
				PyObject* pyGetMethodAnnotationsResult = PyObject_GetAttrString(pyGetMethodArgs, const_cast<char *>("annotations"));

				Py_DECREF(pyGetMethodArgs);

				if (!pyGetMethodArgsResult || !pyGetMethodAnnotationsResult)
				{
					Py_XDECREF(pyGetMethodArgsResult);
					Py_XDECREF(pyGetMethodAnnotationsResult);
					PY_RETURN_ERROR;
				}

				PyObjectPtr pyGetMethodArgsResultPtr = pyGetMethodArgsResult;
				PyObjectPtr pyGetMethodAnnotationsResultPtr = pyGetMethodAnnotationsResult;
				Py_DECREF(pyGetMethodArgsResult);
				Py_DECREF(pyGetMethodAnnotationsResult);

				if (defContext.optionName != "rename")
				{
					Py_ssize_t argsSize = PyList_Size(pyGetMethodArgsResult);
					if (argsSize == 0)
					{
						PyErr_Format(PyExc_AssertionError, "Def.%s: \'%s\' did not find \'self\' parameter!\n", defContext.optionName.c_str(), moduleQualname);
						PY_RETURN_ERROR;
					}

					for (Py_ssize_t i = 1; i < argsSize; ++i)
					{
						PyObject* pyItem = PyList_GetItem(pyGetMethodArgsResult, i);

						const char* ccattr = PyUnicode_AsUTF8AndSize(pyItem, NULL);
						if (!ccattr)
						{
							PY_RETURN_ERROR;
						}

						defContext.argsvecs.push_back(ccattr);
					}
				}

				PyObject *key, *value;
				Py_ssize_t pos = 0;

				while (PyDict_Next(pyGetMethodAnnotationsResult, &pos, &key, &value)) {
					const char* skey = PyUnicode_AsUTF8AndSize(key, NULL);
					if (!skey)
					{
						PY_RETURN_ERROR;
					}

					std::string svalue = "";

					if (PyUnicode_Check(value))
					{
						svalue = PyUnicode_AsUTF8AndSize(value, NULL);
					}
					else
					{
						PyObject* pyQualname = PyObject_GetAttrString(value, "__qualname__");
						if (!pyQualname)
							PY_RETURN_ERROR;

						svalue = PyUnicode_AsUTF8AndSize(pyQualname, NULL);
						Py_DECREF(pyQualname);
					}

					if (svalue.size() == 0)
					{
						PY_RETURN_ERROR;
					}

					if (std::string(skey) == "return")
						defContext.returnType = svalue;
					else
						defContext.annotationsMaps[skey] = svalue;
				}
			}
		}
		else
		{
			PY_RETURN_ERROR;
		}
	}
	else
	{
		defContext.moduleName = moduleQualname;

		PyObject* pyBases = PyObject_GetAttrString(pyFunc, "__bases__");
		if (!pyBases)
			PY_RETURN_ERROR;

		Py_ssize_t basesSize = PyTuple_Size(pyBases);
		if (basesSize == 0)
		{
			PyErr_Format(PyExc_AssertionError, "Def.%s: \'%s\' does not inherit the KBEngine.Entity class!\n", defContext.optionName.c_str(), moduleQualname);
			Py_XDECREF(pyBases);
			PY_RETURN_ERROR;
		}

		for (Py_ssize_t i = 0; i < basesSize; ++i)
		{
			PyObject* pyClass = PyTuple_GetItem(pyBases, i);

			PyObject* pyQualname = PyObject_GetAttrString(pyClass, "__qualname__");
			if (!pyQualname)
			{
				Py_XDECREF(pyBases);
				PY_RETURN_ERROR;
			}

			std::string parentClass = PyUnicode_AsUTF8AndSize(pyQualname, NULL);
			Py_DECREF(pyQualname);

			if (parentClass == "object")
			{
				continue;
			}
			else if (parentClass == "Entity" || parentClass == "Proxy")
			{
				defContext.inheritEngineModuleType = DefContext::DC_TYPE_ENTITY;
				continue;
			}
			else if (parentClass == "EntityComponent")
			{
				defContext.inheritEngineModuleType = DefContext::DC_TYPE_COMPONENT;
				continue;
			}

			defContext.baseClasses.push_back(parentClass);
		}

		Py_XDECREF(pyBases);
	}

	bool noerror = true;

	if (defContext.optionName == "method" || defContext.optionName == "clientmethod")
	{
		if (defContext.annotationsMaps.size() != defContext.argsvecs.size())
		{
			PyErr_Format(PyExc_AssertionError, "Def.%s: \'%s\' all parameters must have annotations!\n", defContext.optionName.c_str(), moduleQualname);
			PY_RETURN_ERROR;
		}

		if (defContext.optionName == "method")
			noerror = onDefMethod(defContext);
		else
			noerror = onDefClientMethod(defContext);
	}
	else if (defContext.optionName == "rename")
	{
		noerror = onDefRename(defContext);
	}
	else if (defContext.optionName == "property")
	{
		noerror = onDefProperty(defContext);
	}
	else if (defContext.optionName == "entity")
	{
		noerror = onDefEntity(defContext);
	}
	else if (defContext.optionName == "interface")
	{
		noerror = onDefInterface(defContext);
	}
	else if (defContext.optionName == "component")
	{
		noerror = onDefComponent(defContext);
	}
	else if (defContext.optionName == "fixed_dict")
	{
		noerror = onDefFixedDict(defContext);
	}
	else if (defContext.optionName == "fixed_array")
	{
		noerror = onDefFixedArray(defContext);
	}
	else if (defContext.optionName == "fixed_item")
	{
		noerror = onDefFixedItem(defContext);
	}

	if (!noerror)
	{
		PY_RETURN_ERROR;
	}

	Py_INCREF(pyFunc);
	return pyFunc;
}

//-------------------------------------------------------------------------------------
static PyMethodDef __call_def_parse = { "_PyEntityDefParse", (PyCFunction)&__py_def_parse, METH_VARARGS, 0 };

#define PY_DEF_HOOK(NAME)	\
	static PyObject* __py_def_##NAME(PyObject* self, PyObject* args, PyObject* kwargs)	\
	{	\
		CallContext cc;	\
		cc.pyArgs = PyObjectPtr(Copy::deepcopy(args));	\
		cc.pyKwargs = kwargs ? PyObjectPtr(Copy::deepcopy(kwargs)) : PyObjectPtr(NULL);	\
		cc.optionName = #NAME;	\
		g_callContexts.push(cc);	\
		Py_XDECREF(cc.pyArgs.get());	\
		Py_XDECREF(cc.pyKwargs.get());	\
		\
		return PyCFunction_New(&__call_def_parse, self);	\
	}

static PyObject* __py_def_rename(PyObject* self, PyObject* args, PyObject* kwargs)
{
	CallContext cc;
	cc.pyArgs = PyObjectPtr(Copy::deepcopy(args));
	cc.pyKwargs = kwargs ? PyObjectPtr(Copy::deepcopy(kwargs)) : PyObjectPtr(NULL);
	cc.optionName = "rename";
		
	Py_XDECREF(cc.pyArgs.get());
	Py_XDECREF(cc.pyKwargs.get());

	// �������ֶ��巽ʽ Def.rename(ENTITY_ID=int)
	if (kwargs)
	{
		PyObject *key, *value;
		Py_ssize_t pos = 0;

		while (PyDict_Next(kwargs, &pos, &key, &value)) {
			if (!PyType_Check(value))
			{
				PyErr_Format(PyExc_AssertionError, "Def.%s: arg2 not legal type! such as: Def.rename(ENTITY_ID=int)\n", cc.optionName.c_str());
				return NULL;
			}

			PyObject* pyQualname = PyObject_GetAttrString(value, "__qualname__");
			if (!pyQualname)
			{
				PyErr_Format(PyExc_AssertionError, "Def.%s: arg2 get __qualname__ error! such as: Def.rename(ENTITY_ID=int)\n", cc.optionName.c_str());
				return NULL;
			}

			std::string typeName = PyUnicode_AsUTF8AndSize(pyQualname, NULL);
			Py_DECREF(pyQualname);

			if (!PyUnicode_Check(key))
			{
				PyErr_Format(PyExc_AssertionError, "Def.%s: arg1 must be a string! such as: Def.rename(ENTITY_ID=int)\n", cc.optionName.c_str());
				return NULL;
			}

			DefContext defContext;
			defContext.optionName = cc.optionName;
			defContext.moduleName = PyUnicode_AsUTF8AndSize(key, NULL);
			defContext.returnType = typeName;

			if (!onDefRename(defContext))
				return NULL;
		}

		S_Return;
	}

	g_callContexts.push(cc);

	// @Def.rename()
	// def ENTITY_ID() -> int: pass
	return PyCFunction_New(&__call_def_parse, self);
}

#define PY_ADD_METHOD(NAME, DOCS) APPEND_SCRIPT_MODULE_METHOD(entitydefModule, NAME, __py_def_##NAME, METH_VARARGS | METH_KEYWORDS, 0);

#ifdef interface
#undef interface
#endif

#ifdef property
#undef property
#endif

PY_DEF_HOOK(method)
PY_DEF_HOOK(clientmethod)
PY_DEF_HOOK(property)
PY_DEF_HOOK(entity)
PY_DEF_HOOK(interface)
PY_DEF_HOOK(component)
PY_DEF_HOOK(fixed_dict)
PY_DEF_HOOK(fixed_array)
PY_DEF_HOOK(fixed_item)

//-------------------------------------------------------------------------------------
bool installModule(const char* moduleName)
{
	pyDefModuleName = moduleName;

	PyObject *entitydefModule = PyImport_AddModule(pyDefModuleName.c_str());
	PyObject_SetAttrString(entitydefModule, "__doc__", PyUnicode_FromString("This module is created by KBEngine!"));

	PY_ADD_METHOD(rename, "");
	PY_ADD_METHOD(method, "");
	PY_ADD_METHOD(clientmethod, "");
	PY_ADD_METHOD(property, "");
	PY_ADD_METHOD(entity, "");
	PY_ADD_METHOD(interface, "");
	PY_ADD_METHOD(component, "");
	PY_ADD_METHOD(fixed_dict, "");
	PY_ADD_METHOD(fixed_array, "");
	PY_ADD_METHOD(fixed_item, "");

	return true;
}

//-------------------------------------------------------------------------------------
bool uninstallModule()
{
	while (!g_callContexts.empty()) g_callContexts.pop();
	g_allScriptDefContextMaps.clear();
	return true; 
}

//-------------------------------------------------------------------------------------
static bool loadAllScriptForComponentType(COMPONENT_TYPE loadComponentType)
{
	std::string rootPath = Resmgr::getSingleton().getPyUserComponentScriptsPath(loadComponentType);

	if (rootPath.size() == 0)
	{
		ERROR_MSG(fmt::format("PyEntityDef::loadAllScriptForComponentType(): get scripts path error! loadComponentType={}\n", 
			COMPONENT_NAME_EX(loadComponentType)));

		return false;
	}

	while (rootPath[rootPath.size() - 1] == '/' || rootPath[rootPath.size() - 1] == '\\') rootPath.pop_back();

	wchar_t* wpath = strutil::char2wchar((rootPath).c_str());
	std::vector<std::wstring> results;
	Resmgr::getSingleton().listPathRes(wpath, L"py|pyc", results);

	std::vector<std::wstring>::iterator iter = results.begin();
	for (; iter != results.end(); ++iter)
	{
		std::wstring wstrpath = (*iter);

		if (wstrpath.find(L"__pycache__") != std::wstring::npos)
			continue;

		if (wstrpath.find(L"__init__.") != std::wstring::npos)
			continue;

		std::pair<std::wstring, std::wstring> pathPair = script::PyPlatform::splitPath(wstrpath);
		std::pair<std::wstring, std::wstring> filePair = script::PyPlatform::splitText(pathPair.second);

		if (filePair.first.size() == 0)
			continue;

		char* cpacketPath = strutil::wchar2char(pathPair.first.c_str());
		std::string packetPath = cpacketPath;
		free(cpacketPath);

		strutil::kbe_replace(packetPath, rootPath, "");
		while (packetPath.size() > 0 && (packetPath[0] == '/' || packetPath[0] == '\\')) packetPath.erase(0, 1);
		strutil::kbe_replace(packetPath, "/", ".");
		strutil::kbe_replace(packetPath, "\\", ".");

		char* moduleName = strutil::wchar2char(filePair.first.c_str());

		// ���ڽű��ڲ����ܻ�import����ظ�import�� ���ǹ����Ѿ�import����ģ��
		if (g_allScriptDefContextMaps.find(moduleName) == g_allScriptDefContextMaps.end())
		{
			PyObject* pyModule = NULL;

			if (packetPath.size() == 0 || packetPath == "components" || packetPath == "interfaces")
			{
				pyModule = PyImport_ImportModule(const_cast<char*>(moduleName));
			}
			else
			{
				pyModule = PyImport_ImportModule(const_cast<char*>((packetPath + "." + moduleName).c_str()));
			}

			if (!pyModule)
			{
				SCRIPT_ERROR_CHECK();
				return false;
			}
			else
			{
				Py_DECREF(pyModule);
			}
		}

		free(moduleName);
	}

	free(wpath);
	return true;
}

//-------------------------------------------------------------------------------------
class Entity : public script::ScriptObject
{
	BASE_SCRIPT_HREADER(Entity, ScriptObject)
public:
	Entity(PyTypeObject* pyType = getScriptType(), bool isInitialised = true):
		ScriptObject(pyType, isInitialised) {}
	~Entity() {}
};

SCRIPT_METHOD_DECLARE_BEGIN(Entity)
SCRIPT_METHOD_DECLARE_END()

SCRIPT_MEMBER_DECLARE_BEGIN(Entity)
SCRIPT_MEMBER_DECLARE_END()

SCRIPT_GETSET_DECLARE_BEGIN(Entity)
SCRIPT_GETSET_DECLARE_END()
BASE_SCRIPT_INIT(Entity, 0, 0, 0, 0, 0)

class Proxy : public script::ScriptObject
{
	BASE_SCRIPT_HREADER(Proxy, ScriptObject)
public:
	Proxy(PyTypeObject* pyType = getScriptType(), bool isInitialised = true) :
		ScriptObject(pyType, isInitialised) {}
	~Proxy() {}
};

SCRIPT_METHOD_DECLARE_BEGIN(Proxy)
SCRIPT_METHOD_DECLARE_END()

SCRIPT_MEMBER_DECLARE_BEGIN(Proxy)
SCRIPT_MEMBER_DECLARE_END()

SCRIPT_GETSET_DECLARE_BEGIN(Proxy)
SCRIPT_GETSET_DECLARE_END()
BASE_SCRIPT_INIT(Proxy, 0, 0, 0, 0, 0)

class EntityComponent : public script::ScriptObject
{
	BASE_SCRIPT_HREADER(EntityComponent, ScriptObject)
public:
	EntityComponent(PyTypeObject* pyType = getScriptType(), bool isInitialised = true) :
		ScriptObject(pyType, isInitialised) {}
	~EntityComponent() {}
};

SCRIPT_METHOD_DECLARE_BEGIN(EntityComponent)
SCRIPT_METHOD_DECLARE_END()

SCRIPT_MEMBER_DECLARE_BEGIN(EntityComponent)
SCRIPT_MEMBER_DECLARE_END()

SCRIPT_GETSET_DECLARE_BEGIN(EntityComponent)
SCRIPT_GETSET_DECLARE_END()
BASE_SCRIPT_INIT(EntityComponent, 0, 0, 0, 0, 0)

static bool execPython(COMPONENT_TYPE componentType)
{
	std::pair<std::wstring, std::wstring> pyPaths = getComponentPythonPaths(componentType);
	if (pyPaths.first.size() == 0)
	{
		ERROR_MSG(fmt::format("PyEntityDef::execPython(): PythonApp({}) paths error!\n", COMPONENT_NAME_EX(componentType)));
		return false;
	}

	APPEND_PYSYSPATH(pyPaths.second);

	PyObject* modulesOld = PySys_GetObject("modules");

	PyThreadState* pCurInterpreter = PyThreadState_Get();
	PyThreadState* pNewInterpreter = Py_NewInterpreter();

	if (!pNewInterpreter)
	{
		ERROR_MSG(fmt::format("PyEntityDef::execPython(): Py_NewInterpreter()!\n"));
		SCRIPT_ERROR_CHECK();
		return false;
	}

	PySys_SetPath(pyPaths.second.c_str());

	PyObject* modulesNew = PySys_GetObject("modules");
	PyDict_Merge(modulesNew, Script::getSingleton().getSysInitModules(), 0);

	{
		PyObject *key, *value;
		Py_ssize_t pos = 0;

		while (PyDict_Next(modulesOld, &pos, &key, &value))
		{
			const char* typeName = PyUnicode_AsUTF8AndSize(key, NULL);

			if (std::string(typeName) == "KBEngine")
				continue;

			PyObject* pyDoc = PyObject_GetAttrString(value, "__doc__");

			if (pyDoc)
			{
				const char* doc = PyUnicode_AsUTF8AndSize(pyDoc, NULL);

				if (doc && std::string(doc).find("KBEngine") != std::string::npos)
					PyDict_SetItemString(modulesNew, typeName, value);

				if (PyErr_Occurred())
					PyErr_Clear();

				Py_XDECREF(pyDoc);
			}
			else
			{
				SCRIPT_ERROR_CHECK();
			}
		}
	}

	PyObject *m = PyImport_AddModule("__main__");

	// ���һ���ű�����ģ��
	PyObject* kbeModule = PyImport_AddModule("KBEngine");
	KBE_ASSERT(kbeModule);

	Entity::installScript(kbeModule);
	EntityComponent::installScript(kbeModule);

	if (componentType == BASEAPP_TYPE)
		Proxy::installScript(kbeModule);

	const char* componentName = COMPONENT_NAME_EX(componentType);
	if (PyModule_AddStringConstant(kbeModule, "component", componentName))
	{
		ERROR_MSG(fmt::format("PyEntityDef::execPython(): Unable to set KBEngine.component to {}\n",
			componentName));

		return false;
	}

	// ��ģ��������main
	PyObject_SetAttrString(m, "KBEngine", kbeModule);

	if (pNewInterpreter != PyThreadState_Swap(pCurInterpreter))
	{
		KBE_ASSERT(false);
		return false;
	}

	PyThreadState_Swap(pNewInterpreter);

	bool otherPartSuccess = loadAllScriptForComponentType(componentType);

	Entity::uninstallScript();
	EntityComponent::uninstallScript();

	if (componentType == BASEAPP_TYPE)
		Proxy::uninstallScript();

	if (pNewInterpreter != PyThreadState_Swap(pCurInterpreter))
	{
		KBE_ASSERT(false);
		return false;
	}

	// �˴�����ʹ��Py_EndInterpreter���ᵼ��Math��Def��ģ������
	PyInterpreterState_Clear(pNewInterpreter->interp);
	PyInterpreterState_Delete(pNewInterpreter->interp);
	return otherPartSuccess;
}

//-------------------------------------------------------------------------------------
static bool loadAllScripts()
{
	std::vector< COMPONENT_TYPE > loadOtherComponentTypes;

	if (g_componentType == CELLAPP_TYPE || g_componentType == BASEAPP_TYPE)
	{
		bool otherPartSuccess = loadAllScriptForComponentType(g_componentType);
		if (!otherPartSuccess)
			return false;

		loadOtherComponentTypes.push_back((g_componentType == BASEAPP_TYPE) ? CELLAPP_TYPE : BASEAPP_TYPE);
	}
	else
	{
		loadOtherComponentTypes.push_back(BASEAPP_TYPE);
		loadOtherComponentTypes.push_back(CELLAPP_TYPE);
	}

	for (std::vector< COMPONENT_TYPE >::iterator iter = loadOtherComponentTypes.begin(); iter != loadOtherComponentTypes.end(); ++iter)
	{
		COMPONENT_TYPE componentType = (*iter);
		
		if (!execPython(componentType))
			return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
bool initialize()
{
	if (g_inited)
		return false;

	g_inited = true;

	PyObject *entitydefModule = PyImport_AddModule(pyDefModuleName.c_str());

	ENTITYFLAGMAP::iterator iter = g_entityFlagMapping.begin();
	for (; iter != g_entityFlagMapping.end(); ++iter)
	{
		if (PyModule_AddStringConstant(entitydefModule, iter->first.c_str(), iter->first.c_str()))
		{
			ERROR_MSG(fmt::format("PyEntityDef::initialize(): Unable to set Def.{} to {}\n",
				iter->first, iter->first));

			return false;
		}
	}

	static const char* UNIQUE = "UNIQUE";
	if (PyModule_AddStringConstant(entitydefModule, UNIQUE, UNIQUE))
	{
		ERROR_MSG(fmt::format("PyEntityDef::initialize(): Unable to set Def.{} to {}\n",
			iter->first, iter->first));

		return false;
	}

	static const char* INDEX = "INDEX";
	if (PyModule_AddStringConstant(entitydefModule, INDEX, INDEX))
	{
		ERROR_MSG(fmt::format("PyEntityDef::initialize(): Unable to set Def.{} to {}\n",
			iter->first, iter->first));

		return false;
	}

	static const char* thisClass = "thisClass";
	if (PyModule_AddStringConstant(entitydefModule, thisClass, thisClass))
	{
		ERROR_MSG(fmt::format("PyEntityDef::initialize(): Unable to set Def.{} to {}\n",
			iter->first, iter->first));

		return false;
	}

	if (!loadAllScripts())
	{
		SCRIPT_ERROR_CHECK();
		return false;
	}

	if (!assemblyContexts(true))
	{
		SCRIPT_ERROR_CHECK();
		return false;
	}

	while (!g_callContexts.empty())
		g_callContexts.pop();

	g_allScriptDefContextMaps.clear();

	return true;
}

//-------------------------------------------------------------------------------------
bool finalise(bool isReload)
{
	return true;
}

//-------------------------------------------------------------------------------------
void reload(bool fullReload)
{
}

//-------------------------------------------------------------------------------------
bool initializeWatcher()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool addToStream(MemoryStream* pMemoryStream)
{
	int size = g_allScriptDefContextMaps.size();
	(*pMemoryStream) << size;

	DEF_CONTEXT_MAP::iterator iter = g_allScriptDefContextMaps.begin();
	for (; iter != g_allScriptDefContextMaps.end(); ++iter)
	{
		const std::string& name = iter->first;
		DefContext& defContext = iter->second;

		(*pMemoryStream) << name;

		defContext.addToStream(pMemoryStream);
	}

	return true;
}

//-------------------------------------------------------------------------------------
bool createFromStream(MemoryStream* pMemoryStream)
{
	int size = 0;
	(*pMemoryStream) >> size;

	for (int i = 0; i < size; ++i)
	{
		std::string name = "";
		(*pMemoryStream) >> name;

		DefContext defContext;
		defContext.createFromStream(pMemoryStream);
	}

	return true;
}

//-------------------------------------------------------------------------------------

}
}
}
