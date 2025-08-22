#include <cassert>
#include <iostream>
#include <vector>

#include "grlang/parse.h"


static std::vector<std::pair<void(*)(), const char*>> registered_tests;
struct test_register {
    test_register(void(*test)(), const char* name) {
        registered_tests.emplace_back(test, name);
    }
};
#define TEST_CASE(name) void name(); test_register test_register_##name(name, #name); void name()

TEST_CASE(test_return) {
    auto node = grlang::parse::parse("return 123");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(get_value_int(*node->inputs.at(1)) == 123);
}

TEST_CASE(test_arithmetic_peep) {
    auto node = grlang::parse::parse("return 2*-3*4+36/6");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(get_value_int(*node->inputs.at(1)) == -18);

    node = grlang::parse::parse("return (3-1)*2*-(-5+3)");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(get_value_int(*node->inputs.at(1)) == 8);

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

TEST_CASE(test_declarations_peep) {
    auto node = grlang::parse::parse("a:int=13 b:int=7 return a-b");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(get_value_int(*node->inputs.at(1)) == 6);
}

TEST_CASE(test_scopes) {
    auto node = grlang::parse::parse("a:int = 13 { a = 7 } return a");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(get_value_int(*node->inputs.at(1)) == 7);
}

TEST_CASE(test_struct) {
    auto node = grlang::parse::parse("a:{x:int,y:{z:int}}=0 return a");
}

TEST_CASE(test_function) {
    auto node = grlang::parse::parse("a:{x:int}->int=0 return a");
}

TEST_CASE(test_if_else) {
    auto node = grlang::parse::parse("a:int=0 if arg<0 a=-arg else a=2*arg return a");
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

TEST_CASE(test_if_else_peep) {
    auto node = grlang::parse::parse("a:int=13 if a<0 a=-a else a=2*a return a");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(get_value_int(*node->inputs.at(1)) == 26);
}

TEST_CASE(test_while) {
    auto node = grlang::parse::parse("while arg<10 arg=6 return arg");
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
    assert(arg_phi->inputs.at(2)->type == grlang::node::Node::Type::DATA_TERM);
    assert(get_value_int(*arg_phi->inputs.at(2)) == 6);
}

TEST_CASE(test_while_break) {
    auto node = grlang::parse::parse("while arg<10 { arg=5 break arg=6 } return arg");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_REGION);
    assert(node->inputs.at(0)->inputs.at(1)->type == grlang::node::Node::Type::CONTROL_PROJECT);
    assert(node->inputs.at(0)->inputs.at(1)->value == 0);
    assert(node->inputs.at(0)->inputs.at(2)->type == grlang::node::Node::Type::CONTROL_PROJECT);
    assert(node->inputs.at(0)->inputs.at(2)->value == 1);

    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_PHI);
    auto arg_phi = node->inputs.at(1);
    assert(get_value_int(*arg_phi->inputs.at(1)) == 5);
    assert(arg_phi->inputs.at(2)->type == grlang::node::Node::Type::DATA_PHI);
    assert(arg_phi->inputs.at(2)->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(get_value_int(*arg_phi->inputs.at(2)->inputs.at(2)) == 6);
}

TEST_CASE(test_while_continue) {
    auto node = grlang::parse::parse("while arg<10 { arg=5 continue arg=6 } return arg");
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
    assert(reg->inputs.at(2)->type == grlang::node::Node::Type::CONTROL_REGION);

    assert(reg->inputs.at(2)->inputs.at(1)->type == grlang::node::Node::Type::CONTROL_PROJECT);
    assert(reg->inputs.at(2)->inputs.at(2)->value == 0);
    assert(reg->inputs.at(2)->inputs.at(2)->type == grlang::node::Node::Type::CONTROL_PROJECT);
    assert(reg->inputs.at(2)->inputs.at(2)->value == 0);

    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_PHI);
    auto arg_phi = node->inputs.at(1);
    assert(arg_phi->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(arg_phi->inputs.at(2)->type == grlang::node::Node::Type::DATA_PHI);
    assert(get_value_int(*arg_phi->inputs.at(2)->inputs.at(1)) == 5);
    assert(get_value_int(*arg_phi->inputs.at(2)->inputs.at(2)) == 6);
}

int main() {
    for (auto [test, name]: registered_tests) {
        std::cout << "running " << name << "..." << std::endl;
        test();
    }
    return 0;
}
