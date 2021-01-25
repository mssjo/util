#ifndef DOM_ELEMENT_H
#define DOM_ELEMENT_H

#include <list>
#include <unordered_map>
#include <memory>

#include "file_parser.hpp"

#define REF_PTR_ASSIGN_FROM_SMART
#include "ref_ptr.hpp"


namespace DOM {
    
class dom_element;

namespace detail {
    using ptr_type        = std::unique_ptr<dom_element>;
    using elem_list_type  = std::list< ptr_type >;
    using indir_list_type = std::list< elem_list_type::iterator >;
    using indir_map_type  = std::unordered_map< std::string, indir_list_type >;
    using attr_map_type   = std::unordered_map< std::string, std::string >;    
};

#include "dom_detail.hpp"

/**
 * @brief A class for parsing and modifying XML-esquely structured data.
 * 
 * DOM stands for "Document Object Model". It represents data hierarchically
 * as a tree of elements, each with attributes and sub-elements. The structure
 * is modelled on XML, but is more abstract and can be adapted to other
 * systems such as JSON and (TODO) YAML.
 * 
 * Each element has a <i>name</i>, a list of key-value string pairs called 
 * <i>attributes</i>, a string called the <i>content</i>, and an ordered list
 * of <i>sub-elements</i>. The sub-elements can be accessed by name.
 * (XML considers the sub-elements to be part of the content, but we separate
 * them for simplicity. However, this means that all information about where
 * in the context the sub-element appears is lost.)
 * 
 * Based on a root element, the DOM is mainly accessed through making
 * <i>queries</i>, enabling commands such as "for each sub-element with name X,
 * access the first sub-element with name Y (error if not found) and get the
 * attribute with key Z (give default value if not found)".
 * 
 * A DOM can be parsed from XML files, and can be used to generate a new XML
 * file that is, besides formatting, identical to the original. It will provide
 * error messages if the file is improperly formatted. It can then be used
 * by a program to parse semantic meaning from the data, and provides facilities
 * for useful error messages if the data is incorrect.
 */
class dom_element {
private:
    
    template<typename Target, class Iter, class Ptr>
    friend class detail::dom_query;
    template<typename Target, class SpecIter, class GlobIter>
    friend class detail::dom_iterator;
       
    /** The name of the element. */
    std::string name;
    /** The content of the element. */
    std::string cont;
    
    /** Information about where in the input file the element was defined;
     *  useful for error messages. */
    util::file_parser::source source;
    
    /** List of sub-elements in order of appearance. */
    detail::elem_list_type elem_list;
    /** Maps each sub-element name to a list of references to all sub-elements
     *  with that name. */
    detail::indir_map_type elem_map;
    /** Maps attribute keys to attribute values. */
    detail::attr_map_type  attrs;
    
    /** Move-inserts a sub-element under this element. */
    dom_element& add_element(detail::ptr_type&& child);
    
#define XML_PARSER util::file_parser& parser, std::unordered_map< std::string, std::string >& entities
        
    static void parse_xml_prologue(XML_PARSER);
    static void parse_xml_xmldecl(XML_PARSER);
    static void parse_xml_doctypedecl(XML_PARSER);
    static void parse_xml_markupdecl(XML_PARSER);
    static void parse_xml_entity(XML_PARSER);
    static bool skip_xml_comments(XML_PARSER);
    static void validate_xml_name(XML_PARSER, const std::string& name);
    static std::string expand_xml_entities(XML_PARSER, const std::string& str);
    
    void parse_xml_content(XML_PARSER);
    /**
     * @brief Parses a single XML tag, filling its contents into this element.
     * @param parser file parser to an XML file, currently pointing to the
     *               character after the '<' opening the tag.
     * @param context contextual information.
     * @return the type of tag that was parsed.
     */
    bool parse_xml_tag(XML_PARSER, const std::string& parent_name);
    /**
     * @brief Parses a key-value pair defining an XML attribute, and similar
     *        constructs.
     * @param parser file parser to an XML file.
     * @param equal_sign whether to require an equal sign between key and value.
     * @param parse TODO
     */
    static std::pair<std::string, std::string> 
    parse_xml_keyval(XML_PARSER, bool equal_sign = true, bool parse = true);
    
#undef XML_PARSER
    
    void parse_json_object(util::file_parser& parser);
    void parse_json_array(util::file_parser& parser);
    void parse_json_value(util::file_parser& parser);
         
    static void unparse_string(std::ostream& out, const std::string& str);
    
public:
    using query       = detail::mutbl_dom_query;
    using const_query = detail::const_dom_query;
    
    dom_element() : name(""), cont(""), source(), elem_list(), elem_map(), attrs() {};
    virtual ~dom_element() = default;
    
    dom_element(const dom_element&) = delete;
    dom_element(dom_element&&) = default;
    
    dom_element& operator= (const dom_element&) = delete;
    dom_element& operator= (dom_element&&) = default;
    
    dom_element copy() const;
    
    void parse_xml(const std::string& filename);
    void parse_json(const std::string& filename);
    
    void error(const std::string& message) const;
    
    void print() const;
    void print(std::ostream& out, size_t indent = 0) const;
    
    std::string               get_name()    const { return name;    }   
    util::file_parser::source get_source()  const { return source;  }
    
    void set_name(const std::string& name) { this->name = name; }
    
    dom_element& add_element(const std::string& tag);
//     void remove_element(const std::string& tag);
//     void remove_all_elements(const std::string& tag);
    
    const_query all_elements(const std::string& tag = "") const {
        return const_query(this).all_elements(tag);
    }
    const_query element(const std::string& tag = "") const {
        return const_query(this).element(tag);
    }
    const_query unique_element(const std::string& tag = "", const std::string& err = "") const {
        return const_query(this).unique_element(tag, err);
    }
    const_query attribute(const std::string& key) const {
        return const_query(this).attribute(key);
    }
    const_query content() const {
        return const_query(this).content();
    }
    
    query all_elements(const std::string& tag = ""){
        return query(this).all_elements(tag);
    }
    query element(const std::string& tag = ""){
        return query(this).element(tag);
    }
    query unique_element(const std::string& tag = "", const std::string& err = ""){
        return query(this).unique_element(tag, err);
    }
    query attribute(const std::string& key){
        return query(this).attribute(key);
    }
    query content(){
        return query(this).content();
    }
};

};

#endif
