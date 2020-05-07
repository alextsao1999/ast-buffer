//
// Created by Alex on 2020/5/7.
//

#ifndef GEDITOR_AST_BUFFER_H
#define GEDITOR_AST_BUFFER_H
#include <piece_table.h>
#include <tree_sitter.h>
template <class char_t = char, class string_t = std::basic_string<char_t>>
class ASTBuffer {
    using buffer_t = PieceTable<char_t, string_t>;
    buffer_t m_buffer;
    ts::Parser m_parser;
    ts::Tree m_tree;
public:
    ASTBuffer(ts::Language language) : m_parser(language) {}
    buffer_t &buffer() { return m_buffer; }
    ts::Tree &tree() { return m_tree; }
    ts::Parser &parser() { return m_buffer; }
    void insert_origin(uint32_t pos, const char_t *map, size_t length) {
        if (!m_tree.empty()) {
            TSInputEdit input;
            input.start_byte = pos;
            input.old_end_byte = pos;
            input.new_end_byte = pos + length;
            fixup_input(input);
            m_tree.edit(input);
        }
        m_buffer.insert_origin(pos, map, length);
        paritial_parse();
    }
    void append_origin(const char_t *map, size_t length) {
        if (!m_tree.empty()) {
            TSInputEdit input;
            input.start_byte = m_buffer.size();
            input.old_end_byte = input.start_byte;
            input.new_end_byte = input.start_byte + length;
            fixup_input(input);
            m_tree.edit(input);
        }
        m_buffer.append_origin(map, length);
        paritial_parse();
    }
    void append(const string_t &str) {
        if (!m_tree.empty()) {
            TSInputEdit input;
            input.start_byte = m_buffer.size();
            input.old_end_byte = input.start_byte;
            input.new_end_byte = input.start_byte + str.length();
            fixup_input(input);
            m_tree.edit(input);
        }
        m_buffer.append(str);
        paritial_parse();
    }
    void insert(uint32_t pos, const string_t &str) {
        if (!m_tree.empty()) {
            TSInputEdit input;
            input.start_byte = pos;
            input.old_end_byte = pos;
            input.new_end_byte = pos + str.length();
            fixup_input(input);
            m_tree.edit(input);
        }
        m_buffer.insert(pos, str);
        paritial_parse();
    }
    void erase(uint32_t start, uint32_t end) {
        if (!m_tree.empty()) {
            TSInputEdit input;
            input.start_byte = start;
            input.old_end_byte = end;
            input.new_end_byte = start;
            fixup_input(input);
            m_tree.edit(input);
        }
        m_buffer.erase(start, end);
        paritial_parse();
    }
    string_t node_string(const ts::Node& node) {
        return m_buffer.range_string(node.start_byte(), node.start_byte() + node.length());
    };
    void fixup_input(TSInputEdit &input) {
        uint32_t line = m_buffer.get_line(input.start_byte);
        input.start_point = {line, input.start_byte - m_buffer.line_start(line)};
        line = m_buffer.get_line(input.old_end_byte);
        input.old_end_point = {line, input.old_end_byte - m_buffer.line_start(line)};
        line = m_buffer.get_line(input.new_end_byte);
        input.new_end_point = {line, input.new_end_byte - m_buffer.line_start(line)};
    };
    void full_parse() {
        typename buffer_t::Iterator iter;
        m_parser.parse(m_tree, [&](uint32_t byte, TSPoint point, uint32_t &read) -> const char * {
            if (iter.empty()) {
                iter = m_buffer.iter(byte, m_buffer.size());
            }
            if (iter.next()) {
                read = iter.length();
                std::cout << iter.string();
                return iter.c_str();
            }
            return nullptr;
        });
    }
    void paritial_parse() {
        size_t size = m_buffer.size();
        m_parser.parse(m_tree, [&](uint32_t byte, TSPoint pt, uint32_t &read) -> const void * {
            if (byte >= size) {
                return nullptr;
            }
            read = 1;
            return &m_buffer[byte];
        });
    }
    void dump() {
        auto print_intent = [](int num) {
            for (int i = 0; i < num * 4; ++i) {
                printf(" ");
            }
        };
        int indent = 0;
        auto cursor = m_tree.root().walk();
        bool visitedChildren = false;
        for (int i = 0;; i++) {
            if (visitedChildren) {
                if (cursor.goto_next()) {
                    visitedChildren = false;
                } else if (cursor.goto_parent()) {
                    visitedChildren = true;
                    indent--;
                } else {
                    break;
                }
            } else {
                print_intent(indent);
                std::cout << cursor.node().type()
                          << " -> [" << cursor.node().start_byte() << "," << cursor.node().end_byte() << "]";
                if (indent) {
                    //std::cout << " [" << node_string(cursor.node()) << "]";
                }
                std::cout << std::endl;
                if (cursor.goto_first()) {
                    visitedChildren = false;
                    indent++;
                } else {
                    visitedChildren = true;
                }
            }
        }

    }
};

#endif //GEDITOR_AST_BUFFER_H
