#include "file_parser.hpp"

using namespace util;

#define BUF bufs[max_line - line].first
#define MARK_COUNT bufs[max_line - line].second
#define MARK marks.top()
#define NPOS std::string::npos

#include <unistd.h>


const std::string file_parser::whitespace = " \t\n\r\v\f";
const std::string file_parser::code_chars = std::string("\00\01\02\03\04\05\06\07\10\11\12\13\14\15\16\17\20\21\22\23\24\25\26\27\30\31\32\33\34\35\36\37", 040);

file_parser::file_parser()
 : filename(""), in(nullptr), owns_in(false),
   out(nullptr), echo(false), owns_out(false), echo_prefix(""),
   cont_char(0),
   min_line(0), max_line(0), line(0), col(0),
   marks(), bufs({{"", 0}})
{}
file_parser::file_parser(std::istream& ist)
 : file_parser()
{
    filename = "<input stream>";
    in = &ist;
    
    get_line();
    skip_byte_order_mark();
}

file_parser::file_parser(const std::string& filename, const std::string& err)
 : file_parser()
{
    this->filename = filename;
    in = new std::ifstream(filename);
    owns_in = true;
    
    if(!in->good()){
        if(err.empty())
            error(false, "File not found: " + filename);
        else
            error(false, err);
    }  
    get_line(err);
    skip_byte_order_mark();
}

void file_parser::skip_byte_order_mark(){
    if(match("\xEF\xBB\xBF"))
        *this += 3;
}

file_parser::~file_parser(){
    disable_echoing();
    if(owns_in)
        delete in;
}

void file_parser::enable_echoing(bool print_current, const std::string& prefix){
    enable_echoing(std::cout, print_current, prefix);
    owns_out = false;
}
void file_parser::enable_echoing(std::ostream& ost, bool print_current, const std::string& prefix){
    disable_echoing();
    
    echo = true;
    echo_prefix = prefix;
    
    out = &ost;
    owns_out = true;
    
    if(print_current)
        ost << echo_prefix << BUF << "\n";
}
void file_parser::disable_echoing(){
    if(owns_out)
        delete out;
    owns_out = false;
    
    echo = false;
}

bool file_parser::get_line(const std::string& err){
    std::string tmp_buf;
        
    if(!std::getline(*in, tmp_buf)){
        bufs.push_front( std::make_pair("", 0) );
        
        if(!err.empty())
            error(err);
        else
            return false;
    }
    
    //Clear unneeded lines
    for(; min_line < line && bufs.back().second == 0; ++min_line)
        bufs.pop_back();
    
    ++line;
    ++max_line;
    col = 0;
        
    if(marks.empty()){
        //Simpler than push_front followed by pop_back
        ++min_line;
        std::swap(BUF, tmp_buf);
    }
    else
        bufs.push_front( std::make_pair(tmp_buf, 0) );
    
    
    while(!BUF.empty() && BUF[BUF.length() - 1] == cont_char){
        if(!std::getline(*in, tmp_buf)){        
            if(echo)
                *out << echo_prefix << BUF << "\n";
            
            if(!err.empty())
                error(err);
            else
                return false;
        }
        
        BUF += tmp_buf;
    }
    
    if(echo)
        *out << echo_prefix << BUF << "\n";
    
    return true;
}

bool file_parser::advance_char(size_t opts){
    if(opts & backwards){
        if(col > 0){
            --col;
            return true;
        }
        else if(opts & single_line)
            return false;
        else
            return advance_line(true);
    }
    else{
        if(col < BUF.length()){
            ++col;
            return true;
        }
        else if(opts & single_line)
            return false;
        else
            return advance_line();
    }
}
bool file_parser::advance_line(bool backwards){
    if(backwards){
        if(line > min_line){
            --line;
            col = BUF.length();
            return true;
        }
        else
            return false;   //no backwards get_line()
    }
    else{
        if(line < max_line){
            ++line;
            col = 0;
            return true;
        }
        else
            return get_line();
    }
}

file_parser& file_parser::operator++ () {
    advance_char();
    return *this;
}
file_parser& file_parser::operator-- () {
    advance_char(backwards);
    return *this;
}
file_parser& file_parser::operator+= (size_t incr) {
    for(size_t n = 0; n < incr; ++n)
        advance_char();
    return *this;
}
file_parser& file_parser::operator-= (size_t decr) {
    for(size_t n = 0; n < decr; ++n)
        advance_char(backwards);
    return *this;
}

char file_parser::get_char(bool backwards) const {
    if(backwards){
        if(col > 0)
            return BUF[col-1];
        else
            return '\n';
    }
    else{
        if(col < BUF.length())
            return BUF[col];
        else
            return '\n';
    }        
}

char file_parser::operator* () const {
    return get_char();
}

file_parser::operator bool () const {
    return in->good() && col < BUF.length();
}

void file_parser::set_cont_char(char cont){
    cont_char = cont;
}
            

bool file_parser::seek(char chr, size_t opts, const std::string& err){
    return seek_impl(std::string(1, chr), opts, err, CHAR);
}
bool file_parser::seek(const std::string& str, size_t opts, const std::string& err){
    return seek_impl(str, opts, err, STRING);
}
bool file_parser::seek_any_of(const std::string& chrs, size_t opts, const std::string& err){
    return seek_impl(chrs, opts, err, CHARS);
}
bool file_parser::seek_not_of(const std::string& chrs, size_t opts, const std::string& err){
    return seek_impl(chrs, opts, err, NOT_CHARS);
}
bool file_parser::seek_word_boundary(size_t opts, const std::string& err){
    return seek_impl("", opts, err, WORD_BOUNDARY);
}
bool file_parser::seek_impl(const std::string& str, size_t opts, const std::string& err, match_style style){
    
    if(opts & lookahead)
        set_mark();
    
    do{
        if( match_impl(str, opts, "", style) ){
            if(opts & lookahead)
                revert_to_mark();
            
            return true;
        }
    } while (advance_char(opts));

    if(opts & lookahead)
        revert_to_mark();
    
    if(err.empty())
        return false;
    else
        error(err);
    
}

bool file_parser::match(char chr, size_t opts, const std::string& err){
    return match_impl(std::string(1, chr), opts, err, CHAR);
}
bool file_parser::match(const std::string& str, size_t opts, const std::string& err){
    return match_impl(str, opts, err, STRING);
}
bool file_parser::match_any_of(const std::string& chrs, size_t opts, const std::string& err){
    return match_impl(chrs, opts, err, CHARS);
}
bool file_parser::match_not_of(const std::string& chrs, size_t opts, const std::string& err){
    return match_impl(chrs, opts, err, NOT_CHARS);
}
bool file_parser::match_word_boundary(size_t opts, const std::string& err){
    return match_impl("", opts, err, WORD_BOUNDARY);
}
bool file_parser::match_impl(const std::string& str, size_t opts, const std::string& err, match_style style){
            
    set_mark();
    
    bool match = true;
    if(match){
        switch(style){
            case CHAR:
                if( str[0] != get_char(opts & backwards) )
                    match = false;
                else
                    advance_char(opts);
                break;
                
            case STRING:
                if(opts & backwards){
                    for(size_t i = str.length(); match && i > 0; --i){
                        if( str[i-1] != get_char(true) )
                            match = false;
                        else
                            advance_char(opts);
                    }
                }    
                else{
                    for(size_t i = 0; match && i < str.length(); ++i){
                        if( str[i] != get_char() )
                            match = false;
                        else
                            advance_char(opts);
                    }
                }   
                break;

            case CHARS: 
                if( str.find( get_char(opts & backwards) ) == NPOS )
                    match = false;
                else
                    advance_char(opts);
                break;

            case NOT_CHARS: 
                if( str.find( get_char(opts & backwards) ) != NPOS )
                    match = false;
                else
                    advance_char(opts);
                break;

            case WORD_BOUNDARY:
                match = (util::word_char(BUF, col-1) != util::word_char(BUF, col));
                break;
        }
    }
    
    if(match){
        if(opts & consume)
            unset_mark();
        else
            revert_to_mark( remove_mark );
        
        return true;
    }
    else if(err.empty()){
        revert_to_mark( remove_mark );
        return false;
    }
    else
        error(err);
}

const std::string& file_parser::get_buffer() const {
    return BUF;
}
size_t file_parser::get_column() const {
    return col;
}
size_t file_parser::get_line_number() const {
    return line;
}

void file_parser::set_mark(){
    marks.push({line, col});
    ++MARK_COUNT;
}

void file_parser::unset_mark(){
    
    //Decrement mark counter
    --(bufs[max_line - MARK.line].second);
    
    //Clear unneeded lines
    if(MARK.line == min_line){
        for(; min_line < line && bufs.back().second == 0; ++min_line)
            bufs.pop_back();
    }
    
    marks.pop();
}

void file_parser::reset_mark(){
    unset_mark();
    set_mark();
}

void file_parser::revert_to_mark(bool keep_mark){
    while(line > MARK.line)
        --line;
    while(line < MARK.line)
        ++line;
    
    col = MARK.col;
    
    if(!keep_mark)
        unset_mark();
}

std::string file_parser::substr(size_t flags){
    std::ostringstream ost;
    substr(ost, flags);
    return ost.str();
}
bool file_parser::substr(std::ostream& out, size_t flags, const std::string& chrs){
        
    bool check_passed = false;
    
    if(marks.empty())
        return check_passed;
        
    size_t begin_line = std::min(MARK.line, line);
    size_t end_line   = std::max(MARK.line, line);
    size_t begin_col, end_col;
    if(begin_line == end_line){
        begin_col = std::min(MARK.col, col);
        end_col   = std::max(MARK.col, col);
    }
    else if(begin_line == line){
        begin_col = col;
        end_col   = MARK.col;
    }
    else{
        begin_col = MARK.col;
        end_col   = col;
    }
        
    for(size_t tmp_line = begin_line; tmp_line <= end_line; ++tmp_line){
        std::string& tmp_buf = bufs[max_line - tmp_line].first;
        
        for(
            size_t tmp_col = (tmp_line == begin_line ? begin_col : 0); 
                   tmp_col < (tmp_line == end_line   ? end_col   : tmp_buf.length());
            ++tmp_col
        ){
            if(!check_passed){
                if(flags & CONTAINS_ANY)
                    check_passed = (chrs.find( tmp_buf[tmp_col] ) != NPOS);
                else if(flags & CONTAINS_NOT)
                    check_passed = (chrs.find( tmp_buf[tmp_col] ) == NPOS);
            }
            
            out << tmp_buf[tmp_col];
        }
        
        if((flags & KEEP_NEWLINE) && tmp_line < end_line)
            out << '\n';
    }
    
    if(!(flags & KEEP_MARK))
        unset_mark();
    
    return check_passed;
}

file_parser::source file_parser::store_source() const {
    if(!owns_in)
        error(false, "Cannot store source of stream-based (rather than file-based) parser");
    
    return {filename, line, col};
}
file_parser file_parser::load_source(const file_parser::source& src, const std::string& err){
    file_parser parser(src.filename, err);
    
    while(parser.line < src.line){
        if(!parser.advance_line()){
            if(err.empty())
                return parser;
            else
                parser.error(false, err);
        }
    }
    
    parser.col = src.col;
    return parser;
}

void file_parser::error(const file_parser::source& src, const std::string& message) {
    load_source(src, message).error(true, message);
}
void file_parser::error(const std::string& message) const {
    error(true, message);
}
void file_parser::error(const std::string& message, const std::string& str, size_t pos, bool compute_offset) const {
    error(true, message, str, pos, compute_offset);
}
void file_parser::error(bool show_context, const std::string& message) const{
    error(show_context, message, BUF, col);
}
void file_parser::error(bool show_context, const std::string& message, const std::string& buf, size_t pos, bool compute_offset) const {
    
#if UTIL_FILE_PARSER_ERROR_THROW
    std::ostrinstream err;
#    define ERR_STR err
#else
#    define ERR_STR std::cerr
#endif
    
    size_t offs_pos = col;
    if(compute_offset)
        offs_pos = col + pos - buf.length();
    
    ERR_STR << "\nERROR in file \"" << filename << "\"";
    if(!show_context)
        ERR_STR << " (not found)";
    ERR_STR << ", line " << line << ", column " << offs_pos;
    
    ERR_STR << "\nERROR: " << message << "\n\n";
    
    if(show_context)
        show_error_context(ERR_STR, buf, pos);
        
#if UTIL_FILE_PARSER_ERROR_THROW
    throw file_parser_error(err.str());
#else
    exit(EXIT_FAILURE);
#endif
    
}
void file_parser::show_error_context(std::ostream& err, const std::string& buf, size_t pos){
    const size_t max_print_length = 64;
    
    if(pos >= buf.length())
        pos = buf.empty() ? 0 : buf.length() - 1;
    
    if(!buf.empty() && buf.length() < max_print_length){
        err << "\t" << buf << "\n";
        err << "\t" << std::string(pos, '_') << "^\n";
    }
    else if(buf.length() > max_print_length){
        if(pos < max_print_length/2){
            err << "\t" << buf.substr(0, max_print_length - 3) << "...\n";
            err << "\t" << std::string(pos, '_') << "^\n";
        }
        else if(buf.length() - pos < max_print_length/2){
            err << "\t..." << buf.substr(buf.length() - (max_print_length - 3)) << "\n";
            err << "\t" << std::string(pos - (buf.length() - max_print_length), '_') << "^\n";
        }
        else{
            err << "\t..." << buf.substr(pos - (max_print_length/2) + 3, max_print_length - 6) << "...\n";
            err << "\t" << std::string(max_print_length/2, '_') << "^\n";
        }
    }
    err << "\n";
}
