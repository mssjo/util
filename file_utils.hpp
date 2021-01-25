#ifndef UTIL_FILE_UTILS_H
#define UTIL_FILE_UTILS_H

#include <iostream>
#include <fstream>
#include <string>

namespace util {

    namespace detail {
    
        //Returns the index of the character following the last slash
        //not escaped by a backslash, or 0 if it doesn't exist
        size_t start_of_filename( const std::string& file );
        
        //Returns the index of the first dot after start_of_filename that
        //is separated from start_of_filename by at least one non-dot character,
        //or file.length() if no such dot exists
        size_t start_of_extension( const std::string& file );
    }
    
    std::string get_extension( const std::string& file );
    std::string get_dir( const std::string& file );
    std::string get_filename( const std::string& file );

    bool file_exists( const std::string& file );
    
    std::string replace_extension( const std::string& file, const std::string& ext );
    std::string replace_dir( const std::string& file, const std::string& dir );
    
    std::string default_extension_and_dir( const std::string& file, 
                                           const std::string& ext, const std::string& dir );
}

#endif
