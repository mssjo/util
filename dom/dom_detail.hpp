#ifndef DOM_DETAIL_H
#define DOM_DETAIL_H

#define NOT_IMPL static_assert(false, __FUNCTION__ " is not supported for " + type_descr());

namespace detail{
    
    template<typename Target, class SpecIter, class GlobIter>
    class dom_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;    
        using value_type = Target;                            
        using difference_type = ptrdiff_t;                      
        using pointer = Target*;                            
        using reference = Target&;
        
    private:
        union glob_or_spec {
            GlobIter glob;
            SpecIter spec;
                    
            glob_or_spec(SpecIter& it) : spec(it) {}
            glob_or_spec(GlobIter& it) : glob(it) {}
        } iter;
        
        bool global;
        
        
    public:
        dom_iterator(SpecIter begin) : global(false), iter(begin) {}
        dom_iterator(GlobIter begin) : global(true ), iter(begin) {}
        
        dom_iterator& operator++() {
            if(global)
                ++iter.glob;
            else
                ++iter.spec;
            
            return *this;
        }
        
        reference operator* () {
            if(global)
                return **iter.glob;
            else
                return ***iter.spec;
        }
    
        friend bool operator== (const dom_iterator& a, const dom_iterator& b)
        { 
            return a.global == b.global && a.global ? 
                (a.iter.glob == b.iter.glob) : (a.iter.spec == b.iter.spec);
        }
        friend bool operator!= (const dom_iterator& a, const dom_iterator& b)
        { 
            return a.global != b.global || a.global ? 
                (a.iter.glob != b.iter.glob) : (a.iter.spec != b.iter.spec);
        }
    };
    
    using mutbl_dom_iterator = dom_iterator<DOM::dom_element, 
                                            indir_list_type::iterator,
                                            elem_list_type ::iterator>;
                                            
    using const_dom_iterator = dom_iterator<const DOM::dom_element, 
                                            indir_list_type::const_iterator,
                                            elem_list_type ::const_iterator>;
           
    template<typename Target, class Iter, typename Ptr>
    class dom_query {
        
    private:
        
        dom_query() = default;
        
        friend class DOM::dom_element;
        
        using Ref = Target&;
        
        Ptr target;
        std::string query;
        
        //We use this clumsier approach, rather than polymorphism or template specialisation,
        //for a very good reason: we want to be able to request a whole chain of sub-entries,
        //use a single or_error(), and make the error appear at the first invalid step, rather
        //than the most recent one. Thus, if
        //    X.element("foo").element("bar").attribute("baz").or_error();
        //returned a special attribute_query, then it would be very hard for it to give
        //the proper error as an element_query if "foo" is in fact nonexistent!
        //Allowing queries to change type at runtime resolves this, although it moves
        //some errors (such as X.attribute("foo").element("bar")) to runtime rather than
        //compile time. This may cause hidden bugs, but may also increase readability,
        //compared to your typical template error messages.
        //If I were the Boost authors, I would use expression template magic to achieve this more elegantly.
        enum {
            ELEMENT,
            ELEMENT_LIST,
            ATTRIBUTE,
            CONTENT
        } type;
        enum {
            VALID,
            INVALID,
            DEFAULTED
        } validity;
        
        std::string what() const { 
            switch(type){
                case ELEMENT:
                    return query.empty() ? "first element" : "element with tag <" + query + ">";
                case ELEMENT_LIST:
                    return query.empty() ? "all elements" : "all elements with tag <" + query + ">";
                case ATTRIBUTE:
                    return "attribute \"" + query + "\"";
                case CONTENT:
                    return "tag content";
            }
        }
        
        std::string type_descr() const {
            switch(type){
                case ELEMENT:
                    return "element";
                case ELEMENT_LIST:
                    return "list of elements";
                case ATTRIBUTE:
                    return "attribute";
                case CONTENT:
                    return "tag content";
            }
        }
                
        void add_elements_impl(const std::string& tag){
            target->add_element(tag);
        }
        template<class... Strings>
        void add_elements_impl(const std::string& tag, Strings... tags){            
            target->add_element(tag);
            add_elements_impl(tags...);
        }
              
    private:    //...but intended for dom_element
        
        dom_query(Ptr target) 
        : target(target), query(target->name), 
          validity(VALID), type(ELEMENT) 
        {}
              
    public:
            
        dom_query& all_elements(const std::string& tag = ""){
            if(type != ELEMENT)
                target->error("Invalid operation: requesting element list from " + type_descr());
            
            type = ELEMENT_LIST;
            
            if(validity == VALID){
            
                query = tag;
                
                if(!tag.empty()){
                    auto iter = target->elem_map.find(query);
                    if(iter == target->elem_map.end())
                        validity = INVALID;
                }
            }
                
            return *this;
        }
        dom_query& element(const std::string& tag = ""){
            if(type != ELEMENT)
                target->error("Invalid operation: requesting element from " + type_descr());
            
            type = ELEMENT;
            
            if(validity == VALID){
                
                query = tag;
                
                if(tag.empty()){
                    if(target->elem_list.empty())
                        validity = INVALID;
                    else
                        target = target->elem_list.front();
                }
                else{
                    auto iter = target->elem_map.find(query);
                    if(iter == target->elem_map.end())
                        validity = INVALID;
                    else
                        target = *( iter->second.front() );
                }
            }
            
            return *this;
        }
        dom_query& unique_element(const std::string& tag, const std::string& err = ""){
            if(type != ELEMENT)
                target->error("Invalid operation: requesting element from " + type_descr());
            
            type = ELEMENT;
            
            if(validity == VALID){
            
                query = tag;
                
                if(tag.empty()){
                    if(target->elem_list.size() != 1)
                        target->error(err.empty() ? 
                            "Unique " + what() + " requested, but " 
                            + (target->elem_list.empty() ? "none" : std::to_string(target->elem_list.size())) 
                            + " found"
                            : err);
                    else
                        target = target->elem_list.front();
                }
                else{
                    auto iter = target->elem_map.find(query);
                    if(iter == target->elem_map.end()){
                        validity = INVALID;
                    }
                    else if(iter->second.size() != 1){
                        validity = INVALID;
                        target->error(err.empty() ? 
                                "Unique " + what() + " requested, but " + std::to_string(iter->second.size()) + " found"
                                : err);
                    }
                    else
                        target = *( iter->second.front() );
                }
            }
            
            return *this;
        }
        dom_query& attribute(const std::string& name){
            if(type != ELEMENT)
                target->error("Invalid operation: requesting attribute from " + type_descr());
            
            type = ATTRIBUTE;
            
            if(validity == VALID){
            
                query = name;
                
                auto iter = target->attrs.find(query);
                if(iter == target->attrs.end())
                    validity = INVALID;
            }
            
            return *this;
        }
        dom_query& content(){
            if(type != ELEMENT)
                target->error("Invalid operation: requesting attribute from " + type_descr());
            
            type = CONTENT;
            
            if(validity == VALID)
                query = target->cont;
            
            return *this;
        }
        
        
        Iter begin() { 
            if(type != ELEMENT_LIST)
                target->error("Invalid operation: iterating over " + type_descr());
            if(validity != VALID)
                return Iter( target->elem_list.end() );
            
            return query.empty() ? Iter( target->elem_list.begin() ) : Iter( target->elem_map.find(query)->second.begin() );
        };
        Iter end()   { 
            if(type != ELEMENT_LIST)
                target->error("Invalid operation: iterating over " + type_descr());
            if(validity != VALID)
                return Iter( target->elem_list.end() );
            
            return query.empty() ? Iter( target->elem_list.end() ) : Iter( target->elem_map.find(query)->second.end() );
        };
        
        dom_query& or_error(const std::string& err = "") {
            if(validity == INVALID)
                target->error(err.empty() ? what() + " requested but not found" : err);
            
            return *this;
        }
        dom_query& or_default(const std::string& deflt) {
            switch(type){
                case ELEMENT:
                case ELEMENT_LIST:
                    target->error("Invalid operation: defaulting value of " + type_descr());
                    
                case ATTRIBUTE:
                case CONTENT:
                    switch(validity){
                        case VALID:
                            break;
                            
                        case INVALID:
                        case DEFAULTED:
                            query = deflt;
                            validity = DEFAULTED;
                            break;
                    }
            }
            
            return *this;
        }
        dom_query& nonempty(const std::string& err = "") {
            const std::string error = err.empty() ? "Nonempty " + type_descr() + " required" : err;
            
            switch(validity){
                case INVALID:
                    break;
                
                case DEFAULTED:
                    if(query.empty())
                        target->error(error);
                    break;
                    
                case VALID:
                    switch(type){
                        case ELEMENT:
                            target->error("Invalid operation: checking if element is empty");
                            
                        case ELEMENT_LIST:
                            if(target->elem_map.find(query)->second.empty())
                                target->error(error);
                            break;
                            
                        case ATTRIBUTE:
                            if(target->attrs.find(query)->second.empty())
                                target->error(error);
                            break;
                            
                        case CONTENT:
                            if(query.empty())
                                target->error(error);
                            break;
                    }
            }
            
            return *this;                    
        }
        
        std::string val() const { 
            switch(type){
                case ELEMENT:
                case ELEMENT_LIST:
                    target->error("Invalid operation: dereferencing " + type_descr() + " as value");
                    
                case ATTRIBUTE:
                case CONTENT:
                    switch(validity){
                        case VALID:
                            if(type == ATTRIBUTE)
                                return target->attrs.find(query)->second;
                            else
                                return query;
                            
                        case INVALID:
                            target->error("Invalid operation: dereferencing invalid query");
                            
                        case DEFAULTED:
                            return query;
                    }
            }
        }
        
        //When you want something more explicit than val()
        std::string string_val() const { return val(); }
        //When you want something less explicit than val()
        operator std::string() const { return val(); }
        
        bool bool_val() const { 
            std::string str = val();
    
            if(std::isdigit(str[0]))
                return (bool) int_val();
                
            std::string ins = util::lowercase(str);
                
            if(ins == "true")
                return true;
            else if(ins == "false")
                return false;
            else
                target->error(what() + " has value \"" + str + "\", true/false (or number) expected");
        }
        char char_val() const { 
            std::string str = val();
            
            if(str.length() != 1)
                target->error(what() + " has value \"" + str + "\", single character expected");
            else
                return str[0];
        }
        long int_val() const { 
            size_t pos = 0;
            std::string str = val();
            
            long int v = std::stol(str, &pos, 0);
            while(pos < str.length() && std::isspace(str[pos]))
                ++pos;
            
            if(pos != str.length())
                target->error(what() + " has value \"" + str + "\", integer expected");
            
            return v;
        }
        size_t  uint_val() const { 
            size_t pos = 0;
            std::string str = val();
            
            size_t v = std::stoul(str, &pos, 0);
            while(pos < str.length() && std::isspace(str[pos]))
                ++pos;
            
            if(pos != str.length())
                target->error(what() + " has value \"" + str + "\", unsigned integer expected");
            
            return v;
        }
        double float_val() const {
            size_t pos = 0;
            std::string str = val();
            
            double v = std::stod(str, &pos);
            while(pos < str.length() && std::isspace(str[pos]))
                ++pos;
            
            if(pos != str.length())
                target->error(what() + " has value \"" + str + "\", floating-point number expected");
            
            return v;
        }
        
        Ref get() { 
            if(type != ELEMENT)
                target->error("Invalid operation: dereferencing " + type_descr() + " as element");
            if(validity != VALID)
                target->error("Invalid operation: dereferencing non-existent " + what());
            
            return *target;          
        }
        operator Ref () { return get(); }
        
        dom_query& add_element(const std::string& tag){
            if(type != ELEMENT)
                target->error("Invalid operation: adding element to " + type_descr());
            if(validity != VALID)
                target->error("Invalid operation: modifying target of invalid query");
            
            target->add_element(tag);
            return *this;
        }
        dom_query& add_element_and_access(const std::string& tag){
            if(type != ELEMENT)
                target->error("Invalid operation: adding element to " + type_descr());
            if(validity != VALID)
                target->error("Invalid operation: modifying target of invalid query");
            
            query = tag;
            target = target->add_element(tag);
            
            return *this;
        }
        template<class... Strings>
        dom_query& add_elements(Strings... tags){
            if(type != ELEMENT)
                target->error("Invalid operation: adding element to " + type_descr());
            if(validity != VALID)
                target->error("Invalid operation: modifying target of invalid query");
            
            add_elements_impl(tags...);
            return *this;
        }
        
        dom_query& set_value(const std::string& val){
            switch(type){
                case ELEMENT:
                case ELEMENT_LIST:
                    target->error("Invalid operation: setting value of " + type_descr());
                    
                case ATTRIBUTE:
                    //No regard to validity: create attribute if needed!
                    target->elems[ query ] = val;
                    break;
                    
                case CONTENT:
                    target->_content = val;
                    break;
            }
            
            //Enables chaining like xml.<lots of .element(...)>.or_error()
            //                         .attribute("foo").set_value("bar")
            //                         .attribute("eggs").set_value("spam")
            //                          ...
            type = ELEMENT;
            return *this;
        }
        
        dom_query& operator= (const std::string& val){
            return set_value(val);
        }
        dom_query& operator= (bool val){
            return set_value( val ? "true" : "false" );
        }
        dom_query& operator= (char val){
            return set_value( std::string(1, val) );
        }
        template<typename Numeric>
        dom_query& operator= (Numeric val){
            return set_value( std::to_string(val) );
        }
        
        
        dom_query& remove_element(const std::string& tag = ""){
            if(validity != VALID)
                target->error("Invalid operation: modifying target of invalid query");
            
            switch(type){
                case ELEMENT:
                    target->remove_element(tag);
                    break;
                    
                case ELEMENT_LIST:
                    for(auto iter = begin(); iter != end(); ++iter)
                        iter->remove_element(tag);
                    break;
                    
                case ATTRIBUTE:
                case CONTENT:
                    target->error("Invalid operation: removing element of " + type_descr());
            }
        }
        dom_query& remove_all_elements(const std::string& tag){
            if(validity != VALID)
                target->error("Invalid operation: modifying target of invalid query");
            
            switch(type){
                case ELEMENT:
                    target->remove_all_elements(tag);
                    break;
                    
                case ELEMENT_LIST:
                    for(auto iter = begin(); iter != end(); ++iter)
                        iter->remove_all_elements(tag);
                    break;
                    
                case ATTRIBUTE:
                case CONTENT:
                    target->error("Invalid operation: removing element of " + type_descr());
            }
        }
        
        dom_query& unset_value(){
            switch(type){
                case ELEMENT:
                case ELEMENT_LIST:
                    target->error("Invalid operation: unsetting value of " + type_descr());
                    
                case ATTRIBUTE:
                    target->elems.remove(query);
                    break;
                    
                case CONTENT:
                    target->_content = "";
                    break;
            }
            
            type = ELEMENT;
            return *this;
        }
        dom_query& clear_attributes(){
            if(validity != VALID)
                target->error("Invalid operation: modifying target of invalid query");
            
            switch(type){
                case ELEMENT:
                    target->attrs.clear();
                    break;
                    
                case ELEMENT_LIST:
                    for(auto iter = begin(); iter != end(); ++iter)
                        iter->clear_attributes();
                    break;
                    
                case ATTRIBUTE:
                case CONTENT:
                    target->error("Invalid operation: clearing attributes of " + type_descr());
            }
                             
            return *this;
        }        
    };
    
    using mutbl_dom_query = dom_query<      dom_element, mutbl_dom_iterator, util::ref_ptr<dom_element>>;
    using const_dom_query = dom_query<const dom_element, const_dom_iterator, util::cref_ptr<dom_element>>;
    
     /*   
    //Overloading all the setters to provide more uniform and useful error messages
#define CONST_ERROR target->error("Invalid operation: modifying target of const query");
    
    template<>
    class dom_query<const dom_element, const_dom_iterator> {
        const_dom_query& add_element(const std::string& tag){
            CONST_ERROR
        }
        const_dom_query& add_element(dom_element&& elem){
            CONST_ERROR
        }
        const_dom_query& add_element(const dom_element& elem){
            CONST_ERROR
        }
        
        const_dom_query& add_element(const std::string& tag){
            CONST_ERROR
        }
        template<class... Strings>
        const_dom_query& add_elements(Strings... tags){
            CONST_ERROR
        }
        
        const_dom_query& set_value(const std::string& val){
            CONST_ERROR
        }
        
        const_dom_query& operator= (const std::string& val){
            CONST_ERROR
        }
        const_dom_query& operator= (bool val){
            CONST_ERROR
        }
        const_dom_query& operator= (char val){
            CONST_ERROR
        }
        template<typename Numeric>
        const_dom_query& operator= (Numeric val){
            CONST_ERROR
        }
        
        
//         const_dom_query& remove_element(const std::string& tag){
//             CONST_ERROR
//         }
//         const_dom_query& remove_all_elements(const std::string& tag){
//             CONST_ERROR
//         }
        
        const_dom_query& unset_value(){
            CONST_ERROR
        }
        const_dom_query& clear_attributes(){
            CONST_ERROR
        }    
    };
    
#undef CONST_ERROR*/
    
};


#endif
