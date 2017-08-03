#pragma once

#include <iomanip>
#include <ostream>
#include <vector>

class table {
 public:
  table(std::ostream& out, int cols, std::initializer_list<int> widths = {10},
        int prec = 6)
      : m_out(out), m_cols(cols), m_col(0), m_prec(prec), m_widths(widths) {}

  template <class Input>
  table& operator<<(const Input& i) {
    if (std::is_scalar<Input>::value)
      m_out << std::right << std::fixed << std::setprecision(m_prec);
    else
      m_out << std::left;
    if (!m_widths.empty())
      m_out << std::setw(
          m_widths[m_col < m_widths.size() ? m_col : m_widths.size() - 1]);
    m_out << i;
    if (++m_col >= m_cols) {
      m_out << std::endl;
      m_col = 0;
    } else {
      m_out << "  ";
    }
    return *this;
  }

 private:
  std::ostream& m_out;
  int m_cols, m_col, m_prec;
  std::vector<int> m_widths;
};