# writing-a-c-compiler-in-c

从零开发一个 C 语言编译器 —— C 语言实现，跟随《Writing a C Compiler in C》教程。

## 进度

- [x] 第 1 章：极简编译器 — 支持 `int main(void) { return <integer>; }`

## 构建

```bash
cd chapter_1
make
```

## 使用

```bash
./compiler source.c          # 完整编译
./compiler --lex source.c    # 词法分析后停止
./compiler --parse source.c  # 语法分析后停止
./compiler --codegen source.c # 汇编生成后停止
./compiler -S source.c       # 仅生成汇编文件
```

## 测试

```bash
make test
```
