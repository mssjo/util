#ifndef UTIL_FILE_PARSER_H
#define UTIL_FILE_PARSER_H

#ifndef UTIL_FILE_PARSER_ERROR_THROW
#    define UTIL_FILE_PARSER_ERROR_THROW 0
#endif

#include <iostream>
#include <sstream>
#include <fstream>
#include <deque>
#include <stack>
#include <string>

#include "keyword_map.hpp"

#if UTIL_FILE_PARSER_ERROR_THROW
#include <exception>

namespace util {
    class file_parser_error : public std::exception {}
};

#endif

namespace util {
    
    class file_parser {
    private:  
        file_parser();
        
        std::string filename;
        
        bool owns_in;
        std::istream* in;
        bool owns_out;
        std::ostream* out;
        
        char cont_char;
        
        std::deque< std::pair<std::string, size_t> > bufs;
        size_t max_line;
        size_t min_line;
        
        size_t line;
        size_t col;
        
        bool echo;
        std::string echo_prefix;
        
        struct mark_location {
            size_t line;
            size_t col;
        };
        std::stack< mark_location > marks;
                
        bool advance_char(size_t opts = 0);
        bool get_line(const std::string& err = "");
        char get_char(bool backwards = false) const;
                
        enum match_style {
            CHAR, STRING, CHARS, NOT_CHARS, WORD_BOUNDARY
        };
        bool seek_impl(const std::string& str, size_t opts, const std::string& err, match_style style);
        bool match_impl(const std::string& str, size_t opts, const std::string& err, match_style style);
                                
        void error(bool show_context, const std::string& message) const;
        void error(bool show_context, const std::string& message, const std::string& buf, size_t pos, bool compute_offset = false) const;
        static void show_error_context(std::ostream& err, const std::string& buf, size_t pos);
        
        void skip_byte_order_mark();
        
    public:
        file_parser(std::istream& ist);
        file_parser(const std::string& filename, const std::string& err = "");
        
        void enable_echoing(bool print_current = true, const std::string& prefix = "");
        void enable_echoing(std::ostream& ost, bool print_current = true, const std::string& prefix = "");
        void disable_echoing();
        
        virtual ~file_parser();
        
        file_parser(const file_parser&) = delete;
        file_parser(file_parser&&) = default;
        
        file_parser& operator= (const file_parser&) = delete;
        file_parser& operator= (file_parser&&) = default;

        bool advance_line(bool backwards = false);
        file_parser& operator++ ();
        file_parser& operator-- ();
        file_parser& operator+= (size_t incr);
        file_parser& operator-= (size_t decr);
        
        char operator* () const;
        
        operator bool () const;
        
        void set_cont_char(char cont);
        
        static const size_t single_line = 0b0001;
        static const size_t consume     = 0b0010;
        static const size_t backwards   = 0b0100;
        static const size_t lookahead   = 0b1000;
        
        bool seek(char ch, size_t opts = 0, const std::string& err = "");
        bool seek(const std::string& str, size_t opts = 0, const std::string& err = "");
        bool seek_any_of(const std::string& str, size_t opts = 0, const std::string& err = "");
        bool seek_not_of(const std::string& str, size_t opts = 0, const std::string& err = "");
        bool seek_word_boundary(size_t opts = 0, const std::string& err = "");
        
        bool match(char ch, size_t opts = 0, const std::string& err = "");
        bool match(const std::string& str, size_t opts = 0, const std::string& err = "");
        bool match_any_of(const std::string& str, size_t opts = 0, const std::string& err = "");
        bool match_not_of(const std::string& str, size_t opts = 0, const std::string& err = "");
        bool match_word_boundary(size_t opts = 0, const std::string& err = "");
        
        const std::string& get_buffer() const;
        size_t get_column() const;
        size_t get_line_number() const;
        
        void set_mark();
        void reset_mark();
        void unset_mark();
        
        static const bool keep_mark = true;
        static const bool remove_mark = false; 
        void revert_to_mark(bool keep_mark = true);
        
        enum substr_flags {
            NONE                = 0,
            PARSE               = 1 << 0,
            KEEP_MARK           = 1 << 1,
            KEEP_NEWLINE        = 1 << 2,
            CONTAINS_ANY        = 1 << 3,
            CONTAINS_NOT        = 1 << 4
        };
        
        static const std::string whitespace; 
        static const std::string code_chars;
        
        std::string substr(size_t flags = substr_flags::NONE);
        bool substr(std::ostream& out, size_t flags = substr_flags::NONE, const std::string& chrs = "");
                                
        void error(const std::string& message) const;
        void error(const std::string& message, const std::string& str, size_t pos = 0, bool compute_offset = true) const;
        
        struct source {
            std::string filename;
            size_t line;
            size_t col;
        };
        
        source store_source() const;
        static file_parser load_source(const source& src, const std::string& err = "");
        static void error(const source& src, const std::string& message);
    };
    
};

#endif
