# writing-a-c-compiler-in-c

从零开发一个 C 语言编译器。

## 项目结构

```
chapter_1/
  main.c          # 编译器驱动程序
  lexer.h/c       # 词法分析器（tokenizer）
  parser.h/c      # 语法分析器（递归下降）
  ast.h/c         # C 抽象语法树
  tacky.h/c       # TACKY 三地址码中间表示
  tacky_gen.h/c   # AST → TACKY 转换
  asm_ast.h/c     # 汇编 AST
  codegen.h/c     # TACKY → 汇编 AST（三阶段）
  emit.h/c        # 代码发射（.s 文件输出）
  Makefile        # 构建脚本
```

## 构建

```bash
cd chapter_1
make
```

## 命令行选项

```bash
./compiler source.c          # 预处理 → 编译 → 汇编 → 链接
./compiler --lex source.c    # 词法分析后停止
./compiler --parse source.c  # 语法分析后停止
./compiler --tacky source.c  # TACKY 生成后停止
./compiler --codegen source.c # 汇编生成后停止
./compiler -S source.c       # 仅生成 .s 汇编文件
```

## 支持的语言特性

- `int main(void) { return <exp>; }`
- 表达式支持：整数常量、取负（`-`）、按位取反（`~`）、括号（`(<exp>)`）
- 嵌套一元表达式

<<<<<<< HEAD

=======
## 使用示例

```bash
echo 'int main(void) { return 1+2*3; }' > test.c
./compiler test.c
./test
echo $?   # 输出 7
```
>>>>>>> 685bcd5 (chore: remove test targets (tests not in repo))
