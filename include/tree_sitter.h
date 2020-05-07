//
// Created by Alex on 2020/5/4.
//

#ifndef GEDITOR_TREE_SITTER_H
#define GEDITOR_TREE_SITTER_H
#include <tree_sitter/api.h>
#include <string>
#include <functional>
namespace ts {
    class Tree;
    class Node;
    class Cursor;
    using FeedFunction = std::function<const void *(uint32_t byte, TSPoint pt, uint32_t &read_length)>;
    class Query {
        TSQuery *m_query = nullptr;
        uint32_t m_offset;
        TSQueryError m_error;
    public:
        class Cursor {
            friend class Query;
            TSQueryCursor *m_cursor = nullptr;
            TSQueryMatch m_match;
        public:
            Cursor() : m_cursor(ts_query_cursor_new()) {}
            Cursor(const Cursor &rhs) = delete;
            Cursor(Cursor &&rhs) {
                m_cursor = rhs.m_cursor;
                rhs.m_cursor = nullptr;
            }
            ~Cursor() {
                if (m_cursor) {
                    ts_query_cursor_delete(m_cursor);
                }
            }
            bool next_capture(uint32_t &index) {
                return ts_query_cursor_next_capture(m_cursor, &m_match, &index);
            }
            bool next_match() {
                return ts_query_cursor_next_match(m_cursor, &m_match);
            }
            void set_byte_range(uint32_t start, uint32_t end) {
                ts_query_cursor_set_byte_range(m_cursor, start, end);
            }
            void set_point_range(TSPoint start, TSPoint end) {
                ts_query_cursor_set_point_range(m_cursor, start, end);
            }
            inline TSQueryMatch &match() {
                return m_match;
            }
            inline uint32_t capture_count() { return m_match.capture_count; }
            inline Node capture_node(uint32_t index = 0);
            std::string capture_name(uint32_t index = 0) {
                uint32_t length = 0;
                auto *str = ts_query_cursor_get_name(m_cursor, m_match.captures[index].index, &length);
                return std::string(str, length);
            }
        };
        Query(TSLanguage *language, const std::string &query) {
            m_query = ts_query_new(language, query.c_str(), query.length(), &m_offset, &m_error);
        }
        Query(const Query &rhs) = delete;
        Query(Query &&rhs) {
            m_query = rhs.m_query;
            m_offset = rhs.m_offset;
            m_error = rhs.m_error;
            rhs.m_query = nullptr;
        }
        ~Query() {
            ts_query_delete(m_query);
        }
        uint32_t offset() { return m_offset; }
        TSQueryError error() { return m_error; }
        void disable_pattern(uint32_t ptn) {
            ts_query_disable_pattern(m_query, ptn);
        }
        void disable_capture(const std::string &name) {
            ts_query_disable_capture(m_query, name.c_str(), name.length());
        }
        uint32_t capture_count() {
            return ts_query_capture_count(m_query);
        }
        uint32_t pattern_count() {
            return ts_query_pattern_count(m_query);
        }
        uint32_t string_count() {
            return ts_query_string_count(m_query);
        }
        std::string capture_name(uint32_t id) {
            uint32_t length;
            auto *str = ts_query_capture_name_for_id(m_query, id, &length);
            return std::string(str, length);
        }
        std::string string_value_for_id(uint32_t id) {
            uint32_t length;
            auto *str = ts_query_string_value_for_id(m_query, id, &length);
            return std::string(str, length);
        }
        const TSQueryPredicateStep *predicates_for_pattern(uint32_t id, uint32_t &length) {
            return ts_query_predicates_for_pattern(m_query, id, &length);
        }
        uint32_t start_byte_for_pattern(uint32_t pattern_index) {
            return ts_query_start_byte_for_pattern(m_query, pattern_index);
        }
        Cursor inline exec(const Node &node);
    };


    class Language {
        const TSLanguage *m_language = nullptr;
    public:
        constexpr Language(const TSLanguage *language) : m_language(language) {}
        inline operator TSLanguage *() { return (TSLanguage *) m_language; }
        inline operator const TSLanguage *() { return m_language; }
        inline Query query(const std::string &query) { return Query(*this, query); }
        inline uint32_t field_count() { return ts_language_field_count(m_language); }
        inline uint32_t symbol_count() { return ts_language_symbol_count(m_language); }
        inline TSSymbol symbol(const char *name, uint32_t length, bool is_named) {
            return ts_language_symbol_for_name(m_language, name, length, is_named);
        }
        inline const char *symbol_name(TSSymbol symbol) {
            return ts_language_symbol_name(m_language, symbol);
        }
        inline TSSymbolType symbol_type(TSSymbol symbol) {
            return ts_language_symbol_type(m_language, symbol);
        }
        inline const char *field_name(TSFieldId id) {
            return ts_language_field_name_for_id(m_language, id);
        }
        inline TSFieldId field_id(const char *name, uint32_t length) {
            return ts_language_field_id_for_name(m_language, name, length);
        }
        static Language cpp(){
            extern TSLanguage *language_cpp();
            return language_cpp();
        };
    };
    class Logger {
    public:
        virtual void report(TSLogType type, const char *) = 0;
        static void Callback(void *payload, TSLogType type, const char *str) {
            auto *logger = (Logger *) payload;
            logger->report(type, str);
        }
    };
    class Parser {
        TSParser *m_parser = nullptr;
    public:
        Parser() = default;
        Parser(Language language) : m_parser(ts_parser_new()) {
            ts_parser_set_language(m_parser, language);
        }
        Parser(const Parser &rhs) : m_parser(ts_parser_new()) {
            ts_parser_set_language(m_parser, ts_parser_language(rhs.m_parser));
        }
        Parser(Parser &&rhs) : m_parser(rhs.m_parser) {
            rhs.m_parser = nullptr;
        }
        ~Parser() {
            ts_parser_delete(m_parser);
        }
        inline Tree parse(const std::string &str);
        inline void parse(Tree &tree, const std::string &str, TSInputEncoding encoding = TSInputEncodingUTF8);
        inline void parse(Tree &tree, FeedFunction feeder, TSInputEncoding encoding = TSInputEncodingUTF8);
        Language language() { return ts_parser_language(m_parser); }
        void set_range(TSRange *range, uint32_t count) {
            ts_parser_set_included_ranges(m_parser, range, count);
        }
        void set_language(Language language) {
            ts_parser_set_language(m_parser, language);
        }
        void set_logger(Logger *logger) {
            ts_parser_set_logger(m_parser, {logger, Logger::Callback});
        }
        void set_timeout(uint64_t micros) {
            ts_parser_set_timeout_micros(m_parser, micros);
        }
        const size_t *get_cancel_position() {
            return ts_parser_cancellation_flag(m_parser);
        }
        void set_cancel_position(size_t *pos) {
            ts_parser_set_cancellation_flag(m_parser, pos);
        }
        Logger *get_logger() {
            return (Logger *) ts_parser_logger(m_parser).payload;
        }
    };
    class Node {
        TSNode m_node;
        friend class Cursor;
        friend class Query;
    public:
        Node(const TSNode &mNode) : m_node(mNode) {}
        bool empty() const { return ts_node_is_null(m_node); }
        TSSymbol symbol() const { return ts_node_symbol(m_node); }
        bool has_changes() const { return ts_node_has_changes(m_node); }
        bool is_extra() const { return ts_node_is_extra(m_node); }
        bool is_missing() const { return ts_node_is_missing(m_node); }
        bool is_named() const { return ts_node_is_named(m_node); }
        uint32_t count() const { return ts_node_child_count(m_node); }
        char *string() { return ts_node_string(m_node); }
        const char *type() { return ts_node_type(m_node); }
        uint32_t start_byte() const { return ts_node_start_byte(m_node); }
        uint32_t end_byte() const { return ts_node_end_byte(m_node); }
        uint32_t length() const { return end_byte() - start_byte(); }
        TSPoint start_point() const { return ts_node_start_point(m_node); }
        TSPoint end_point() const { return ts_node_end_point(m_node); }
        Node named_descendant_for_byte_range(uint32_t start, uint32_t end) const {
            return ts_node_named_descendant_for_byte_range(m_node, start, end);
        }
        Node named_descendant_for_point_range(TSPoint start, TSPoint end) const {
            return ts_node_named_descendant_for_point_range(m_node, start, end);
        }
        Node parent() { return ts_node_parent(m_node); }
        Node prev() { return ts_node_prev_sibling(m_node); }
        Node next() { return ts_node_next_sibling(m_node); }
        Node prev_named() { return ts_node_prev_named_sibling(m_node); }
        Node next_named() { return ts_node_next_named_sibling(m_node); }
        bool operator==(const Node &rhs) { return ts_node_eq(m_node, rhs.m_node); }
        bool operator!=(const Node &rhs) { return !ts_node_eq(m_node, rhs.m_node); }
        operator TSNode& () { return m_node; }
        Node by_field_id (TSFieldId id) { return ts_node_child_by_field_id(m_node, id); }
        Node by_index (const uint32_t &index) { return ts_node_child(m_node, index); }
        Node operator[](const uint32_t &index) { return ts_node_child(m_node, index); }
        Node operator[](const std::string &index) {
            return ts_node_child_by_field_name(m_node, index.c_str(), index.length());
        }
        Node &operator++() { m_node = ts_node_next_sibling(m_node); return *this; }
        Node operator++(int) { return next(); }
        inline Node begin() { return ts_node_child(m_node, 0); }
        inline Node end() { return ts_node_child(m_node, count() - 1); }
        inline Node &operator*() { return *this; }
        inline TSRange range() {
            return {start_point(), end_point(), start_byte(), end_byte()};
        }
        Cursor walk();
        void edit(const TSInputEdit &input) {
            ts_node_edit(&m_node, &input);
        }
    };
    class Tree {
        friend class Parser;
        TSTree *m_tree = nullptr;
    public:
        Tree() = default;
        Tree(TSTree *tree) : m_tree(tree) {}
        Tree(const Tree &rhs) {
            m_tree = ts_tree_copy(rhs.m_tree);
        }
        Tree(Tree &&rhs) {
            m_tree = rhs.m_tree;
            rhs.m_tree = nullptr;
        }
        ~Tree() {
            ts_tree_delete(m_tree);
        }
        inline Tree &operator=(const Tree &rhs) {
            m_tree = ts_tree_copy(rhs.m_tree);;
            return *this;
        }
        inline bool empty() { return !m_tree; }
        Node root() { return ts_tree_root_node(m_tree); }
        void print_dot_graph(FILE *io) {
            ts_tree_print_dot_graph(m_tree, io);
        }
        void edit(const TSInputEdit &input) {
            ts_tree_edit(m_tree, &input);
        }
    };
    class Cursor {
        TSTreeCursor m_cursor;
    public:
        Cursor(const Node &node) : m_cursor(ts_tree_cursor_new(node.m_node)) {}
        Cursor(const Cursor &rhs) : m_cursor(ts_tree_cursor_copy(&rhs.m_cursor)) {}
        ~Cursor() {
            ts_tree_cursor_delete(&m_cursor);
        }
        Cursor &operator=(const Cursor &rhs) = delete;
        inline void reset(const Node &node) {
            ts_tree_cursor_reset(&m_cursor, node.m_node);
        }
        inline Node node() { return Node(ts_tree_cursor_current_node(&m_cursor)); }
        inline TSFieldId field_id() {
            return ts_tree_cursor_current_field_id(&m_cursor);
        }
        inline const char *field_name() {
            return ts_tree_cursor_current_field_name(&m_cursor);
        }
        inline bool goto_parent() {
            return ts_tree_cursor_goto_parent(&m_cursor);
        }
        inline bool goto_next() {
            return ts_tree_cursor_goto_next_sibling(&m_cursor);
        }
        inline bool goto_first() {
            return ts_tree_cursor_goto_first_child(&m_cursor);
        }
        inline bool goto_first_by_byte(uint32_t byte) {
            return ts_tree_cursor_goto_first_child_for_byte(&m_cursor, byte) != -1;
        }

    };
    inline Cursor Node::walk() {
        return Cursor(*this);
    }
    inline Tree Parser::parse(const std::string &str) {
        return Tree(ts_parser_parse_string(m_parser, nullptr, str.c_str(), str.length()));
    }
    inline void Parser::parse(Tree &tree, const std::string &str, TSInputEncoding encoding) {
        tree.m_tree = ts_parser_parse_string_encoding(m_parser, tree.m_tree, str.c_str(), str.length(), encoding);
    }
    static const char *InputRead(
            void *payload, uint32_t byte_index, TSPoint position, uint32_t *bytes_read) {
        FeedFunction *feed = (FeedFunction *) payload;
        auto *chunk = (*feed)(byte_index, position, *bytes_read);
        if (chunk == nullptr) {
            *bytes_read = 0;
            return "";
        }
        return (const char *) chunk;
    }
    inline void Parser::parse(Tree &tree, FeedFunction feeder, TSInputEncoding encoding) {
        TSInput input;
        input.payload = &feeder;
        input.read = InputRead;
        input.encoding = encoding;
        tree.m_tree = ts_parser_parse(m_parser, tree.m_tree, input);
    }
    inline Query::Cursor Query::exec(const Node &node) {
        Cursor cursor;
        ts_query_cursor_exec(cursor.m_cursor, m_query, node.m_node);
        return cursor;
    }
    inline Node Query::Cursor::capture_node(uint32_t index) {
        return Node(m_match.captures[index].node);
    }
}

#endif //GEDITOR_TREE_SITTER_H
