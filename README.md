# ast-buffer
Fast incremental parsing using tree-sitter and piece table



使用tree-sitter+piece-table实现快速增量解析 并生成ast

piece-table 测试了一下300MB左右的文件插入5000行数据需要48ms

## Usage

```c++
#include <ast_buffer.h>
int main() {
    //using namespace ts;
    ASTBuffer<char> ast;
    ast.append("int main(int abc) {\n");
    ast.append("    auto *str = \"asdf\";\n");
    ast.append("    *str = test();\n");
    ast.append("    return 0;\n");
    ast.append("}\n");
    
    ast.insert(ast.buffer().line_end(1), "a = 200;");
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


```

