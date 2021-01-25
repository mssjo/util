#ifndef UTIL_REGEX_H
#define UTIL_REGEX_H

#include <string>
#include <list>

namespace util {
    
    class file_parser;

    class regex_impl {
    protected:
        size_t min_rep; //minimum allowed repetitions
        size_t max_rep; //maximum allowed repetitions
        
        size_t group;   //capturing group index (zero for none)
                
        enum {
            SINGLE             = 0b0 00 00 000 000001,
            ZERO_OR_ONE        = 0b0 00 00 000 000010,
            ZERO_OR_MORE       = 0b0 00 00 000 000100,
            ONE_OR_MORE        = 0b0 00 00 000 001000,
            MIN_UPTO_MAX       = 0b0 00 00 000 010011,
            MIN_OR_MORE        = 0b0 00 00 000 101100,  
            NUMBER_MODIFIER    = 0b0 00 00 000 111111,
            
            GREEDY             = 0b0 00 00 001 000000, 
            RELUCTANT          = 0b0 00 00 010 000000,
            POSSESSIVE         = 0b0 00 00 100 000000,
            BACKTRACK_MODIFIER = 0b0 00 00 111 000000,
            
            FORWARDS           = 0b0 00 01 000 000000,
            BACKWARDS          = 0b0 00 10 000 000000,
            DIRECTION_MODIFIER = 0b0 00 11 000 000000,
            
            LOOKAHEAD          = 0b0 01 00 000 000000,
            NEG_LOOKAHEAD      = 0b0 10 00 000 000000,
            LOOKAHEAD_MODIFIER = 0b0 11 00 000 000000,
            
            ATOMIC             = 0b1 00 00 000 000000
        } modifier;            
        
        struct attempt {
            size_t len;
            size_t rep;
                        
            ~attempt();
        };
        std::stack< std::unique_ptr<attempt> > attempts
                
        void pop_attempt();
        void pop_repeated_attempt();
                
        virtual bool change_match(file_parser& parser, size_t& len);
        virtual bool match_single(file_parser& parser, size_t rep, size_t& len) = 0;
        
    public:
        bool match(file_parser& parser, size_t& len);
    };
    
    class literal : public regex_impl {
    private:
        std::string literal;
        
        virtual bool match_single(file_parser& parser, size_t rep, size_t& len);
    }
    
    class char_class : public regex_impl {
    private:
        std::string chars;
        bool negated;
        
        virtual bool match_single(file_parser& parser, size_t rep, size_t& len);
    };
    
    class assertion : public regex_impl {
    private:
        enum {
            BEGIN, END, LINE BOUNDARY, NOT_BOUNDARY
        } type;
        
        virtual bool match_single(file_parser& parser, size_t rep, size_t& len);
    };
        
    class alternative : public regex_impl {
    private: 
        //First option
        std::unique_ptr< regex_impl > head;
        //Second option, which may itself be an alternative
        std::unique_ptr< regex_impl > tail;
        
        virtual bool match_single(file_parser& parser, size_t rep, size_t& len);
        
        struct alt_attempt : public attempt {
            bool tail_attempted;
        };
    };
    
    class sequence : public regex_impl {
    private:
        //First option
        std::unique_ptr< regex_impl > head;
        //Second option, which may itself be a sequence
        std::unique_ptr< regex_impl > tail;
        
        virtual bool match_single(file_parser& parser, size_t rep, size_t& len);
    };
    
}

#endif
