# 最终提交归档说明

## 1. 目标

本文件用于把“可运行代码”整理为“可直接提交的课程材料”，重点覆盖：

- PPT 第 34 页要求的提交项映射
- 三模块联调截图附件
- `demo_ir` 正式演示样例归档
- 最新测试报告入口
- AI 辅助过程记录

根目录正式核对表见：

- [实践PPT核对表.md](/Users/llawliet/代码/seuCompiler/实践PPT核对表.md)

## 2. 最终提交材料目录

### 核心模块

- [seuLex/](/Users/llawliet/代码/seuCompiler/seuLex)
- [seuYacc/](/Users/llawliet/代码/seuCompiler/seuYacc)
- [intermediate/](/Users/llawliet/代码/seuCompiler/intermediate)

### 输入规格与课程资源

- [resources/minic.l](/Users/llawliet/代码/seuCompiler/resources/minic.l)
- [resources/minic.y](/Users/llawliet/代码/seuCompiler/resources/minic.y)
- [resources/编译原理中期报告.docx](/Users/llawliet/代码/seuCompiler/resources/编译原理中期报告.docx)
- [resources/编译原理课程实践 2026.pptx](/Users/llawliet/代码/seuCompiler/resources/编译原理课程实践%202026.pptx)

### 正式生成产物样例

- 当前 `minic-plus` 分支不再提交固定的 `generated_*.cpp/.h` 样例文件
- 对应产物由 `seuLex` / `seuYacc` 在构建、自测和集成测试中现场生成

### demo_ir 演示样例

- 输入：
  - [demo_ir.l](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/test_cases/demo_ir.l)
  - [demo_ir.y](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/test_cases/demo_ir.y)
  - [demo_ir.c](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/test_cases/demo_ir.c)
- 输出：
  - [demo_ir.tac](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/expected/demo_ir.tac)
  - [demo_ir.ll](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/expected/demo_ir.ll)
  - [demo_ir.jimple](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/expected/demo_ir.jimple)

## 3. 三模块联调截图与最新结果

正式截图附件：

- [ctest_snapshot_20260415.svg](/Users/llawliet/代码/seuCompiler/docs/assets/final_submission/ctest_snapshot_20260415.svg)
- [pipeline_snapshot_20260415.svg](/Users/llawliet/代码/seuCompiler/docs/assets/final_submission/pipeline_snapshot_20260415.svg)
- [ir_pipeline_snapshot_20260415.svg](/Users/llawliet/代码/seuCompiler/docs/assets/final_submission/ir_pipeline_snapshot_20260415.svg)

### 顶层回归快照

- 命令：`ctest --test-dir build --output-on-failure`
- 最新结果：`5/5` 通过
- 总耗时：`23.72 sec`

### 最小链路快照

对应脚本：

- [run_pipeline_test.sh](/Users/llawliet/代码/seuCompiler/integration/tests/pipeline/run_pipeline_test.sh)
- 最新结果目录：[results/20260416_005216](/Users/llawliet/代码/seuCompiler/integration/tests/pipeline/results/20260416_005216)
- 最新输出文件：[pipeline_expr.ir](/Users/llawliet/代码/seuCompiler/integration/tests/pipeline/results/20260416_005216/pipeline_expr.ir)

输出：

```text
1: t1 = 2 * 3
2: t2 = 42 + t1
3: x = t2
4: return x
```

### 完整 IR 演示快照

对应脚本：

- [run_ir_pipeline_test.sh](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/run_ir_pipeline_test.sh)
- 最新结果目录：[results/20260416_005220](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/results/20260416_005220)
- 最新实际输出：
  - [demo_ir.tac](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/results/20260416_005220/actual/demo_ir.tac)
  - [demo_ir.ll](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/results/20260416_005220/actual/demo_ir.ll)
  - [demo_ir.jimple](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/results/20260416_005220/actual/demo_ir.jimple)

`demo_ir.tac`：

```text
1: t1 = v + 1
2: t = t1
3: return t
4: x = 1
5: y = 2
6: limit = 20
7: result = 0
8: if x < 8 goto 10
9: goto 23
10: t2 = call inc(y)
11: y = t2
12: t3 = y * 2
13: t4 = x + t3
14: result = t4
15: if result > limit goto 17
16: goto 20
17: t5 = result - 3
18: x = t5
19: goto 22
20: t6 = result + 1
21: x = t6
22: goto 8
23: return x
```

`demo_ir.ll` 摘要：

```llvm
define i32 @inc(i32 %v.in) {
entry:
  %v.addr = alloca i32
  %t.addr = alloca i32
  %t1.addr = alloca i32
  store i32 %v.in, i32* %v.addr
  br label %bb_1
...
define i32 @main() {
entry:
  %x.addr = alloca i32
  %y.addr = alloca i32
  %limit.addr = alloca i32
  %result.addr = alloca i32
...
  %r4 = call i32 @inc(i32 %r3)
...
  br i1 %r14, label %bb_17, label %bb_16
...
  ret i32 %r21
}
```

`demo_ir.jimple` 摘要：

```text
.class public final DemoIrPipeline
.super java.lang.Object

.method public static int inc(int v)
{
  int t;
  int t1;
  ...
}

.method public static int main()
{
  int x;
  int y;
  int limit;
  int result;
  int t2;
  ...
  t2 = staticinvoke DemoIrPipeline.inc(y);
  ...
  if result > limit goto label_17;
  ...
  return x;
}
```

## 4. 最新测试报告入口

- 词法分析：
  - [seuLex/tests/lex/LEX_TEST_REPORT.md](/Users/llawliet/代码/seuCompiler/seuLex/tests/lex/LEX_TEST_REPORT.md)
- 语法分析：
  - [seuYacc/tests/yacc/YACC_TEST_REPORT.md](/Users/llawliet/代码/seuCompiler/seuYacc/tests/yacc/YACC_TEST_REPORT.md)
- 中间代码：
  - [intermediate/tests/icg/INTERMEDIATE_TEST_REPORT.md](/Users/llawliet/代码/seuCompiler/intermediate/tests/icg/INTERMEDIATE_TEST_REPORT.md)
- 集成联调：
  - [integration/tests/pipeline/PIPELINE_TEST_REPORT.md](/Users/llawliet/代码/seuCompiler/integration/tests/pipeline/PIPELINE_TEST_REPORT.md)
  - [integration/tests/ir_pipeline/IR_PIPELINE_TEST_REPORT.md](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/IR_PIPELINE_TEST_REPORT.md)

## 5. 实验报告与答辩材料

- 实验报告：[课程实验报告.md](/Users/llawliet/代码/seuCompiler/课程实验报告.md)
- 答辩内容稿：[答辩PPT.md](/Users/llawliet/代码/seuCompiler/答辩PPT.md)

说明：

- 本轮收尾按当前要求不生成答辩 `ppt/pptx/pdf` 成品文件
- 仓库内保留答辩内容稿，供后续人工转制

## 6. AI 辅助过程记录

本项目在收尾阶段使用 AI 作为工程协作工具，主要承担：

- 模块化重构与文档补写
- 测试架构整理与最终测试归档
- LLVM IR / Jimple 输出扩展
- 联调脚本补全与提交材料核对
- 多轮构建、回归和问题修复闭环

本轮明确使用并记录的协作角色包括：

- `explorer`
- `architect`
- `tdd-guide`
- `code-reviewer`
- `security-reviewer`
- `loop-operator`

约束说明：

- 所有最终实现均以本地构建与脚本回归为准
- AI 生成内容均已落到仓库文件并经实际命令验证
- 对 `.l/.y` 的生成与执行链路，当前仍默认建立在“可信输入”前提上

## 7. 最终结论

按当前收尾范围，当前仓库已经把三模块联调截图、`demo_ir` 输入输出、最新测试报告和最终归档说明全部落库。

特别补齐的最终收尾项包括：

- `demo_ir` 完整样例与三份后端产物
- 三张可单独打包的联调截图附件
- 顶层与模块级最新测试报告
- 最终提交核对表
- 实验报告与最终归档文档的同步更新
