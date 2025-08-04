#include <cassert>

#include "grlang/parse.h"


void test_return() {
    auto node = grlang::parse::parse("return 123");
    assert(node->type == grlang::node::NodeType::RETURN);
    assert(node->inputs.at(0)->type == grlang::node::NodeType::CONSTANT_INT);
    assert(node->inputs.at(0)->value_int == 123);
}

void test_arithmetic() {
    auto node = grlang::parse::parse("return 2*3*4+5*6");
    assert(node->type == grlang::node::NodeType::RETURN);
    assert(node->inputs.at(0)->type == grlang::node::NodeType::OPERATION_ADD);
    auto add = node->inputs.at(0);
    assert(add->inputs.at(0)->type == grlang::node::NodeType::OPERATION_MUL);
    assert(add->inputs.at(1)->type == grlang::node::NodeType::OPERATION_MUL);

    auto mul1 = add->inputs.at(0);
    assert(mul1->inputs.at(0)->type == grlang::node::NodeType::CONSTANT_INT);
    assert(mul1->inputs.at(0)->value_int == 2);
    assert(mul1->inputs.at(1)->type == grlang::node::NodeType::OPERATION_MUL);

    auto mul2 = mul1->inputs.at(1);
    assert(mul2->inputs.at(0)->type == grlang::node::NodeType::CONSTANT_INT);
    assert(mul2->inputs.at(0)->value_int == 3);
    assert(mul2->inputs.at(1)->type == grlang::node::NodeType::CONSTANT_INT);
    assert(mul2->inputs.at(1)->value_int == 4);

    auto mul3 = add->inputs.at(1);
    assert(mul3->inputs.at(0)->type == grlang::node::NodeType::CONSTANT_INT);
    assert(mul3->inputs.at(0)->value_int == 5);
    assert(mul3->inputs.at(1)->type == grlang::node::NodeType::CONSTANT_INT);
    assert(mul3->inputs.at(1)->value_int == 6);
}

int main() {
    test_return();
    test_arithmetic();
    return 0;
}
