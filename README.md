# ASTBuffer
Fast incremental parsing using tree-sitter and piece table

使用tree-sitter+piece-table实现快速增量解析 并生成ast

piece-table 测试了一下300MB左右的文件5000行中插入数据需要48ms

* 主要是给我的code editor改进一下储存结构 🤭
* 如果喜欢的话或者可以给您的editor加成个 [LSP](https://github.com/alextsao1999/lsp-cpp) ~


## Usage

```c++
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
            "(call_expression function: (identifier) @fun-call)"
            "(binary_expression left: (identifier) @left right: (identifier) @right)");

    auto cursor = query.exec(ast.tree().root());
    uint32_t index;
    while (cursor.next_capture(index)) {
        auto node = cursor.capture_node(index);
        auto name = cursor.capture_name(index);
        std::cout <<"capture:" << node.string() << " " << name << " -> " << ast.node_string(node) << std::endl;
    }
    return 0;
}
```

