#pragma once

#include <vector>

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

#define KBT_CMD_ADD_OPTION(r, data, elem) \
    (BOOST_PP_STRINGIZE(r) "," BOOST_PP_STRINGIZE(elem), "", cxxopts::value<decltype(elem)>())

#define KBT_CMD_SET_ARG(r, data, elem) \
    .elem = parsedResult[BOOST_PP_STRINGIZE(elem)].as<decltype(elem)>(),

#define KBT_CMD_LOG_ARG(r, args_var, elem) \
    LOG_INFO("{:<20} = {}", BOOST_PP_STRINGIZE(elem), args_var.elem);

#define KBT_CMD_PARSE_ARGS(EXE, DESC, ARGC, ARGV, ...) \
    cxxopts::Options options(EXE, DESC); \
    options.add_options() \
        BOOST_PP_SEQ_FOR_EACH(KBT_CMD_ADD_OPTION, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) \
    ; \
    const auto parsedResult = options.parse(ARGC, ARGV); \
    const Args args{ \
        BOOST_PP_SEQ_FOR_EACH(KBT_CMD_SET_ARG, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) \
    }; \
     BOOST_PP_SEQ_FOR_EACH(KBT_CMD_LOG_ARG, args, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

namespace koalabox::tools::cmd {
    struct normalized_args_t {
        std::vector<std::string> args_containers;
        std::vector<const char*> argv;
    };

    normalized_args_t normalize_args(int argc, const TCHAR** argv);
}
