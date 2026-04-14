# seuIntermediate 开发与验证过程

## 开发顺序

1. 先落报告要求的数据结构
2. 实现 ASTBuilder 和 parse-root
3. 实现符号表
4. 实现三地址码生成
5. 增加模块自测
6. 增加独立测试脚本与测试报告
7. 补整链路联通样例

## 当前验证

- `ctest --test-dir intermediate/build --output-on-failure`
- `integration/tests/pipeline/run_pipeline_test.sh`
