#include "Engine.h"
#include "Exception.h"
#include "Wrapper.h"

#include <iostream>
#include <string.h>

#include <boost/preprocessor.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

void CEngine::Expose(void)
{
    py::class_<CEngine, boost::noncopyable>("JSEngine", "JSEngine is a backend Javascript engine.")
    .def(py::init<>("Create a new script engine instance."))
    .add_static_property("version", &CEngine::GetVersion,
                         "Get the V8 engine version.")
    .add_static_property("boost", &CEngine::GetBoostVersion,
                         "Get the boost version.")
    .add_static_property("dead", &CEngine::IsDead,
                         "Check if V8 is dead and therefore unusable.")

    .def("setFlags", &CEngine::SetFlags, "Sets V8 flags from a string.")
    .staticmethod("setFlags")

    .def("terminateAllThreads", &CEngine::TerminateAllThreads,
         "Forcefully terminate the current thread of JavaScript execution.")
    .staticmethod("terminateAllThreads")

    .def("dispose", &v8::V8::Dispose,
         "Releases any resources used by v8 and stops any utility threads "
         "that may be running. Note that disposing v8 is permanent, "
         "it cannot be reinitialized.")
    .staticmethod("dispose")

    .def("lowMemory", &v8::Isolate::LowMemoryNotification,
         "Optional notification that the system is running low on memory.")
    .staticmethod("lowMemory")

    /*
        .def("setMemoryLimit", &CEngine::SetMemoryLimit, (py::arg("max_young_space_size") = 0,
                                                          py::arg("max_old_space_size") = 0,
                                                          py::arg("max_executable_size") = 0),
             "Specifies the limits of the runtime's memory use."
             "You must set the heap size before initializing the VM"
             "the size cannot be adjusted after the VM is initialized.")
        .staticmethod("setMemoryLimit")
    */

    .def("setStackLimit", &CEngine::SetStackLimit, (py::arg("stack_limit_size") = 0),
         "Uses the address of a local variable to determine the stack top now."
         "Given a size, returns an address that is that far from the current top of stack.")
    .staticmethod("setStackLimit")

    /*
        .def("setMemoryAllocationCallback", &MemoryAllocationManager::SetCallback,
                                            (py::arg("callback"),
                                             py::arg("space") = v8::kObjectSpaceAll,
                                             py::arg("action") = v8::kAllocationActionAll),
                                            "Enables the host application to provide a mechanism to be notified "
                                            "and perform custom logging when V8 Allocates Executable Memory.")
        .staticmethod("setMemoryAllocationCallback")
    */

    .def("compile", &CEngine::Compile, (py::arg("source"),
                                        py::arg("name") = std::string(),
                                        py::arg("line") = -1,
                                        py::arg("col") = -1))
    .def("compile", &CEngine::CompileW, (py::arg("source"),
                                         py::arg("name") = std::wstring(),
                                         py::arg("line") = -1,
                                         py::arg("col") = -1))
    ;

  py::class_<CScript, boost::noncopyable>("JSScript", "JSScript is a compiled JavaScript script.", py::no_init)
    .add_property("source", &CScript::GetSource, "the source code")

    .def("run", &CScript::Run, "Execute the compiled code.")
    ;

    py::objects::class_value_wrapper<std::shared_ptr<CScript>,
    py::objects::make_ptr_instance<CScript,
    py::objects::pointer_holder<std::shared_ptr<CScript>, CScript> > >();

#ifdef SUPPORT_EXTENSION

  py::class_<CExtension, boost::noncopyable>("JSExtension", "JSExtension is a reusable script module.", py::no_init)
    .def(py::init<const std::string&, const std::string&, py::object, py::list, bool>((py::arg("name"),
                                                                                       py::arg("source"),
                                                                                       py::arg("callback") = py::object(),
                                                                                       py::arg("dependencies") = py::list(),
                                                                                       py::arg("register") = true)))
    .add_static_property("extensions", &CExtension::GetExtensions)
    


    .add_property("name", &CExtension::GetName, "The name of extension")
    .add_property("source", &CExtension::GetSource, "The source code of extension")
    .add_property("dependencies", &CExtension::GetDependencies, "The extension dependencies which will be load before this extension")
    .add_property("autoEnable", &CExtension::IsAutoEnable, &CExtension::SetAutoEnable, "Enable the extension by default.")
    .add_property("registered", &CExtension::IsRegistered, "The extension has been registerd")
    .def("register", &CExtension::Register, "Register the extension")


    .add_static_property("extensionslists", &CExtension::GetExtensionsLists, "lists of [name, source, auto_enable]")
    
    .add_property("extensionssize", &CExtension::GetExtensionsSize, "number of registered extensions")
    .add_property("extensionsdeconstruct", &CExtension::deconstructExtensionsAll)
    ;

#endif
}

bool CEngine::IsDead(void)
{
    return v8::Isolate::GetCurrent()->IsDead();
}

void CEngine::TerminateAllThreads(void)
{
    v8::Isolate::GetCurrent()->TerminateExecution();
}

void CEngine::ReportFatalError(const char* location, const char* message)
{
    std::cerr << "<" << location << "> " << message << std::endl;
}

void CEngine::ReportMessage(v8::Handle<v8::Message> message, v8::Handle<v8::Value> data)
{
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    v8::String::Utf8Value filename(isolate, message->GetScriptResourceName());
    int lineno = message->GetLineNumber(context).ToChecked();
    v8::String::Utf8Value sourceline(isolate, message->GetSourceLine(context).ToLocalChecked());

    std::cerr << *filename << ":" << lineno << " -> " << *sourceline << std::endl;
}

bool CEngine::SetMemoryLimit(int max_young_space_size, int max_old_space_size, int max_executable_size)
{
    v8::ResourceConstraints limit;

    if (max_young_space_size) limit.set_max_young_generation_size_in_bytes(max_young_space_size);
    if (max_old_space_size) limit.set_max_old_generation_size_in_bytes(max_old_space_size);
    //TODO should this be code range size instead?
    //if (max_executable_size) limit.set_max_executable_size(max_executable_size);

    //TODO - memory limits are now only settable on isolate creation
    //return v8::SetResourceConstraints(v8::Isolate::GetCurrent(), &limit);
    return false;
}

void CEngine::SetStackLimit(uintptr_t stack_limit_size)
{
    // This function uses a local stack variable to determine the isolate's
    // stack limit
    uint32_t here;
    uintptr_t stack_limit = reinterpret_cast<uintptr_t>(&here) - stack_limit_size;

    // If the size is very large and the stack is near the bottom of memory
    // then the calculation above may wrap around and give an address that is
    // above the (downwards-growing) stack. In such case we alert the user
    // that the new stack limit is not going to be set and return
    if (stack_limit > reinterpret_cast<uintptr_t>(&here)) {
        std::cerr << "[ERROR] Attempted to set a stack limit greater than available memory" << std::endl;
        return;
    }

    v8::Isolate::GetCurrent()->SetStackLimit(stack_limit);
}

std::shared_ptr<CScript> CEngine::InternalCompile(v8::Handle<v8::String> src,
        v8::Handle<v8::Value> name,
        int line, int col)
{
  v8::HandleScope handle_scope(m_isolate);
  v8::TryCatch try_catch(m_isolate);
  /*
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);
  */

  v8::Persistent<v8::String> script_source(m_isolate, src);

  v8::MaybeLocal<v8::Script> script;

  v8::Local<v8::Integer> line_offset, column_offset;

  if (line >= 0) line_offset = v8::Integer::New(m_isolate, line);
  if (col >= 0) column_offset = v8::Integer::New(m_isolate, col);

  Py_BEGIN_ALLOW_THREADS

  v8::ScriptOrigin script_origin(name, line_offset, column_offset);
  v8::ScriptCompiler::Source source(src, script_origin);

  script = v8::ScriptCompiler::Compile(m_isolate->GetCurrentContext(), &source);

  Py_END_ALLOW_THREADS

  if (script.IsEmpty()) CJavascriptException::ThrowIf(m_isolate, try_catch);

  return std::shared_ptr<CScript>(new CScript(m_isolate, *this, src, script.ToLocalChecked()));
}

py::object CEngine::ExecuteScript(v8::Handle<v8::Script> script)
{
  v8::HandleScope handle_scope(m_isolate);
  v8::Local<v8::Context> context = m_isolate->GetCurrentContext();
  v8::TryCatch try_catch(m_isolate);  
  /*
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);
  */
  v8::MaybeLocal<v8::Value> result;

  Py_BEGIN_ALLOW_THREADS

  result = script->Run(context);

  Py_END_ALLOW_THREADS

  if (result.IsEmpty())
  {
    if (try_catch.HasCaught())
    {
      if(!try_catch.CanContinue() && PyErr_OCCURRED())
      {
        throw py::error_already_set();
      }

      CJavascriptException::ThrowIf(m_isolate, try_catch);
    }

    result = v8::Null(m_isolate);
  }

  return CJavascriptObject::Wrap(result.ToLocalChecked());
}



const std::string CScript::GetSource(void) const
{
  v8::HandleScope handle_scope(m_isolate);

  v8::String::Utf8Value source(m_isolate, Source());

  return std::string(*source, source.length());
}

py::object CScript::Run(void)
{
  v8::HandleScope handle_scope(m_isolate);

  return m_engine.ExecuteScript(Script());
}

#ifdef SUPPORT_EXTENSION

class CPythonExtension : public v8::Extension
{
  py::object m_callback;

  static void CallStub(const v8::FunctionCallbackInfo<v8::Value>& args)
  {
    v8::HandleScope handle_scope(args.GetIsolate());
    CPythonGIL python_gil;
    py::object func = *static_cast<py::object *>(v8::External::Cast(*args.Data())->Value());

    py::object result;

    switch (args.Length())
    {
    case 0: result = func(); break;
    case 1: result = func(CJavascriptObject::Wrap(args[0])); break;
    case 2: result = func(CJavascriptObject::Wrap(args[0]), CJavascriptObject::Wrap(args[1])); break;
    case 3: result = func(CJavascriptObject::Wrap(args[0]), CJavascriptObject::Wrap(args[1]),
                          CJavascriptObject::Wrap(args[2])); break;
    case 4: result = func(CJavascriptObject::Wrap(args[0]), CJavascriptObject::Wrap(args[1]),
                          CJavascriptObject::Wrap(args[2]), CJavascriptObject::Wrap(args[3])); break;
    case 5: result = func(CJavascriptObject::Wrap(args[0]), CJavascriptObject::Wrap(args[1]),
                          CJavascriptObject::Wrap(args[2]), CJavascriptObject::Wrap(args[3]),
                          CJavascriptObject::Wrap(args[4])); break;
    case 6: result = func(CJavascriptObject::Wrap(args[0]), CJavascriptObject::Wrap(args[1]),
                          CJavascriptObject::Wrap(args[2]), CJavascriptObject::Wrap(args[3]),
                          CJavascriptObject::Wrap(args[4]), CJavascriptObject::Wrap(args[5])); break;
    case 7: result = func(CJavascriptObject::Wrap(args[0]), CJavascriptObject::Wrap(args[1]),
                          CJavascriptObject::Wrap(args[2]), CJavascriptObject::Wrap(args[3]),
                          CJavascriptObject::Wrap(args[4]), CJavascriptObject::Wrap(args[5]),
                          CJavascriptObject::Wrap(args[6])); break;
    case 8: result = func(CJavascriptObject::Wrap(args[0]), CJavascriptObject::Wrap(args[1]),
                          CJavascriptObject::Wrap(args[2]), CJavascriptObject::Wrap(args[3]),
                          CJavascriptObject::Wrap(args[4]), CJavascriptObject::Wrap(args[5]),
                          CJavascriptObject::Wrap(args[7]), CJavascriptObject::Wrap(args[8])); break;
    default:
      args.GetIsolate()->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(args.GetIsolate(), "too many arguments").ToLocalChecked()));
      break;
    }

    if (result.is_none()) {
      args.GetReturnValue().SetNull();
    } else if (result.ptr() == Py_True) {
      args.GetReturnValue().Set(true);
    } else if (result.ptr() == Py_False) {
      args.GetReturnValue().Set(false);
    } else {
      args.GetReturnValue().Set(CPythonObject::Wrap(result));
    }
  }
public:
  CPythonExtension(const char *name, const char *source, py::object callback, int dep_count, const char**deps)
    : v8::Extension(name, source, dep_count, deps), m_callback(callback)
  {

  }

  virtual v8::Handle<v8::FunctionTemplate> GetNativeFunctionTemplate(v8::Isolate* isolate, v8::Handle<v8::String> name)
  {
    v8::EscapableHandleScope handle_scope(isolate);
    CPythonGIL python_gil;

    py::object func;
    v8::String::Utf8Value func_name(isolate, name);
    std::string func_name_str(*func_name, func_name.length());

    try
    {
      if (::PyCallable_Check(m_callback.ptr()))
      {
        func = m_callback(func_name_str);
      }
      else if (::PyObject_HasAttrString(m_callback.ptr(), *func_name))
      {
        func = m_callback.attr(func_name_str.c_str());
      }
      else
      {
        return v8::Handle<v8::FunctionTemplate>();
      }
    }
    catch (const std::exception& ex) { isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(isolate, ex.what()).ToLocalChecked())); }
    catch (const py::error_already_set&) { CPythonObject::ThrowIf(isolate); }
    catch (...) { isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(isolate, "unknown exception").ToLocalChecked())); }

    v8::Handle<v8::External> func_data = v8::External::New(isolate, new py::object(func));
    v8::Handle<v8::FunctionTemplate> func_tmpl = v8::FunctionTemplate::New(isolate, CallStub, func_data);

    return handle_scope.Escape(v8::Local<v8::FunctionTemplate>(func_tmpl));
  }
};

// std::vector< boost::shared_ptr<v8::Extension> > CExtension::s_extensions;

CExtension::CExtension(const std::string& name, const std::string& source,
                       py::object callback, py::list deps, bool autoRegister)
  : m_name(name), m_source(source), m_deps(deps), m_registered(false)
{
  for (Py_ssize_t i=0; i<PyList_Size(deps.ptr()); i++)
  {
    py::extract<const std::string> extractor(::PyList_GetItem(deps.ptr(), i));

    if (extractor.check())
    {
      m_depNames.push_back(extractor());
      m_depPtrs.push_back(m_depNames.rbegin()->c_str());
    }
  }

  m_extension.reset(new CPythonExtension(m_name.c_str(), m_source.c_str(),
    callback, m_depPtrs.size(), m_depPtrs.empty() ? NULL : &m_depPtrs[0]));

  m_extension_unique.reset(new CPythonExtension(m_name.c_str(), m_source.c_str(),
    callback, m_depPtrs.size(), m_depPtrs.empty() ? NULL : &m_depPtrs[0]));

  if (autoRegister) Register();
}

void CExtension::Register(void)
{
  v8::RegisteredExtension *ext = v8::RegisteredExtension::first_extension();
  py::list extensions;

  // m_registered = false;

  if ((m_extension) && (m_extension_unique))
  {
    std::string n = m_extension->name();
    if(n.size())  
    {
      while (ext)
      {
        if (n.compare((std::string) ext->extension()->name()) == 0)
        {
          // same name has been already existed.
          m_registered = true;
          return; 
        }
        ext = ext->next();
      }  

      v8::RegisterExtension(std::move(m_extension_unique));

      m_registered = true;

      // s_extensions.push_back(m_extension);
    }
  }
}

void CExtension::SetAutoEnable(bool value)
{
  v8::RegisteredExtension *ext = v8::RegisteredExtension::first_extension();
  py::list extensions;

  if (m_extension)
  {
    std::string n = m_extension->name();
    if(n.size())  
    {
      while (ext)
      {
        if (n.compare((std::string) ext->extension()->name()) == 0)
        {
          m_extension->set_auto_enable(value);
          ext->extension()->set_auto_enable(value);
          break;
        }
        ext = ext->next();
      }  
    }
  }
}


bool CExtension::IsAutoEnable(void)
{
  v8::RegisteredExtension *ext = v8::RegisteredExtension::first_extension();

  bool m_auto_enable = false;
  if (m_extension)
  {
    std::string n = m_extension->name();
    if(n.size())
    {
      while (ext)
      {
        if (n.compare((std::string) ext->extension()->name()) == 0)
        {
          m_auto_enable = ext->extension()->auto_enable();
          break;
        }
        ext = ext->next();
      }  
    }
  }
  return m_auto_enable;
}


py::list CExtension::GetExtensions(void)
{
  v8::RegisteredExtension *ext = v8::RegisteredExtension::first_extension();
  py::list extensions;

  while (ext)
  {
    // if (ext->extension()->name())
    std::string n = ext->extension()->name();
    if(n.size())
    {
      extensions.append(ext->extension()->name());
    }
    ext = ext->next();
  }

  return extensions;
}

py::list CExtension::GetExtensionsSize(void)
{
  v8::RegisteredExtension *ext = v8::RegisteredExtension::first_extension();
  py::list extensions;
  int nmsize = 0;

  while (ext)
  {
    // if (ext->extension()->name())
    std::string n = ext->extension()->name();
    if(n.size())
    {
      ++nmsize;
    }
    ext = ext->next();
  }

  extensions.append(nmsize);
  return extensions;
}

py::list CExtension::GetExtensionsLists(void)
{
  v8::RegisteredExtension *ext = v8::RegisteredExtension::first_extension();
  py::list extensions;

  while (ext)
  {
    std::string n = ext->extension()->name();
    if(n.size())
    {
      py::list u_extensions;        
      u_extensions.append(ext->extension()->name());
      u_extensions.append(ext->extension()->source()->data());
      u_extensions.append(ext->extension()->auto_enable());

      extensions.append(u_extensions);
      }

    ext = ext->next();
  }
  return extensions;
}

#endif // SUPPORT_EXTENSION