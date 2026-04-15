# 最终提交材料核对表

严格对照 `resources/编译原理课程实践 2026.pptx` 第 34 页“期末提交内容”整理。

PPT 原文要点：

- 全部电子文档提交给课代表
- 以“全组同学学号 + 姓名”命名文件夹
- 提交内容包括：
  - `Lex` 和 `Yacc` 输入文件
  - 词法、语法分析程序生成器源程序
  - 中间代码生成模块源程序
  - 生成的词法、语法分析程序源程序
  - 词法分析、语法分析测试用例和测试结果
  - 生成的中间代码
  - 实验报告及 `ppt`

状态说明：

- `已就绪`：仓库内已有正式文件，可直接纳入提交包
- `需人工转制`：仓库内已有内容稿，但本轮不生成对应成品文件

## 一、最终提交包命名要求

- 提交文件夹命名：`全组同学学号+姓名`
- 建议在最终打包时保留如下一级目录：
  - `seuLex/`
  - `seuYacc/`
  - `intermediate/`
  - `integration/`
  - `docs/`
  - `resources/`
  - 根目录报告与答辩材料

## 二、PPT 第 34 页逐项核对

| PPT 提交项 | 状态 | 仓库内对应材料 |
| --- | --- | --- |
| `Lex` 输入文件 | 已就绪 | [resources/minic.l](/Users/llawliet/代码/seuCompiler/resources/minic.l) |
| `Yacc` 输入文件 | 已就绪 | [resources/minic.y](/Users/llawliet/代码/seuCompiler/resources/minic.y) |
| 含语义规则与 translation scheme 的联调用输入 | 已就绪 | [integration/tests/pipeline/test_cases/pipeline_expr.l](/Users/llawliet/代码/seuCompiler/integration/tests/pipeline/test_cases/pipeline_expr.l), [integration/tests/pipeline/test_cases/pipeline_expr.y](/Users/llawliet/代码/seuCompiler/integration/tests/pipeline/test_cases/pipeline_expr.y), [integration/tests/ir_pipeline/test_cases/demo_ir.l](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/test_cases/demo_ir.l), [integration/tests/ir_pipeline/test_cases/demo_ir.y](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/test_cases/demo_ir.y) |
| 词法分析程序生成器源程序 | 已就绪 | [seuLex/](/Users/llawliet/代码/seuCompiler/seuLex) |
| 语法分析程序生成器源程序 | 已就绪 | [seuYacc/](/Users/llawliet/代码/seuCompiler/seuYacc) |
| 中间代码生成模块源程序 | 已就绪 | [intermediate/](/Users/llawliet/代码/seuCompiler/intermediate) |
| 生成的词法分析程序源程序 | 已就绪 | 由 [seuLex/](/Users/llawliet/代码/seuCompiler/seuLex) 在构建或测试时现场生成 |
| 生成的语法分析程序源程序 | 已就绪 | 由 [seuYacc/](/Users/llawliet/代码/seuCompiler/seuYacc) 在构建或测试时现场生成 |
| 词法分析测试用例和测试结果 | 已就绪 | [seuLex/tests/lex/](/Users/llawliet/代码/seuCompiler/seuLex/tests/lex), [seuLex/tests/lex/LEX_TEST_REPORT.md](/Users/llawliet/代码/seuCompiler/seuLex/tests/lex/LEX_TEST_REPORT.md) |
| 语法分析测试用例和测试结果 | 已就绪 | [seuYacc/tests/yacc/](/Users/llawliet/代码/seuCompiler/seuYacc/tests/yacc), [seuYacc/tests/yacc/YACC_TEST_REPORT.md](/Users/llawliet/代码/seuCompiler/seuYacc/tests/yacc/YACC_TEST_REPORT.md) |
| 中间代码模块测试用例和测试结果 | 已就绪 | [intermediate/tests/icg/](/Users/llawliet/代码/seuCompiler/intermediate/tests/icg), [intermediate/tests/icg/INTERMEDIATE_TEST_REPORT.md](/Users/llawliet/代码/seuCompiler/intermediate/tests/icg/INTERMEDIATE_TEST_REPORT.md) |
| 三模块联调测试与结果 | 已就绪 | [integration/tests/pipeline/PIPELINE_TEST_REPORT.md](/Users/llawliet/代码/seuCompiler/integration/tests/pipeline/PIPELINE_TEST_REPORT.md), [integration/tests/ir_pipeline/IR_PIPELINE_TEST_REPORT.md](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/IR_PIPELINE_TEST_REPORT.md) |
| 生成的中间代码 | 已就绪 | [integration/tests/pipeline/expected/pipeline_expr.ir](/Users/llawliet/代码/seuCompiler/integration/tests/pipeline/expected/pipeline_expr.ir), [integration/tests/ir_pipeline/expected/demo_ir.tac](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/expected/demo_ir.tac), [integration/tests/ir_pipeline/expected/demo_ir.ll](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/expected/demo_ir.ll), [integration/tests/ir_pipeline/expected/demo_ir.jimple](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/expected/demo_ir.jimple) |
| 实验报告 | 已就绪 | [课程实验报告.md](/Users/llawliet/代码/seuCompiler/课程实验报告.md) |
| 答辩 PPT | 需人工转制 | [答辩PPT.md](/Users/llawliet/代码/seuCompiler/答辩PPT.md) |

## 三、建议直接打包的最终材料

### 1. 核心源码

- [seuLex/](/Users/llawliet/代码/seuCompiler/seuLex)
- [seuYacc/](/Users/llawliet/代码/seuCompiler/seuYacc)
- [intermediate/](/Users/llawliet/代码/seuCompiler/intermediate)

### 2. 输入规格与演示样例

- [resources/](/Users/llawliet/代码/seuCompiler/resources)
- [integration/tests/pipeline/test_cases/](/Users/llawliet/代码/seuCompiler/integration/tests/pipeline/test_cases)
- [integration/tests/ir_pipeline/test_cases/](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/test_cases)

### 3. 生成产物样例

- 生成产物改为通过构建或测试脚本现场生成，不再在仓库中提交固定样例文件
- [integration/tests/pipeline/expected/pipeline_expr.ir](/Users/llawliet/代码/seuCompiler/integration/tests/pipeline/expected/pipeline_expr.ir)
- [integration/tests/ir_pipeline/expected/demo_ir.tac](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/expected/demo_ir.tac)
- [integration/tests/ir_pipeline/expected/demo_ir.ll](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/expected/demo_ir.ll)
- [integration/tests/ir_pipeline/expected/demo_ir.jimple](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/expected/demo_ir.jimple)

### 4. 正式文档

- [README.md](/Users/llawliet/代码/seuCompiler/README.md)
- [docs/FINAL_SUBMISSION.md](/Users/llawliet/代码/seuCompiler/docs/FINAL_SUBMISSION.md)
- [docs/FINAL_AUDIT.md](/Users/llawliet/代码/seuCompiler/docs/FINAL_AUDIT.md)
- [docs/INTEGRATION.md](/Users/llawliet/代码/seuCompiler/docs/INTEGRATION.md)
- [课程实验报告.md](/Users/llawliet/代码/seuCompiler/课程实验报告.md)
- [答辩PPT.md](/Users/llawliet/代码/seuCompiler/答辩PPT.md)

## 四、最终结论

对照 PPT 第 34 页，除答辩 PPT 成品外，其余提交项均已具备仓库内正式入口。

本轮新增并正式归档的关键材料：

- 完整 IR 演示样例：`.l/.y/.c + TAC/LLVM IR/Jimple`
- 最终提交归档说明：`docs/FINAL_SUBMISSION.md`
- 最新联调截图与测试材料入口
- 更新后的实验报告
