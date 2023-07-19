#include <type_traits>
#include <cstdint>

namespace custom_variant {
	template<typename... Ts>
	class variant_t;

	namespace impl {
		namespace tools {
			namespace void_details {
				template <typename...>
				struct make_void { 
					using type = void;
				};
			}

			template <typename... T>
			using void_t = typename void_details::make_void<T...>::type;

			template<bool...>
			struct bool_pack {};

			template<bool... Bs>
			using conjunction = std::is_same<bool_pack<true, Bs...>, bool_pack<Bs..., true>>;

			template <bool B>
			using bool_constant = std::integral_constant<bool, B>;

			template<bool... Bs>
			struct disjunction : bool_constant<!conjunction<!Bs...>::value> {};

			template <typename T, typename... Ts>
			struct index_of_type;

			template <typename T, typename... Rest>
			struct index_of_type<T, T, Rest...> : std::integral_constant<std::size_t, 0> {};

			template <typename T, typename First, typename... Rest>
			struct index_of_type<T, First, Rest...> : std::integral_constant<std::size_t, 1 + index_of_type<T, Rest...>::value> {};

			template<typename T, typename VarType, typename = void>
			struct variant_converter_helper {
				using type = std::conditional_t<std::is_same<T, VarType>::value, VarType, void>;
			};

			template<typename T, typename VarType>
			struct variant_converter_helper<T, VarType, void_t<
				std::enable_if_t<
				(std::is_integral<T>::value&& std::is_integral<VarType>::value&& std::is_signed<T>::value&& std::is_signed<VarType>::value)
				>>
				> {
				using type = VarType;
			};

			template<typename T, typename VarType>
			struct variant_converter_helper<T, VarType, void_t<
				std::enable_if_t<
				(std::is_integral<T>::value&& std::is_integral<VarType>::value&& std::is_unsigned<T>::value::value&& std::is_unsigned<VarType>::value)
				>>
				> {
				using type = VarType;
			};

			template<typename T, typename VarType>
			struct variant_converter_helper<T, VarType, void_t<
				std::enable_if_t<(std::is_floating_point<T>::value&& std::is_floating_point<VarType>::value)>>
				> {
				using type = VarType;
			};

			template<typename T, typename VarType>
			struct variant_converter_helper<T, VarType, void_t<
				std::enable_if_t<!std::is_arithmetic<T>::value&& std::is_constructible<VarType, T>::value>>
				> {
				using type = VarType;
			};

			template <typename T, typename First, typename... Rest>
			struct variant_converter {
				using type = std::conditional_t<
					std::is_void<typename variant_converter_helper<T, First>::type>::value,
					typename variant_converter<T, Rest...>::type,
					typename variant_converter_helper<T, First>::type
				>;
			};

			template <typename T, typename Last>
			struct variant_converter<T, Last> {
				using type = std::conditional_t<std::is_void<typename variant_converter_helper<T, Last>::type>::value, void, Last>;
			};

			template <std::size_t N, typename... Types>
			struct nth_type;

			template <std::size_t N, typename First, typename... Rest>
			struct nth_type<N, First, Rest...> {
				using type = typename nth_type<N - 1, Rest...>::type;
			};

			template <typename First, typename... Rest>
			struct nth_type<0, First, Rest...> {
				using type = First;
			};

			template <std::size_t N>
			struct nth_type<N> {
				using type = void;
			};

			template<std::size_t N, typename... Ts>
			using nth_type_t = typename nth_type<N, Ts...>::type;

			template <typename Callable, typename... Args>
			struct is_callable {
				template <typename C, typename... A>
				static auto test(std::size_t) -> decltype(std::declval<C>()(std::declval<A>()...), std::true_type{});

				template <typename C, typename... A>
				static auto test(...)->std::false_type;

				static constexpr bool value = decltype(test<Callable, Args...>(0))::value;
			};
		}

		template<typename, typename>
		struct is_convertible_to_variant;

		template<typename T, typename... Ts>
		struct is_convertible_to_variant<T, variant_t<Ts...>> {
			constexpr static bool value = tools::disjunction<std::is_convertible<T, Ts>::value...>::value;
		};

		template<typename, typename>
		struct is_any;

		template<typename T, typename... Ts>
		struct is_any<T, variant_t<Ts...>> {
			constexpr static bool value = tools::conjunction<std::is_convertible<T, Ts>::value...>::value;
		};

		template<typename T, typename... Ts>
		struct variant_converter;

		template<typename T, typename... Ts>
		struct variant_converter<T, variant_t<Ts...>> : tools::variant_converter<T, Ts...> {};

		template<typename T, typename Variant>
		using variant_converter_t = typename variant_converter<T, Variant>::type;

		template<typename...>
		struct recursive_union_t {};

		template<typename Last>
		struct recursive_union_t<Last> {
			recursive_union_t() noexcept {}
			~recursive_union_t() noexcept {}

			union {
				Last value;
			};

			using value_type = Last;
		};

		template<typename First, typename... Rest>
		struct recursive_union_t<First, Rest...> {
			recursive_union_t() noexcept {}
			~recursive_union_t() noexcept {}

			union {
				First value;
				recursive_union_t<Rest...> next;
			};

			using value_type = First;
		};

		template<typename T, typename Last>
		T& recursive_union_get_value(recursive_union_t<Last>& u) {
			return u.value;
		}

		template<typename T, typename First, typename... Rest, typename = std::enable_if_t<(std::is_same<First, T>::value && sizeof...(Rest) > 0)>>
		T& recursive_union_get_value(recursive_union_t<First, Rest...>& u) {
			return u.value;
		}

		template<typename T, typename First, typename... Rest, typename = std::enable_if_t<(!std::is_same<First, T>::value && sizeof...(Rest) > 0)>>
		decltype(auto) recursive_union_get_value(recursive_union_t<First, Rest...>& u) {
			return recursive_union_get_value<T>(u.next);
		}

		template<std::size_t I = 0, typename T, typename Last>
		typename std::enable_if_t<!std::is_same<Last, T>::value>
			recursive_union_set_value(recursive_union_t<Last>& u, T&& v, std::size_t last_index) {}

		template<std::size_t I = 0, typename T, typename Last>
		typename std::enable_if_t<std::is_same<Last, T>::value>
			recursive_union_set_value(recursive_union_t<Last>& u, T&& v, std::size_t last_index) {
			new (&u.value) T();
			u.value = std::forward<T>(v);
		}

		template<std::size_t I = 0, typename T, typename First, typename... Rest>
		typename std::enable_if_t<(std::is_same<First, T>::value && sizeof...(Rest) > 0)>
			recursive_union_set_value(recursive_union_t<First, Rest...>& u, T&& v, std::size_t last_index) {
			new (&u.value) T();
			u.value = std::forward<T>(v);
		}

		template<std::size_t I = 0, typename T, typename First, typename... Rest>
		typename std::enable_if_t<(!std::is_same<First, T>::value && sizeof...(Rest) > 0)>
			recursive_union_set_value(recursive_union_t<First, Rest...>& u, T&& v, std::size_t last_index) {
			if (I == last_index) {
				u.value.~First();
				new (&u.next) recursive_union_t<Rest...>();
			}

			recursive_union_set_value<I + 1>(u.next, std::forward<T>(v), last_index);
		}
	}


	template<typename... Ts>
	class variant_t {
	public:
		using type = variant_t<Ts...>;

		template<std::size_t N> using nth_type = typename impl::tools::nth_type_t<N, Ts...>;

		variant_t() noexcept = default;
		~variant_t() noexcept = default;

		template<typename T>
		variant_t(T&& value) noexcept {
			set(std::forward<T>(value));
		}

		template<typename T>
		decltype(auto) get() {
			static_assert(impl::is_convertible_to_variant<T, type>::value, "type T must be convertible to one of variant_t Ts types!");
			using result_type = typename impl::variant_converter_t<T, type>;
			std::size_t result_index = impl::tools::index_of_type<result_type, Ts...>::value;

			if (result_index != index()) {
				throw "variant_t doesn't hold the given type T!";
			}

			return impl::recursive_union_get_value<result_type>(storage);
		}

		template<std::size_t I, typename = std::enable_if_t<(I < sizeof...(Ts))>, typename Result = nth_type<I>>
		Result& get() {
			static_assert(impl::is_convertible_to_variant<Result, type>::value, "type T must be convertible to one of variant_t Ts types!");
			using result_type = typename impl::variant_converter_t<Result, type>;

			if (I != index()) {
				throw "variant_t doesn't hold the given type at index I!";
			}

			return impl::recursive_union_get_value<result_type>(storage);
		}

		template<typename T>
		void set(T&& value) noexcept {
			static_assert(impl::is_convertible_to_variant<T, type>::value, "type T must be convertible to one of variant_t Ts types!");
			using result_type = typename impl::variant_converter_t<T, type>;

			impl::recursive_union_set_value(storage, result_type(value), index());
			type_index = impl::tools::index_of_type<result_type, Ts...>::value;
		}

		template <typename Visitor>
		decltype(auto) visit(Visitor&& visitor) {
			return visit_impl(visitor);
		}

		std::size_t index() noexcept {
			return type_index;
		}

		template<typename T>
		void operator=(T&& value) {
			set(std::forward<T>(value));
		}

		template<typename T>
		operator T()& {
			return get<T>();
		}

	private:
		template<std::size_t I = 0, typename Visitor, typename
			std::enable_if_t<(I >= sizeof...(Ts))>* = nullptr>
			decltype(auto) visit_impl(Visitor&& visitor) {
			return;
		}

		template<std::size_t I = 0, typename Visitor, typename
			std::enable_if_t <(I < sizeof...(Ts)) && !impl::tools::is_callable<nth_type<I>, Visitor>::value>* = nullptr>
			decltype(auto) visit_impl(Visitor&& visitor) {
			return visit_impl<I + 1>(visitor);
		}

		template<std::size_t I = 0, typename Visitor, typename
			std::enable_if_t <(I < sizeof...(Ts)) && impl::tools::is_callable<nth_type<I>, Visitor>::value>* = nullptr>
			decltype(auto) visit_impl(Visitor&& visitor) {
			if (I == index()) {
				return visitor(get<I>());
			}
			visit_impl<I + 1>(visitor);
		}

		std::size_t type_index = -1;
		impl::recursive_union_t<Ts...> storage;
	};
}
