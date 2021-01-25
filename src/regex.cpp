#include "../regex.hpp"

using namespace util;


    //Different types match differently, but they all have the same behaviour:
    // - they set the mark
    // - they advance the parser (forwards or backwards)
    // - if they match, they
    //   - set match to true,
    //   - set len to the length of the match,
    //   - push an attempt(len, rep, alt [optional] )
    // - if they do not, they
    //   - revert to and unset the mark
    //   - leave match false.

bool literal::match_single(file_parser& parser, size_t rep, size_t& len){

    if(parser.match(string, !(modifier & LOOKAHEAD_MODIFIER)){
        len = string.length()
        push_attempt(len, rep);
    }
    
bool regex::match_single(file_parser& parser, size_t rep, size_t& len){
    
    parser.set_mark();
    
    bool match = false;
    
    
    switch(type){
        case LITERAL:
            if(parser.match(string)){
                len = string.length()
                attempts.push( attempt(len, rep) );
                match = true;
            }
            break;
            
        case CHAR_CLASS:
            if(parser && string.find(*parser) != NPOS){
                len = 1;
                attempts.push( attempt(len, rep) );
                match = true;
            }
            break;
            
        case COMPLEX:
            match = match_alternative(parser, alternatives.begin(), rep, len);
            break;
            
    }
    
    if(match){
        if(modifier & LOOKAHEAD_MODIFIER)
            parser.revert_to_mark( parser::keep_mark );
        
        return true;
    }
    else{
        parser.revert_to_mark( parser::remove_mark );
        
        return false;
    }
    
    
}

bool regex::match_alternative(file_parser& parser, regex::alt_list::iterator begin, 
                          size_t rep, size_t& len){
    
    for(auto alt_iter = begin; alt_iter != alternatives.end(); ++alt_iter){
        if(match_sequence(parser, alt_iter->begin(), *alt_iter, len)){
            attempts.push_back( attempt(sub_len, rep, alt_iter) );
            return true;
        }
    }
    
    return false;
}

bool regex::match_sequence(file_parser& parser, 
                              regex::seq_list::iterator start, regex::seq_list& list, 
                              size_t& len){
    len = 0;
    size_t sub_len;
    for(auto seq_iter = start; seq_iter != list.end(); ){
        auto& sub = *seq_iter;
        sub_len = 0;
        
        //If we have a match, advance to next in sequence
        if(sub->match(parser, sub_len)){
            len += sub_len;
            ++seq_iter;
        }
        //Otherwise, backtrack!
        else{
            for(;;){                
                //If we can't backtrack, report the failure of this alternative
                if(seq_iter == list.begin())
                    return false;
                
                //Step back
                --seq_iter;
                
                //Try to change previous match, repeat if necessary,
                //otherwise resume main loop
                sub = *seq_iter;
                if(sub->change_match(parser, len))
                    break;
            }
        }
    }
    
    return true;
}
            
bool regex::match(file_parser& parser, size_t& len){
        
    len = 0;
    size_t rep_len;
        
    for(size_t rep = 0;; ++rep){
        
        if(rep >= min_reps && modifier & RELUCTANT)
            break;
        if(rep >= max_reps)
            break;
        
        if(match_single(parser, rep+1, rep_len)){
            len += rep_len;
            
            //Short-circuit in case of zero-length match:
            //it will match as many times as needed!
            if(rep_len == 0){
                rep = std::max(rep, min_reps);
                break;
            }
        }
        else
            break;
    }
    
    if(rep < min_reps){
        //Insufficient number of matches: remove those that were made
        clear_attempt();
        return false;
    }
    else
        return true;
    
}

bool regex::change_submatch(file_parser& parser, size_t& len){
    attempt att = attempts.top();
    
    //Try changing the match of the current sequence
    for(auto seq_iter = --att.alt->end(); seq_iter != att.alt->begin(); --seq_iter){
        if(
            seq_iter->change_match(parser, len) 
            && 
            match_sequence(parser, seq_iter + 1, *att.alt, sub_len)
        ){
            len += sub_len;
            att.len = len;
            return true;
        }
    }
    
    auto next_alt = ++att.alt;
    pop_alternative();
    
    //Try the next alternative
    if(match_alternative(parser, next_alt, len)){
        return true;
    else
        return false;
    
    //Once we have returned false, the current attempt has been popped
}

bool regex::change_match(file_parser& parser, size_t& len){
    
    attempt att = attempts.top();
    size_t sub_len;
    
    //Try changing the number of repetitions
    switch(modifier & BACKTRACK_MODIFIER){
        
        case POSSESSIVE:
            return false;
            
        case RELUCTANT:
            if(att.rep >= max_reps)
                return false;
            
            if(match_single(parser, att.rep + 1, sub_len)){
                len += sub_len;
                return true;
            }
            else
                return false;
            
        case GREEDY:
            if(att.rep <= min_reps)
                return false;
            
            if(type == COMPLEX){
                //Try to change the last submatch
                if(change_submatch(parser, len))
                    //Match as many more as allowed given the new submatch
                else
                    //Let go of one repetition (destroyed by failed change_submatch), 
                    //recursively change previous one
                    return change_match(parser, len);
            }
            else{
                //Simply release one attempt
                len -= attempts.top().len;
                pop_attempt();
            }
            
            return true;
    }
}

bool regex::pop_attempt(){
    size_t rep = attempts.top().rep;
    attempts.pop();
    return rep != 0;
}

void regex::clear_attempt(){
    while(pop_attempt())
        parser.revert_to_mark( parser::remove_mark );
}
void regex::start_attempt(){
    attempts.push(attempt());
}

void regex::attempt::~attempt(){
    //Iterates backwards over the sequence match tried by the attempt,
    //clearing their latest attempt as well, since it corresponds to a sub-match
    //of this attempt.
    for(auto iter = alt->rbegin(); iter != alt->rend(); ++iter)
        (*iter)->clear_attempt();
}
