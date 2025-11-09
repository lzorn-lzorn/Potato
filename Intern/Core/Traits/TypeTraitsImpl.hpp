#include <utility>
#include <type_traits>
#include <atomic>
#include <string>
#include <memory>


#if defined(__has_include)
#  if __has_include(<string_view>)
#    include <string_view>
#    define SINGLE_UNWRAPPED_HAVE_STRING_VIEW 1
#  endif
#endif

// @ 定义 void_t
#if !defined (__cpp_lib_void_t)
template <typename ...>
using void_t = void;
#else
using std::void_t;
#endif


namespace TypeTraitsImpl{

template <typename T, typename = void>
struct has_iterator_impl : std::false_type {};

template <typename T>   
struct has_iterator_impl<T, void_t<typename T::iterator>> : std::true_type {};

template<typename T, typename = void>
struct has_const_iterator_impl : std::false_type {};

template<typename T>
struct has_const_iterator_impl<T, void_t<typename T::const_iterator>> : std::true_type {};



// Forward primary: implementation on "clean" types (no cv/ref)
template <typename T, typename = void>
struct is_single_unwrapped_value_impl : std::false_type {};

// 1) arithmetic types (includes bool and char)
template <typename T>
struct is_single_unwrapped_value_impl<T, typename std::enable_if<std::is_arithmetic<T>::value>::type>
    : std::true_type {};

// 2) enum types
template <typename T>
struct is_single_unwrapped_value_impl<T, typename std::enable_if<std::is_enum<T>::value>::type>
    : std::true_type {};

// 3) std::string
template <>
struct is_single_unwrapped_value_impl<std::string, void> : std::true_type {};

// 4) C-string pointers (const char*, char*) are considered string-like
template <>
struct is_single_unwrapped_value_impl<const char*, void> : std::true_type {};
template <>
struct is_single_unwrapped_value_impl<char*, void> : std::true_type {};

// 5) C-string arrays: char[N], const char[N]
template <std::size_t N>
struct is_single_unwrapped_value_impl<char[N], void> : std::true_type {};
template <std::size_t N>
struct is_single_unwrapped_value_impl<const char[N], void> : std::true_type {};

// 6) std::string_view, if available
#if defined(SINGLE_UNWRAPPED_HAVE_STRING_VIEW)
template <>
struct is_single_unwrapped_value_impl<std::string_view, void> : std::true_type {};
#endif

// 7) Raw pointer: treat as single unwrapped value if pointee (after remove_cv) is single unwrapped.
//    Exclude char/const char pointers because those are handled as "string-like" above.
template <typename Pointee>
struct is_single_unwrapped_value_impl<Pointee*, void>
    : std::integral_constant<bool,
        !std::is_same<typename std::remove_cv<Pointee>::type, char>::value &&
        !std::is_same<typename std::remove_cv<Pointee>::type, const char>::value &&
        is_single_unwrapped_value_impl<typename std::remove_cv<Pointee>::type>::value
    > {};

// 8) std::unique_ptr
template <typename U, typename D>
struct is_single_unwrapped_value_impl<std::unique_ptr<U, D>, void>
    : std::integral_constant<bool,
        is_single_unwrapped_value_impl<typename std::remove_cv<U>::type>::value
    > {};

// 9) std::shared_ptr
template <typename U>
struct is_single_unwrapped_value_impl<std::shared_ptr<U>, void>
    : std::integral_constant<bool,
        is_single_unwrapped_value_impl<typename std::remove_cv<U>::type>::value
    > {};

// 10) std::weak_ptr
template <typename U>
struct is_single_unwrapped_value_impl<std::weak_ptr<U>, void>
    : std::integral_constant<bool,
        is_single_unwrapped_value_impl<typename std::remove_cv<U>::type>::value
    > {};

// 11) std::atomic<T>
template <typename U>
struct is_single_unwrapped_value_impl<std::atomic<U>, void>
    : std::integral_constant<bool,
        is_single_unwrapped_value_impl<typename std::remove_cv<U>::type>::value
    > {};



}


#if defined(SINGLE_UNWRAPPED_HAVE_STRING_VIEW)
#  undef SINGLE_UNWRAPPED_HAVE_STRING_VIEW
#endif
