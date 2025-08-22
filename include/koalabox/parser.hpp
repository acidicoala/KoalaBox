#pragma once

#include <regex>
#include <string>

#include <cpp-tree-sitter.h>

/**
 * Utilities for parsing C/C++ headers and sources.
 */
namespace koalabox::parser {
    ts::Tree parse_source(const std::string_view& buffer);

    struct query_entry {
        std::string path;
        std::string_view value;
    };

    /**
     * Finds all nodes that match the given selector.
     * Assumes that there are no syntax errors. The regex will need to match a path like this:
     * "/translation_unit/function_definition/function_declaration/identifier".
     * Convenient to use, but is relatively slow because of regex.
     */
    std::vector<query_entry> query(const std::string_view& source, const std::regex& selector);

    enum class visit_result {
        Continue,
        SkipChildren,
        Stop,
    };

    /**
     * Walks through the AST tree and calls the `visit` function on each node.
     * Cam be used for writing custom parsing logic that significantly speeds up analysis.
     */
    void walk(const ts::Node& root, const std::function<visit_result(const ts::Node &)>& visit);
}