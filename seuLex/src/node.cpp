/**
 * @file node.cpp
 * @brief 实现报告规定自动机结点类型的基础访问接口。
 */

#include "node.h"

#include <utility>

namespace seu_lex {

/**
 * @brief 构造一个默认结点，初始状态编号为 0，且不是接受态。
 */
node::node() : label(0), accepted(false) {}

/**
 * @brief 按给定状态编号和接受标记构造结点。
 */
node::node(int state, bool accepttag) : label(state), accepted(accepttag) {}

/**
 * @brief 为当前结点添加一条字符转移边。
 */
void node::Addoutstate(char ch, node* nd) {
  outstate.insert({ch, nd});
}

/**
 * @brief 返回当前结点是否为接受态。
 */
bool node::IsAccepted() {
  return accepted;
}

/**
 * @brief 以常量方式返回当前结点是否为接受态。
 */
bool node::IsAccepted() const {
  return accepted;
}

/**
 * @brief 修改当前结点的接受态标记。
 */
void node::SetAccept(bool tag) {
  accepted = tag;
}

/**
 * @brief 查询给定字符对应的下一跳状态范围。
 */
mulit node::GetNextStates(char ch) {
  return outstate.find(ch);
}

/**
 * @brief 返回当前结点的状态编号。
 */
int node::GetState() {
  return label;
}

/**
 * @brief 以常量方式返回当前结点的状态编号。
 */
int node::GetState() const {
  return label;
}

/**
 * @brief 返回内部多重映射的一个副本。
 */
std::multimap<char, node*> node::getMultimap() {
  return outstate;
}

/**
 * @brief 以常量方式返回内部多重映射的一个副本。
 */
std::multimap<char, node*> node::getMultimap() const {
  return outstate;
}

/**
 * @brief 用新的转移映射整体替换当前结点的出边集合。
 */
void node::setNextState(myMul next) {
  outstate = std::move(next);
}

/**
 * @brief 修改当前结点的状态编号。
 */
void node::Setstate(int state) {
  label = state;
}

}  // 命名空间 seu_lex
