#pragma once

// #include "Base/Core/Assert.hpp"

#include <cstdint>
#include <atomic>

namespace TerraForge3D
{

	template <typename Base>
	class Ref
	{
	protected:
		struct ControlBlock
		{
#ifdef TF3D_ATOMIC_REF
			std::atomic<uint64_t> count = 0;
#else
			uint64_t count = 0;
#endif
			void* handle = nullptr;

			ControlBlock(void* handle = nullptr, uint64_t count = 0)
				:count(count), handle(handle)
			{}
		};

	public:

		Ref()
		{
			this->controlBlock = nullptr;
		}

		template <typename Derived>
		Ref(Derived* derived)
		{
			static_assert(std::is_base_of_v<Base, Derived>);
			this->Release();
			this->controlBlock = new ControlBlock(derived, 1);
		}

		template <typename Derived>
		Ref(const Ref<Derived>& derivedRef)
		{
			static_assert(std::is_base_of_v<Base, Derived>);
			this->Release();
			this->controlBlock = derivedRef.controlBlock;
			if (this->controlBlock)
				this->controlBlock->count += 1;
		}

		~Ref()
		{
			this->Release();
		}

		template <typename Derived>
		inline Ref<Base>& operator=(Ref<Derived>& ptr)
		{
			static_assert(std::is_base_of_v<Base, Derived>);
			this->Release();
			this->controlBlock = ptr.controlBlock;
			if (this->controlBlock)
				this->controlBlock->count += 1;
			return *this;
		}

		template <typename Derived>
		inline Ref<Base>& operator=(Derived* ptr)
		{
			static_assert(std::is_base_of_v<Base, Derived>);
			this->Release();
			this->controlBlock = new ControlBlock(ptr, 1);
			return *this;
		}

		inline operator bool() const
		{
			return this->controlBlock != nullptr;
		}

		inline Base& operator*() const
		{
			return *(reinterpret_cast<Base*>(controlBlock->handle));
		}

		inline Base* operator->() const
		{
			return reinterpret_cast<Base*>(controlBlock->handle);
		}

		inline Base* Get() const
		{
			return reinterpret_cast<Base*>(controlBlock->handle);
		}

	public:

		template <typename NewType, typename OldType>
		inline static Ref<NewType> Cast(Ref<OldType> oref)
		{
			Ref<NewType> nref;
			nref.controlBlock = oref.controlBlock;
			if (nref.controlBlock)
				nref.controlBlock->count += 1;
			return nref;
		}

	private:

		inline void Release()
		{
			if (this->controlBlock)
			{
				this->controlBlock->count -= 1;
				if (this->controlBlock->count <= 0)
				{
					if (this->controlBlock->handle)
						delete reinterpret_cast<Base*>(this->controlBlock->handle);
					delete this->controlBlock;
					this->controlBlock = nullptr;
				}
			}
		}


	private:
		ControlBlock* controlBlock = nullptr;
	};

}