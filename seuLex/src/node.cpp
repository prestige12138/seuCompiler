#include "node.h"

#include <utility>

namespace seu_lex {

node::node() : label(0), accepted(false) {}

node::node(int state, bool accepttag) : label(state), accepted(accepttag) {}

void node::Addoutstate(char ch, node* nd) {
  outstate.insert({ch, nd});
}

bool node::IsAccepted() {
  return accepted;
}

bool node::IsAccepted() const {
  return accepted;
}

void node::SetAccept(bool tag) {
  accepted = tag;
}

mulit node::GetNextStates(char ch) {
  return outstate.find(ch);
}

int node::GetState() {
  return label;
}

int node::GetState() const {
  return label;
}

std::multimap<char, node*> node::getMultimap() {
  return outstate;
}

std::multimap<char, node*> node::getMultimap() const {
  return outstate;
}

void node::setNextState(myMul next) {
  outstate = std::move(next);
}

void node::Setstate(int state) {
  label = state;
}

}  // namespace seu_lex
