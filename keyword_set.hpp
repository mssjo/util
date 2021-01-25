#ifndef UTIL_KEYWORD_SET_H
#define UTIL_KEYWORD_SET_H

#include <algorithm>
#include <map>
#include <unordered_set>

#include "char_utils.hpp"

namespace util {

    /**
     * @brief A set of strings that enables for efficient checking
     * whether a substring of unspecified length, appearing at a specific 
     * location in a string, is contained in the set.
     * 
     * This class supports a very limited number of operations. It is intended
     * to be constructed and filled with elements, and then used in a read-only fashion.
     * Also, this very simple implementation is probably not as efficient as a prefix tree.
     */
    class keyword_set {
    private:
        using internal_set = std::unordered_set<std::string>;
        
        size_t max_len;
        std::map<size_t, internal_set, std::greater<size_t>> set;
        
    public:
        
        /** @brief Creates an empty keyword set. */
        keyword_set() : max_len(0), set() {}
        
        /**
         * @brief Creates a keyword set from a list of keywords
         * 
         * @param init an @c std::initializer_list containing the keywords that
         *      should initially be contained in the set.
         */
        keyword_set( std::initializer_list<std::string> init ) : max_len(0), set() {
            size_t len;
            for(auto key : init){
                len = key.length();
                if(max_len < len)
                    max_len = len;
                
                set[len].insert(key);
            }
        }
        
        /**
         * @brief Inserts a keyword into the set.
         *
         * @param key the keyword.
         * 
         * @return @c true if the keyword was actually inserted, @c false if it already existed
         *          (this has no significance for the behaviour of the set).
         */
        bool insert(const std::string& key){
            size_t len = key.length();
            if(len > max_len)
                max_len = len;
            set[len].insert(key).second;
        }
        bool insert(std::string&& key){
            size_t len = key.length();
            if(len > max_len)
                max_len = len;
            set[len].insert(std::move(key)).second;
        }
        
        
        /**
         * @brief Matches the substring at a specified location against the set.
         * 
         * @param str the string containing the potential match.
         * @param pos the position where the match should start.
         * @param whole_word if true, the keyword will only match if the characters
         *      before and after it are non-word characters 
         *      (or the beginning/end of the string).
         * 
         * If one keyword is a prefix of another, the longest possible match is always chosen.
         * 
         * @return the length of the match, or @c std::string::npos if no keyword matched.
         */
        size_t match(const std::string& str, size_t pos = 0, bool whole_word = true) const {
            if(whole_word && util::word_char(str, pos-1))
                std::string::npos;
            
            size_t max = std::min(max_len, str.length() - pos);
            
            for(auto it = set.lower_bound(max); it != set.end(); ++it){
                const auto& [len, sub_map] = *it;
                
                if(whole_word && util::word_char(str, pos+len))
                    continue;
                
                auto match = sub_map.find(str.substr(pos, len));
                
                if(match != sub_map.end())
                    return len;
            }
            
            return std::string::npos;
        }    
        
        /**
         * @brief Like @c match, but matches against the full string.
         */
        size_t match_whole(const std::strng& str) const {
            auto it = set.find(str.length());
            if(it != set.end()){
                const auto& [len, sub_map] = *it;
                
                auto match = sub_map.find(str);
                
                if(match != sub_map.end())
                    return len;
            }
            return std::string::npos;
        }
            
        /**
         * @brief Erases a string from the set, if it exists.
         * @param key the string.
         * @return true if the string was contained in the set,
         *          false otherwise.
         */
        bool erase(const std::string& key) {
            auto it = map.find(key.length());
            
            if(it == map.end())
                return false;
            
            if(it->second.size() == 1 && !key.empty())
                map.erase(it);
            else
                it->second.erase(key);
            
            return true;
        }
        
    };

};


#endif
