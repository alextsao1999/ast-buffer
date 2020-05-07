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
    //ast.append_origin((char *) origin.ptr(), origin.size());

    ast.append("int main(int abc) {\n");
    ast.append("    auto *str = \"asdf\";\n");
    ast.append("    *str = test();\n");
    ast.append("    return 0;\n");
    ast.append("}\n");
    ast.append("test(get(\"1234\"));\n");
    //std::cout << ast.tree().root().string();
    ast.append("int add(int x, int y) {return x + y;}\n");
    ast.dump();

    auto query = ts::Language::cpp().query(
            "(number_literal) @number"
            "(string_literal) @string"
            "(call_expression function: (identifier) @fun-call)");

    auto cursor = query.exec(ast.tree().root());
    uint32_t index = 0;
    while (cursor.next_capture(index)) {
        ts::Node node = cursor.match().captures->node;
        auto name = cursor.capture_name(index);
        std::cout <<"capture:" << node.string() << " " << name << " -> " << ast.node_string(node) << std::endl;
    }

    return 0;
}

