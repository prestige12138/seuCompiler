# seuLex 开发与验证过程

## 1. 当前状态

当前 `seuLex` 子树已经不再是单文件实验代码，而是一个模块化后的词法分析器生成器。

目前代码已分离为：

- Lex 规格解析
- 扩展正则规范化
- NFA 构造
- DFA 子集构造
- DFA 最小化
- 代码生成与可视化
- CLI 入口

重构过程中保留了中期报告要求的数据结构名称，没有把课程概念改写成完全不同的工程命名体系。

## 2. 主要实现阶段

### 阶段 A：模块化拆分

把原先偏单体的实现拆成：

- `include/node.h`
- `include/nfa.h`
- `include/dfa.h`
- `include/lex_parser.h`
- `include/regex_expander.h`
- `include/nfa_constructor.h`
- `include/dfa_builder.h`
- `include/dfa_minimizer.h`
- `include/code_generator.h`
- `src/internal/lex_state.h`
- `src/node.cpp`
- `src/lex_state.cpp`
- `src/lex_parser.cpp`
- `src/regex_expander.cpp`
- `src/nfa_constructor.cpp`
- `src/dfa_builder.cpp`
- `src/dfa_minimizer.cpp`
- `src/code_generator.cpp`
- `src/main.cpp`

### 阶段 B：构建系统固化

`CMakeLists.txt` 明确了：

- C++17
- UNIX / POSIX 约束
- Clang / GNU 警告级别
- CTest 入口
- 自测编译器注入宏 `SEU_LEX_TEST_CXX`

### 阶段 C：Lex 解析器加固

补强内容包括：

- 只把独立 `%%` 视为分段符
- 避免 `%{...%}` 中的 `%%` 干扰分段
- 避免动作块中的 `%%` 干扰分段
- 对字符串、字符常量、注释中的花括号做平衡判断

### 阶段 D：正则和运行期安全性补强

补强内容包括：

- `{m,n}` 重复边界溢出检测
- 重复上限限制
- 自测子进程通过 `fork/execvp` 调起，而不是 `std::system()`

## 3. 当前设计演化特征

### 保留报告命名，收束到命名空间

当前策略不是改掉报告中的命名，而是把它们整体放进 `namespace seu_lex`。

这样做的收益：

- 对课程报告和代码审阅更友好
- 减少全局命名污染

### 公开接口和内部实现已经有初步分离

当前头文件只保留主要接口，辅助函数大多放在：

- 匿名命名空间
- 内部局部结构

### 仍保留明显的“共享状态热点”

当前最明显的是：

- 报告要求保留的多张全局表

在最近一次重构后：

- `src/regex_expander.cpp` 负责扩展 RE
- `src/lex_state.cpp` 负责全局状态和 reset
- `src/node.cpp` 负责基础 `node` 实现
- `src/nfa_constructor.cpp` 负责中缀转后缀和 Thompson NFA
- `src/dfa_builder.cpp` 负责确定化

单文件职责已经明显收束，但共享状态仍然决定了几个模块之间的耦合强度。

## 4. 当前验证方式

### 构建验证

```bash
cmake -S seuLex -B seuLex/build
cmake --build seuLex/build
```

### 自动测试

```bash
ctest --test-dir seuLex/build --output-on-failure
```

### 手动自测入口

```bash
cd seuLex
./build/seuLex --self-test
```

## 5. `runSelfTests()` 当前覆盖

- 小样例 lexer 的生成和编译
- 直接 DFA 识别行为验证
- 非法正则失败验证
- Lex 解析器边界回归
- `minic.l` 输出生成
- `c99.l` 输出生成

## 6. 当前边界

这些是当前实现已经明确的性质，不是未来目标：

- `.l` 动作块和用户代码按可信输入处理
- 运行字符域当前基于 ASCII
- 空串规则仍是受限运行场景
- `minic.l` 和 `c99.l` 在自测中会生成，但不会在 `seuLex` 子树内完成最终编译联调

## 7. 与 seuYacc 的工程成熟度对比

当前结论：

- `seuLex` 和 `seuYacc` 的整体质量已经比较接近
- `seuYacc` 仍在“对象化封装”和“阶段数据传递”上略好

原因主要在于：

- `seuLex` 的全局共享状态更多
- `seuLex` 的代码生成与自测辅助仍偏集中
- `seuYacc` 的文法解析、自动机构造、分析表、代码生成拆分更均匀

如果用工程化程度来描述：

- `seuLex`：模块化已经完成，剩余短板主要是共享状态
- `seuYacc`：已经更接近“结构清晰的生成器框架”

## 8. AI 使用记录

这组文档和近期模块整理过程中使用了 AI 辅助，但遵循的原则是：

- 先读当前代码，再写文档
- 文档只能反映现状，不能替代实现
- 审阅发现的问题必须回到代码层面修复

在近期流程中，AI 主要承担：

- 源码结构扫描
- 模块边界归纳
- 算法链和全局表关系梳理
- 文档补写与整理

## 9. 后续建议

如果后面继续提升 `seuLex`，优先建议如下：

1. 进一步收窄共享全局表的写入面
2. 继续降低全局共享表的直接读写面
3. 拆分 `code_generator.cpp` 中的自测和运行辅助
4. 增加更细粒度的回归样例，而不只依赖端到端 smoke test
