#pragma once
#include "Common.h"
#include "SpinLock.h"
namespace Process
{
	namespace LazyLoad
	{
		namespace Detail
		{
			template<typename TFunc, typename TLoadDll, typename TDecryptStrPolicy, typename... T>
			class DynamicCallBase;

			template<typename TFunc, typename TLoadDll, typename TDecryptStrPolicy, typename TFuncUnpack>
			struct GetFunctionType;

			// 定义去压栈特性标识的类, 用到vs stl里的一些未发布的东西，可能将来会失效
#define GET_FUNCTION_TYPE(CALL_OPT, X1, X2) \
template<typename TFunc, typename TLoadDll, typename TDecryptStrPolicy, typename TRet, typename... TArgs> \
	struct GetFunctionType<TFunc, TLoadDll, TDecryptStrPolicy, TRet CALL_OPT (TArgs...)> \
	{\
	typedef DynamicCallBase<TFunc, TLoadDll, TDecryptStrPolicy, TRet, TArgs...> type; \
	};

			_NON_MEMBER_CALL(GET_FUNCTION_TYPE, , );
#undef GET_FUNCTION_TYPE

			// FIX: 为了线程同步一个函数耗费太多空间
			template<typename TFunc, typename TLoadDll, typename TDecryptStrPolicy, typename TRet, typename... TArgs>
			class DynamicCallBase<TFunc, TLoadDll, TDecryptStrPolicy, TRet, TArgs...>
			{
			public:
				typedef TRet TRet;
				typedef DynamicCallBase TDynamicCallBase;
				using EncryptStr = typename TDecryptStrPolicy::EncryptStr;
				DynamicCallBase(TLoadDll& dll, EncryptStr funcName) :
					dll_(dll),
					funcName_(funcName)
				{

				}

				TRet operator()(TArgs... args) const
				{
					auto loaded = Load();
					assert(loaded);
					return GetFunction()(_STD forward<TArgs>(args)...);
				}
				bool Load() const
				{
					if (!func_)
					{
						auto dll = dll_.GetDllModule();
						if (dll == NULL)
						{
							return false;
						}
						_STD lock_guard<SpinLock> lock(lock_);
						if (!func_)
						{
							auto funcName = TDecryptStrPolicy::Decrypt(funcName_);
							func_ = reinterpret_cast<TFunc*>(
								Process::Overwrite::GetProcAddress(dll,
									TDecryptStrPolicy::GetStr(funcName))
								);

							return func_ ? true : false;
						}
					}
					return true;
				}
			protected:
				DynamicCallBase(const DynamicCallBase&) = delete;
				DynamicCallBase& operator=(const DynamicCallBase&) = delete;

				const _STD function<TFunc>& GetFunction() const
				{
					return func_;
				}

				TLoadDll& dll_;
				EncryptStr funcName_;
				mutable _STD function<TFunc> func_;
				mutable SpinLock lock_;
			};
		}  // namespace Detail
	}  // namespace LazyLoad
}  // namespace Process
