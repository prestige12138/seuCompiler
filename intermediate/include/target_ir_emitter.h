#pragma once

#include <iosfwd>
#include <string>

#include "intermediate_code.h"

/**
 * @file target_ir_emitter.h
 * @brief Target-IR text emitters for lowering three-address code to LLVM IR and
 *        Jimple.
 */

namespace seu_icg {

/**
 * @brief Stable options shared by all target-IR emitters.
 */
struct TargetIrOptions {
  std::string moduleName = "seu_module";
  std::string className = "SeuModule";
  std::string entryFunction = "main";
  bool emitComments = true;
};

/**
 * @brief Lower one AST/TAC pair to a stable LLVM IR text form.
 *
 * Complexity: O(F + N + T), where F is the function count, N is the statement
 * count, and T is the total emitted text size.
 */
std::string formatLlvmIr(const ASTNode* root,
                         const IntermediateCode& code,
                         const TargetIrOptions& options = {});

/**
 * @brief Lower one AST/TAC pair to a stable Jimple text form.
 *
 * Complexity: O(F + N + T), where F is the function count, N is the statement
 * count, and T is the total emitted text size.
 */
std::string formatJimple(const ASTNode* root,
                         const IntermediateCode& code,
                         const TargetIrOptions& options = {});

/**
 * @brief Dump LLVM IR text to one output stream.
 *
 * Complexity: O(F + N + T).
 */
void dumpLlvmIr(const ASTNode* root,
                const IntermediateCode& code,
                std::ostream& out,
                const TargetIrOptions& options = {});

/**
 * @brief Dump Jimple text to one output stream.
 *
 * Complexity: O(F + N + T).
 */
void dumpJimple(const ASTNode* root,
                const IntermediateCode& code,
                std::ostream& out,
                const TargetIrOptions& options = {});

}  // namespace seu_icg
