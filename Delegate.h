#ifndef _DELEGATE_H_
#define _DELEGATE_H_

#include <memory>
#include <sstream>
#include <unordered_map>

#define DECLARE_FUNCTION_DELEGATE(DelegateName, ReturnValueType, ...) typedef xxd::SingleDelegate<ReturnValueType, __VA_ARGS__> (DelegateName);
#define DECLARE_FUNCTION_DELEGATE_NO_PARAMETER(DelegateName, ReturnValueType) typedef xxd::SingleDelegate<ReturnValueType> (DelegateName);

#define DECLARE_FUNCTION_MULTICAST_DELEGATE(DelegateName, ...) typedef xxd::MultiDelegate<__VA_ARGS__> (DelegateName);
#define DECLARE_FUNCTION_MULTICAST_DELEGATE_NO_PARAMETER(DelegateName) typedef xxd::MultiDelegate<> (DelegateName);

namespace xxd
{

class DelegateInterface final
{
private:
    template<typename ReturnT, typename ...ArgsT>
    friend class SingleDelegate;

    template<typename ...ArgsT>
    friend class MultiDelegate;

    template<typename ReturnT, typename ...ArgsT>
    struct IDelegate
    {
        virtual ReturnT operator() (ArgsT... args) = 0;
        virtual ~IDelegate() { };
    };

    // 类成员函数委托模板
    template<class ClassT, typename ReturnT, typename ...ArgsT>
    class ObjFuncDelegate : public IDelegate<ReturnT, ArgsT...>
    {
    public:
        typedef ReturnT(ClassT::* FunType) (ArgsT...);

        ObjFuncDelegate() = delete;
        explicit ObjFuncDelegate(ClassT* objPtr, const FunType& objFun) : obj(objPtr), func(objFun) { };

        virtual ReturnT operator() (ArgsT... args) override
        {
            return (obj->*func)(args...);
        }

        ClassT* obj;
        FunType func;
    };

    // 非成员函数委托模板
    template<typename ReturnT, typename ...ArgsT>
    class FuncDelegate : public IDelegate<ReturnT, ArgsT...>
    {
    public:
        typedef ReturnT(*FunType) (ArgsT...);

        FuncDelegate() = delete;
        explicit FuncDelegate(FunType fun) : func(fun) { };

        virtual ReturnT operator() (ArgsT... args) override
        {
            return (*func)(args...);
        }

        FunType func;
    };
    
    // 带有安全检测的类成员函数委托模板
    template<class ClassT, typename ReturnT, typename ...ArgsT>
    class ObjFuncSafeDelegate : public IDelegate<ReturnT, ArgsT...>
    {
    public:
        typedef ReturnT(ClassT::* FunType) (ArgsT...);

        ObjFuncSafeDelegate() = delete;
        explicit ObjFuncSafeDelegate(const std::shared_ptr<ClassT>& objShared, const FunType& objFun) : obj(objShared), func(objFun) { };
        explicit ObjFuncSafeDelegate(const std::weak_ptr<ClassT>& objWeak, const FunType& objFun) : obj(objWeak), func(objFun) { };

        virtual ReturnT operator() (ArgsT... args) override
        {
            if(!obj.expired())
                return (obj.lock().get()->*func)(args...);
            return ReturnT();
        }
        
        std::weak_ptr<ClassT> obj;
        FunType func;
    };
    
    // 任意可调用对象委托模版
    template<class AnyFunT, typename ReturnT, typename ...ArgsT>
    class AnyFunDelegate : public IDelegate<ReturnT, ArgsT...>
    {
    public:
        AnyFunDelegate() = delete;
        explicit AnyFunDelegate(AnyFunT&& fun) : func(fun) { };

        virtual ReturnT operator() (ArgsT... args) override
        {
            return func(args...);
        }

        typename std::remove_reference<AnyFunT>::type func;
    };
    
};

template<typename ReturnT, typename ...ArgsT>
class SingleDelegate final
{
public:
    inline explicit SingleDelegate() = default;
    
    static inline SingleDelegate<ReturnT, ArgsT...> CreateFunction(typename DelegateInterface::FuncDelegate<ReturnT, ArgsT...>::FunType fun);
    
    template<class ClassT>
    static inline SingleDelegate<ReturnT, ArgsT...> CreateObject(ClassT* obj, const typename DelegateInterface::ObjFuncDelegate<ClassT, ReturnT, ArgsT...>::FunT& objFun);
    
    template<class ClassT>
    static inline SingleDelegate<ReturnT, ArgsT...> CreateSafeObj(const std::shared_ptr<ClassT>& objShared, const typename DelegateInterface::ObjFuncDelegate<ClassT, ReturnT, ArgsT...>::FunT& objFun);
    
    template<class ClassT>
    static inline SingleDelegate<ReturnT, ArgsT...> CreateSafeObj(const std::weak_ptr<ClassT>& objWeak, const typename DelegateInterface::ObjFuncDelegate<ClassT, ReturnT, ArgsT...>::FunT& objFun);
    
    template<class AnyFunT>
    static inline SingleDelegate<ReturnT, ArgsT...> CreateAnyFunc(AnyFunT&& func);
    
    // 绑定全局或静态函数
    inline void BindFunction(typename DelegateInterface::FuncDelegate<ReturnT, ArgsT...>::FunType fun);

    // 绑定类成员函数
    template<class ClassT>
    inline void BindObject(ClassT* obj, const typename DelegateInterface::ObjFuncDelegate<ClassT, ReturnT, ArgsT...>::FunT& objFun);

    // 绑定安全的类成员函数
    template<class ClassT>
    inline void BindSafeObj(const std::shared_ptr<ClassT>& objShared, const typename DelegateInterface::ObjFuncDelegate<ClassT, ReturnT, ArgsT...>::FunT& objFun);
    
    template<class ClassT>
    inline void BindSafeObj(const std::weak_ptr<ClassT>& objWeak, const typename DelegateInterface::ObjFuncDelegate<ClassT, ReturnT, ArgsT...>::FunT& objFun);
    
    // 绑定任意可调用对象
    template<class AnyFunT>
    inline void BindAnyFunc(AnyFunT&& func);

    // 代理执行
    inline ReturnT Invoke(ArgsT... args);
    inline ReturnT operator() (ArgsT... args);
    
    // 解绑函数
    inline void UnBind();
    
private:
    std::shared_ptr<DelegateInterface::IDelegate<ReturnT, ArgsT...> > dlgtPtr;
};

struct DelegateHandle
{
private:
    template<typename ...ArgsT>
    friend class MultiDelegate;

    inline DelegateHandle(uint32_t t, uint32_t i, void* p, void* b) : tdlgt(t), idlgt(i), pdlgt(p), bind(b) { }

    // Handle转化为字符串函数
    inline std::string ToString() const
    {
        static std::stringstream ss;
        ss << tdlgt << idlgt << pdlgt << bind;
        std::string key = ss.str();
        ss.str("");
        return key;
    };

    uint32_t tdlgt; // 代理类型
    uint32_t idlgt; // 代理id
    void* pdlgt; // 代理类
    void* bind; // 绑定地址
};

template<typename ...ArgsT>
class MultiDelegate final
{
public:
    inline explicit MultiDelegate() = default;

    // 添加全局或静态函数
    inline DelegateHandle AddFunction(typename DelegateInterface::FuncDelegate<void, ArgsT...>::FunType fun);

    // 添加类成员函数
    template<class ClassT>
    inline DelegateHandle AddObject(ClassT* obj, const typename DelegateInterface::ObjFuncDelegate<ClassT, void, ArgsT...>::FunT& objFun);
    
    // 添加安全的类成员函数
    template<class ClassT>
    inline DelegateHandle AddSafeObj(const std::shared_ptr<ClassT>& objShared, const typename DelegateInterface::ObjFuncDelegate<ClassT, void, ArgsT...>::FunT& objFun);
    
    template<class ClassT>
    inline DelegateHandle AddSafeObj(const std::weak_ptr<ClassT>& objWeak, const typename DelegateInterface::ObjFuncDelegate<ClassT, void, ArgsT...>::FunT& objFun);

    // 添加任意可调用对象
    template<class AnyFunT>
    inline DelegateHandle AddAnyFunc(AnyFunT&& func);
    
    // 多播代理执行
    void BroadCast(ArgsT... args);
    inline void operator() (ArgsT... args);
    
    // 根据代理句柄移除
    inline bool Remove(const DelegateHandle& handle);
    
    // 移除全局或静态函数
    inline bool Remove(typename DelegateInterface::FuncDelegate<void, ArgsT...>::FunType objFun);

    // 移除类成员函数
    template<class ClassT>
    inline bool Remove(ClassT* obj, const typename DelegateInterface::ObjFuncDelegate<ClassT, void, ArgsT...>::FunT& objFun);
    
    // 移除安全类成员函数
    template<class ClassT>
    inline bool Remove(const std::shared_ptr<ClassT>& objShared, const typename DelegateInterface::ObjFuncDelegate<ClassT, void, ArgsT...>::FunT& objFun);
    
    template<class ClassT>
    inline bool Remove(const std::weak_ptr<ClassT>& objWeak, const typename DelegateInterface::ObjFuncDelegate<ClassT, void, ArgsT...>::FunT& objFun);
    
    // 清空代理
    inline void Clear();
private:
    // 遍历dlgtMap，判断为真才会被移除
    template<class Comp>
    bool Remove(const Comp& rmLambda);
    
    uint32_t dlgtId;
    std::unordered_map<std::string, std::shared_ptr<DelegateInterface::IDelegate<void, ArgsT...> > > dlgtMap;
};

}

template<typename ReturnT, typename ...ArgsT>
inline xxd::SingleDelegate<ReturnT, ArgsT...> xxd::SingleDelegate<ReturnT, ArgsT...>::CreateFunction(typename DelegateInterface::FuncDelegate<ReturnT, ArgsT...>::FunType fun)
{
    SingleDelegate<ReturnT, ArgsT...> dlgt;
    dlgt.BindFunction(fun);
    return dlgt;
}

template<typename ReturnT, typename ...ArgsT>
template<class ClassT>
inline xxd::SingleDelegate<ReturnT, ArgsT...> xxd::SingleDelegate<ReturnT, ArgsT...>::CreateObject(ClassT* obj, const typename DelegateInterface::ObjFuncDelegate<ClassT, ReturnT, ArgsT...>::FunT& objFun)
{
    SingleDelegate<ReturnT, ArgsT...> dlgt;
    dlgt.BindObject(obj, objFun);
    return dlgt;
}

template<typename ReturnT, typename ...ArgsT>
template<class ClassT>
inline xxd::SingleDelegate<ReturnT, ArgsT...> xxd::SingleDelegate<ReturnT, ArgsT...>::CreateSafeObj(const std::shared_ptr<ClassT>& objShared, const typename DelegateInterface::ObjFuncDelegate<ClassT, ReturnT, ArgsT...>::FunT& objFun)
{
    SingleDelegate<ReturnT, ArgsT...> dlgt;
    dlgt.BindSafeObj(objShared, objFun);
    return dlgt;
}

template<typename ReturnT, typename ...ArgsT>
template<class ClassT>
inline xxd::SingleDelegate<ReturnT, ArgsT...> xxd::SingleDelegate<ReturnT, ArgsT...>::CreateSafeObj(const std::weak_ptr<ClassT>& objWeak, const typename DelegateInterface::ObjFuncDelegate<ClassT, ReturnT, ArgsT...>::FunT& objFun)
{
    SingleDelegate<ReturnT, ArgsT...> dlgt;
    dlgt.BindSafeObj(objWeak, objFun);
    return dlgt;
}

template<typename ReturnT, typename ...ArgsT>
template<class AnyFunT>
inline xxd::SingleDelegate<ReturnT, ArgsT...> xxd::SingleDelegate<ReturnT, ArgsT...>::CreateAnyFunc(AnyFunT&& func)
{
    SingleDelegate<ReturnT, ArgsT...> dlgt;
    dlgt.BindAnyFunc(std::forward<AnyFunT>(func));
    return dlgt;
}

template<typename ReturnT, typename ...ArgsT>
inline void xxd::SingleDelegate<ReturnT, ArgsT...>::BindFunction(typename DelegateInterface::FuncDelegate<ReturnT, ArgsT...>::FunType fun)
{
    dlgtPtr = std::make_shared<DelegateInterface::FuncDelegate<ReturnT, ArgsT...> >(fun);
}

template<typename ReturnT, typename ...ArgsT>
template<class ClassT>
inline void xxd::SingleDelegate<ReturnT, ArgsT...>::BindObject(ClassT* obj, const typename DelegateInterface::ObjFuncDelegate<ClassT, ReturnT, ArgsT...>::FunT& objFun)
{
    dlgtPtr = std::make_shared<DelegateInterface::ObjFuncDelegate<ClassT, ReturnT, ArgsT...> >(obj, objFun);
}

template<typename ReturnT, typename ...ArgsT>
template<class ClassT>
inline void xxd::SingleDelegate<ReturnT, ArgsT...>::BindSafeObj(const std::shared_ptr<ClassT>& objShared, const typename DelegateInterface::ObjFuncDelegate<ClassT, ReturnT, ArgsT...>::FunT& objFun)
{
    dlgtPtr = std::make_shared<DelegateInterface::ObjFuncSafeDelegate<ClassT, ReturnT, ArgsT...> >(objShared, objFun);
}

template<typename ReturnT, typename ...ArgsT>
template<class ClassT>
inline void xxd::SingleDelegate<ReturnT, ArgsT...>::BindSafeObj(const std::weak_ptr<ClassT>& objWeak, const typename DelegateInterface::ObjFuncDelegate<ClassT, ReturnT, ArgsT...>::FunT& objFun)
{
    dlgtPtr = std::make_shared<DelegateInterface::ObjFuncSafeDelegate<ClassT, ReturnT, ArgsT...> >(objWeak, objFun);
}

template<typename ReturnT, typename ...ArgsT>
template<class AnyFunT>
inline void xxd::SingleDelegate<ReturnT, ArgsT...>::BindAnyFunc(AnyFunT&& func)
{
    dlgtPtr = std::make_shared<DelegateInterface::AnyFunDelegate<AnyFunT, ReturnT, ArgsT...> >(std::forward<AnyFunT>(func));
}

template<typename ReturnT, typename ...ArgsT>
inline ReturnT xxd::SingleDelegate<ReturnT, ArgsT...>::Invoke(ArgsT ...args)
{
    return dlgtPtr.get() ? (*dlgtPtr)(args...) : ReturnT();
}

template<typename ReturnT, typename ...ArgsT>
inline ReturnT xxd::SingleDelegate<ReturnT, ArgsT...>::operator()(ArgsT ...args)
{
    return Invoke(args...);
}

template<typename ReturnT, typename ...ArgsT>
inline void xxd::SingleDelegate<ReturnT, ArgsT...>::UnBind()
{
    dlgtPtr.reset();
}

template<typename ...ArgsT>
inline xxd::DelegateHandle xxd::MultiDelegate<ArgsT...>::AddFunction(typename DelegateInterface::FuncDelegate<void, ArgsT...>::FunType fun)
{
    DelegateHandle handle = { 0, dlgtId++, this, (void*)fun };
    dlgtMap[handle.ToString()] = std::make_shared<DelegateInterface::FuncDelegate<void, ArgsT...> >(fun);
    
    return handle;
}

template<typename ...ArgsT>
template<class ClassT>
inline xxd::DelegateHandle xxd::MultiDelegate<ArgsT...>::AddObject(ClassT* obj, const typename DelegateInterface::ObjFuncDelegate<ClassT, void, ArgsT...>::FunT& objFun)
{
    DelegateHandle handle(0x1, dlgtId++, this, (void*)obj);
    dlgtMap[handle.ToString()] = std::make_shared<DelegateInterface::ObjFuncDelegate<ClassT, void, ArgsT...> >(obj, objFun);
    
    return handle;
}

template<typename ...ArgsT>
template<class ClassT>
inline xxd::DelegateHandle xxd::MultiDelegate<ArgsT...>::AddSafeObj(const std::shared_ptr<ClassT>& objShared, const typename DelegateInterface::ObjFuncDelegate<ClassT, void, ArgsT...>::FunT& objFun)
{
    DelegateHandle handle(0x2, dlgtId++, this, (void*)objShared.get());
    dlgtMap[handle.ToString()] = std::make_shared<DelegateInterface::ObjFuncSafeDelegate<ClassT, void, ArgsT...> >(objShared, objFun);
    
    return handle;
}

template<typename ...ArgsT>
template<class ClassT>
inline xxd::DelegateHandle xxd::MultiDelegate<ArgsT...>::AddSafeObj(const std::weak_ptr<ClassT>& objWeak, const typename DelegateInterface::ObjFuncDelegate<ClassT, void, ArgsT...>::FunT& objFun)
{
    DelegateHandle handle(0x2, dlgtId++, this, (void*)objWeak.lock().get());
    dlgtMap[handle.ToString()] = std::make_shared<DelegateInterface::ObjFuncSafeDelegate<ClassT, void, ArgsT...> >(objWeak, objFun);
    
    return handle;
}

template<typename ...ArgsT>
template<class AnyFunT>
inline xxd::DelegateHandle xxd::MultiDelegate<ArgsT...>::AddAnyFunc(AnyFunT&& func)
{
    DelegateHandle handle(0x4, dlgtId++, this, 0);
    dlgtMap[handle.ToString()] = std::make_shared<DelegateInterface::AnyFunDelegate<AnyFunT, void, ArgsT...> >(std::forward<AnyFunT>(func));
    
    return handle;
}

template<typename ...ArgsT>
inline void xxd::MultiDelegate<ArgsT...>::BroadCast(ArgsT ...args)
{
    for (const auto& it : dlgtMap)
    {
        (*(it.second))(args...);
    }
}

template<typename ...ArgsT>
inline void xxd::MultiDelegate<ArgsT...>::operator()(ArgsT ...args)
{
    BroadCast(args...);
}

template<typename ...ArgsT>
inline bool xxd::MultiDelegate<ArgsT...>::Remove(const xxd::DelegateHandle& handle)
{
    std::string key = handle.ToString();
    return dlgtMap.erase(key);
}

template<typename ...ArgsT>
inline bool xxd::MultiDelegate<ArgsT...>::Remove(typename DelegateInterface::FuncDelegate<void, ArgsT...>::FunType objFun)
{
    auto rmLambda = [&](const typename std::unordered_map<std::string, std::shared_ptr<DelegateInterface::IDelegate<void, ArgsT...> > >::iterator& it)->bool
    {
        DelegateInterface::IDelegate<void, ArgsT...>* dlgtPtr = (*it).second.get();
        auto flag = dynamic_cast<DelegateInterface::FuncDelegate<void, ArgsT...>*>(dlgtPtr);
        return flag && flag->func == objFun;
    };
    return Remove(rmLambda);
}

template<typename ...ArgsT>
template<class ClassT>
inline bool xxd::MultiDelegate<ArgsT...>::Remove(ClassT* obj, const typename DelegateInterface::ObjFuncDelegate<ClassT, void, ArgsT...>::FunT& objFun)
{
    auto rmLambda = [&](const typename std::unordered_map<std::string, std::shared_ptr<DelegateInterface::IDelegate<void, ArgsT...> > >::iterator& it)->bool
    {
        DelegateInterface::IDelegate<void, ArgsT...>* dlgtPtr = (*it).second.get();
        auto flag = dynamic_cast<DelegateInterface::ObjFuncDelegate<ClassT, void, ArgsT...>*>(dlgtPtr);
        return flag && flag->func == objFun && flag->obj == obj;
    };
    return Remove(rmLambda);
}

template<typename ...ArgsT>
template<class ClassT>
inline bool xxd::MultiDelegate<ArgsT...>::Remove(const std::shared_ptr<ClassT>& objShared, const typename DelegateInterface::ObjFuncDelegate<ClassT, void, ArgsT...>::FunT& objFun)
{
    auto rmLambda = [&](const typename std::unordered_map<std::string, std::shared_ptr<DelegateInterface::IDelegate<void, ArgsT...> > >::iterator& it)->bool
    {
        DelegateInterface::IDelegate<void, ArgsT...>* dlgtPtr = (*it).second.get();
        auto flag = dynamic_cast<DelegateInterface::ObjFuncSafeDelegate<ClassT, void, ArgsT...>*>(dlgtPtr);
        return flag && flag->func == objFun && flag->obj.lock() == objShared;
    };
    return Remove(rmLambda);
}

template<typename ...ArgsT>
template<class ClassT>
inline bool xxd::MultiDelegate<ArgsT...>::Remove(const std::weak_ptr<ClassT>& objWeak, const typename DelegateInterface::ObjFuncDelegate<ClassT, void, ArgsT...>::FunT& objFun)
{
    return !objWeak.expired() ? Remove(objWeak.lock(), objFun) : false;
}

template<typename ...ArgsT>
inline void xxd::MultiDelegate<ArgsT...>::Clear()
{
    //引用计数为0时自动释放对象
    dlgtMap.clear();
}

template<typename ...ArgsT>
template<class Comp>
bool xxd::MultiDelegate<ArgsT...>::Remove(const Comp& rmLambda)
{
    std::vector<typename std::unordered_map<std::string, std::shared_ptr<DelegateInterface::IDelegate<void, ArgsT...> > >::iterator> deleteIts;
    
    for (auto it = dlgtMap.begin(); it != dlgtMap.end(); it++)
    {
        if (rmLambda(it))
        {
            deleteIts.push_back(it);
        }
    }
    
    bool rmSuccess = deleteIts.size();
    if(rmSuccess)
    {
        for(const auto& dlgtIt : deleteIts)
            dlgtMap.erase(dlgtIt);
    }
    return rmSuccess;
}

#endif
