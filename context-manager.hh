#include <cassert>
#include <coroutine>
#include <exception>
#include <functional>
#include <iostream>
#include <utility>

namespace contextlib
{
	template<typename T>
	struct promise;

	template<typename T, typename Awaitable>
	struct awaitable_or_throw;

	template<typename T>
	struct context_manager;

	// Turn coroutine into pair of contructor and destructor that will work with `with`
	template<typename T>
	struct context_manager
	{
		using promise_type = promise<T>;
		using handle_type = std::coroutine_handle<promise_type>;

		handle_type m_handle;

		~context_manager()
		{
			while (!m_handle.done()) {
				m_handle.resume();
			}
			m_handle.destroy();
		}

		T& operator*() { return m_handle.promise().m_value; }
	};

	template<typename T>
	struct promise
	{
		T m_value;
		std::exception_ptr m_exception;

		context_manager<T> get_return_object()
		{
			return context_manager<T>(context_manager<T>::handle_type::from_promise(*this));
		}

		std::suspend_never initial_suspend() const noexcept { return {}; }
		std::suspend_always final_suspend() const noexcept { return {}; }

		awaitable_or_throw<T, std::suspend_always> yield_value(std::convertible_to<T> auto&& from);

		void unhandled_exception()
		{
			m_exception = std::current_exception();
		}
	};

	template<typename T, typename Awaitable>
	struct awaitable_or_throw : Awaitable
	{
		using handle_type = typename context_manager<T>::handle_type;
		mutable handle_type m_handle;

		constexpr void await_suspend(handle_type handle) const noexcept
		{
			m_handle = handle;
		}

		constexpr auto await_resume() const noexcept
		{
			struct Destructor
			{
				std::exception_ptr exception;

				Destructor() = default;
				Destructor(Destructor const&) = delete;
				Destructor(Destructor &&d) : exception(std::exchange(d.exception, nullptr)) {}

				~Destructor()
				{
					if (exception) {
						std::rethrow_exception(exception);
					}
				}

			} destructor;
			destructor.exception = m_handle.promise().m_exception;
			return destructor;
		}
	};

	template<typename T>
	awaitable_or_throw<T, std::suspend_always> promise<T>::yield_value(std::convertible_to<T> auto&& from)
	{
		m_value = std::forward<decltype(from)>(from);
		return {};
	}


	template<typename T>
	auto with(context_manager<T> &&ctx, auto &&block)
	{
		try {
			std::invoke(std::forward<decltype(block)>(block), *ctx);
		} catch (...) {
			ctx.m_handle.promise().m_exception = std::current_exception();
			ctx.m_handle.resume();
		}
	}
}
