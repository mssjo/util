#ifndef UTIL_INSTANCEOF_H
#define UTIL_INSTANCEOF_H

#include <type_traits>
#include <memory>

namespace util {

/**
 * @brief Checks if an object is an instance of a class or one of its subclasses,
 * in the style of Java's @c instanceof operator.
 * 
 * The implementation uses @c std::is_base_of.
 * @code instanceof<B>(a) @endcode is equivalent to @code a instanceof B @endcode in Java.
 * 
 * @tparam Base the base class.
 * @tparam T the type of the argument.
 * 
 * @param t a pointer to an object of type @p T
 * 
 * @return true if @p T is the same as @p Base or a subclass thereof, false otherwise.
 */
template<typename Base, typename T>
inline bool instanceof(const T* t) {
    return std::is_base_of<Base, T>::value;
}
template<typename Base, typename T>
inline bool instanceof(const T& t) {
    return std::is_base_of<Base, T>::value;
}

};

#endif
