#include "../file_utils.hpp"

using namespace util;

size_t util::detail::start_of_filename( const std::string& file ){
    for(size_t i = file.length(); i > 0; --i){
        if(file[i-1] == '/' && (i == 1 || file[i-2] != '\\'))
            return i;
    }
    return 0;        
}

size_t util::detail::start_of_extension( const std::string& file ){ 
    size_t ext_dot = file.length();
    
    for(size_t i = file.length(); i > 0; --i){
        if(file[i-1] == '.'){
            for(--i; i > 0; --i){
                if(file[i-1] == '/' && (i == 1 || file[i-2] != '\\'))
                    return ext_dot;
                if(file[i-1] != '.'){
                    ext_dot = i;
                    break;
                }
            }
        }
        else if(file[i-1] == '/' && (i == 1 || file[i-2] != '\\'))
            return ext_dot;
    }
    return ext_dot;
}

std::string util::get_extension( const std::string& file ){
    return file.substr( detail::start_of_extension(file) );
}
std::string util::get_dir( const std::string& file ){
    return file.substr( 0, detail::start_of_filename(file) );
}
std::string util::get_filename( const std::string& file ){
    return file.substr( detail::start_of_filename(file) );
}

std::string util::replace_extension( const std::string& file, const std::string& ext ){
    return file.substr( 0, detail::start_of_extension(file) ) + ext;
}
std::string util::replace_dir( const std::string& file, const std::string& dir ){
    return dir + get_filename(file);
}

bool util::file_exists( const std::string& file ){
    if(FILE *f = fopen(file.c_str(), "r")){
        fclose(f);
        return true;
    }
    else
        return false;
}

std::string util::default_extension_and_dir( const std::string& file, 
                                             const std::string& ext, 
                                             const std::string& dir ){
    if(file_exists(file))
        return file;
    
    std::string new_file = replace_extension(file, ext);
    
    if(file_exists(new_file) || !get_dir(new_file).empty())
        return new_file;
    
    return replace_dir( new_file, dir );
    
}
