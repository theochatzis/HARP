#ifndef HARP_CORE_FLATTABLE_H
#define HARP_CORE_FLATTABLE_H
// Nano-style flat table: N rows x columns of float / int / bool.
// This is the persistent representation of products written by the
// NanoSkimWriter (branches n<Table>, <Table>_<column>[n<Table>]).
#include <string>
#include <vector>

namespace harp {

class FlatTable {
 public:
  enum class ColumnType { Float, Int, Bool };
  struct Column {
    std::string name;
    ColumnType type;
    std::string doc;
    std::vector<float> f;
    std::vector<int> i;
    std::vector<unsigned char> b;
    std::size_t size() const;
  };

  FlatTable() = default;
  // singleton=true: one row, no counter branch (like nano "singleton" tables).
  FlatTable(std::string name, std::size_t nRows, bool singleton = false) : name_(std::move(name)), nRows_(nRows), singleton_(singleton) {}

  const std::string& name() const { return name_; }
  void setName(const std::string& n) { name_ = n; }
  std::size_t nRows() const { return nRows_; }
  bool singleton() const { return singleton_; }
  const std::vector<Column>& columns() const { return columns_; }

  void addColumn(const std::string& name, std::vector<float> values, const std::string& doc = "");
  void addColumn(const std::string& name, std::vector<int> values, const std::string& doc = "");
  void addColumnBool(const std::string& name, std::vector<unsigned char> values, const std::string& doc = "");

 private:
  void check(const std::string& name, std::size_t n) const;
  std::string name_;
  std::size_t nRows_ = 0;
  bool singleton_ = false;
  std::vector<Column> columns_;
};

}  // namespace harp
#endif
