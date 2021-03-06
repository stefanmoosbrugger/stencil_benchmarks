#pragma once

#include "x86/x86_hdiff_stencil_variant.h"

namespace platform {

    namespace x86 {

        template <class Platform, class ValueType>
        class x86_hdiff_variant_ij_blocked final : public x86_hdiff_stencil_variant<Platform, ValueType> {
          public:
            using value_type = ValueType;

            x86_hdiff_variant_ij_blocked(const arguments_map &args)
                : x86_hdiff_stencil_variant<Platform, ValueType>(args), m_iblocksize(args.get<int>("i-blocksize")),
                  m_jblocksize(args.get<int>("j-blocksize")) {
                if (m_iblocksize <= 0 || m_jblocksize <= 0)
                    throw ERROR("invalid block size");
            }

            void hdiff() override {

                const value_type *__restrict__ in = this->in();
                const value_type *__restrict__ coeff = this->coeff();
                value_type *__restrict__ lap = this->lap();
                value_type *__restrict__ flx = this->flx();
                value_type *__restrict__ fly = this->fly();
                value_type *__restrict__ out = this->out();

                constexpr int istride = 1;
                const int jstride = this->jstride();
                const int kstride = this->kstride();
                const int h = this->halo();
                const int isize = this->isize();
                const int jsize = this->jsize();
                const int ksize = this->ksize();

                if (this->istride() != 1)
                    throw ERROR("this variant is only compatible with unit i-stride layout");
                if (this->halo() < 2)
                    throw ERROR("Minimum required halo is 2");

#pragma omp parallel for collapse(2)
                for (int jb = 0; jb < jsize; jb += m_jblocksize) {
                    for (int ib = 0; ib < isize; ib += m_iblocksize) {
                        const int imax = ib + m_iblocksize <= isize ? ib + m_iblocksize : isize;
                        const int jmax = jb + m_jblocksize <= jsize ? jb + m_jblocksize : jsize;
                        int index_lap = (ib - 1) * istride + (jb - 1) * jstride;
                        int index_flx = ib * istride + jb * jstride - istride;
                        int index_fly = ib * istride + jb * jstride - jstride;

                        for (int k = 0; k < ksize; ++k) {
                            for (int j = jb; j < jmax + 2; ++j) {
                                for (int i = ib; i < imax + 2; ++i) {
                                    lap[index_lap] =
                                        4 * in[index_lap] - (in[index_lap - istride] + in[index_lap + istride] +
                                                                in[index_lap - jstride] + in[index_lap + jstride]);
                                    index_lap += istride;
                                }
                                index_lap += jstride - (imax + 2 - ib) * istride;
                            }

                            for (int j = jb; j < jmax; ++j) {
                                for (int i = ib; i < imax + 1; ++i) {
                                    flx[index_flx] = lap[index_flx + istride] - lap[index_flx];
                                    if (flx[index_flx] * (in[index_flx + istride] - in[index_flx]) > 0)
                                        flx[index_flx] = 0.;
                                    index_flx += istride;
                                }
                                index_flx += jstride - (imax + 1 - ib) * istride;
                            }

                            for (int j = jb; j < jmax + 1; ++j) {
                                for (int i = ib; i < imax; ++i) {
                                    fly[index_fly] = lap[index_fly + jstride] - lap[index_fly];
                                    if (fly[index_fly] * (in[index_fly + jstride] - in[index_fly]) > 0)
                                        fly[index_fly] = 0.;
                                    index_fly += istride;
                                }
                                index_fly += jstride - (imax - ib) * istride;
                            }

                            index_lap += kstride - (jmax + 2 - jb) * jstride;
                            index_flx += kstride - (jmax - jb) * jstride;
                            index_fly += kstride - (jmax + 1 - jb) * jstride;
                        }
                    }
                }

#pragma omp parallel for collapse(2)
                for (int jb = 0; jb < jsize; jb += m_jblocksize) {
                    for (int ib = 0; ib < isize; ib += m_iblocksize) {
                        const int imax = ib + m_iblocksize <= isize ? ib + m_iblocksize : isize;
                        const int jmax = jb + m_jblocksize <= jsize ? jb + m_jblocksize : jsize;

                        int index_out = ib * istride + jb * jstride;
                        for (int k = 0; k < ksize; ++k) {
                            for (int j = jb; j < jmax; ++j) {
                                for (int i = ib; i < imax; ++i) {
                                    out[index_out] = in[index_out] -
                                                     coeff[index_out] * (flx[index_out] - flx[index_out - istride] +
                                                                            fly[index_out] - fly[index_out - jstride]);
                                    index_out += istride;
                                }
                                index_out += jstride - (imax - ib) * istride;
                            }
                            index_out += kstride - (jmax - jb) * jstride;
                        }
                    }
                }
            }

          private:
            int m_iblocksize, m_jblocksize;
        };

    } // namespace x86

} // namespace platform
