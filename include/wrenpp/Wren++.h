#define _CRT_SECURE_NO_WARNINGS
#ifndef WRENPP_H_INCLUDED
#define WRENPP_H_INCLUDED

extern "C" {
#include <wren/include/wren.h>
}
#include <string>
#include <functional> // for std::hash
#include <cassert>
#include <cstdint>
#include <cstdlib> // for std::size_t
#include <cstring> // for memcpy, strcpy
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <vector>
#include <type_traits>

namespace wrenpp
{

using LoadModuleFn = std::function<char*(const char*)>;
using WriteFn = std::function<void(const char*)>;
using ReallocateFn = std::function<void*(void*, std::size_t)>;
using ErrorFn = std::function<void(WrenErrorType, const char*, int, const char*)>;

namespace detail
{

/***
 *     ______              ____   __
 *    /_  __/_ _____  ___ /  _/__/ /
 *     / / / // / _ \/ -_)/ // _  /
 *    /_/  \_, / .__/\__/___/\_,_/
 *        /___/_/
 */

inline uint32_t& typeId()
{
    static uint32_t id{0u};
    return id;
}

template<typename T>
uint32_t getTypeIdImpl()
{
    static uint32_t id = typeId()++;
    return id;
}

template<typename T>
uint32_t getTypeId()
{
    return getTypeIdImpl<std::decay_t<T>>();
}

/***
 *       ____             _                  __     _         __
 *      / __/__  _______ (_)__ ____    ___  / /    (_)__ ____/ /_
 *     / _// _ \/ __/ -_) / _ `/ _ \  / _ \/ _ \  / / -_) __/ __/
 *    /_/  \___/_/  \__/_/\_, /_//_/  \___/_.__/_/ /\__/\__/\__/
 *                       /___/                |___/
 */

inline std::vector<std::string>& classNameStorage()
{
    static std::vector<std::string> classNames{};
    return classNames;
}

inline std::vector<std::string>& moduleNameStorage()
{
    static std::vector<std::string> moduleNames{};
    return moduleNames;
}

inline void storeClassName(std::uint32_t index, const std::string& className)
{
    assert(classNameStorage().size() == index);
    classNameStorage().push_back(className);
}

inline void storeModuleName(std::uint32_t index, const std::string& moduleName)
{
    assert(moduleNameStorage().size() == index);
    moduleNameStorage().push_back(moduleName);
}

template<typename T>
void bindTypeToClassName(const std::string& className)
{
    std::uint32_t id = getTypeId<T>();
    storeClassName(id, className);
}

template<typename T>
void bindTypeToModuleName(const std::string& moduleName)
{
    std::uint32_t id = getTypeId<T>();
    storeModuleName(id, moduleName);
}

template<typename T>
const char* getWrenClassString()
{
    std::uint32_t id = getTypeId<T>();
    assert(id < classNameStorage().size());
    return classNameStorage()[id].c_str();
}

template<typename T>
const char* getWrenModuleString()
{
    std::uint32_t id = getTypeId<T>();
    assert(id < moduleNameStorage().size());
    return moduleNameStorage()[id].c_str();
}

/*
 * The interface for getting the object pointer. The actual C++ object may lie within the Wren
 * object, or may live in C++.
 */
class ForeignObject
{
public:
    virtual ~ForeignObject() = default;
    virtual void* objectPtr() = 0;
};

/*
 * This wraps a class object by value. The lifetimes of these objects are managed in Wren.
 */
template<typename T>
class ForeignObjectValue : public ForeignObject
{
public:
    ForeignObjectValue() : data_() {}

    virtual ~ForeignObjectValue()
    {
        T* obj = static_cast<T*>(objectPtr());
        obj->~T();
    }

    void* objectPtr() override { return &data_; }

    template<typename... Args>
    static void setInSlot(WrenVM* vm, int slot, Args... arg)
    {
        wrenEnsureSlots(vm, slot + 1);
        wrenGetVariable(vm, getWrenModuleString<T>(), getWrenClassString<T>(), slot);
        ForeignObjectValue<T>* val =
            new (wrenSetSlotNewForeign(vm, slot, slot, sizeof(ForeignObjectValue<T>)))
                ForeignObjectValue<T>();
        new (val->objectPtr()) T{std::forward<Args>(arg)...};
    }

private:
    typename std::aligned_storage<sizeof(T), alignof(T)>::type data_;
};

/*
 * Wraps a pointer to a class object. The lifetimes of the pointed-to objects are managed by the
 * host program.
 */
template<typename T>
class ForeignObjectPtr : public ForeignObject
{
public:
    explicit ForeignObjectPtr(T* object) : object_{object} {}
    virtual ~ForeignObjectPtr() = default;

    void* objectPtr() override { return object_; }

    static void setInSlot(WrenVM* vm, int slot, T* obj)
    {
        wrenEnsureSlots(vm, slot + 1);
        wrenGetVariable(vm, getWrenModuleString<T>(), getWrenClassString<T>(), slot);
        void* bytes = wrenSetSlotNewForeign(vm, slot, slot, sizeof(ForeignObjectPtr<T>));
        new (bytes) ForeignObjectPtr<T>{obj};
    }

private:
    T* object_;
};

/***
 *       ____             _                       __  __           __
 *      / __/__  _______ (_)__ ____    __ _  ___ / /_/ /  ___  ___/ /
 *     / _// _ \/ __/ -_) / _ `/ _ \  /  ' \/ -_) __/ _ \/ _ \/ _  /
 *    /_/  \___/_/  \__/_/\_, /_//_/ /_/_/_/\__/\__/_//_/\___/\_,_/
 *                       /___/
 */

// given a Wren method signature, this returns a unique value
inline std::size_t hashMethodSignature(
    const char* module,
    const char* className,
    bool isStatic,
    const char* signature)
{
    std::hash<std::string> hash;
    std::string qualified(module);
    qualified += className;
    qualified += signature;
    if (isStatic)
    {
        qualified += 's';
    }
    return hash(qualified);
}

template<typename F>
struct FunctionTraits;

template<typename R, typename... Args>
struct FunctionTraits<R(Args...)>
{
    using ReturnType = R;

    constexpr static const std::size_t Arity = sizeof...(Args);

    template<std::size_t N>
    struct Argument
    {
        static_assert(N < Arity, "FunctionTraits error: invalid argument index parameter");
        using Type = std::tuple_element_t<N, std::tuple<Args...>>;
    };

    template<std::size_t N>
    using ArgumentType = typename Argument<N>::Type;
};

template<typename... Args>
struct ParameterPackTraits
{

    constexpr static const std::size_t size = sizeof...(Args);

    template<std::size_t N>
    struct Parameter
    {
        static_assert(N < size, "ParameterPackTraits error: invalid parameter index");
        using Type = std::tuple_element_t<N, std::tuple<Args...>>;
    };

    template<std::size_t N>
    using ParameterType = typename Parameter<N>::Type;
};

template<typename R, typename... Args>
struct FunctionTraits<R (*)(Args...)> : public FunctionTraits<R(Args...)>
{
};

template<typename R, typename... Args>
struct FunctionTraits<R (&)(Args...)> : public FunctionTraits<R(Args...)>
{
};

// member function pointer
template<typename C, typename R, typename... Args>
struct FunctionTraits<R (C::*)(Args...)> : public FunctionTraits<R(Args...)>
{
};

// const member function pointer
template<typename C, typename R, typename... Args>
struct FunctionTraits<R (C::*)(Args...) const> : public FunctionTraits<R(Args...)>
{
};

template<typename T>
struct WrenSlotAPI
{
    static T get(WrenVM* vm, int slot)
    {
        ForeignObject* obj = static_cast<ForeignObject*>(wrenGetSlotForeign(vm, slot));
        return *static_cast<T*>(obj->objectPtr());
    }

    static void set(WrenVM* vm, int slot, T t) { ForeignObjectValue<T>::setInSlot(vm, slot, t); }
};

template<typename T>
struct WrenSlotAPI<T&>
{
    static T& get(WrenVM* vm, int slot)
    {
        ForeignObject* obj = static_cast<ForeignObject*>(wrenGetSlotForeign(vm, slot));
        return *static_cast<T*>(obj->objectPtr());
    }

    static void set(WrenVM* vm, int slot, T& t) { ForeignObjectPtr<T>::setInSlot(vm, slot, &t); }
};

template<typename T>
struct WrenSlotAPI<const T&>
{
    static const T& get(WrenVM* vm, int slot)
    {
        ForeignObject* obj = static_cast<ForeignObject*>(wrenGetSlotForeign(vm, slot));
        return *static_cast<T*>(obj->objectPtr());
    }

    static void set(WrenVM* vm, int slot, const T& t)
    {
        ForeignObjectPtr<T>::setInSlot(vm, 0, const_cast<T*>(&t));
    }
};

template<typename T>
struct WrenSlotAPI<T*>
{
    static T* get(WrenVM* vm, int slot)
    {
        ForeignObject* obj = static_cast<ForeignObject*>(wrenGetSlotForeign(vm, slot));
        return static_cast<T*>(obj->objectPtr());
    }

    static void set(WrenVM* vm, int slot, T* t) { ForeignObjectPtr<T>::setInSlot(vm, slot, t); }
};

template<typename T>
struct WrenSlotAPI<const T*>
{
    static const T* get(WrenVM* vm, int slot)
    {
        ForeignObject* obj = static_cast<ForeignObject*>(wrenGetSlotForeign(vm, slot));
        return static_cast<const T*>(obj->objectPtr());
    }

    static void set(WrenVM* vm, int slot, const T* t)
    {
        ForeignObjectPtr<T>::setInSlot(vm, slot, const_cast<T*>(t));
    }
};

template<>
struct WrenSlotAPI<float>
{
    static float get(WrenVM* vm, int slot) { return float(wrenGetSlotDouble(vm, slot)); }

    static void set(WrenVM* vm, int slot, float val) { wrenSetSlotDouble(vm, slot, double(val)); }
};

template<>
struct WrenSlotAPI<double>
{
    static double get(WrenVM* vm, int slot) { return wrenGetSlotDouble(vm, slot); }

    static void set(WrenVM* vm, int slot, double val) { wrenSetSlotDouble(vm, slot, val); }
};

template<>
struct WrenSlotAPI<int>
{
    static int get(WrenVM* vm, int slot) { return int(wrenGetSlotDouble(vm, slot)); }

    static void set(WrenVM* vm, int slot, int val) { wrenSetSlotDouble(vm, slot, double(val)); }
};

template<>
struct WrenSlotAPI<unsigned>
{
    static unsigned get(WrenVM* vm, int slot) { return unsigned(wrenGetSlotDouble(vm, slot)); }

    static void set(WrenVM* vm, int slot, unsigned val)
    {
        wrenSetSlotDouble(vm, slot, double(val));
    }
};

template<>
struct WrenSlotAPI<bool>
{
    static bool get(WrenVM* vm, int slot) { return wrenGetSlotBool(vm, slot); }

    static void set(WrenVM* vm, int slot, bool val) { wrenSetSlotBool(vm, slot, val); }
};

template<>
struct WrenSlotAPI<const char*>
{
    static const char* get(WrenVM* vm, int slot) { return wrenGetSlotString(vm, slot); }

    static void set(WrenVM* vm, int slot, const char* val) { wrenSetSlotString(vm, slot, val); }
};

template<>
struct WrenSlotAPI<std::string>
{
    static std::string get(WrenVM* vm, int slot)
    {
        return std::string(wrenGetSlotString(vm, slot));
    }

    static void set(WrenVM* vm, int slot, const std::string& str)
    {
        wrenSetSlotString(vm, slot, str.c_str());
    }
};

template<>
struct WrenSlotAPI<const std::string&>
{
    static const char* get(WrenVM* vm, int slot) { return wrenGetSlotString(vm, slot); }

    static void set(WrenVM* vm, int slot, const std::string& str)
    {
        wrenSetSlotString(vm, slot, str.c_str());
    }
};

struct ExpandType
{
    template<typename... T>
    ExpandType(T&&...)
    {
    }
};

// a helper for passing arguments to Wren
// explained here:
// http://stackoverflow.com/questions/17339789/how-to-call-a-function-on-all-variadic-template-args
template<typename... Args, std::size_t... index>
void passArgumentsToWren(
    WrenVM* vm,
    const std::tuple<Args...>& tuple,
    std::index_sequence<index...>)
{
    using Traits = ParameterPackTraits<Args...>;
    ExpandType{0,
               (WrenSlotAPI<typename Traits::template ParameterType<index>>::set(
                    vm, index + 1, std::get<index>(tuple)),
                0)...};
}

template<typename Function, std::size_t... index>
decltype(auto) invokeHelper(WrenVM* vm, Function&& f, std::index_sequence<index...>)
{
    using Traits = FunctionTraits<std::remove_reference_t<decltype(f)>>;
    return f(WrenSlotAPI<typename Traits::template ArgumentType<index>>::get(vm, index + 1)...);
}

template<typename Function>
decltype(auto) invokeWithWrenArguments(WrenVM* vm, Function&& f)
{
    constexpr auto Arity = FunctionTraits<std::remove_reference_t<decltype(f)>>::Arity;
    return invokeHelper<Function>(vm, std::forward<Function>(f), std::make_index_sequence<Arity>{});
}

template<typename R, typename C, typename... Args, std::size_t... index>
decltype(auto) invokeHelper(WrenVM* vm, R (C::*f)(Args...), std::index_sequence<index...>)
{
    using Traits = FunctionTraits<decltype(f)>;
    ForeignObject* objWrapper = static_cast<ForeignObject*>(wrenGetSlotForeign(vm, 0));
    C* obj = static_cast<C*>(objWrapper->objectPtr());
    return (obj->*f)(
        WrenSlotAPI<typename Traits::template ArgumentType<index>>::get(vm, index + 1)...);
}

// const variant
template<typename R, typename C, typename... Args, std::size_t... index>
decltype(auto) invokeHelper(WrenVM* vm, R (C::*f)(Args...) const, std::index_sequence<index...>)
{
    using Traits = FunctionTraits<decltype(f)>;
    ForeignObject* objWrapper = static_cast<ForeignObject*>(wrenGetSlotForeign(vm, 0));
    const C* obj = static_cast<const C*>(objWrapper->objectPtr());
    return (obj->*f)(
        WrenSlotAPI<typename Traits::template ArgumentType<index>>::get(vm, index + 1)...);
}

template<typename R, typename C, typename... Args>
decltype(auto) invokeWithWrenArguments(WrenVM* vm, R (C::*f)(Args...))
{
    constexpr auto Arity = FunctionTraits<decltype(f)>::Arity;
    return invokeHelper(vm, f, std::make_index_sequence<Arity>{});
}

// const variant
template<typename R, typename C, typename... Args>
decltype(auto) invokeWithWrenArguments(WrenVM* vm, R (C::*f)(Args...) const)
{
    constexpr auto Arity = FunctionTraits<decltype(f)>::Arity;
    return invokeHelper(vm, f, std::make_index_sequence<Arity>{});
}

// invokes plain invokeWithWrenArguments if true
// invokes invokeWithWrenArguments within WrenReturn if false
// to be used with std::is_void as the predicate
template<bool predicate>
struct InvokeWithoutReturningIf
{

    template<typename Function>
    static void invoke(WrenVM* vm, Function&& f)
    {
        invokeWithWrenArguments(vm, std::forward<Function>(f));
    }

    template<typename R, typename C, typename... Args>
    static void invoke(WrenVM* vm, R (C::*f)(Args...))
    {
        invokeWithWrenArguments(vm, std::forward<R (C::*)(Args...)>(f));
    }
};

template<>
struct InvokeWithoutReturningIf<false>
{

    template<typename Function>
    static void invoke(WrenVM* vm, Function&& f)
    {
        using ReturnType =
            typename FunctionTraits<std::remove_reference_t<decltype(f)>>::ReturnType;
        WrenSlotAPI<ReturnType>::set(vm, 0, invokeWithWrenArguments(vm, std::forward<Function>(f)));
    }

    template<typename R, typename C, typename... Args>
    static void invoke(WrenVM* vm, R (C::*f)(Args...))
    {
        WrenSlotAPI<R>::set(vm, 0, invokeWithWrenArguments(vm, f));
    }

    template<typename R, typename C, typename... Args>
    static void invoke(WrenVM* vm, R (C::*f)(Args...) const)
    {
        WrenSlotAPI<R>::set(vm, 0, invokeWithWrenArguments(vm, f));
    }
};

template<typename Signature, Signature>
struct ForeignMethodWrapper;

// free function variant
template<typename R, typename... Args, R (*f)(Args...)>
struct ForeignMethodWrapper<R (*)(Args...), f>
{

    static void call(WrenVM* vm)
    {
        InvokeWithoutReturningIf<std::is_void<R>::value>::invoke(vm, f);
    }
};

// method variant
template<typename R, typename C, typename... Args, R (C::*m)(Args...)>
struct ForeignMethodWrapper<R (C::*)(Args...), m>
{

    static void call(WrenVM* vm)
    {
        InvokeWithoutReturningIf<std::is_void<R>::value>::invoke(vm, m);
    }
};

// const method variant
template<typename R, typename C, typename... Args, R (C::*m)(Args...) const>
struct ForeignMethodWrapper<R (C::*)(Args...) const, m>
{

    static void call(WrenVM* vm)
    {
        InvokeWithoutReturningIf<std::is_void<R>::value>::invoke(vm, m);
    }
};

/***
 *       ____             _                                        __
 *      / __/__  _______ (_)__ ____    ___  _______  ___  ___ ____/ /___ __
 *     / _// _ \/ __/ -_) / _ `/ _ \  / _ \/ __/ _ \/ _ \/ -_) __/ __/ // /
 *    /_/  \___/_/  \__/_/\_, /_//_/ / .__/_/  \___/ .__/\__/_/  \__/\_, /
 *                       /___/      /_/           /_/               /___/
 */

// See this link for more about writing a metaprogramming type is_sharable<t>:
// http://anthony.noided.media/blog/programming/c++/ruby/2016/05/12/mruby-cpp-and-template-magic.html

template<bool isClass>
struct SetFieldInSlot
{
    template<typename T>
    static void set(WrenVM* vm, int slot, T& obj)
    {
        ForeignObjectPtr<T>::setInSlot(vm, slot, &obj);
    }
};

template<>
struct SetFieldInSlot<false>
{
    template<typename T>
    static void set(WrenVM* vm, int slot, T value)
    {
        WrenSlotAPI<T>::set(vm, slot, value);
    }
};

template<typename T, typename U, U T::*Field>
void propertyGetter(WrenVM* vm)
{
    ForeignObject* objWrapper = static_cast<ForeignObject*>(wrenGetSlotForeign(vm, 0));
    T* obj = static_cast<T*>(objWrapper->objectPtr());
    SetFieldInSlot<std::is_class<U>::value>::set(vm, 0, obj->*Field);
}

template<typename T, typename U, U T::*Field>
void propertySetter(WrenVM* vm)
{
    ForeignObject* objWrapper = static_cast<ForeignObject*>(wrenGetSlotForeign(vm, 0));
    T* obj = static_cast<T*>(objWrapper->objectPtr());
    obj->*Field = WrenSlotAPI<U>::get(vm, 1);
}

/***
 *       ____             _                 __
 *      / __/__  _______ (_)__ ____    ____/ /__ ____ ___
 *     / _// _ \/ __/ -_) / _ `/ _ \  / __/ / _ `(_-<(_-<
 *    /_/  \___/_/  \__/_/\_, /_//_/  \__/_/\_,_/___/___/
 *                       /___/
 */

// given a Wren class signature, this returns a unique value
inline std::size_t hashClassSignature(const char* module, const char* className)
{
    std::hash<std::string> hash;
    std::string qualified(module);
    qualified += className;
    return hash(qualified);
}

template<typename T, typename... Args, std::size_t... index>
void construct(WrenVM* vm, void* memory, std::index_sequence<index...>)
{
    using Traits = ParameterPackTraits<Args...>;
    ForeignObjectValue<T>* obj = new (memory) ForeignObjectValue<T>{};
    constexpr std::size_t arity = sizeof...(Args);
    wrenEnsureSlots(vm, arity);
    new (obj->objectPtr())
        T{WrenSlotAPI<typename Traits::template ParameterType<index>>::get(vm, index + 1)...};
}

template<typename T, typename... Args>
void allocate(WrenVM* vm)
{
    void* memory = wrenSetSlotNewForeign(vm, 0, 0, sizeof(ForeignObjectValue<T>));
    construct<T, Args...>(
        vm, memory, std::make_index_sequence<ParameterPackTraits<Args...>::size>{});
}

template<typename T>
void finalize(void* bytes)
{
    // might be a foreign value OR ptr
    ForeignObject* objWrapper = static_cast<ForeignObject*>(bytes);
    objWrapper->~ForeignObject();
}

void registerFunction(
    WrenVM* vm,
    const std::string& mod,
    const std::string& clss,
    bool isStatic,
    std::string sig,
    WrenForeignMethodFn function);
void registerClass(
    WrenVM* vm,
    const std::string& mod,
    std::string clss,
    WrenForeignClassMethods methods);

inline bool fileExists(const std::string& file)
{
    struct stat buffer;
    return (stat(file.c_str(), &buffer) == 0);
}

inline std::string fileToString(const std::string& file)
{
    std::ifstream fin;

    if (!fileExists(file))
    {
        throw std::runtime_error("file not found!");
    }

    fin.open(file, std::ios::in);

    std::stringstream buffer;
    buffer << fin.rdbuf() << '\0';
    return buffer.str();
}

} // namespace detail

class VM;
class Method;

// This class can hold any one of the values corresponding to the WrenType
// enum defined in wren.h
class Value
{
public:
    Value() = default;
    ~Value();

    Value(bool);
    Value(float);
    Value(double);
    Value(int);
    Value(unsigned int);
    Value(const char*);
    template<typename T>
    Value(T*);

    template<typename T>
    T as() const;

private:
    template<typename T>
    void set(T&& t);

    WrenType type_{WREN_TYPE_NULL};
    char* string_{nullptr};
    std::uint8_t storage_[8]{0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};
};

extern Value null;

/**
 * Note that this class stores a reference to the owning VM instance!
 * Make sure you don't move the VM instance which created this object, before
 * this object goes out of scope!
 */
class Method
{
public:
    Method(VM* vm, WrenHandle* variable, WrenHandle* method);
    Method() = default;
    Method(const Method&) = delete;
    Method(Method&&);
    Method& operator=(const Method&) = delete;
    Method& operator=(Method&&);
    ~Method();

    // this is const because we want to be able to pass this around like
    // immutable data
    template<typename... Args>
    Value operator()(Args... args) const;

private:
    mutable VM* vm_{nullptr};
    mutable WrenHandle* method_{nullptr};
    mutable WrenHandle* variable_{nullptr};
};

class ModuleContext;

class ClassContext
{
public:
    ClassContext() = delete;
    ClassContext(std::string c, ModuleContext& mod);
    virtual ~ClassContext() = default;

    template<typename F, F f>
    ClassContext& bindFunction(bool isStatic, std::string signature);
    ClassContext& bindCFunction(bool isStatic, std::string signature, WrenForeignMethodFn function);

    ModuleContext& endClass();

protected:
    ModuleContext& module_;
    std::string class_;
};

template<typename T>
class RegisteredClassContext : public ClassContext
{
public:
    RegisteredClassContext(std::string c, ModuleContext& mod) : ClassContext(c, mod) {}

    virtual ~RegisteredClassContext() = default;

    template<typename F, F f>
    RegisteredClassContext& bindMethod(bool isStatic, std::string signature);
    template<typename U, U T::*Field>
    RegisteredClassContext& bindGetter(std::string signature);
    template<typename U, U T::*Field>
    RegisteredClassContext& bindSetter(std::string signature);
    RegisteredClassContext& bindCFunction(
        bool isStatic,
        std::string signature,
        WrenForeignMethodFn function);
};

class ModuleContext
{
public:
    ModuleContext() = delete;
    ModuleContext(WrenVM* vm, std::string mod) : vm_(vm), name_(mod) {}

    ClassContext beginClass(std::string className);

    template<typename T, typename... Args>
    RegisteredClassContext<T> bindClass(std::string className);

    void endModule();

private:
    friend class ClassContext;
    template<typename T>
    friend class RegisteredClassContext;

    WrenVM* vm_;
    std::string name_;
};

enum class Result
{
    Success,
    CompileError,
    RuntimeError
};

class VM
{
public:
    VM();
    VM(const VM&) = delete;
    VM(VM&&);
    VM& operator=(const VM&) = delete;
    VM& operator=(VM&&);
    ~VM();

    inline WrenVM* ptr() { return vm_; }

    inline operator WrenVM*() const { return vm_; }

    Result executeModule(const std::string&);
    Result executeString(const std::string&);

    void collectGarbage();

    /**
     * The signature consists of the name of the method, followed by a
     * parenthesis enclosed list of of underscores representing each argument.
     */
    Method method(
        const std::string& module,
        const std::string& variable,
        const std::string& signature);

    ModuleContext beginModule(std::string name);

    static LoadModuleFn loadModuleFn;
    static WriteFn writeFn;
    static ReallocateFn reallocateFn;
    static ErrorFn errorFn;
    static std::size_t initialHeapSize;
    static std::size_t minHeapSize;
    static int heapGrowthPercent;
    static std::size_t chunkSize;

private:
    friend class ModuleContext;
    friend class ClassContext;
    friend class Method;
    template<typename T>
    friend class RegisteredClassContext;

    WrenVM* vm_;
};

template<typename T>
Value::Value(T* t) : type_{WREN_TYPE_FOREIGN}, string_{nullptr}
{
    std::memcpy(storage_, &t, sizeof(T*));
}

template<>
inline float Value::as<float>() const
{
    assert(type_ == WREN_TYPE_NUM);
    return *reinterpret_cast<const float*>(&storage_[0]);
}

template<typename T>
void Value::set(T&& t)
{
    static_assert(sizeof(T) <= 8, "The type is invalid!");
    std::memcpy(storage_, &t, sizeof(T));
}

template<>
inline double Value::as<double>() const
{
    assert(type_ == WREN_TYPE_NUM);
    return *reinterpret_cast<const double*>(&storage_[0]);
}

template<>
inline bool Value::as<bool>() const
{
    assert(type_ == WREN_TYPE_BOOL);
    return *reinterpret_cast<const bool*>(&storage_[0]);
}

template<>
inline const char* Value::as<const char*>() const
{
    assert(type_ == WREN_TYPE_STRING);
    return string_;
}

template<typename... Args>
Value Method::operator()(Args... args) const
{
    assert(vm_ && variable_ && method_);
    constexpr const std::size_t Arity = sizeof...(Args);
    wrenEnsureSlots(vm_->ptr(), Arity + 1u);
    wrenSetSlotHandle(vm_->ptr(), 0, variable_);

    std::tuple<Args...> tuple = std::make_tuple(args...);
    detail::passArgumentsToWren(vm_->ptr(), tuple, std::make_index_sequence<Arity>{});

    auto result = wrenCall(vm_->ptr(), method_);

    if (result == WREN_RESULT_SUCCESS)
    {
        WrenType type = wrenGetSlotType(vm_->ptr(), 0);

        switch (type)
        {
        case WREN_TYPE_BOOL: return Value(wrenGetSlotBool(vm_->ptr(), 0));
        case WREN_TYPE_NUM: return Value(wrenGetSlotDouble(vm_->ptr(), 0));
        case WREN_TYPE_STRING: return Value(wrenGetSlotString(vm_->ptr(), 0));
        case WREN_TYPE_FOREIGN: return Value(wrenGetSlotForeign(vm_->ptr(), 0));
        default: assert("Invalid Wren type"); break;
        }
    }

    return null;
}

template<typename T, typename... Args>
RegisteredClassContext<T> ModuleContext::bindClass(std::string className)
{
    WrenForeignClassMethods wrapper{&detail::allocate<T, Args...>, &detail::finalize<T>};
    detail::registerClass(vm_, name_, className, wrapper);

    // store the name and module if not already done
    if (detail::classNameStorage().size() == detail::getTypeId<T>())
    {
        assert(detail::classNameStorage().size() == detail::moduleNameStorage().size());
        detail::bindTypeToModuleName<T>(name_);
        detail::bindTypeToClassName<T>(className);
    }
    return RegisteredClassContext<T>(className, *this);
}

template<typename F, F f>
ClassContext& ClassContext::bindFunction(bool isStatic, std::string s)
{
    detail::registerFunction(
        module_.vm_,
        module_.name_,
        class_,
        isStatic,
        s,
        detail::ForeignMethodWrapper<decltype(f), f>::call);
    return *this;
}

template<typename T>
template<typename F, F f>
RegisteredClassContext<T>& RegisteredClassContext<T>::bindMethod(bool isStatic, std::string s)
{
    detail::registerFunction(
        module_.vm_,
        module_.name_,
        class_,
        isStatic,
        s,
        detail::ForeignMethodWrapper<decltype(f), f>::call);
    return *this;
}

template<typename T>
template<typename U, U T::*Field>
RegisteredClassContext<T>& RegisteredClassContext<T>::bindGetter(std::string s)
{
    detail::registerFunction(
        module_.vm_, module_.name_, class_, false, s, detail::propertyGetter<T, U, Field>);
    return *this;
}

template<typename T>
template<typename U, U T::*Field>
RegisteredClassContext<T>& RegisteredClassContext<T>::bindSetter(std::string s)
{
    detail::registerFunction(
        module_.vm_, module_.name_, class_, false, s, detail::propertySetter<T, U, Field>);
    return *this;
}

template<typename T>
RegisteredClassContext<T>& RegisteredClassContext<T>::bindCFunction(
    bool isStatic,
    std::string s,
    WrenForeignMethodFn function)
{
    detail::registerFunction(module_.vm_, module_.name_, class_, isStatic, s, function);
    return *this;
}

template<typename T>
T* getSlotForeign(WrenVM* vm, int slot)
{
    detail::ForeignObject* obj = static_cast<detail::ForeignObject*>(wrenGetSlotForeign(vm, slot));
    return static_cast<T*>(obj->objectPtr());
}

template<typename T>
void setSlotForeignValue(WrenVM* vm, int slot, const T& obj)
{
    detail::ForeignObjectValue<T>::setInSlot(vm, slot, obj);
}

template<typename T>
void setSlotForeignPtr(WrenVM* vm, int slot, T* obj)
{
    detail::ForeignObjectPtr<T>::setInSlot(vm, slot, obj);
}

} // namespace wrenpp

#endif // WRENPP_H_INCLUDED
