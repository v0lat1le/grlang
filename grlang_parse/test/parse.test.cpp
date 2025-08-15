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
    assert(get_value_int(*node->inputs.at(1)) == 123);
}

void test_arithmetic_peep() {
    auto node = grlang::parse::parse("return 2*-3*4+36/6");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(get_value_int(*node->inputs.at(1)) == -18);

    node = grlang::parse::parse("return 2+arg+3");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_OP_ADD);

    auto add = node->inputs.at(1);
    assert(add->inputs.at(0)->type == grlang::node::Node::Type::DATA_TERM);
    assert(static_cast<grlang::node::ValueNode&>(*add->inputs.at(0)).value.clazz == grlang::node::Value::Class::VARIABLE);
    assert(add->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(get_value_int(*add->inputs.at(1)) == 5);
}

void test_declarations_peep() {
    auto node = grlang::parse::parse("a:int = 13 b:int = 7 return a-b");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(get_value_int(*node->inputs.at(1)) == 6);
}

void test_scopes() {
    auto node = grlang::parse::parse("a:int = 13 { a = 7 } return a");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(get_value_int(*node->inputs.at(1)) == 7);
}

void test_if_else() {
    auto node = grlang::parse::parse("a:int=0 if arg<0 a = -arg else a=2*arg return a");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_REGION);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_PHI);

    auto region = node->inputs.at(0);
    assert(region->inputs.at(0) == nullptr);
    assert(region->inputs.at(1)->type == grlang::node::Node::Type::CONTROL_PROJECT);
    assert(region->inputs.at(1)->value == 0);
    assert(region->inputs.at(2)->type == grlang::node::Node::Type::CONTROL_PROJECT);
    assert(region->inputs.at(2)->value == 1);

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

void test_if_else_peep() {
    auto node = grlang::parse::parse("a:int = 13 if a<0 a = -a else a=2*a return a");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(get_value_int(*node->inputs.at(1)) == 26);
}

void test_if_else_dom() {
    auto node = grlang::parse::parse("a:int=0 b:int=1 if arg { a=2 if arg b=2 else b=3 } return a+b");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_REGION);
}

void test_while() {
    auto node = grlang::parse::parse("a:int=0 while a < 10 { a = a+1 arg = arg+a } return arg");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_PROJECT);
    assert(node->inputs.at(0)->value == 1);
    assert(node->inputs.at(0)->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_IFELSE);
    assert(node->inputs.at(0)->inputs.at(0)->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_REGION);
    auto reg = node->inputs.at(0)->inputs.at(0)->inputs.at(0);
    assert(reg->inputs.at(1)->type == grlang::node::Node::Type::CONTROL_START);
    assert(reg->inputs.at(2)->type == grlang::node::Node::Type::CONTROL_PROJECT);
    assert(reg->inputs.at(2)->value == 0);
    assert(reg->inputs.at(2)->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_IFELSE);
    assert(reg->inputs.at(2)->inputs.at(0) == node->inputs.at(0)->inputs.at(0));

    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_PHI);
    auto arg_phi = node->inputs.at(1);
    assert(arg_phi->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_REGION);
    assert(arg_phi->inputs.at(0) == reg);

    assert(arg_phi->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(arg_phi->inputs.at(2)->type == grlang::node::Node::Type::DATA_OP_ADD);
    assert(arg_phi->inputs.at(2)->inputs.at(0) == arg_phi);
    assert(arg_phi->inputs.at(2)->inputs.at(1)->type == grlang::node::Node::Type::DATA_OP_ADD);

    auto a_add = arg_phi->inputs.at(2)->inputs.at(1);
    assert(a_add->inputs.at(0)->type == grlang::node::Node::Type::DATA_PHI);
    assert(a_add->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);

    auto a_phi = node->inputs.at(1);
    assert(a_phi->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(a_phi->inputs.at(2)->type == grlang::node::Node::Type::DATA_OP_ADD);
    assert(a_phi->inputs.at(2)->inputs.at(0) == a_phi);
    assert(a_phi->inputs.at(2)->inputs.at(1)->type == grlang::node::Node::Type::DATA_OP_ADD);
}

int main() {
    test_return();
    test_arithmetic_peep();
    test_declarations_peep();
    test_scopes();
    test_if_else();
    test_if_else_peep();
    test_if_else_dom();
    test_while();
    return 0;
}
