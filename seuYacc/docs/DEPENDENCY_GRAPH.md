# seuYacc 依赖图

## 1. 模块依赖图

```mermaid
graph TD
    MAIN[main.cpp]
    DRIVER[SeuYaccDriver]
    PARSER[yacc_parser.*]
    SYMBOL[symbol_table.*]
    LR1[lr1_pda.*]
    LALR[lalr_converter.*]
    TABLE[parse_table.*]
    GEN[generated parser.cpp / tokens.h]

    MAIN --> DRIVER
    DRIVER --> PARSER
    DRIVER --> SYMBOL
    DRIVER --> LR1
    DRIVER --> LALR
    DRIVER --> TABLE
    TABLE --> GEN
```

说明：

- `SeuYaccDriver` 是总控入口
- `parse_table.*` 依赖前面所有分析结果并最终输出代码
- `LALRConverter` 不是默认生成路径的必经节点，但仍是项目内的正式模块

## 2. 头文件依赖图

```mermaid
graph TD
    YH[yacc_parser.h]
    SH[symbol_table.h]
    LH[lr1_pda.h]
    LAH[lalr_converter.h]
    PH[parse_table.h]

    SH --> YH
    LH --> SH
    LH --> YH
    LAH --> LH
    PH --> LAH
    PH --> LH
    PH --> SH
    PH --> YH
```

## 3. 生成流程图

```mermaid
flowchart TD
    A[读取 .y 文件] --> B[切分三段]
    B --> C[解析定义段]
    B --> D[解析规则段]
    C --> E[构造 YaccSpecification]
    D --> E
    E --> F[填充全局文法表]
    F --> G1[构造 canonical LR(1)]
    F --> G2[构造 direct LALR(1)]
    G1 --> H1[构建 LR(1) 表]
    G2 --> H2[构建 LALR(1) 表]
    H1 --> I[代码生成]
    H2 --> I
    I --> J[输出 parser.cpp]
    I --> K[输出 tokens.h]
```

## 4. 自测依赖图

```mermaid
flowchart TD
    T0[runSelfTests] --> T1[表达式文法]
    T0 --> T2[语义动作文法]
    T0 --> T3[minic.y]

    T1 --> T1A[生成]
    T1A --> T1B[编译]
    T1B --> T1C[运行]

    T2 --> T2A[生成]
    T2A --> T2B[编译]
    T2B --> T2C[运行]

    T3 --> T3A[生成]
    T3A --> T3B[编译]
```

## 5. 运行期 parser 依赖图

```mermaid
graph TD
    TOK[vector<Token>]
    ACTION[ACTION table]
    GOTO[GOTO table]
    META[production metadata]
    EXEC[executeAction]
    PARSE[yyparse]

    TOK --> PARSE
    ACTION --> PARSE
    GOTO --> PARSE
    META --> PARSE
    EXEC --> PARSE
```

## 6. 依赖关系总结

最关键的工程判断有两点：

- `yacc_parser.*` 和 `lr1_pda.*` 必须松耦合，因为前者处理文本结构，后者处理形式文法
- `parse_table.*` 必须承担集成角色，但不能反过来污染头文件层级

当前结构基本满足：

- 解析、自动机构造、合并、代码生成职责分离
- 所有对外接口仍可通过 `SeuYaccDriver` 汇总使用
