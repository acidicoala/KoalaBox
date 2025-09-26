#include <deque>

#include <koalabox/logger.hpp>

#include "koalabox_tools/parser.hpp"

extern "C" const TSLanguage* tree_sitter_cpp();

namespace {
    struct query_entry_impl {
        std::string path;
        ts::Node node;
    };

    std::vector<query_entry_impl> query_impl( // NOLINT(*-no-recursion)
        const std::regex& selector,
        const ts::Node& node,
        const bool single_match,
        const std::string& parent_path = ""
    ) {
        std::vector<query_entry_impl> results{};

        const auto current_path = std::format("{}/{}", parent_path, node.getType());

        if(std::regex_match(current_path, selector)) {
            results.emplace_back(
                query_entry_impl{
                    .path = current_path,
                    .node = node,
                }
            );

            if(single_match) {
                return results;
            }
        }

        if(current_path.ends_with("parameter_declaration/array_declarator")) {
            // Special case for array declarators (e.g. int buffer[123])
            results.emplace_back(current_path + "/identifier", node.getNamedChild(0));
            results.emplace_back(current_path + "/dimension", node.getNamedChild(1));
            return results;
        }

        for(auto i = 0U, count = node.getNumNamedChildren(); i < count; ++i) {
            if(const auto child = node.getNamedChild(i); not child.isNull()) {
                const auto child_results = query_impl(selector, child, single_match, current_path);

                // Move child results to current results
                results.insert(
                    results.end(),
                    std::make_move_iterator(child_results.begin()),
                    std::make_move_iterator(child_results.end())
                );
            }
        }

        return results;
    }
}

namespace koalabox::tools::parser {
    ts::Tree parse_source(const std::string_view& buffer) {
        const auto language = ts::Language(tree_sitter_cpp());
        auto parser = ts::Parser(language);
        return parser.parseString(buffer);
    }

    std::vector<query_entry> query(const std::string_view& source, const std::regex& selector, bool single_match) {
        const auto root = parse_source(source);

        const auto node_results = query_impl(selector, root.getRootNode(), single_match, "");

        std::vector<query_entry> string_results;
        string_results.reserve(node_results.size());
        for(const auto& [path, node] : node_results) {
            string_results.emplace_back(
                query_entry{
                    .path = path,
                    .value = node.getSourceRange(source),
                }
            );
        }

        return string_results;
    }

    void walk(const ts::Node& root, const std::function<visit_result(const ts::Node&)>& visit) {
        // DFS traversal
        std::deque<ts::Node> queue;
        queue.push_back(root);
        auto first_visit = true;

        while(not queue.empty()) {
            const auto node = queue.front();
            queue.pop_front();

            switch(first_visit ? visit_result::Continue : visit(node)) {
            case visit_result::Continue:
                break;
            case visit_result::SkipChildren:
                continue;
            case visit_result::Stop:
                return;
            }

            for(uint32_t i = 0, count = node.getNumNamedChildren(); i < count; ++i) {
                if(const auto child = node.getNamedChild(i); not child.isNull()) {
                    queue.push_back(child);
                }
            }

            first_visit = false;
        }
    }
}
