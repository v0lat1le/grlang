#include "grtest.h"
#include "grlang/parse.h"

grlang::node::Node::Ptr run_in_main(std::string code) {
    std::string main = "main:= (arg:int)->int {\n" + code + "\n}";
    auto exports = grlang::parse::parse_unit(main);
    return exports.at("main")->inputs.at(0);
}

TEST_CASE(test_return) {
    auto node = run_in_main("return 123");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(get_value_int(*node->inputs.at(1)) == 123);
}

TEST_CASE(test_arithmetic_peep) {
    auto node = run_in_main("return 2*-3*4+36/6");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(get_value_int(*node->inputs.at(1)) == -18);

    node = run_in_main("return (3-1)*2*-(-5+3)");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(get_value_int(*node->inputs.at(1)) == 8);

    node = run_in_main("return 2+arg+3");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_OP_ADD);

    auto add = node->inputs.at(1);
    assert(add->inputs.at(0)->type == grlang::node::Node::Type::DATA_PROJECT);
    assert(add->inputs.at(0)->value == 1);
    assert(add->inputs.at(0)->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(add->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(get_value_int(*add->inputs.at(1)) == 5);
}

TEST_CASE(test_declarations_peep) {
    auto node = run_in_main("a:int=13 b:=7 a=9 return a-b");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(get_value_int(*node->inputs.at(1)) == 2);
}

TEST_CASE(test_scopes) {
    auto node = run_in_main("a:int = 13 { a = 7 } return a");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(get_value_int(*node->inputs.at(1)) == 7);
}

TEST_CASE(test_function) {
    auto exports = grlang::parse::parse_unit("f:= (x:int y:int) -> int { return x*y } main:= (arg:int) -> int { return f(arg+1 13) }");
    auto node = exports.at("main")->inputs.at(0);
    auto ret = node->inputs.at(0);
    assert(ret->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(ret->inputs.at(1)->type == grlang::node::Node::Type::DATA_CALL);
    assert(ret->inputs.at(1)->inputs.size() == 3);
    assert(get_value_int(*ret->inputs.at(1)->inputs.at(0)) == 0x0FEFEFE0);
    assert(ret->inputs.at(1)->inputs.at(1)->type == grlang::node::Node::Type::DATA_OP_ADD);
    assert(ret->inputs.at(1)->inputs.at(2)->type == grlang::node::Node::Type::DATA_TERM);
    auto func_ptr = ret->inputs.at(1)->inputs.at(0);
    assert(func_ptr->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(func_ptr->inputs.at(0)->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(func_ptr->inputs.at(0)->inputs.at(0)->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(func_ptr->inputs.at(0)->inputs.at(0)->inputs.at(1)->type == grlang::node::Node::Type::DATA_OP_MUL);

    grlang::parse::parse_unit("f:= (n:int) -> int { if n==0 return 0 if n==1 return 1 return f(n-1)+f(n-2) }");
}

TEST_CASE(test_if_else) {
    auto node = run_in_main("a:int=0 if arg<0 a=-arg else a=2*arg return a");
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
    auto node = run_in_main("a:int=13 if a<0 a=-a else a=2*a return a");
    assert(node->type == grlang::node::Node::Type::CONTROL_STOP);
    assert(node->inputs.size() == 1);

    node = node->inputs.at(0);
    assert(node->type == grlang::node::Node::Type::CONTROL_RETURN);
    assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM);
    assert(get_value_int(*node->inputs.at(1)) == 26);
}

TEST_CASE(test_while) {
    auto node = run_in_main("while arg<10 arg=6 return arg");
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

    assert(arg_phi->inputs.at(1)->type == grlang::node::Node::Type::DATA_PROJECT);
    assert(arg_phi->inputs.at(1)->value == 1);
    assert(arg_phi->inputs.at(2)->type == grlang::node::Node::Type::DATA_TERM);
    assert(get_value_int(*arg_phi->inputs.at(2)) == 6);
}

TEST_CASE(test_while_break) {
    auto node = run_in_main("while arg<10 { arg=5 break arg=6 } return arg");
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
    assert(arg_phi->inputs.at(2)->inputs.at(1)->type == grlang::node::Node::Type::DATA_PROJECT);
    assert(arg_phi->inputs.at(2)->inputs.at(1)->value == 1);
    assert(get_value_int(*arg_phi->inputs.at(2)->inputs.at(2)) == 6);
    assert(arg_phi->inputs.at(2)->inputs.at(0)->inputs.at(2)->type == grlang::node::Node::Type::CONTROL_DEAD);
}

TEST_CASE(test_while_continue) {
    auto node = run_in_main("while arg<10 { arg=5 continue arg=6 } return arg");
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

    assert(node->inputs.at(1)->type == grlang::node::Node::Type::DATA_PHI);
    auto arg_phi = node->inputs.at(1);
    assert(arg_phi->inputs.at(1)->type == grlang::node::Node::Type::DATA_PROJECT);
    assert(arg_phi->inputs.at(1)->value == 1);
    assert(get_value_int(*arg_phi->inputs.at(2)) == 5);
}

#include <sstream>
#include <random>
#include <vector>
#include <functional>
#include <span>

namespace {
    template<std::uniform_random_bit_generator RndGen>
    struct GeneratorState {
        RndGen rand_eng;
        std::ostream& output;
    };
    template<std::uniform_random_bit_generator RndGen>
    using GeneratorFunction = std::function<int (GeneratorState<RndGen>&)>;

    const auto& choose(const auto& container, std::uniform_random_bit_generator auto& rndgen) {
        std::uniform_int_distribution<int> distr(0, container.size()-1);
        return container.at(distr(rndgen));
    }

    int generate_char(std::string_view chars, std::uniform_random_bit_generator auto& rndgen, std::ostream& stream) {
        stream << choose(chars, rndgen);
        return 1;
    }

    template<std::uniform_random_bit_generator RndGen>
    GeneratorFunction<RndGen> generate_word(std::string word) {
        return [word](GeneratorState<RndGen>& state) {
            state.output << word;
            return word.size();
        };
    }

    template<std::uniform_random_bit_generator RndGen>
    GeneratorFunction<RndGen> generate_any(std::vector<GeneratorFunction<RndGen>> gens) {
        return [gens](GeneratorState<RndGen>& state) {
            return choose(gens, state.rand_eng)(state);
        };
    }

    template<std::uniform_random_bit_generator RndGen>
    GeneratorFunction<RndGen> generate_many(GeneratorFunction<RndGen> gen, int n) {
        return [gen, n](GeneratorState<RndGen>& state) {
            int count = 0;
            for (int i=0; i<n; ++i) {
                count += gen(state);
            }
            return count;
        };
    }
    template<std::uniform_random_bit_generator RndGen>
    GeneratorFunction<RndGen> generate_all(std::vector<GeneratorFunction<RndGen>> gens) {
        return [gens](GeneratorState<RndGen>& state) {
            int count = 0;
            for (auto f: gens) {
                count += f(state);
            }
            return count;
        };
    }

    void generate(int seed, std::ostream& stream) {
        using RndGenImpl = std::minstd_rand;

        GeneratorFunction<RndGenImpl> gen_space = [](GeneratorState<RndGenImpl>& state) {
            return generate_char(" \n\t", state.rand_eng, state.output);
        };
        GeneratorFunction<RndGenImpl> gen_digit = [](GeneratorState<RndGenImpl>& state) {
            return generate_char("0123456789", state.rand_eng, state.output);
        };
        GeneratorFunction<RndGenImpl> gen_non_digit = [](GeneratorState<RndGenImpl>& state) {
            return generate_char("abcdefABCDEF", state.rand_eng, state.output);
        };
        GeneratorFunction<RndGenImpl> gen_lit = generate_many<RndGenImpl>(gen_digit, 3);
        GeneratorFunction<RndGenImpl> gen_ident = generate_all<RndGenImpl>({
            gen_non_digit,
            generate_many(generate_any<RndGenImpl>({gen_digit, gen_non_digit}), 4)
        });
        GeneratorFunction<RndGenImpl> gen_type = generate_word<RndGenImpl>("int");
        GeneratorFunction<RndGenImpl> gen_expression;
        GeneratorFunction<RndGenImpl> gen_expression_recurse = [&gen_expression](GeneratorState<RndGenImpl>& state) { return gen_expression(state); };
        gen_expression = generate_any<RndGenImpl>({
            generate_all<RndGenImpl>({
                generate_word<RndGenImpl>("("),
                gen_expression_recurse,
                generate_word<RndGenImpl>(")")
            }),
            generate_all<RndGenImpl>({
                generate_word<RndGenImpl>("-"),
                gen_expression_recurse
            }),
            generate_all<RndGenImpl>({
                gen_expression_recurse,
                generate_any<RndGenImpl>({generate_word<RndGenImpl>("+"), generate_word<RndGenImpl>("-")}),
                gen_expression_recurse
            }),
            gen_ident,
            gen_lit
        });
        GeneratorFunction<RndGenImpl> gen_statement = generate_all<RndGenImpl>({
            generate_word<RndGenImpl>("return"),
            gen_space,
            gen_expression
        });
        GeneratorFunction<RndGenImpl> gen_program = generate_many(
            generate_all<RndGenImpl>({gen_statement, gen_space}), 1);

        GeneratorState<RndGenImpl> state{RndGenImpl(seed), stream};
        gen_program(state);
    }
}

TEST_CASE(test_fuzz) {
    auto node = run_in_main("while arg<10 arg=6 return arg");
    std::ostringstream stream;
    generate(0, stream);
    auto exports = grlang::parse::parse_unit(stream.str());
}
