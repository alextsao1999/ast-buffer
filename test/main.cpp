//
// Created by Alex on 2020/5/4.
//
#include <ast_buffer.h>
#include <origin.h>
int main() {
    ASTBuffer<> ast(ts::Language::cpp());
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

    //std::cout << ast.tree().root().string();
    ast.insert(ast.buffer().line_end(1), "a = 200;");
    ast.insert(ast.buffer().line_end(1), "int b = 1000;");
    ast.append("int add(int x, int y) {return x + y;}\n");
    ast.dump();

    auto query = ts::Language::cpp().query("(string_literal) @string\n"
                                           "(number_literal) @number");

    auto cursor = query.exec(ast.tree().root());
    while (cursor.next_match()) {
        for (int i = 0; i < cursor.match().capture_count; ++i) {
            ts::Node node = cursor.match().captures[i].node;
            std::cout << "capture:" << node.string() << "->" << ast.node_string(node) << std::endl;
        }
    }

    return 0;
}

