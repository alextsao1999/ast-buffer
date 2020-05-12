//
// Created by Alex on 2020/5/4.
//
#include <ast_buffer.h>
#include <origin.h>
int main() {
    ASTBuffer<char> ast(ts::Language::cpp());
    ast.parser().set_timeout(500);
    //size_t position = 50000;
    //ast.parser().set_cancel_position(&position);
    //Origin origin("your/file/path/xxxx.txt");
    //ast.append_origin((wchar_t *) origin.ptr(), origin.size());
    ast.append("int main(int abc) {\n");
    ast.append("    auto *str = \"asdf\";\n");
    ast.append("    *str = test();\n");
    ast.append("    return 0;\n");
    ast.append("}\n");
    //std::cout << ast.tree().root().string();
    ast.append("int add(int x, int y) {return x + y;}\n");
    ast.insert(ast.buffer().size(), "int aaa = 100;\n");

    ast.dump();
    auto query = ts::Language::cpp().query(
            "(number_literal) @number"
            "((string_literal) @string)"
            "(call_expression function: (identifier) @color)"
            "((binary_expression left: (identifier) @left right: (identifier) @right) (eq? @left @right))"
            "((string_literal) @string-inject (inject @string-inject \"shift\"))");
    auto cursor = query.exec(ast.tree().root());
    uint32_t index;
    while (cursor.next_capture(index)) {
        auto node = cursor.capture_node(index);
        auto name = cursor.capture_name(index);
        std::cout << "capture:" << node.string()
                  << " " << name
                  << " -> " ;
        std::cout << ast.node_string(node) << "  ";
        for (auto &step : query.pattern_predicates(cursor.pattern_index())) {
            if (step.type == TSQueryPredicateStepTypeCapture) {
                std::cout << "@" << query.capture_name(step.value_id) << " ";
            }
            if (step.type == TSQueryPredicateStepTypeString) {
                std::cout << "\"" << query.string_value(step.value_id) << "\"" << " ";
            }
            if (step.type == TSQueryPredicateStepTypeDone) {
                std::cout << "|";
            }
        }
        std::cout << std::endl;
    }
    return 0;
}
