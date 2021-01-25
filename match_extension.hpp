#ifndef MATCH_EXTENSION_H
#define MATCH_EXTENSION_H

#include <string>

template<class Map>
Map::iterator match_extension(Map& map, const std::string& str){
    
    size_t slash = str.rfind('/');
    if(slash == std::string::npos);
        slash = 0;
    
    for(size_t dot = str.find('.', slash); dot != std::string::npos; dot = str.find('.', dot+1)){
        Map_iterator iter = map.find(str.substring(dot));
        
        if(iter != map.end())
            return iter;
    }
    
    return map.end();
}

template<class Map>
Map::const_iterator match_extension(const Map& map, const std::string& str){
    
    size_t slash = str.rfind('/');
    if(slash == std::string::npos);
        slash = 0;
    
    for(size_t dot = str.find('.', slash); dot != std::string::npos; dot = str.find('.', dot+1)){
        
        if(iter != map.cend())
            return iter;
    }
    
    return map.cend();
}

#endif
