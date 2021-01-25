
#include <algorithm>

#include "../dom/dom_element.hpp"


using namespace DOM;
using namespace util;

using fp = util::file_parser;
using substr_flags = fp::substr_flags;

void dom_element::parse_xml(const std::string& filename){
    //Clear all fields
    elem_list.clear();
    elem_map.clear();
    name = "";
    cont = "";
    
    //Setup parsing
    file_parser parser(filename);
    std::unordered_map< std::string, std::string > entities = {
        { "gt", ">" },
        { "lt", "<" },
        { "amp", "&" },
        { "apos", "'" },
        { "quot", "\"" }
    };
    
#define XML_PARSER file_parser& parser, std::unordered_map< std::string, std::string >& entities
#define THE_XML_PARSER parser, entities
    
    source = {filename, 1, 0};
        
    //Parse the prologue
    parse_xml_prologue(THE_XML_PARSER);
    
    parser.match('<', fp::consume, "Missing XML root tag, '<' expected");
    
    //Parse the single top-level element
    detail::ptr_type child = std::make_unique<dom_element>();
    child->parse_xml_tag(THE_XML_PARSER, "");
    add_element( std::move(child) );
}

void dom_element::parse_xml_prologue(XML_PARSER){
    parse_xml_xmldecl(THE_XML_PARSER);
    
    skip_xml_comments(THE_XML_PARSER);
    parse_xml_doctypedecl(THE_XML_PARSER);
    skip_xml_comments(THE_XML_PARSER);        
}

void dom_element::parse_xml_xmldecl(XML_PARSER){
    if(!parser.match("<?xml", fp::consume)){
        std::cout << "No XML declaration\n";
        return;
    }
    
    //TODO: actually use (or at least verify) contents of XML decl
    
    parser.seek("?>", fp::consume, "Premature end of file: XML declaration not terminated");
}

void dom_element::parse_xml_doctypedecl(XML_PARSER){
    if(!parser.match("<!DOCTYPE"))
        return;
    
    parser.seek_any_of("[>", 0, "Doctype declaration not terminated");
    
    if(parser.match('[', fp::consume)){
        for(;;){
            parser.seek_not_of(fp::whitespace, 0, "Premature end of file: ']' expected");
            if(parser.match(']', fp::consume))
                break;

            if(*parser != '<')
                parser.error("Rogue character inside document type declaration, '<' or ']' expected");
            
            if(skip_xml_comments(THE_XML_PARSER))
                continue;
            
            ++parser;
            
            parse_xml_markupdecl(THE_XML_PARSER);
        }
    }
    parser.seek_not_of(fp::whitespace, 0, "Doctype declaration not terminated");
    if(*parser != '>')
        parser.error("Rogue character inside doctype declaration");
    
    ++parser;
}

void dom_element::parse_xml_markupdecl(XML_PARSER){
    parser.match('!', fp::consume, "Invalid declaration, '!' expected");
    
    if(parser.match("ENTITY", fp::consume)){
        parse_xml_entity(THE_XML_PARSER);
    }
    //The other kinds are associated with validation, and are currently ignored.
    else if(parser.match("ELEMENT")){
        parser.seek('>', 0, "Element declaration not terminated");
    }
    else if(parser.match("ATTLIST")){
        parser.seek('>', 0, "Attribute list declaration not terminated");
    }
    else if(parser.match("NOTATION")){
        parser.seek('>', 0, "Notation declaration not terminated");
    }
    else
        parser.error("Invalid declaration");
        
}

void dom_element::parse_xml_entity(XML_PARSER){
    parser.seek_not_of(fp::whitespace, 0, "Premature end of file: entity name expected");
    
    auto [key, val] = parse_xml_keyval(THE_XML_PARSER, false, false);
    
    validate_xml_name(THE_XML_PARSER, key);
    
    if(!entities.insert(std::make_pair(key, val)).second)
        parser.error("Entity &" + key + "; already defined");
    
    parser.seek_not_of(fp::whitespace, 0, "Premature end of file: '>' expected");
    if(*parser != '>')
        parser.error("Extra characters in entity definition, '>' expected");
    
    ++parser;
}

bool dom_element::skip_xml_comments(XML_PARSER) {
    bool skipped_comments = false;
    for(;;){
        parser.seek_not_of(fp::whitespace, 0);
        
        if(*parser != '<')
            return skipped_comments;
    
        if(parser.match("<?", fp::consume)){
            //TODO: actually parse processing instructions
            parser.seek("?>", fp::consume, "Processing instruction not terminated");
        }
        else if(parser.match("<!--", fp::consume)){
            //Parses a comment by discarding its contents
            skipped_comments = true;
            
            //Finds the terminating -->, making sure there are no rogue --
            parser.seek("--", fp::consume, "Comment not terminated");
            parser.match('>', fp::consume, "\"--\" is not allowed inside comments");
        }
        else
            return skipped_comments;
    }
}
    
            
/**
 * This is where the main work in parsing XML happens. It looks for the next
 * tag or other token, uses @p context to determine how to interpret it and
 * if it is valid, and then parses the simpler cases and delegates the
 * more complicated cases to other methods, including itself.
 * 
 * <p> This method loops until there are no more tokens to process, which
 * typically means that this tag is fully parsed, or the whole document in
 * case of the top-level call.
 */
void dom_element::parse_xml_content(XML_PARSER){
    std::ostringstream content;
    bool only_space = true;
    
    for(;;){        
        
        parser.set_mark();
        
        parser.seek('<', 0, "Premature end of file: <" + name + "> not closed");
        //Make sure to ignore whitespace-only content
        bool not_only_space = parser.substr(content,
                                            substr_flags::KEEP_NEWLINE |
                                            substr_flags::CONTAINS_NOT,
                                            fp::whitespace);
        if(not_only_space)
            only_space = false;
        
        if(skip_xml_comments(THE_XML_PARSER))
            continue;       
                
        ++parser;
        detail::ptr_type child = std::make_unique<dom_element>();
        
        //Parse the tag and return when the closing tag is found
        if(child->parse_xml_tag(THE_XML_PARSER, name)){
            cont = only_space ? "" : expand_xml_entities(THE_XML_PARSER, content.str());
            return;
        }
        
        add_element( std::move(child) );
    }
}

/**
 * This method starts at the character after the '<' that opens a tag, and
 * parses until the character after the '>' closing it. It handles the parsing
 * of attributes and such, as well as the content of DOCTYPE declarations.
 */
bool dom_element::parse_xml_tag(XML_PARSER, const std::string& parent_name){
    
    source = parser.store_source();
    
    bool close_tag = parser.match('/');
    if(close_tag)
        ++parser;
        
    parser.set_mark();
    parser.seek_any_of(fp::whitespace + "/>", fp::single_line);
    name = parser.substr();
        
    validate_xml_name(THE_XML_PARSER, name);
    
    if(close_tag && parent_name.empty())
        parser.error("Closing tag without matching opening tag");
    if(close_tag && name != parent_name)
        parser.error("Tag mismatch: <" + parent_name + "> closed by </" + name + ">");
        
    // Parse tokens inside tag until the end is found
    for(;;){
        parser.seek_not_of(fp::whitespace, 0, 
                           "Premature end of file: '>' expected");
        
        switch(*parser){
            case '/':
                if(close_tag)
                    parser.error("Invalidly formatted tag: both closing and empty");
                
                ++parser;
                
                parser.match('>', fp::consume, "Expected '>' after '/'");
                    
                return false;
        
            case '>':
                ++parser;
                
                if(!close_tag)
                    parse_xml_content(THE_XML_PARSER);
                
                return close_tag;                
                
            default:
                if(close_tag)
                    parser.error("Closing tags cannot have attributes");
                
                auto[key, val] = parse_xml_keyval(THE_XML_PARSER);
                
                if(!attrs.insert(std::make_pair(key, val)).second)
                    parser.error("Attribute \"" + key + "\" already exists");
        }
        
    }
}

/**
 * The parser should point to the character after the end of the name,
 * as if it just parsed it.
 * 
 * <p>A valid name consists of alphanumeric characters, underscores,
 * periods, dashes, and colons. The first character may not be a period, 
 * colon, or digit.
 */
void dom_element::validate_xml_name(XML_PARSER, const std::string& name){
    if(name.empty())
        parser.error("Empty name");
    
    if( !(std::isalpha(name[0]) || std::string(":_").find(name[0]) != std::string::npos) )
        parser.error("Invalid character in name", name, 0);
    
    for(size_t i = 1; i < name.length(); i++){
        if( !(std::isalpha(name[i]) || std::isdigit(name[i]) || std::string(".-:_").find(name[i]) != std::string::npos) )
            parser.error("Invalid character in name", name, i);
    }     
}

dom_element& dom_element::add_element(detail::ptr_type&& child){
    auto child_iter = elem_list.insert(elem_list.cend(), std::move(child));
    
    elem_map[(*child_iter)->name].push_back( child_iter );
    elem_map[""] .push_back( child_iter );
    
    return **child_iter;
}
dom_element& dom_element::add_element(const std::string& tag){
    detail::ptr_type child = std::make_unique<dom_element>();
    child->name = tag;
    
    return add_element(std::move(child));
};

std::pair<std::string, std::string> dom_element::parse_xml_keyval(XML_PARSER, 
                                                                  bool equal_sign, bool parse){
    parser.set_mark();
    parser.seek_any_of(fp::whitespace + "=/>");
        
    std::string key = parser.substr();
    validate_xml_name(THE_XML_PARSER, key);
    
    if(equal_sign){
        parser.seek_not_of(fp::whitespace, 0, "Premature end of file: '=' expected");
        parser.match('=', fp::consume, "Expected '=' after key");
    }
    else if(*parser == '=')
        parser.error("Unexpected '='");
    
    parser.seek_not_of(fp::whitespace, 0, "Premature end of file: value expected");
    char quote_char = *parser;
    if(quote_char != '"' && quote_char != '\'')
        parser.error("Value must be quoted");
        
    ++parser;
    
    parser.set_mark();
    parser.seek(quote_char, 0, "Premature end of file: end quote expected");
       
    std::string val = parser.substr(substr_flags::KEEP_NEWLINE);
    
    if(parse)
        val = expand_xml_entities(THE_XML_PARSER, val);
    
    ++parser;
    return std::make_pair(key, val); 
}

std::string dom_element::expand_xml_entities(XML_PARSER, const std::string& str){
    std::ostringstream ost;
    
    for(size_t pos = 0; pos < str.length(); ++pos){
        if(str[pos] == '&'){
            size_t sc = str.find(';', pos);
            
            if(sc == std::string::npos)
                parser.error("Unterminated entity (or rogue '&'), ';' expected", str, pos);
            
            ++pos;
            if(str[pos] == '#'){
                ++pos;
                if(str[pos] == 'x'){
                    char val = 0;
                    for(size_t i = pos+1; i < sc; ++i){
                        if(str[i] >= '0' && str[i] <= '9')
                            val = 16*val + (str[i] - '0');
                        else if(str[i] >= 'a' && str[i] <= 'f')
                            val = 16*val + 10 + (str[i] - 'a');
                        else if(str[i] >= 'A' && str[i] <= 'F')
                            val = 16*val + 10 + (str[i] - 'A');
                        else
                            parser.error("Invalid hexadecimal digit in character reference", str, i);
                    }
                    
                    ost << val;
                }
                else{
                    char val = 0;
                    for(size_t i = pos; i < sc; ++i){
                        if(str[i] >= '0' && str[i] <= '9')
                            val = 10*val + (str[i] - '0');
                        else
                            parser.error("Invalid digit in character reference", str, i);
                    }
                    
                    ost << val;
                }
            }
            else{
                auto ent = entities.find(str.substr(pos, sc - pos));
                if(ent == entities.end())
                    parser.error("Undefined entity", str, pos);
                
                //Recursively parse entities defined in terms of other entities,
                //but avoid recursing on &amp; for obvious reasons
                if(ent->second != "&")
                    ost << expand_xml_entities(THE_XML_PARSER, ent->second);
                else
                    ost << "&";
            }
            
            pos = sc;
        }
        else
            ost << str[pos];
    }                
    
    return ost.str();
}

void dom_element::parse_json(const std::string& filename){
    elem_list.clear();
    elem_map.clear();
    name = "";
    
    file_parser parser(filename);
    parser.seek_not_of(fp::whitespace);
    if(!parser)
        parser.error("Empty JSON file");
    
    source = {filename, 1, 0};
    
    detail::ptr_type child = std::make_unique<dom_element>();
    child->name = "JSON-root";
                
    child->parse_json_value(parser);
    
    add_element(std::move(child));
    
    parser.seek_not_of(fp::whitespace);
    if(parser)
        parser.error("Rogue character outside JSON value");
        
}

void dom_element::parse_json_object(file_parser& parser){    
    
    parser.seek_not_of(fp::whitespace, 0, "File ended prematurely, '}' expected");
    
    if(*parser == '}'){
        ++parser;
        return;
    }
    else if(*parser == ']')
        parser.error("Mismatched brackets: '[' terminated by '}'");
    
    for(;;){
        if(*parser != '"')
            parser.error("Rogue character in JSON object");
        
        detail::ptr_type sub = std::make_unique<dom_element>();
        sub->source = parser.store_source();
        
        ++parser;
        parser.set_mark();
        parser.seek('"', fp::single_line, "Unterminated element name, '\"' expected");
        
        std::string tag = parser.substr();
        
        parser.seek(':', 0, "File ended prematurely, ':' expected");
        ++parser;
        parser.seek_not_of(fp::whitespace, 0, "File ended prematurely, value expected");
        
        sub->name = tag;
        sub->parse_json_value(parser);
        
        add_element(std::move(sub));
        
        parser.seek_not_of(fp::whitespace, 0, "File ended prematurely, '}' expected");
        
        if(*parser == '}')
            break;
        else if(*parser == ']')
            parser.error("Mismatched brackets: '{' terminated by ']'");
        else if(*parser != ',')
            parser.error("Values must be separated by ','");
        
        ++parser;
        parser.seek_not_of(fp::whitespace, 0, "File ended prematurely, '}' expected");
    }
    
    ++parser;
}
void dom_element::parse_json_array(file_parser& parser){
    
    parser.seek_not_of(fp::whitespace, 0, "File ended prematurely, ']' expected");
    
    if(*parser == ']'){
        ++parser;
        return;
    }
    else if(*parser == '}')
        parser.error("Mismatched brackets: '[' terminated by '}'");
    
    for(;;){
        detail::ptr_type item = std::make_unique<dom_element>();
        item->name = "item";
        
        item->parse_json_value(parser);
        
        add_element(std::move(item));
        
        parser.seek_not_of(fp::whitespace, 0, "File ended prematurely, ']' expected");
        
        if(*parser == ']')
            break;
        else if(*parser == '}')
            parser.error("Mismatched brackets: '[' terminated by '}'");
        else if(*parser != ',')
            parser.error("Values must be separated by ','");
        
        ++parser;
        parser.seek_not_of(fp::whitespace, 0, "File ended prematurely, value expected");
    }
    
    ++parser;
}

void dom_element::parse_json_value(file_parser& parser){
    source = parser.store_source();
    
    switch(*parser){
        case '[':
            
            attrs["type"] = "array";
            
            ++parser;
            parse_json_array(parser);
            
            break;
            
        case '{':
            attrs["type"] = "object";
            
            ++parser;
            parse_json_object(parser);
            
            break;
            
        case '"':
            attrs["type"] = "string";
            
            ++parser;
            parser.set_mark();
            
            //Matches the pattern ([^"\\\u0000-\u001f]|\\["\\/bfnrt]|\\u[0-9a-fA-F]{4})*(?=")
            for(;;){
                parser.seek_any_of("\"\\" + fp::code_chars, fp::single_line, 
                                   "Unterminated value string, '\"' expected");
                
                if(!parser)
                    parser.error("Unterminated string (linebreaks are not allowed)");
                if(*parser == '"')
                    break;
                
                if(parser.match_any_of(fp::code_chars))
                    parser.error("Control characters are not permitted in JSON strings");
                
                //Escape sequence
                if(!++parser)
                    parser.error("Empty control sequence: expected character after '\\'");
                if(parser.match_any_of("\"\\/bfnrt"))
                    ++parser;
                else if(*parser == 'u'){
                    for(size_t i = 0; i < 4; ++i){
                        ++parser;
                        if(!std::isxdigit(*parser))
                            parser.error("A \\u control sequence must be followed by 4 hexadecimal digits");
                    }
                    ++parser;
                }
            }
                            
            cont = parser.substr();
            
            ++parser;
            break;
           
        case '+':
            //Don't shoot me, shoot ECMA!
            parser.error("Invalidly formatted number: leading '+' not allowed");
            
            
            //Fun and hacky interweaving of cases and ifs to implement the pattern
            // -?(0|[1-9][0-9]*)(\.[0-9]+)?([eE][-+]?[0-9]+)?
            //This is far from best pratice, but occasionally one has to indulge.
        case '-':
            parser.set_mark();
            ++parser;
            if(!++parser)
                parser.error("Invalidly formatted number: expected digit after '-'");
            
            if(*parser == '0'){
                
        case '0':
                parser.set_mark(); 
            
                if(++parser && std::isdigit(*parser))
                    parser.error("Invalidly formatted number: leading '0' not allowed");
                
            } else {
                
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            
                parser.set_mark();
                parser.seek_not_of("0123456789", fp::single_line);            
            }
            
            if(parser.match('.', fp::consume)){
                if(std::isdigit(*parser))
                    parser.seek_not_of("0123456789", fp::single_line);
                else
                    parser.error("Invalidly formatted number: at least one digit required after '.'");
            }
            
            if(parser.match_any_of("eE", fp::consume)){
                parser.match_any_of("+-", fp::consume);
                
                if(!parser)
                    parser.error("Invalidly formatted number: at least one digit required in exponent");
                
                parser.seek_not_of("0123456789", fp::single_line);
            }
            
            parser.match_any_of(fp::whitespace + ",]}", 0, 
                                "Invalidly formatted number: unexpected '" + std::string(1,*parser) + "'");
            
            attrs["type"] = "number";
            cont = parser.substr();
            break;
            
        default:
            parser.set_mark();
            parser.seek_any_of(fp::whitespace + ",]}", fp::single_line);
            
            std::string val = parser.substr();
            
            if(val == "true" || val == "false" || val == "null")
                attrs["type"] = val;
            else
                error("Invalid value: \"" + val + "\"");                
    }
}

void dom_element::print() const {
    print(std::cout);
}
void dom_element::print(std::ostream& out, size_t indent) const {
    if(indent == 0)
    
    if(name.empty()){
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n\n";
        for(const dom_element& sub : all_elements())
            sub.print(out, indent);
        return;
    }
    
    out << std::string(indent*4, ' ');
    
    out << "<" << name;
//     out << "@" << source.filename.substr(source.filename.find_last_of("/")+1) << "[" << source.line << ":" << source.col << "]";
    
    for(auto& attr : attrs){
        out << " " << attr.first << "=\""; 
        unparse_string(out, attr.second);
        out << "\"";
    }
        
    if(elem_list.empty() && cont.empty())
        out << (name == "?xml" ? "?>" : "/>") << "\n";
    else{
        out << ">";
        unparse_string(out, cont);
        
        if(!elem_list.empty()){
            out << "\n";
            for(const dom_element& sub : all_elements())
                sub.print(out, indent+1);
            out << std::string(indent*4, ' ');
        }
        
        out << "</" << name << ">\n";
    }
}

void dom_element::error(const std::string& message) const {
    fp::load_source(source).error(message);
}

void dom_element::unparse_string(std::ostream& out, const std::string& str){
    for(size_t i = 0; i < str.length(); ++i){
        switch(str[i]){
            case '<':   out << "&lt;";      break;
            case '>':   out << "&gt;";      break;
            case '&':   out << "&amp;";     break;
            case '"':   out << "&quot;";    break;
            case '\'':  out << "&apos;";    break;
            default:    out << str[i];
        }
    }
}

