#pragma once
#include "Common.h"
namespace Process
{
	namespace Fiber
	{
		// fls的东西就忽略吧
		using ::GetCurrentFiber;
		using ::SwitchToFiber;
		using ::ConvertFiberToThread;
		using ::ConvertThreadToFiber;
		using ::CreateFiber;
		using ::DeleteFiber;
		using ::IsThreadAFiber;
		using ::GetFiberData;
		template<typename T>
		inline T* GetFiberData()
		{
			return static_cast<T*>(::GetFiberData());
		}
		// 这是看winnt得出的结论
		void SetCurrentFiberData(void* ptr);
		void SetFiberData(PVOID fiber, void* ptr);

		// 这个可能不对，有一些成员的值不对，应该有一部分是teb的内容
		const PNT_TIB GetCurrentFiberContext();

		template<typename T>
		inline T* GetFiberData(PVOID fiber)
		{
			assert(fiber != nullptr);
			return static_cast<T*>(*(PVOID *)fiber);
		}




		namespace Detail
		{

			struct FiberDeletor
			{
				void operator()(void* fiber);
			};

			template<typename TChild, typename TStorage>
			class FiberBase
			{
			public:
				virtual ~FiberBase() = default;
				LPVOID NativeHandle()
				{
					return fiber_.get();
				}
			protected:
				typedef TStorage TStorage;
				typedef FiberBase Base;
				using TStoragePtr = _STD unique_ptr<TStorage>;
				using FiberPtr = _STD shared_ptr<void>;


				struct TStorageParam
				{
					PVOID Creator;
					TStoragePtr Storage;
				};

				FiberBase()
				{
				}

				// 注意创建过程中不能出现异常
				// 传入的参数为TStorage的构造参数
				template<typename... TArgs>
				FiberBase(TArgs... args)
				{
					assert(!fiber_);
					PVOID currentThreadFiber = nullptr;
					bool isThreadAFiber = Process::Fiber::IsThreadAFiber() != 0;
					if (isThreadAFiber)
					{
						currentThreadFiber = Process::Fiber::GetCurrentFiber();
					}
					else
					{
						currentThreadFiber = Process::Fiber::ConvertThreadToFiber(NULL);
						if (currentThreadFiber == nullptr)
						{
							return;
						}
					}
					assert(currentThreadFiber != nullptr);

					TStoragePtr storage(new TStorage(_STD forward<TArgs>(args)...));
					assert(storage);

					// 附带Storage智能指针，让回调持有指针副本，外部不持有
					auto param = new TStorageParam{ currentThreadFiber, _STD move(storage) };

					// 默认stacksize
					fiber_ = FiberPtr(Process::Fiber::CreateFiber(0, Run, param), FiberDeletor());
					if (!fiber_)
					{
						return;
					}

					// 先到回调里把TStorageParam给delete了，防止不使用时造成泄露
					Switch();

					if (!isThreadAFiber)
					{
						Process::Fiber::ConvertFiberToThread();
					}
				}




				void Switch()
				{
					// 切换线程必须转换为fiber
					assert(Process::Fiber::IsThreadAFiber());

					assert(fiber_);
					Process::Fiber::SwitchToFiber(fiber_.get());
				}


				static void WINAPI Run(LPVOID param)
				{
					assert(param != nullptr);
					auto storageParam = static_cast<TStorageParam*>(param);
					auto creatorFiber = storageParam->Creator;
					// 回调持有Storage智能指针，
					auto storage = _STD move(storageParam->Storage);
					delete storageParam;

					// 重新设置fiberdata
					Process::Fiber::SetCurrentFiberData(storage.get());

					// 再次切回就运行函数(此时可能会恢复fiber成thread)
					Process::Fiber::SwitchToFiber(creatorFiber);

					// 子类必须设置友元
					TChild::Callback(storage);
					// 运行结束了未跳回
					assert(false);
					__assume(false);
				}

				FiberPtr fiber_;
			};
		}  // namespace Detail
	}  // namespace Fiber
}  // namespace Process