#include "test/type_graph_utils.h"

#include <gtest/gtest.h>

#include "oi/ContainerInfo.h"
#include "oi/type_graph/NodeTracker.h"
#include "oi/type_graph/PassManager.h"
#include "oi/type_graph/Printer.h"
#include "oi/type_graph/TypeGraph.h"
#include "test/TypeGraphParser.h"

using type_graph::Container;
using type_graph::NodeId;
using type_graph::Pass;
using type_graph::Type;
using type_graph::TypeGraph;

template <typename T>
using ref = std::reference_wrapper<T>;

void check(const TypeGraph& typeGraph,
           std::string_view expected,
           std::string_view comment) {
  std::stringstream out;
  NodeTracker tracker;
  type_graph::Printer printer(out, tracker, typeGraph.size());

  for (const auto& type : typeGraph.rootTypes()) {
    printer.print(type);
  }

  if (expected[0] == '\n')
    expected.remove_prefix(1);  // Remove initial '\n'
  ASSERT_EQ(expected, out.str()) << "Test failure " << comment;
}

void test(type_graph::Pass pass,
          std::string_view input,
          std::string_view expectedAfter) {
  input.remove_prefix(1);  // Remove initial '\n'
  TypeGraph typeGraph;
  TypeGraphParser parser{typeGraph};
  try {
    parser.parse(input);
  } catch (const TypeGraphParserError& err) {
    FAIL() << "Error parsing input graph: " << err.what();
  }

  // Validate input formatting
  check(typeGraph, input, "parsing input graph");

  NodeTracker tracker;
  pass.run(typeGraph, tracker);

  check(typeGraph, expectedAfter, "after running pass");
}

void testNoChange(type_graph::Pass pass, std::string_view input) {
  test(pass, input, input);
}

std::vector<std::unique_ptr<ContainerInfo>> getContainerInfos() {
  auto std_vector =
      std::make_unique<ContainerInfo>("std::vector", SEQ_TYPE, "vector");
  std_vector->stubTemplateParams = {1};

  auto std_map = std::make_unique<ContainerInfo>("std::map", SEQ_TYPE, "map");
  std_map->stubTemplateParams = {2, 3};

  auto std_list =
      std::make_unique<ContainerInfo>("std::list", SEQ_TYPE, "list");
  std_list->stubTemplateParams = {1};

  auto std_pair =
      std::make_unique<ContainerInfo>("std::pair", SEQ_TYPE, "list");

  std::vector<std::unique_ptr<ContainerInfo>> containers;
  containers.emplace_back(std::move(std_vector));
  containers.emplace_back(std::move(std_map));
  containers.emplace_back(std::move(std_list));
  containers.emplace_back(std::move(std_pair));

  return containers;
}

Container getVector(NodeId id) {
  static ContainerInfo info{"std::vector", SEQ_TYPE, "vector"};
  info.stubTemplateParams = {1};
  return Container{id, info, 24, nullptr};
}

Container getMap(NodeId id) {
  static ContainerInfo info{"std::map", STD_MAP_TYPE, "map"};
  info.stubTemplateParams = {2, 3};
  return Container{id, info, 48, nullptr};
}

Container getList(NodeId id) {
  static ContainerInfo info{"std::list", LIST_TYPE, "list"};
  info.stubTemplateParams = {1};
  return Container{id, info, 24, nullptr};
}

Container getPair(NodeId id) {
  static ContainerInfo info{"std::pair", PAIR_TYPE, "utility"};
  return Container{
      id, info, 8, nullptr};  // Nonsense size, shouldn't matter for tests
}

std::pair<size_t, size_t> globMatch(std::string_view pattern,
                                    std::string_view str) {
  size_t i = 0;
  size_t j = 0;
  size_t prevWildcardIdx = -1;
  size_t backtrackIdx = -1;
  size_t patternLineStart = 0;
  size_t strLineStart = 0;

  while (i < str.size()) {
    if (i + 1 < str.size() && str[i] == '\n')
      strLineStart = i + 1;
    if (j + 1 < pattern.size() && pattern[j] == '\n')
      patternLineStart = j + 1;

    if (j < pattern.size() && str[i] == pattern[j]) {
      // Exact character match
      i++;
      j++;
    } else if (j < pattern.size() && pattern[j] == '*') {
      // Wildcard
      backtrackIdx = i + 1;
      prevWildcardIdx = j++;
    } else if (prevWildcardIdx != static_cast<size_t>(-1)) {
      // No match, backtrack to previous wildcard
      i = ++backtrackIdx;
      j = prevWildcardIdx + 1;
    } else {
      // No match
      return {patternLineStart, strLineStart};
    }
  }

  while (j < pattern.size() && pattern[j] == '*') {
    j++;
  }

  // If the pattern has been fully consumed then it's a match
  return {j, i};
}

std::string prefixLines(std::string_view str,
                        std::string_view prefix,
                        size_t maxLen) {
  std::string res;
  res += prefix;
  for (size_t i = 0; i < maxLen && i < str.size(); i++) {
    char c = str[i];
    res += c;
    if (c == '\n') {
      res += prefix;
    }
  }
  if (str.size() > maxLen)
    res += "...";
  return res;
}
