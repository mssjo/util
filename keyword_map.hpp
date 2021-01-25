#ifndef UTIL_KEYWORD_MAP_H
#define UTIL_KEYWORD_MAP_H

#include <algorithm>
#include <map>
#include <unordered_map>

#include "char_utils.hpp"

namespace util{
    /**
     * @brief Like @c keyword_set, but each keyword maps to a value.
     * 
     * Typical values would be labels for actions to be taken, or strings to be
     * substituted for the keywords in the string. Otherwise, the behaviour and 
     * performance is very similar to @c keyword_set.
     * 
     * @tparam T the type of the values. Must be default-constructible.
     */
    template<typename T>
    class keyword_map {
    private:
        using value_type = T;
        using internal_map = std::unordered_map<std::string, value_type>;
        
        size_t max_len;
        std::map<size_t, internal_map, std::greater<size_t>> map;
        
        std::pair<typename internal_map::const_iterator, size_t> sentinel() const {
            return std::make_pair(map.find(0)->second.end(), std::string::npos);
        }
        
    public:
        
        /** @brief Creates an empty keyword map. */
        keyword_map() : max_len(0), map({{0, internal_map()}}) {}
        
        /**
         * @brief Creates a keyword map from a list of keyword-value pairs
         * 
         * @param init an @c std::initializer_list containing the keyword-value pairs
         *      should initially be contained in the map.
         */
        keyword_map( std::initializer_list< std::pair<std::string, value_type> > init ) : max_len(0), map({{0, internal_map()}}) {
            size_t len;
            for(auto[key, val] : init){
                len = key.length();
                if(max_len < len)
                    max_len = len;
                
                map[len][key] = val;
            }
        }
        
        /**
         * @brief Retrieves a reference to the value associated with a keyword.
         * 
         * @param key the keyword.
         * 
         * @return a reference to the associated value.
         * 
         * If no value is associated with the keyword, a new one is created and
         * can be assigned to using the returned reference.
         */
        value_type& operator[] (const std::string& key){
            return map[key.length()][key];
        }
        
        /**
         * @brief Inserts a keyword-value pair into the map.
         *
         * @param key_val the keyword-value pair.
         * 
         * @return an iterator-bool pair. The iterator points to the keyword-value pair
         *      corresponding to the provided keyword, and the bool is @c true if the
         *      insertion actually happened. It is @c false if the value already existed,
         *      in which case the value is not overwritten and the old value can be accessed
         *      using the iterator. In/decrementing the iterator does not make much sense.
         */
        std::pair<typename internal_map::iterator, bool> 
        insert(const std::pair<std::string, value_type>& key_val){
            size_t len = key_val.first.length();
            if(len > max_len)
                max_len = len;
            return map[len].insert(key_val);
        }
        std::pair<typename internal_map::iterator, bool> 
        insert(std::pair<std::string, value_type>&& key_val){
            size_t len = key_val.first.length();
            if(len > max_len)
                max_len = len;
            return map[len].insert(key_val);
        }
        
        /**
         * @brief Matches the substring at a specified location against the map.
         * 
         * @param str the string containing the potential match.
         * @param pos the position where the match should start.
         * @param whole_word if true, the keyword will only match if the characters
         *      before and after it are non-word characters 
         *      (or the beginning/end of the string).
         * 
         * If one keyword is a prefix of another, the longest possible match is always chosen.
         * 
         * @return an iterator-integer pai. The integer is the length of the match, or 
         * @c std::string::npos if no keyword matched (like the corresponding return value 
         * for @c keyword_set). The iterator points to the matching keyword-value pair
         * if there was a match, and is unspecified if there was none.
         */
        std::pair<typename internal_map::const_iterator, size_t> 
        match(const std::string& str, size_t pos = 0, bool whole_word = true) const {
            if(whole_word && util::word_char(str, pos-1))
                return sentinel();
            
            size_t max = std::min(max_len, str.length() - pos);
            
            for(auto it = map.lower_bound(max); it != map.end(); ++it){
                const auto& [len, sub_map] = *it;
                
                if(whole_word && util::word_char(str, pos+len))
                    continue;
                
                auto match = sub_map.find(str.substr(pos, len));
                
                if(match != sub_map.end())
                    return std::make_pair(match, len);
            }
            
            return sentinel();
        }   
        
        /**
         * @brief Like @c match, but matches against the full string.
         */
        std::pair<typename internal_map::const_iterator, size_t> 
        match_whole(const std::string& str) const {
            auto it = map.find(str.length());
            if(it != map.end()){
                const auto& [len, sub_map] = *it;
                
                auto match = sub_map.find(str);
                
                if(match != sub_map.end())
                    return std::make_pair(match, len);
            }
            return sentinel();
        }
        
        /**
         * @brief Erases a string from the map, if it exists.
         * @param key the string.
         * @return true if the string was contained in the map,
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
