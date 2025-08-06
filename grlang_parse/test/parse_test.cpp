#include <cassert>

#include "grlang/parse.h"


void test_return() {
    auto node = grlang::parse::parse("return 123");
    assert(node->type == grlang::node::NodeType::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::NodeType::CONSTANT_INT);
    assert(node->inputs.at(0)->value_int == 123);
}

void test_arithmetic() {
    auto node = grlang::parse::parse("return 2*-3*4+5/6");
    assert(node->type == grlang::node::NodeType::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::NodeType::OPERATION_ADD);
    auto add = node->inputs.at(0);
    assert(add->inputs.at(0)->type == grlang::node::NodeType::OPERATION_MUL);
    assert(add->inputs.at(1)->type == grlang::node::NodeType::OPERATION_DIV);

    auto mul1 = add->inputs.at(0);
    assert(mul1->inputs.at(0)->type == grlang::node::NodeType::CONSTANT_INT);
    assert(mul1->inputs.at(0)->value_int == 2);
    assert(mul1->inputs.at(1)->type == grlang::node::NodeType::OPERATION_MUL);

    auto mul2 = mul1->inputs.at(1);
    assert(mul2->inputs.at(0)->type == grlang::node::NodeType::OPERATION_NEG);
    assert(mul2->inputs.at(0)->inputs.at(0)->type == grlang::node::NodeType::CONSTANT_INT);
    assert(mul2->inputs.at(0)->inputs.at(0)->value_int == 3);
    assert(mul2->inputs.at(1)->type == grlang::node::NodeType::CONSTANT_INT);
    assert(mul2->inputs.at(1)->value_int == 4);

    auto mul3 = add->inputs.at(1);
    assert(mul3->inputs.at(0)->type == grlang::node::NodeType::CONSTANT_INT);
    assert(mul3->inputs.at(0)->value_int == 5);
    assert(mul3->inputs.at(1)->type == grlang::node::NodeType::CONSTANT_INT);
    assert(mul3->inputs.at(1)->value_int == 6);
}

void test_declarations() {
    auto node = grlang::parse::parse("a = 13 b = 7 return a-b");
    assert(node->type == grlang::node::NodeType::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::NodeType::OPERATION_SUB);
    auto sub = node->inputs.at(0);
    assert(sub->inputs.at(0)->type == grlang::node::NodeType::CONSTANT_INT);
    assert(sub->inputs.at(0)->value_int == 13);
    assert(sub->inputs.at(1)->type == grlang::node::NodeType::CONSTANT_INT);
    assert(sub->inputs.at(1)->value_int == 7);
}

void test_scopes() {
    auto node = grlang::parse::parse("a = 13 { a = 7 } return a");
    assert(node->type == grlang::node::NodeType::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::NodeType::CONSTANT_INT);
    assert(node->inputs.at(0)->value_int == 7);
}

void test_if_else() {
    auto node = grlang::parse::parse("a = 13 if a { return a } else { return -a }");
}

int main() {
    test_return();
    test_arithmetic();
    test_declarations();
    test_scopes();
    test_if_else();
    return 0;
}
