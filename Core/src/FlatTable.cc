#include "Core/interface/FlatTable.h"

#include <stdexcept>

namespace harp {

std::size_t FlatTable::Column::size() const {
  switch (type) {
    case ColumnType::Float: return f.size();
    case ColumnType::Int: return i.size();
    case ColumnType::Bool: return b.size();
  }
  return 0;
}

void FlatTable::check(const std::string& name, std::size_t n) const {
  if (n != nRows_) throw std::runtime_error("FlatTable '" + name_ + "': column '" + name + "' has " + std::to_string(n) + " rows, expected " + std::to_string(nRows_));
  for (const auto& c : columns_)
    if (c.name == name) throw std::runtime_error("FlatTable '" + name_ + "': duplicate column '" + name + "'");
}

void FlatTable::addColumn(const std::string& name, std::vector<float> values, const std::string& doc) {
  check(name, values.size());
  Column c;
  c.name = name; c.type = ColumnType::Float; c.doc = doc; c.f = std::move(values);
  columns_.push_back(std::move(c));
}
void FlatTable::addColumn(const std::string& name, std::vector<int> values, const std::string& doc) {
  check(name, values.size());
  Column c;
  c.name = name; c.type = ColumnType::Int; c.doc = doc; c.i = std::move(values);
  columns_.push_back(std::move(c));
}
void FlatTable::addColumnBool(const std::string& name, std::vector<unsigned char> values, const std::string& doc) {
  check(name, values.size());
  Column c;
  c.name = name; c.type = ColumnType::Bool; c.doc = doc; c.b = std::move(values);
  columns_.push_back(std::move(c));
}

}  // namespace harp
