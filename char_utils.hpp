#ifndef UTIL_CHAR_UTILS_H
#define UTIL_CHAR_UTILS_H

#include <cctype>
#include <string>

namespace util {

    /** Makes an all-lowercase copy of a string. */
    std::string lowercase(const std::string& str);
    /** Makes an all-uppercase copy of a string. */
    std::string uppercase(const std::string& str);

    /** Converts a string to lowercase in-place. */
    std::string& convert_lowercase(std::string& str);
    /** Converts a string to uppercase in-place. */
    std::string& convert_uppercase(std::string& str);

    /**
     * @brief Checks if a character is a word character.
     * 
     * @param str the string containing the character.
     * @param pos the character's position in the string.
     * 
     * @return @c true if the character is a word character, @c false otherwise.
     * 
     * "Word character" means letter (as per @c std::isalpha) or underscore.
     * Characters outside the bounds of the string count as non-word characters,
     * and do not cause any errors.
     */
    bool word_char(const std::string& str, size_t pos);

};

#endif
