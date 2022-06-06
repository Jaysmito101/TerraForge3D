#pragma once

// #include "Base/Core/Assert.hpp"

#include <cstdint>
#include <atomic>
#include <iostream>

namespace TerraForge3D
{
	struct SharedPtrControlBlock
	{
#ifdef TF3D_ATOMIC_REF
		std::atomic<uint64_t> count = 0;
#else
		uint64_t count = 0;
#endif
		void* handle = nullptr;

		SharedPtrControlBlock(void* handle = nullptr, uint64_t count = 0)
			:count(count), handle(handle)
		{}
	};

	template <typename Base>
	class SharedPtr
	{
	protected:

	public:
		template<typename>
		friend class SharedPtr;


		SharedPtr()
		{
			this->controlBlock = nullptr;
		}

		template <typename Derived>
		SharedPtr(Derived* derived)
		{
			if (derived == nullptr)
			{
				this->controlBlock = nullptr;
			}
			else
			{
				static_assert(std::is_base_of_v<Base, Derived>);
				this->Release();
				this->controlBlock = new SharedPtrControlBlock(derived, 1);
			}
		}

		SharedPtr(const SharedPtr& other)
		{
			this->controlBlock = other.controlBlock;
			if (this->controlBlock)
				this->controlBlock->count += 1;
		}

		template <typename Derived>
		SharedPtr(const SharedPtr<Derived>& derivedRef)
		{
			static_assert(std::is_base_of_v<Base, Derived>);
			this->controlBlock = derivedRef.controlBlock;
			if (this->controlBlock)
				this->controlBlock->count += 1;
		}

		virtual ~SharedPtr()
		{
			this->Release();
		}

		template <typename Derived>
		inline SharedPtr<Base>& operator=(SharedPtr<Derived>& ptr)
		{
			static_assert(std::is_base_of_v<Base, Derived>);
			this->Release();
			this->controlBlock = ptr.controlBlock;
			if (this->controlBlock)
				this->controlBlock->count += 1;
			return *this;
		}

		template <typename Derived>
		inline SharedPtr<Base>& operator=(Derived* ptr)
		{
			if(ptr == nullptr)
			{
				this->Release();
				this->controlBlock = nullptr;
			}
			else
			{
				static_assert(std::is_base_of_v<Base, Derived>);
				this->Release();
				this->controlBlock = new SharedPtrControlBlock(ptr, 1);
			}
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


	public:
		SharedPtrControlBlock* controlBlock = nullptr;
	};

	template <typename NewType, typename OldType>
	inline static SharedPtr<NewType> ReinterpretCast(SharedPtr<OldType> oref)
	{
		SharedPtr<NewType> nref;
		nref.controlBlock = oref.controlBlock;
		if (nref.controlBlock)
			nref.controlBlock->count += 1;
		return nref;
	}

	template <typename NewType, typename OldType>
	inline static SharedPtr<NewType> StaticCast(SharedPtr<OldType> oref)
	{
		SharedPtr<NewType> nref;
		if (oref.controlBlock)
		{
			nref.controlBlock->handle = static_cast<NewType>(reinterpret_cast<OldType>(oref.controlBlock->handle));
			nref.controlBlock->count = oref.controlBlock->count;
		}
		if (nref.controlBlock)
			nref.controlBlock->count += 1;
		return nref;
	}

	template <typename NewType, typename OldType>
	inline static SharedPtr<NewType> DynamicCast(SharedPtr<OldType> oref)
	{
		SharedPtr<NewType> nref;
		if (oref.controlBlock)
		{
			nref.controlBlock->handle = dynamic_cast<NewType>(reinterpret_cast<OldType>(oref.controlBlock->handle));
			nref.controlBlock->count = oref.controlBlock->count;
		}
		if (nref.controlBlock)
			nref.controlBlock->count += 1;
		return nref;
	}
}