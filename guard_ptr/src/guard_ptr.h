#ifndef GUARD_PTR_H_INCLUDED
#define GUARD_PTR_H_INCLUDED
#include <atomic>

// There are some types of raw pointers which we can not use such smart pointer as std::shared_ptr to protect/manage,
// such as pointers to UI/Widget objects which are generally already managed(especially destroying) by a corresponding framework like Qt etc,
// so here we implement another smart pointer to protect/guard such pointers(to detect whether the pointed object was already destroyed/deleted), 
// it is guard_ptr with a helper class named support_guard_ptr which are inspired by std::shared_ptr¡¢std::enable_shared_from_this and Qt::QPointer.
// NOTICE: the guard_ptr is NOT thread-safe, so when protecting/managing non-ui pointers, consider other smart pointer like std::shared_ptr.
namespace guard
{
	template<typename T>
	struct guard_block
	{
		guard_block(T* ptr, unsigned int ref)
		{
			ptr_.store(static_cast<T*>(ptr));
			ref_.store(2);
		}

		inline ~guard_block() {}

		inline T* data() { return ptr_.load(); }

		void acquire()
		{
			// fetch_add return the OLD value of block_ before addition
			// OLD + 1 --> return OLD
			ref_.fetch_add(1);
		}

		bool release()
		{
			// fetch_add return the OLD value of block_ before subtraction
			// OLD - 1 --> return OLD
			// true: still alive (block_ is sill greater than 0), false: block_ is 0 and self is deleted.
			if (ref_.fetch_sub(1) == 1)
			{
				delete this; // CAN NO LONGER access me by the external calling pointer or reference which turns to be DANGLING !!!

				return false;
			}

			return true;
		}

		std::atomic<T*> ptr_{ nullptr };
		std::atomic<unsigned int> ref_{ 0 };
	};

	template<typename T>
	class support_guard_ptr
	{
		typedef guard_block<T> guard_block_t;

	public:
		static guard_block_t* getAndRef(T* ptr)
		{
			return internalGetAndRef(ptr);
		}

		inline ~support_guard_ptr()
		{
			auto block_ptr = block_.load();
			if (block_ptr)
			{
				block_ptr->ptr_.store(nullptr);

				if (!block_ptr->release())
				{
					block_.store(nullptr);
				}
			}
		}

		inline unsigned int refCount() const
		{
			auto block_ptr = block_.load();

			return block_ptr ? block_ptr->ref_.load() : 0;
		}

	private:
		// CRTP: T : public support_guard_ptr<T>, T* can convert to support_guard_ptr<T>*
		static guard_block_t* internalGetAndRef(support_guard_ptr<T>* base)
		{
			guard_block_t* block_ptr = base->block_.load();

			if (block_ptr)
			{
				block_ptr->acquire();
			}
			else
			{
				block_ptr = new guard_block_t(static_cast<T*>(base), 2); // 2: 1 held by support_guard_ptr self, 1 for the first guard_ptr !!!

				guard_block_t* block_ptr_held{ nullptr };
				if (!base->block_.compare_exchange_strong(block_ptr_held, block_ptr))
				{
					delete block_ptr;

					block_ptr = block_ptr_held;
					block_ptr->acquire();
				}
			}

			return block_ptr;
		}

		static guard_block_t* internalGetAndRef(...)
		{
			static_assert(false, "support_guard_ptr<T> is not base of T");
		}

	private:
		std::atomic<guard_block_t*> block_{ nullptr };
	};

	template<typename T>
	class guard_ptr
	{
		typedef guard_block<T> guard_block_t;

	public:
		inline guard_ptr() {}
		inline guard_ptr(T* ptr) : block_(ptr ? support_guard_ptr<T>::getAndRef(ptr) : nullptr) {}
		inline guard_ptr(const guard_ptr& rhs) : block_(rhs.block_) { if (block_) { block_->acquire(); } }
		inline guard_ptr(guard_ptr&& rhs) : block_(rhs.block_) { if (block_) { rhs.block_ = nullptr; } }
		inline ~guard_ptr() { tryToRelease(); }

		inline guard_ptr& operator=(const guard_ptr& rhs);
		inline guard_ptr& operator=(guard_ptr&& rhs);
		inline void swap(guard_ptr& rhs);

		inline bool isNull() const { return block_ == nullptr || block_->data() == nullptr; }
		inline bool isAlive() const { return !isNull(); }

		inline unsigned int refCount() const { return block_ ? block_->ref_.load() : 0; }

		inline T *data() const { return block_ ? block_->data() : nullptr; }
		inline T* operator->() const { return data(); }
		inline T& operator*() const { return *data(); }
		inline operator T*() const { return data(); }

		inline void clear() { swap(guard_ptr()); }

	private:
		inline void tryToRelease();

	private:
		guard_block_t* block_{ nullptr };
	};

	template<typename T>
	inline guard_ptr<T>& guard_ptr<T>::operator=(const guard_ptr& rhs)
	{
		if (rhs.block_ != block_)
		{
			if (rhs.block_) { rhs.block_->acquire(); }

			tryToRelease();

			block_ = rhs.block_;
		}

		return *this;
	}

	template<typename T>
	inline guard_ptr<T>& guard_ptr<T>::operator=(guard_ptr&& rhs)
	{
		if (rhs.block_ != block_)
		{
			tryToRelease();

			block_ = rhs.block_;
			rhs.block_ = nullptr;
		}

		return *this;
	}

	template<typename T>
	inline void guard_ptr<T>::swap(guard_ptr& rhs)
	{
		if (rhs.block_ != block_)
		{
			std::swap(block_, rhs.block_);
		}
	}

	template<typename T>
	inline void guard_ptr<T>::tryToRelease()
	{
		if (block_ && !block_->release())
		{
			block_ = nullptr;
		}
	}
}

#endif