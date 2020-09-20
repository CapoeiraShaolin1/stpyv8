#pragma once

#include <cassert>
#include <memory>
// #include <functional>
#include "Isolate.h"
#include "Platform.h"
#include "Wrapper.h"
#include "Utils.h"


class CContext;
class CIsolate;

typedef std::shared_ptr<CContext> CContextPtr;
typedef std::shared_ptr<CIsolate> CIsolatePtr;

class CContext final
{
    py::object m_global;
    v8::Persistent<v8::Context> m_context;
/*
private: // Embeded Data
  enum EmbedderDataFields
  {
    DebugIdIndex = v8::Context::kDebugIdIndex,
    LoggerIndex,
    GlobalObjectIndex,
  };

  template <typename T>
  static T *GetEmbedderData(v8::Handle<v8::Context> context, EmbedderDataFields index, std::function<T *()> creator = nullptr)
  {
    assert(!context.IsEmpty());
    assert(index > DebugIdIndex);

    auto value = static_cast<T *>(v8::Handle<v8::External>::Cast(context->GetEmbedderData(index))->Value());

    if (!value && creator)
    {
      value = creator();

      SetEmbedderData(context, index, value);
    }

    return value;
  }

  template <typename T>
  static void SetEmbedderData(v8::Handle<v8::Context> context, EmbedderDataFields index, T *data)
  {
    assert(!context.IsEmpty());
    assert(index > DebugIdIndex);
    assert(data);

    context->SetEmbedderData(index, v8::External::New(context->GetIsolate(), (void *)data));
  }

  static logger_t &GetLogger(v8::Handle<v8::Context> context);

  logger_t &logger(v8::Isolate *isolate = v8::Isolate::GetCurrent())
  {
    v8::HandleScope handle_scope(isolate);

    auto context = m_context.Get(isolate);

    return GetLogger(context);
  }
*/
public:
    CContext(v8::Handle<v8::Context> context, v8::Isolate *isolate = v8::Isolate::GetCurrent());
    CContext(const CContext &context, v8::Isolate *isolate = v8::Isolate::GetCurrent());
    CContext(py::object global, py::list extensions, v8::Isolate *isolate = v8::Isolate::GetCurrent());

    ~CContext()
    { 
        m_context.Reset();
    }
    /*
    v8::Handle<v8::Context> Context(v8::Isolate *isolate = v8::Isolate::GetCurrent()) const {
        return m_context.Get(isolate);
    }
    */
    v8::Handle<v8::Context> Handle(void) const {
        return v8::Local<v8::Context>::New(v8::Isolate::GetCurrent(), m_context);
    }

    py::object GetGlobal(void);

    py::str GetSecurityToken(void);
    void SetSecurityToken(py::str token);

    bool IsEntered(void) {
        return !m_context.IsEmpty();
    }
    void Enter(void) {
        v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
        Handle()->Enter();
    }
    void Leave(void) {
        v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
        Handle()->Exit();
    }

    py::object Evaluate(const std::string& src, const std::string name = std::string(), int line = -1, int col = -1);
    py::object EvaluateW(const std::wstring& src, const std::wstring name = std::wstring(), int line = -1, int col = -1);

    static py::object GetEntered(void);
    static py::object GetCurrent(void);
    static py::object GetCalling(void);

    static bool InContext(void) {
        return v8::Isolate::GetCurrent()->InContext();
    }

    static void Expose(void);
};
