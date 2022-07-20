#pragma once

// #include "Base/Core/Assert.hpp"

#include <cstdint>
#include <atomic>
#include <iostream>

#ifdef TerraForge3D
#error "Error"
#endif

namespace TerraForge3D
{
	struct SharedPtrControlBlock
	{
#ifdef TF3D_ATOMIC_REF
		std::atomic<uint64_t> count = 0;
#else
		uint64_t count = 0;
#endif

		SharedPtrControlBlock(uint64_t count = 0)
			:count(count)
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
			this->handle = nullptr;
		}

		template <typename Derived>
		SharedPtr(Derived* derived)
		{
			if (derived == nullptr)
			{
				this->controlBlock = nullptr;
				this->handle = nullptr;
			}
			else
			{
				static_assert(std::is_base_of_v<Base, Derived>);
				this->controlBlock = new SharedPtrControlBlock(1);
				this->handle = derived;
			}
		}

		SharedPtr(const SharedPtr& other)
		{
			this->controlBlock = other.controlBlock;
			this->handle = other.handle;
			if (this->controlBlock)
				this->controlBlock->count += 1;
		}

		template <typename Derived>
		SharedPtr(const SharedPtr<Derived>& other)
		{
			static_assert(std::is_base_of_v<Base, Derived>);
			this->controlBlock = other.controlBlock;
			this->handle = other.handle;
			if (this->controlBlock)
				this->controlBlock->count += 1;
		}

		virtual ~SharedPtr()
		{
			this->Release();
		}

		inline SharedPtr& operator=(SharedPtr& other)
		{
			this->Release();
			this->controlBlock = other.controlBlock;
			if (this->controlBlock)
				this->controlBlock->count += 1;
			return *this;
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
			this->Release();
			if (ptr == nullptr)
			{
				this->controlBlock = nullptr;
				this->handle = nullptr;
			}
			else
			{
				static_assert(std::is_base_of_v<Base, Derived>);
				this->controlBlock = new SharedPtrControlBlock(1);
				this->handle = ptr;
			}
			return *this;
		}

		inline operator bool() const
		{
			return this->controlBlock != nullptr;
		}

		inline Base& operator*() const
		{
			return *(this->handle);
		}

		inline Base* operator->() const
		{
			return this->handle;
		}

		inline Base* Get() const
		{
			return this->handle;
		}


	private:

		inline void Release()
		{
			if (this->controlBlock && this->handle)
			{
				this->controlBlock->count -= 1;
				if (this->controlBlock->count <= 0)
				{
					delete this->handle;
					this->handle = nullptr;

					delete this->controlBlock;
					this->controlBlock = nullptr;
				}
			}
		}


	public:
		Base* handle = nullptr;
		SharedPtrControlBlock* controlBlock = nullptr;
	};

	template <typename NewType, typename OldType>
	inline static SharedPtr<NewType> ReinterpretCast(SharedPtr<OldType> oref)
	{
		SharedPtr<NewType> nref;
		nref.controlBlock = oref.controlBlock;
		nref.handle = reinterpret_cast<NewType>(oref.handle);
		if (nref.controlBlock)
			nref.controlBlock->count += 1;
		return nref;
	}

	template <typename NewType, typename OldType>
	inline static SharedPtr<NewType> StaticCast(SharedPtr<OldType> oref)
	{
		SharedPtr<NewType> nref;
		nref.controlBlock = oref.controlBlock;
		nref.handle = static_cast<NewType>(oref.handle);
		if (oref.handle)
			assert(nref.handle);
		if (nref.controlBlock)
			nref.controlBlock->count += 1;
		return nref;
	}

	template <typename NewType, typename OldType>
	inline static SharedPtr<NewType> DynamicCast(SharedPtr<OldType> oref)
	{
		SharedPtr<NewType> nref;
		nref.controlBlock = oref.controlBlock;
		nref.handle = dynamic_cast<NewType>(oref.handle);
		if (oref.handle)
			assert(nref.handle);
		if (nref.controlBlock)
			nref.controlBlock->count += 1;
		return nref;
	}
}