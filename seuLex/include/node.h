#pragma once

#include <map>

/**
 * @file node.h
 * @brief 词法分析模块使用的报告规定状态结点类型与类型别名。
 */

namespace seu_lex {

class node;
typedef std::multimap<char, node*>::iterator mulit;
typedef std::multimap<char, node*> myMul;

/**
 * @brief 中期报告规定的自动机状态结点。
 *
 * 结点保存状态编号、接受标记以及一张多重映射形式的出边表。
 * 内部使用 `'\0'` 表示 epsilon 边。
 */
class node {
 public:
  /**
   * @brief 构造一个编号为 0 的非接受结点。
   *
   * 复杂度：O(1)。
   */
  node();

  /**
   * @brief 构造一个带指定编号和接受标记的结点。
   *
   * 复杂度：O(1)。
   */
  node(int state, bool accepttag);

  /**
   * @brief 添加一条出边转移。
   *
   * 复杂度：O(log E)，其中 E 为出边条数。
   */
  void Addoutstate(char ch, node* nd);

  /**
   * @brief 判断当前结点是否为接受结点。
   *
   * 复杂度：O(1)。
   */
  bool IsAccepted();
  bool IsAccepted() const;

  /**
   * @brief 更新接受标记。
   *
   * 复杂度：O(1)。
   */
  void SetAccept(bool tag);

  /**
   * @brief 返回第一条匹配字符 @p ch 的转移迭代器。
   *
   * 复杂度：O(log E)。
   */
  mulit GetNextStates(char ch);

  /**
   * @brief 返回状态编号。
   *
   * 复杂度：O(1)。
   */
  int GetState();
  int GetState() const;

  /**
   * @brief 返回转移多重映射的一份拷贝。
   *
   * 复杂度：O(E)。
   */
  std::multimap<char, node*> getMultimap();
  std::multimap<char, node*> getMultimap() const;

  /**
   * @brief 整体替换转移多重映射。
   *
   * 复杂度：O(E)。
   */
  void setNextState(myMul next);

  /**
   * @brief 覆盖写入状态编号。
   *
   * 复杂度：O(1)。
   */
  void Setstate(int state);

 private:
  int label;
  bool accepted;
  std::multimap<char, node*> outstate;
};

}  // 命名空间 seu_lex
