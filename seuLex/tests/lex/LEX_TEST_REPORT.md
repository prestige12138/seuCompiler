# seuLex 测试报告

## 1. 测试概述

- 模块：`seuLex`
- 测试执行时间：`2026-04-15`
- 本次结果目录：`tests/lex/results/20260415-220208`
- 执行脚本：`tests/lex/run_lex_tests.sh`
- 总体结果：`12 / 12` 通过

本测试集面向当前 `minic-plus` 子集口径，目标是把现有 `seuLex` 能力整理成可复跑、可归档、可审阅的黑盒/灰盒验证集合。

## 2. 覆盖范围

本轮覆盖了以下能力面：

- Lex 三段式输入解析
- `%{...%}` verbatim definitions 保留
- User subroutines 拼接
- 多行动作与花括号平衡解析
- 扩展 RE：命名定义、字符串、字符类、取反类、范围、`.`、转义、`?`、`+`、`{m}`、`{m,n}`、`{m,}`
- 中缀转后缀
- NFA 合并
- DFA 确定化
- DFA 最小化
- 最长匹配
- 规则优先级
- 空白跳过
- 错误字符回退
- dot 可视化文件输出
- 生成 lexer 的编译与运行
- 负向错误处理
- `resources/minic.l` 生成回归
- CLI `--self-test`
- 长输入运行性能

当前未作为“通过用例”纳入的能力边界：

- start conditions：`%s` / `%x` / `BEGIN`
- trailing context：`r/s`
- `REJECT` / `yymore()` 一类传统 Lex 高级运行机制

原因不是测试缺失，而是当前代码中未发现这些能力的明确实现。

## 3. 测试覆盖率总结

| 维度 | 覆盖情况 | 说明 |
|---|---|---|
| 输入解析 | 高 | 正常路径与 malformed rule 均覆盖 |
| 扩展 RE | 高 | 主要语法特性均覆盖 |
| NFA/DFA/最小化 | 中高 | 通过行为与 dot 状态数双重验证 |
| 最长匹配与二义性 | 高 | 关键字/标识符、`=`/`==` 已覆盖 |
| 代码生成 | 高 | 生成、编译、运行、dot 输出均覆盖 |
| 错误处理 | 高 | 未定义定义、非法 RE、重复上界溢出、未闭合动作均覆盖 |
| 回归 | 高 | `minic.l`、`--self-test` 均执行 |
| 性能 | 中 | 覆盖生成性能与长输入扫描性能，但阈值仍较粗 |
| 上下文敏感 Lex 特性 | 低 | 当前实现未见显式支持，报告中记录为能力边界 |

## 4. 执行步骤

统一执行步骤如下：

1. 运行 `tests/lex/run_lex_tests.sh`
2. 脚本自动配置并构建 `seuLex/build/seuLex`
3. 对成功型用例执行：
   - 生成 lexer
   - 编译生成的 `generated.cpp`
   - 调用 `analysis()` / `tokenize()` 做断言
   - 检查 dot 文件和状态数
4. 对负向用例执行：
   - 直接运行生成器
   - 校验 stderr 是否包含预期错误文本
5. 对回归/性能用例执行：
   - 运行 `resources/minic.l` 生成
   - 运行长输入 tokenization
   - 运行 `--self-test`
6. 把实际结果写入 `tests/lex/results/20260415-220208/`

## 5. 详细测试记录

### 5.1 `01_parser_sections`

- 名称：三段解析与用户代码拼接
- 目的：验证 Definitions / Rules / User subroutines 三段切分、`%{...%}` 保留、多行动作、`%%` 字符串与注释中的 `}` 不会破坏解析。
- 输入：
  - 测试文件：`tests/lex/test_cases/01_parser_sections.l`
  - 关键输入串：`alpha`、`beta_2`、`alpha beta_2`
- 预期输出：
  - `analysis("alpha") == 11`
  - `analysis("beta_2") == 20`
  - `tokenize("alpha beta_2") == [11, 20]`
  - 三个 dot 文件生成成功
- 测试步骤：
  1. 生成 lexer
  2. 编译生成物
  3. 调用 `analysis()` / `tokenize()`
  4. 校验 dot 文件
- 实际结果：
  - `analysis|alpha|11`
  - `analysis|beta_2|20`
  - `tokenize|alpha beta_2|11,20`
  - `merged_nfa.dot / dfa.dot / min_dfa.dot` 均存在
- 结论分析：通过。该用例说明 parser 对多行动作、字符串中的 `%%` 以及用户子程序拼接都可正常处理。

### 5.2 `02_regex_features`

- 名称：扩展 RE 特性矩阵
- 目的：覆盖命名定义、字符串、字符类、范围、`.`、`?`、`+`、`{m}`、`{m,n}`、`{m,}`。
- 输入：
  - 测试文件：`tests/lex/test_cases/02_regex_features.l`
  - 关键输入串：`if`、`42`、`z`、`@`、空串、`e`、`fff`、`gg`、`hhh`、`iiiii`
- 预期输出：
  - 依次返回 `1 2 3 5 6 6 7 8 9 10`
- 测试步骤：
  1. 生成并编译 lexer
  2. 逐条调用 `analysis()`
  3. 校验 dot 文件
- 实际结果：
  - `analysis|if|1`
  - `analysis|42|2`
  - `analysis|z|3`
  - `analysis|@|5`
  - `analysis||6`
  - `analysis|e|6`
  - `analysis|fff|7`
  - `analysis|gg|8`
  - `analysis|hhh|9`
  - `analysis|iiiii|10`
- 结论分析：通过。扩展 RE 的主流特性在当前实现中工作正常，空串接受场景也可正常返回。

### 5.3 `03_negated_escape`

- 名称：取反类与转义字符
- 目的：覆盖 `[^...]`、`\t`、`\n`。
- 输入：
  - 测试文件：`tests/lex/test_cases/03_negated_escape.l`
  - 关键输入串：`Q`、`\t`、`\n`
- 预期输出：
  - `analysis("Q") == 1`
  - `analysis("\\t") == 2`
  - `analysis("\\n") == 3`
- 测试步骤：
  1. 生成并编译 lexer
  2. 分别断言三个输入
  3. 校验 dot 文件
- 实际结果：
  - `analysis|Q|1`
  - `analysis|\t|2`
  - `analysis|\n|3`
- 结论分析：通过。说明取反字符类和常见转义在当前 RE 展开逻辑中可用。

### 5.4 `04_longest_match_priority`

- 名称：最长匹配与规则优先级
- 目的：覆盖关键字/标识符冲突、`=` / `==` 前缀冲突、空白跳过、非法字符兜底。
- 输入：
  - 测试文件：`tests/lex/test_cases/04_longest_match_priority.l`
  - 关键输入串：`if`、`ifa`、`==`、`=`、`42`、`@`
  - token stream：`if ifa == = 42 @`
- 预期输出：
  - `analysis` 结果分别为 `1 4 2 3 5 -1`
  - `tokenize(...) == [1, 4, 2, 3, 5, -1]`
- 测试步骤：
  1. 生成并编译 lexer
  2. 单词素调用 `analysis()`
  3. 整串调用 `tokenize()`
  4. 校验 dot 文件
- 实际结果：
  - `analysis|if|1`
  - `analysis|ifa|4`
  - `analysis|==|2`
  - `analysis|=|3`
  - `analysis|42|5`
  - `analysis|@|-1`
  - `tokenize|if ifa == = 42 @|1,4,2,3,5,-1`
- 结论分析：通过。最长匹配与“先写规则优先”的语义在生成 lexer 中是成立的。

### 5.5 `05_minimization_merge`

- 名称：最小化状态合并
- 目的：验证最小化前后语言保持一致，且可合并状态确实被合并。
- 输入：
  - 测试文件：`tests/lex/test_cases/05_minimization_merge.l`
  - 关键输入串：`a`、`b`、`ab`
- 预期输出：
  - `analysis("a") == 1`
  - `analysis("b") == 1`
  - `tokenize("ab") == [1, 1]`
  - `dfa.dot` 状态数为 `3`
  - `min_dfa.dot` 状态数为 `2`
- 测试步骤：
  1. 生成并编译 lexer
  2. 行为断言
  3. 统计原 DFA 与最小 DFA dot 文件状态数
- 实际结果：
  - `analysis|a|1`
  - `analysis|b|1`
  - `tokenize|ab|1,1`
  - `state_count|dfa.dot|3`
  - `state_count|min_dfa.dot|2`
- 结论分析：通过。该用例证明当前最小化既保留行为，又实际降低了状态数。

### 5.6 `06_error_undefined_definition`

- 名称：未定义命名定义报错
- 目的：验证 `{MISSING}` 这类未定义引用能明确报错。
- 输入：
  - 测试文件：`tests/lex/test_cases/06_error_undefined_definition.l`
- 预期输出：
  - 生成失败
  - stderr 包含 `undefined regular definition reference: MISSING`
- 测试步骤：
  1. 直接调用 `seuLex <spec>`
  2. 检查退出码和 stderr
- 实际结果：
  - `seuLex error: undefined regular definition reference: MISSING`
- 结论分析：通过。错误提示可直接定位根因。

### 5.7 `07_error_invalid_regex`

- 名称：非法 RE 语法报错
- 目的：验证 `a||b` 这类缺操作数表达式会被拒绝。
- 输入：
  - 测试文件：`tests/lex/test_cases/07_error_invalid_regex.l`
- 预期输出：
  - stderr 包含 `missing operand in regular expression`
- 测试步骤：
  1. 直接运行生成器
  2. 校验错误文本
- 实际结果：
  - `seuLex error: missing operand in regular expression`
- 结论分析：通过。RE 语法错误没有被静默吞掉。

### 5.8 `08_error_repeat_overflow`

- 名称：重复上界实现上限保护
- 目的：验证过大 `{m}` 重复不会拖垮展开器。
- 输入：
  - 测试文件：`tests/lex/test_cases/08_error_repeat_overflow.l`
- 预期输出：
  - stderr 包含 `repetition bound exceeds implementation limit`
- 测试步骤：
  1. 直接运行生成器
  2. 校验错误文本
- 实际结果：
  - `seuLex error: repetition bound exceeds implementation limit`
- 结论分析：通过。实现上限保护已生效。

### 5.9 `09_error_unterminated_action`

- 名称：未闭合动作块报错
- 目的：验证规则 action 未闭合时 parser 能在 Lex 层报错。
- 输入：
  - 测试文件：`tests/lex/test_cases/09_error_unterminated_action.l`
- 预期输出：
  - stderr 包含 `unterminated or malformed Lex rule near:`
- 测试步骤：
  1. 直接运行生成器
  2. 校验错误文本
- 实际结果：
  - `seuLex error: unterminated or malformed Lex rule near: "a" {`
  - 下一行：`return 1;`
- 结论分析：通过。错误信息带上下文，有利于定位。

### 5.10 `10_regression_minic_generate`

- 名称：`minic.l` 生成回归
- 目的：验证 `resources/minic.l` 仍能跑完整生成链。
- 输入：
  - 外部输入：`resources/minic.l`
- 预期输出：
  - 生成成功
  - dot 文件齐全
  - 生成时间在阈值内
- 测试步骤：
  1. 调用生成器
  2. 检查 `generated.cpp` 与 dot 文件
  3. 记录耗时
- 实际结果：
  - 通过
  - `elapsed_seconds=6`
- 结论分析：通过。`minic.l` 回归稳定，生成性能较好。

### 5.11 `12_runtime_perf_long_stream`

- 名称：长输入扫描性能
- 目的：验证生成 lexer 在长输入上的运行吞吐量。
- 输入：
  - 基于 `04_longest_match_priority` 生成的 lexer
  - 重复输入块 `if ifa == = 42 @ ` 共 `5000` 次
- 预期输出：
  - token 数量为 `30000`
  - 运行时间在阈值内
- 测试步骤：
  1. 编译性能 driver
  2. 构造长输入
  3. 调用 `tokenize()`
  4. 比较 token 数和耗时
- 实际结果：
  - `30000`
  - `elapsed_seconds=1`
- 结论分析：通过。当前生成 lexer 的基础扫描性能没有异常。

### 5.12 `13_cli_self_test`

- 名称：CLI 自带回归入口
- 目的：验证 `seuLex --self-test` 可直接作为黑盒回归入口。
- 输入：
  - CLI 参数：`--self-test`
- 预期输出：
  - 命令返回 `0`
  - 在阈值内完成
- 测试步骤：
  1. 直接运行 `seuLex --self-test`
  2. 记录退出码和耗时
- 实际结果：
  - 通过
  - `elapsed_seconds=11`
- 结论分析：通过。该入口本身就是一条有效的总控回归链。

## 6. 总体结论

本轮新增的专业化外部测试集已经达到可直接运行、可归档结果、可复审的状态。

结论如下：

- 当前新增测试共 `12` 项，全部通过
- 正常路径、负向路径、回归路径、性能路径均已落地
- `seuLex` 当前已经具备较稳定的黑盒可测性
- 最长匹配、规则优先级、最小化语义保持、`minic` 生成回归都已被外部测试覆盖

## 7. 发现的问题

本轮没有发现新的阻塞性失败。

但有三个需要在后续测试架构中继续注意的点：

1. 当前“上下文敏感 Lex 特性”并无显式实现，因此无法把 start conditions / trailing context 作为通过用例加入主回归集。
2. 性能测试当前是阈值型粗粒度检查，适合作为回归哨兵，但还不构成严格基准测试。
3. 现阶段多数测试仍是黑盒与端到端验证，针对 `LexParser`、`REExpander`、`NFABuilder`、`DFABuilder`、`DFAMinimizer` 的细粒度单元测试仍可继续补强。

## 8. 改进建议

1. 后续如果 `seuLex` 明确实现 `%s/%x/BEGIN` 或 `r/s`，应立即补充对应负向/正向用例，而不是继续留在报告边界说明里。
2. 可以在脚本中增加 dot 状态数、生成时间和扫描时间的历史对比，形成简单趋势回归。
3. 若后续允许增加测试辅助程序，建议为 `LexParser` 和 `REExpander` 单独提供可执行测试入口，减少端到端定位成本。
4. 可以把 `resources/minic.l` 的生成日志进一步提炼成结构化摘要，便于 CI 场景消费。

## 9. 产物清单

- 用例目录：`tests/lex/test_cases/`
- 预期目录：`tests/lex/expected/`
- 结果目录：`tests/lex/results/20260415-220208/`
- 执行脚本：`tests/lex/run_lex_tests.sh`
- 本报告：`tests/lex/LEX_TEST_REPORT.md`
