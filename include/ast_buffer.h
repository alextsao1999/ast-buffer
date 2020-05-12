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
    using buffer_iter_t = typename buffer_t ::iter_t;
    buffer_t m_buffer;
    ts::Parser m_parser;
    ts::Tree m_tree;
public:
    ASTBuffer() = default;
    ASTBuffer(ts::Language language) : m_parser(language) {}
    inline buffer_t &buffer() { return m_buffer; }
    inline ts::Tree &tree() { return m_tree; }
    inline ts::Parser &parser() { return m_parser; }
    inline uint32_t length() { return m_buffer.size(); }
    inline TSPoint get_point(uint32_t pos) {
        size_t line = m_buffer.get_line(pos);
        return {line, (pos - m_buffer.line_start(line)) * sizeof(char_t)};
    }
    inline uint32_t get_pos(TSPoint pt) {
        return m_buffer.line_start(pt.row) + pt.column / sizeof(char_t);
    }
    inline void fixup_input(TSInputEdit &input) {
        input.start_point = get_point(input.start_byte / sizeof(char_t));
        input.old_end_point = get_point(input.old_end_byte / sizeof(char_t));
        input.new_end_point = get_point(input.new_end_byte / sizeof(char_t));
    };
    inline const char_t &operator[] (const size_t &index) { return m_buffer.char_at(index); }
    buffer_iter_t insert_origin(uint32_t pos, const char_t *map, size_t length) {
        if (!m_tree.empty()) {
            TSInputEdit input;
            input.start_byte = pos * sizeof(char_t);
            input.old_end_byte = input.start_byte;
            input.new_end_byte = input.start_byte + length * sizeof(char_t);
            fixup_input(input);
            m_tree.edit(input);
        }
        auto iter = m_buffer.insert_origin(pos, map, length);
        parse();
        return iter;
    }
    buffer_iter_t append_origin(const char_t *map, size_t length) {
        if (!m_tree.empty()) {
            TSInputEdit input;
            input.start_byte = m_buffer.size() * sizeof(char_t);
            input.old_end_byte = input.start_byte;
            input.new_end_byte = input.start_byte + length * sizeof(char_t);
            fixup_input(input);
            m_tree.edit(input);
        }
        auto iter = m_buffer.append_origin(map, length);
        parse();
        return iter;
    }
    buffer_iter_t append(const string_t &str) {
        if (!m_tree.empty()) {
            TSInputEdit input;
            input.start_byte = m_buffer.size() * sizeof(char_t);
            input.old_end_byte = input.start_byte;
            input.new_end_byte = input.start_byte + str.length() * sizeof(char_t);
            fixup_input(input);
            m_tree.edit(input);
        }
        auto iter = m_buffer.append(str);
        parse();
        return iter;
    }
    buffer_iter_t insert(uint32_t pos, const string_t &str) {
        if (!m_tree.empty()) {
            TSInputEdit input;
            input.start_byte = pos * sizeof(char_t);
            input.old_end_byte = input.start_byte;
            input.new_end_byte = input.start_byte + str.length() * sizeof(char_t);
            fixup_input(input);
            m_tree.edit(input);
        }
        auto iter = m_buffer.insert(pos, str);
        parse();
        return iter;
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
        parse();
    }
    string_t node_string(const ts::Node& node) {
        return m_buffer.range_string(node.start_byte() / sizeof(char_t),
                                     (node.start_byte() + node.length()) / sizeof(char_t));
    };
    void parse() {
        size_t byte_size = m_buffer.size() * sizeof(char_t);
        m_parser.parse(m_tree, [&](uint32_t byte, TSPoint pt, uint32_t &read_byte) -> const void * {
            if (byte >= byte_size) {
                return nullptr;
            }
            read_byte = sizeof(char_t);
            return &m_buffer[byte / sizeof(char_t)];
        }, sizeof(char_t) == 1 ? TSInputEncodingUTF8 : TSInputEncodingUTF16);
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
                auto node = cursor.node();
                if (cursor.field_name()) {
                    std::cout << cursor.field_name() << ": ";
                }
                std::cout << node.type() << " [" << node.start_byte() << "," << node.end_byte() << "]";
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
private:
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

};

#endif //GEDITOR_AST_BUFFER_H
