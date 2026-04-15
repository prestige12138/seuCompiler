# seuYacc 开发与验证过程

## 1. 开发目标

本轮 `seuYacc` 的目标不是写一个临时单文件实验品，而是交付一个：

- 可编译
- 可维护
- 可审阅
- 可和 `seuLex` 对接

的模块化项目。

## 2. 实现阶段

### 阶段 0：结构确认

完成内容：

- 对齐 `seuLex` 的模块化工程结构
- 确认中期报告中的数据结构名称必须保留
- 当前 `minic-plus` 分支以 `resources/minic.y` 为主参考文法

产出：

- `seuYacc/` 目录骨架

### 阶段 1：`.y` 解析与文法建模

完成内容：

- 三段切分
- 定义段解析
- 规则段解析
- `%union`、`%prec`、quoted terminal 支持
- action block 解析

后续补强：

- 多行 `%union`
- mid-rule action 转合成空产生式

### 阶段 2：LR 自动机构造

完成内容：

- FIRST / FOLLOW
- closure / goto
- canonical LR(1)

问题：

- 历史上的完整规格文法在规范 LR(1) 路径上代价过高

决策：

- 增加 direct-LALR 构造
- 保留 canonical LR(1) 作为完整功能和验证路径

### 阶段 3：分析表与冲突处理

完成内容：

- ACTION / GOTO 构造
- shift/reduce 处理
- reduce/reduce 处理
- `%left` / `%right` / `%nonassoc` 生效

### 阶段 4：代码生成

完成内容：

- 生成头文件
- 生成 parser 源文件
- 嵌入用户代码
- 输出 `yyparse`

后续补强：

- 输出路径分离时的 include 修正
- grammar symbol 转义与枚举名合法化
- typed action 翻译
- 第三段用户代码前置

### 阶段 5：审查驱动修复

审查暴露过的关键问题包括：

- 历史完整规格文法在 canonical LR(1) 上卡住
- grammar symbol 直接写入 C++ 代码存在生成风险
- `%union` 多行解析不足
- typed semantic action 未真正生效
- mid-rule action 被误当成末尾 action
- 第三段 helper 定义位置不适合被 action 调用

对应修复已经全部落地。

## 3. 验证过程

### 构建验证

执行：

```bash
cmake -S seuYacc -B seuYacc/build
cmake --build seuYacc/build
```

结果：

- 通过

### 自测验证

执行：

```bash
./seuYacc/build/seuYacc --self-test
ctest --test-dir seuYacc/build --output-on-failure
```

结果：

- 通过

### 自测内容

1. 表达式文法
   - 验证最小生成闭环
2. 语义动作文法
   - 验证 `%union`
   - 验证 `%type`
   - 验证 typed action
   - 验证第三段 helper
3. `minic.y`
   - 验证当前主规格文法生成
   - 验证生成后的 parser 编译

## 4. 当前工程边界

虽然模块已经可交付，但当前仍应明确其边界：

- 默认接口是 token 向量，不是流式 lexer
- 支持的是 Yacc 常用子集，不是完整 Bison 超集
- 第三段用户代码按可信代码处理
- 运行期符号表是 parser 生成辅助，不是完整编译器语义分析框架

## 5. 推荐后续工作

下一阶段最合理的工程动作有三类：

### 方案一：和 seuLex 联调

目标：

- 用真实 token 序列驱动生成 parser

### 方案二：增加回归样例

优先补：

- 更多含优先级冲突的文法
- 更多 mid-rule action 文法
- 错误输入与报错路径

### 方案三：继续细化生成 parser 的语义接口

例如：

- 更完整的错误恢复
- 更丰富的 runtime trace
- 更清晰的 AST/语义回调接口

## 6. AI 使用记录

本模块开发过程使用了 AI 辅助，但策略是工程化约束优先：

- 先根据现有代码和报告要求确认结构
- 再逐步实现和验证
- 审查发现的问题必须回到代码里修复，而不是仅修改说明

实际使用方式包括：

- 代码结构梳理
- LR / LALR 构造问题定位
- 生成器行为补丁
- 文档补写与整理

## 7. 当前结论

`seuYacc` 当前已经达到：

- 模块结构清晰
- 算法链完整
- 文法规模可支撑 `minic.y`
- 生成代码可编译
- 自测可复现

因此它已经适合作为后续 `seuLex + seuYacc` 联调阶段的 parser 生成模块基础。
