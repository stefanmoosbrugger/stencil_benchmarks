#pragma once

#include "variant_base.h"

namespace platform {

template <class Platform, class ValueType>
class variant : public variant_base {
 public:
  using platform = Platform;
  using value_type = ValueType;
  using allocator = typename platform::allocator<value_type>;

  variant(const arguments_map& args)
      : variant_base(args),
        m_src_data(storage_size()),
        m_dst_data(storage_size()) {
    int imin = -m_halo, imax = m_isize + m_halo;
    int jmin = -m_halo, jmax = m_jsize + m_halo;
    int kmin = -m_halo, kmax = m_ksize + m_halo;
#pragma omp parallel for collapse(3)
    for (int k = kmin; k < kmax; ++k)
      for (int j = jmin; j < jmax; ++j)
        for (int i = imin; i < imax; ++i) {
          m_src_data.at(zero_offset() + index(i, j, k)) = index(i, j, k);
        }
  }

 protected:
  value_type *m_src, *m_dst;

  value_type* src_data() { return m_src_data.data(); }
  value_type* dst_data() { return m_dst_data.data(); }

 private:
  const value_type& src(int i, int j, int k) const {
    return m_src_data[zero_offset() + index(i, j, k)];
  }

  const value_type& dst(int i, int j, int k) const {
    return m_dst_data[zero_offset() + index(i, j, k)];
  }

  bool verify(const std::string& kernel) const override {
    auto equal = [](value_type a, value_type b) { return a == b; };
    if (kernel == "copy") {
      return verify_loop([&](int i, int j, int k) {
        return equal(dst(i, j, k), src(i, j, k));
      });
    }
    if (kernel == "copyi") {
      return verify_loop([&](int i, int j, int k) {
        return equal(dst(i, j, k), src(i + 1, j, k));
      });
    }
    if (kernel == "copyj") {
      return verify_loop([&](int i, int j, int k) {
        return equal(dst(i, j, k), src(i, j + 1, k));
      });
    }
    if (kernel == "copyk") {
      return verify_loop([&](int i, int j, int k) {
        return equal(dst(i, j, k), src(i, j, k + 1));
      });
    }
    if (kernel == "avgi") {
      return verify_loop([&](int i, int j, int k) {
        return equal(dst(i, j, k), src(i - 1, j, k) + src(i + 1, j, k));
      });
    }
    if (kernel == "avgj") {
      return verify_loop([&](int i, int j, int k) {
        return equal(dst(i, j, k), src(i, j - 1, k) + src(i, j + 1, k));
      });
    }
    if (kernel == "avgk") {
      return verify_loop([&](int i, int j, int k) {
        return equal(dst(i, j, k), src(i, j, k - 1) + src(i, j, k + 1));
      });
    }
    if (kernel == "sumi") {
      return verify_loop([&](int i, int j, int k) {
        return equal(dst(i, j, k), src(i, j, k) + src(i + 1, j, k));
      });
    }
    if (kernel == "sumj") {
      return verify_loop([&](int i, int j, int k) {
        return equal(dst(i, j, k), src(i, j, k) + src(i, j + 1, k));
      });
    }
    if (kernel == "sumk") {
      return verify_loop([&](int i, int j, int k) {
        return equal(dst(i, j, k), src(i, j, k) + src(i, j, k + 1));
      });
    }
    if (kernel == "lapij") {
      return verify_loop([&](int i, int j, int k) {
        return equal(dst(i, j, k), src(i, j, k) + src(i - 1, j, k) +
                                       src(i + 1, j, k) + src(i, j - 1, k) +
                                       src(i, j + 1, k));
      });
    }
    throw ERROR("unknown stencil '" + kernel + "'");
  }

  std::size_t bytes(const std::string& kernel) const override {
    if (kernel == "copy" || kernel == "copyi" || kernel == "copyj" ||
        kernel == "copyk")
      return sizeof(value_type) * 2 * isize() * jsize() * ksize();
    if (kernel == "avgi")
      return sizeof(value_type) *
             ((isize() + 2) * jsize() * ksize() + isize() * jsize() * ksize());
    if (kernel == "avgj")
      return sizeof(value_type) *
             (isize() * (jsize() + 2) * ksize() + isize() * jsize() * ksize());
    if (kernel == "avgk")
      return sizeof(value_type) *
             (isize() * jsize() * (ksize() + 2) + isize() * jsize() * ksize());
    if (kernel == "sumi")
      return sizeof(value_type) *
             ((isize() + 1) * jsize() * ksize() + isize() * jsize() * ksize());
    if (kernel == "sumj")
      return sizeof(value_type) *
             (isize() * (jsize() + 1) * ksize() + isize() * jsize() * ksize());
    if (kernel == "sumk")
      return sizeof(value_type) *
             (isize() * jsize() * (ksize() + 1) + isize() * jsize() * ksize());
    if (kernel == "lapij")
      return sizeof(value_type) *
             ((isize() + 2) * (jsize() + 2) * (ksize() * 2) +
              isize() * jsize() * ksize());
    throw std::logic_error("Error: unknown stencil '" + kernel + "'");
  }

  std::vector<value_type, allocator> m_src_data, m_dst_data;
};

}  // platform