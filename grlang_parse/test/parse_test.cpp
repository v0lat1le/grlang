#include <cassert>

#include "grlang/parse.h"


void test_return() {
    auto node = grlang::parse::parse("return 123");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(node->inputs.at(1)->value_int == 123);
}

void test_arithmetic() {
    auto node = grlang::parse::parse("return 2*-3*4+36/6");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_OP_ADD);

    auto add = node->inputs.at(1);
    assert(add->inputs.at(0)->type == grlang::node::Node::Type::DATA_OP_MUL);
    assert(add->inputs.at(1)->type == grlang::node::Node::Type::DATA_OP_DIV);

    auto mul1 = add->inputs.at(0);
    assert(mul1->inputs.at(0)->type == grlang::node::Node::Type::DATA_TERM);
    assert(mul1->inputs.at(0)->value_int == 2);
    assert(mul1->inputs.at(1)->type == grlang::node::Node::Type::DATA_OP_MUL);

    auto mul2 = mul1->inputs.at(1);
    assert(mul2->inputs.at(0)->type == grlang::node::Node::Type::DATA_OP_NEG);
    assert(mul2->inputs.at(0)->inputs.at(0)->type == grlang::node::Node::Type::DATA_TERM);
    assert(mul2->inputs.at(0)->inputs.at(0)->value_int == 3);
    assert(mul2->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(mul2->inputs.at(1)->value_int == 4);

    auto mul3 = add->inputs.at(1);
    assert(mul3->inputs.at(0)->type == grlang::node::Node::Type::DATA_TERM);
    assert(mul3->inputs.at(0)->value_int == 36);
    assert(mul3->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(mul3->inputs.at(1)->value_int == 6);
}

void test_declarations() {
    auto node = grlang::parse::parse("a:int = 13 b:int = 7 return a-b");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_OP_SUB);
    auto sub = node->inputs.at(1);
    assert(sub->inputs.at(0)->type == grlang::node::Node::Type::DATA_TERM);
    assert(sub->inputs.at(0)->value_int == 13);
    assert(sub->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(sub->inputs.at(1)->value_int == 7);
}

void test_scopes() {
    auto node = grlang::parse::parse("a:int = 13 { a = 7 } return a");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(node->inputs.at(1)->value_int == 7);
}

void test_if_else() {
    auto node = grlang::parse::parse("a:int = 13 if a<0 a = -a else a=2*a return a");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_REGION);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_PHI);

    auto region = node->inputs.at(0);
    assert(region->inputs.at(0) == nullptr);
    assert(region->inputs.at(1)->type == grlang::node::Node::Type::CONTROL_PROJECT);
    assert(region->inputs.at(1)->value_int == 0);
    assert(region->inputs.at(2)->type == grlang::node::Node::Type::CONTROL_PROJECT);
    assert(region->inputs.at(2)->value_int == 1);

    assert(region->inputs.at(1)->inputs.at(0) == region->inputs.at(2)->inputs.at(0));
    auto ifelse = region->inputs.at(1)->inputs.at(0);
    assert(ifelse->type == grlang::node::Node::Type::CONTROL_IFELSE);
    assert(ifelse->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(ifelse->inputs.at(1)->type == grlang::node::Node::Type::DATA_OP_LT);

    auto phi = node->inputs.at(1);
    assert(phi->inputs.at(0) == node->inputs.at(0));
    assert(phi->inputs.at(1)->type == grlang::node::Node::Type::DATA_OP_NEG);
    assert(phi->inputs.at(2)->type == grlang::node::Node::Type::DATA_OP_MUL);
}

int main() {
    test_return();
    test_arithmetic();
    test_declarations();
    test_scopes();
    test_if_else();
    return 0;
}
