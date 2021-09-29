#define _CRT_SECURE_NO_WARNINGS

#include "Wren++.h"
#include <cstdlib> // for malloc
#include <cstring> // for strcmp, memcpy
#include <cassert>
#include <iostream>
#include <unordered_map>

namespace
{
struct BoundState
{
    std::unordered_map<std::size_t, WrenForeignMethodFn> methods{};
    std::unordered_map<std::size_t, WrenForeignClassMethods> classes{};
};

WrenForeignMethodFn foreignMethodProvider(
    WrenVM* vm,
    const char* module,
    const char* className,
    bool isStatic,
    const char* signature)
{
    auto* boundState = (BoundState*)wrenGetUserData(vm);
    auto it = boundState->methods.find(
        wrenpp::detail::hashMethodSignature(module, className, isStatic, signature));
    if (it == boundState->methods.end())
    {
        return NULL;
    }

    return it->second;
}

WrenForeignClassMethods foreignClassProvider(WrenVM* vm, const char* m, const char* c)
{
    auto* boundState = (BoundState*)wrenGetUserData(vm);
    auto it = boundState->classes.find(wrenpp::detail::hashClassSignature(m, c));
    if (it == boundState->classes.end())
    {
        return WrenForeignClassMethods{nullptr, nullptr};
    }

    return it->second;
}

inline const char* errorTypeToString(WrenErrorType type)
{
    switch (type)
    {
    case WREN_ERROR_COMPILE: return "WREN_ERROR_COMPILE";
    case WREN_ERROR_RUNTIME: return "WREN_ERROR_RUNTIME";
    case WREN_ERROR_STACK_TRACE: return "WREN_ERROR_STACK_TRACE";
    default: assert(false); return "";
    }
}

char* loadModuleFnWrapper(WrenVM* vm, const char* mod) { return wrenpp::VM::loadModuleFn(mod); }

void writeFnWrapper(WrenVM* vm, const char* text) { wrenpp::VM::writeFn(text); }

void errorFnWrapper(WrenVM*, WrenErrorType type, const char* module, int line, const char* message)
{
    wrenpp::VM::errorFn(type, module, line, message);
}

void* reallocateFnWrapper(void* memory, std::size_t newSize)
{
    return wrenpp::VM::reallocateFn(memory, newSize);
}
} // namespace

namespace wrenpp
{
namespace detail
{
void registerFunction(
    WrenVM* vm,
    const std::string& mod,
    const std::string& cName,
    bool isStatic,
    std::string sig,
    WrenForeignMethodFn function)
{
    BoundState* boundState = (BoundState*)wrenGetUserData(vm);
    std::size_t hash =
        detail::hashMethodSignature(mod.c_str(), cName.c_str(), isStatic, sig.c_str());
    boundState->methods.insert(std::make_pair(hash, function));
}

void registerClass(
    WrenVM* vm,
    const std::string& mod,
    std::string cName,
    WrenForeignClassMethods methods)
{
    BoundState* boundState = (BoundState*)wrenGetUserData(vm);
    std::size_t hash = detail::hashClassSignature(mod.c_str(), cName.c_str());
    boundState->classes.insert(std::make_pair(hash, methods));
}
} // namespace detail

Value null = Value();

Value::Value(bool val) : type_{WREN_TYPE_BOOL}, string_{nullptr} { set(val); }

Value::Value(float val) : type_{WREN_TYPE_NUM}, string_{nullptr} { set(val); }

Value::Value(double val) : type_{WREN_TYPE_NUM}, string_{nullptr} { set(val); }

Value::Value(int val) : type_{WREN_TYPE_NUM}, string_{nullptr} { set(val); }

Value::Value(unsigned int val) : type_{WREN_TYPE_NUM}, string_{nullptr} { set(val); }

Value::Value(const char* str) : type_{WREN_TYPE_STRING}, string_{nullptr}
{
    string_ = (char*)VM::reallocateFn(nullptr, std::strlen(str) + 1);
    std::strcpy(string_, str);
}

Value::~Value()
{
    if (string_)
    {
        VM::reallocateFn(string_, 0u);
    }
}

Method::Method(VM* vm, WrenHandle* variable, WrenHandle* method)
    : vm_(vm), method_(method), variable_(variable)
{
}

Method::Method(Method&& other) : vm_(other.vm_), method_(other.method_), variable_(other.variable_)
{
    other.vm_ = nullptr;
    other.method_ = nullptr;
    other.variable_ = nullptr;
}

Method::~Method()
{
    if (vm_)
    {
        assert(method_ && variable_);
        wrenReleaseHandle(vm_->ptr(), method_);
        wrenReleaseHandle(vm_->ptr(), variable_);
    }
}

Method& Method::operator=(Method&& rhs)
{
    if (&rhs != this)
    {
        vm_ = rhs.vm_;
        method_ = rhs.method_;
        variable_ = rhs.variable_;
        rhs.vm_ = nullptr;
        rhs.method_ = nullptr;
        rhs.variable_ = nullptr;
    }

    return *this;
}

ClassContext ModuleContext::beginClass(std::string c) { return ClassContext(c, *this); }

void ModuleContext::endModule() {}

ModuleContext& ClassContext::endClass() { return module_; }

ClassContext::ClassContext(std::string c, ModuleContext& mod) : module_(mod), class_(c) {}

ClassContext& ClassContext::bindCFunction(
    bool isStatic,
    std::string signature,
    WrenForeignMethodFn function)
{
    detail::registerFunction(module_.vm_, module_.name_, class_, isStatic, signature, function);
    return *this;
}

/*
 * Returns the source as a heap-allocated string.
 * Uses malloc, because our reallocateFn is set to default:
 * it uses malloc, realloc and free.
 * */
LoadModuleFn VM::loadModuleFn = [](const char* mod) -> char* {
    std::string path(mod);
    path += ".wren";
    std::string source;
    try
    {
        source = wrenpp::detail::fileToString(path);
    }
    catch (const std::exception&)
    {
        return NULL;
    }
    char* buffer = (char*)malloc(source.size());
    assert(buffer != nullptr);
    memcpy(buffer, source.c_str(), source.size());
    return buffer;
};

WriteFn VM::writeFn = [](const char* text) -> void { std::cout << text; };

ErrorFn VM::errorFn =
    [](WrenErrorType type, const char* module_name, int line, const char* message) -> void {
    const char* typeStr = errorTypeToString(type);
    if (module_name)
    {
        std::cout << typeStr << " in " << module_name << ":" << line << "> " << message
                  << std::endl;
    }
    else
    {
        std::cout << typeStr << "> " << message << std::endl;
    }
};

ReallocateFn VM::reallocateFn = std::realloc;

std::size_t VM::initialHeapSize = 0xA00000u;

std::size_t VM::minHeapSize = 0x100000u;

int VM::heapGrowthPercent = 50;

std::size_t VM::chunkSize = 0x500000u;

VM::VM() : vm_{nullptr}
{
    WrenConfiguration configuration{};
    wrenInitConfiguration(&configuration);
    //configuration.reallocateFn = reallocateFnWrapper;
    configuration.initialHeapSize = initialHeapSize;
    configuration.minHeapSize = minHeapSize;
    configuration.heapGrowthPercent = heapGrowthPercent;
    configuration.bindForeignMethodFn = foreignMethodProvider;
    //configuration.loadModuleFn = loadModuleFnWrapper;
    configuration.bindForeignClassFn = foreignClassProvider;
    configuration.writeFn = writeFnWrapper;
    configuration.errorFn = errorFnWrapper;
    configuration.userData = new BoundState();

    vm_ = wrenNewVM(&configuration);
}

VM::VM(VM&& other) : vm_{other.vm_} { other.vm_ = nullptr; }

VM& VM::operator=(VM&& rhs)
{
    if (&rhs != this)
    {
        vm_ = rhs.vm_;
        rhs.vm_ = nullptr;
    }
    return *this;
}

VM::~VM()
{
    if (vm_ != nullptr)
    {
        delete (BoundState*)wrenGetUserData(vm_);
        wrenFreeVM(vm_);
    }
}

Result VM::executeModule(const std::string& mod)
{
    const std::string source(loadModuleFn(mod.c_str()));
    auto res = wrenInterpret(vm_, mod.c_str(), source.c_str());

    if (res == WrenInterpretResult::WREN_RESULT_COMPILE_ERROR)
    {
        return Result::CompileError;
    }

    if (res == WrenInterpretResult::WREN_RESULT_RUNTIME_ERROR)
    {
        return Result::RuntimeError;
    }

    return Result::Success;
}

Result VM::executeString(const std::string& code)
{
    auto res = wrenInterpret(vm_, "main", code.c_str());

    if (res == WrenInterpretResult::WREN_RESULT_COMPILE_ERROR)
    {
        return Result::CompileError;
    }

    if (res == WrenInterpretResult::WREN_RESULT_RUNTIME_ERROR)
    {
        return Result::RuntimeError;
    }

    return Result::Success;
}

void VM::collectGarbage() { wrenCollectGarbage(vm_); }

Method VM::method(const std::string& mod, const std::string& var, const std::string& sig)
{
    wrenEnsureSlots(vm_, 1);
    wrenGetVariable(vm_, mod.c_str(), var.c_str(), 0);
    WrenHandle* variable = wrenGetSlotHandle(vm_, 0);
    WrenHandle* handle = wrenMakeCallHandle(vm_, sig.c_str());
    return Method(this, variable, handle);
}

ModuleContext VM::beginModule(std::string name) { return ModuleContext(vm_, name); }
} // namespace wrenpp
