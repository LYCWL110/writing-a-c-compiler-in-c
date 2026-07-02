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

- 函数体支持多条语句和声明
- 语句：`return <exp>;`、`<exp>;`、`;`（空语句）
- 声明：`int <name> [= <exp>];`
- 表达式：整数常量、变量引用、取负（`-`）、按位取反（`~`）、逻辑非（`!`）、赋值（`=`）、括号、加减乘除取余、逻辑与或（`&&`/`||`）、比较运算（`<`/`>`/`<=`/`>=`/`==`/`!=`）
