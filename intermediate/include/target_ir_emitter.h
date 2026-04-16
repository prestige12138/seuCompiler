#pragma once

#include <iosfwd>
#include <string>

#include "intermediate_code.h"

/**
 * @file target_ir_emitter.h
 * @brief 定义将三地址码进一步输出为 LLVM IR 与 Jimple 文本的接口。
 */

namespace seu_icg {

/**
 * @brief 所有目标 IR 输出器共享的稳定配置项。
 */
struct TargetIrOptions {
  std::string moduleName = "seu_module";
  std::string className = "SeuModule";
  std::string entryFunction = "main";
  bool emitComments = true;
};

/**
 * @brief 将一组 AST 与三地址码降低为稳定的 LLVM IR 文本。
 *
 * 时间复杂度：O(F + N + T)，其中 F 为函数数目，N 为语句数，
 * T 为输出文本总长度。
 */
std::string formatLlvmIr(const ASTNode* root,
                         const IntermediateCode& code,
                         const TargetIrOptions& options = {});

/**
 * @brief 将一组 AST 与三地址码降低为稳定的 Jimple 文本。
 *
 * 时间复杂度：O(F + N + T)，其中 F 为函数数目，N 为语句数，
 * T 为输出文本总长度。
 */
std::string formatJimple(const ASTNode* root,
                         const IntermediateCode& code,
                         const TargetIrOptions& options = {});

/**
 * @brief 将 LLVM IR 文本输出到给定流。
 *
 * 时间复杂度：O(F + N + T)。
 */
void dumpLlvmIr(const ASTNode* root,
                const IntermediateCode& code,
                std::ostream& out,
                const TargetIrOptions& options = {});

/**
 * @brief 将 Jimple 文本输出到给定流。
 *
 * 时间复杂度：O(F + N + T)。
 */
void dumpJimple(const ASTNode* root,
                const IntermediateCode& code,
                std::ostream& out,
                const TargetIrOptions& options = {});

}  // 命名空间 seu_icg
