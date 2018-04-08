// Copyright 2018 leoetlino <leo@leolam.fr>
// Licensed under GPLv2

#pragma once

// Small helper class to keep results and descriptions since
// they cannot be sent over network immediately.

#include <string>
#include <utility>
#include <vector>

template <typename T>
class ResultPrinter final {
public:
  void Add(const std::string& description, T&& result) {
    m_results.emplace_back(description, std::move(result));
  }

  using PrintFunction = void(*)(const std::string& description, const T& result);
  void Print(PrintFunction print_function) const {
    for (const auto& result : m_results) {
      print_function(result.first, result.second);
    }
  }

private:
  std::vector<std::pair<std::string, T>> m_results;
};
