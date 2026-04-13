#pragma once

#include <map>

/**
 * @file node.h
 * @brief Report-defined node type and typedefs for seuLex.
 */

namespace seu_lex {

class node;
typedef std::multimap<char, node*>::iterator mulit;
typedef std::multimap<char, node*> myMul;

/**
 * @brief Report-defined automaton state node.
 *
 * The state stores a numeric label, an accepting tag, and a multimap of
 * outgoing edges. `'\0'` is used internally as epsilon.
 */
class node {
 public:
  /**
   * @brief Construct a non-accepting node with label 0.
   *
   * Complexity: O(1).
   */
  node();

  /**
   * @brief Construct a node with explicit label and accepting tag.
   *
   * Complexity: O(1).
   */
  node(int state, bool accepttag);

  /**
   * @brief Add one outgoing transition.
   *
   * Complexity: O(log E), where E is the number of outgoing edges.
   */
  void Addoutstate(char ch, node* nd);

  /**
   * @brief Query whether the node is accepting.
   *
   * Complexity: O(1).
   */
  bool IsAccepted();
  bool IsAccepted() const;

  /**
   * @brief Update accepting tag.
   *
   * Complexity: O(1).
   */
  void SetAccept(bool tag);

  /**
   * @brief Return iterator to the first transition matching @p ch.
   *
   * Complexity: O(log E).
   */
  mulit GetNextStates(char ch);

  /**
   * @brief Return numeric state label.
   *
   * Complexity: O(1).
   */
  int GetState();
  int GetState() const;

  /**
   * @brief Return a copy of the transition multimap.
   *
   * Complexity: O(E).
   */
  std::multimap<char, node*> getMultimap();
  std::multimap<char, node*> getMultimap() const;

  /**
   * @brief Replace transition multimap.
   *
   * Complexity: O(E).
   */
  void setNextState(myMul next);

  /**
   * @brief Overwrite state label.
   *
   * Complexity: O(1).
   */
  void Setstate(int state);

 private:
  int label;
  bool accepted;
  std::multimap<char, node*> outstate;
};

}  // namespace seu_lex
