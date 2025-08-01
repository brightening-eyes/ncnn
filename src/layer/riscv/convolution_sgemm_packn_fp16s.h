// Copyright 2021 Tencent
// SPDX-License-Identifier: BSD-3-Clause

static void im2col_sgemm_packn_fp16sa_rvv(const Mat& bottom_im2col, Mat& top_blob, const Mat& kernel, const Mat& _bias, const Option& opt)
{
    const int packn = csrr_vlenb() / 2;
    const size_t vl = __riscv_vsetvl_e16m1(packn);

    // Mat bottom_im2col(size, maxk, inch, 2u * packn, packn, opt.workspace_allocator);

    const int size = bottom_im2col.w;
    const int maxk = bottom_im2col.h;
    const int inch = bottom_im2col.c;

    const int outch = top_blob.c;

    const __fp16* bias = _bias;

    // permute
    Mat tmp;
    if (size >= 8)
        tmp.create(8 * maxk, inch, size / 8 + (size % 8) / 4 + (size % 4) / 2 + size % 2, 2u * packn, packn, opt.workspace_allocator);
    else if (size >= 4)
        tmp.create(4 * maxk, inch, size / 4 + (size % 4) / 2 + size % 2, 2u * packn, packn, opt.workspace_allocator);
    else if (size >= 2)
        tmp.create(2 * maxk, inch, size / 2 + size % 2, 2u * packn, packn, opt.workspace_allocator);
    else
        tmp.create(maxk, inch, size, 2u * packn, packn, opt.workspace_allocator);
    {
        int remain_size_start = 0;
        int nn_size = size >> 3;

        #pragma omp parallel for num_threads(opt.num_threads)
        for (int ii = 0; ii < nn_size; ii++)
        {
            int i = remain_size_start + ii * 8;

            __fp16* tmpptr = tmp.channel(i / 8);

            for (int q = 0; q < inch; q++)
            {
                const __fp16* img0 = (const __fp16*)bottom_im2col.channel(q) + i * packn;

                for (int k = 0; k < maxk; k++)
                {
#if C906
#ifdef RVV_SPEC_0_7
                    asm volatile(
                        "mv        t3,     %[LEN]  \n\t"
                        "mv        t1,     %[SRC]  \n\t"
                        "mv        t2,     %[TMP]  \n\t"
                        "slli      t3,     t3,     1       \n\t"
                        "vle.v     v0,     (t1)    \n\t"
                        "add       t1,     t1,     t3      \n\t"
                        "vle.v     v1,     (t1)    \n\t"
                        "add       t1,     t1,     t3      \n\t"
                        "vle.v     v2,     (t1)    \n\t"
                        "add       t1,     t1,     t3      \n\t"
                        "vle.v     v3,     (t1)    \n\t"
                        "add       t1,     t1,     t3      \n\t"
                        "vle.v     v4,     (t1)    \n\t"
                        "add       t1,     t1,     t3      \n\t"
                        "vle.v     v5,     (t1)    \n\t"
                        "add       t1,     t1,     t3      \n\t"
                        "vle.v     v6,     (t1)    \n\t"
                        "add       t1,     t1,     t3      \n\t"
                        "vle.v     v7,     (t1)    \n\t"
                        "add       t1,     t1,     t3      \n\t"
                        "vsseg8e.v v0,     (t2)    \n\t"
                        :
                        : [LEN] "r"(packn), [SRC] "r"(img0), [TMP] "r"(tmpptr)
                        : "cc", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "t1", "t2", "t3");

                    img0 += size * packn;
                    tmpptr += packn * 8;
#else
                    for (int l = 0; l < packn; l++)
                    {
                        tmpptr[0] = img0[l];
                        tmpptr[1] = img0[l + packn];
                        tmpptr[2] = img0[l + packn * 2];
                        tmpptr[3] = img0[l + packn * 3];
                        tmpptr[4] = img0[l + packn * 4];
                        tmpptr[5] = img0[l + packn * 5];
                        tmpptr[6] = img0[l + packn * 6];
                        tmpptr[7] = img0[l + packn * 7];
                        tmpptr += 8;
                    }

                    img0 += size * packn;
#endif
#else
                    vfloat16m1_t _val0 = __riscv_vle16_v_f16m1(img0, vl);
                    vfloat16m1_t _val1 = __riscv_vle16_v_f16m1(img0 + packn, vl);
                    vfloat16m1_t _val2 = __riscv_vle16_v_f16m1(img0 + packn * 2, vl);
                    vfloat16m1_t _val3 = __riscv_vle16_v_f16m1(img0 + packn * 3, vl);
                    vfloat16m1_t _val4 = __riscv_vle16_v_f16m1(img0 + packn * 4, vl);
                    vfloat16m1_t _val5 = __riscv_vle16_v_f16m1(img0 + packn * 5, vl);
                    vfloat16m1_t _val6 = __riscv_vle16_v_f16m1(img0 + packn * 6, vl);
                    vfloat16m1_t _val7 = __riscv_vle16_v_f16m1(img0 + packn * 7, vl);
                    __riscv_vsseg8e16_v_f16m1x8(tmpptr, __riscv_vcreate_v_f16m1x8(_val0, _val1, _val2, _val3, _val4, _val5, _val6, _val7), vl);

                    img0 += size * packn;
                    tmpptr += packn * 8;
#endif
                }
            }
        }

        remain_size_start += nn_size << 3;
        nn_size = (size - remain_size_start) >> 2;

        #pragma omp parallel for num_threads(opt.num_threads)
        for (int ii = 0; ii < nn_size; ii++)
        {
            int i = remain_size_start + ii * 4;

            __fp16* tmpptr = tmp.channel(i / 8 + (i % 8) / 4);

            for (int q = 0; q < inch; q++)
            {
                const __fp16* img0 = (const __fp16*)bottom_im2col.channel(q) + i * packn;

                for (int k = 0; k < maxk; k++)
                {
#if C906
#ifdef RVV_SPEC_0_7
                    asm volatile(
                        "mv        t3,     %[LEN]  \n\t"
                        "mv        t1,     %[SRC]  \n\t"
                        "mv        t2,     %[TMP]  \n\t"
                        "slli      t3,     t3,     1       \n\t"
                        "vle.v     v0,     (t1)    \n\t"
                        "add       t1,     t1,     t3      \n\t"
                        "vle.v     v1,     (t1)    \n\t"
                        "add       t1,     t1,     t3      \n\t"
                        "vle.v     v2,     (t1)    \n\t"
                        "add       t1,     t1,     t3      \n\t"
                        "vle.v     v3,     (t1)    \n\t"
                        "vsseg4e.v v0,     (t2)    \n\t"
                        :
                        : [LEN] "r"(packn), [SRC] "r"(img0), [TMP] "r"(tmpptr)
                        : "cc", "memory", "v0", "v1", "v2", "v3", "t1", "t2", "t3");

                    img0 += size * packn;
                    tmpptr += packn * 4;
#else
                    for (int l = 0; l < packn; l++)
                    {
                        tmpptr[0] = img0[l];
                        tmpptr[1] = img0[l + packn];
                        tmpptr[2] = img0[l + packn * 2];
                        tmpptr[3] = img0[l + packn * 3];
                        tmpptr += 4;
                    }

                    img0 += size * packn;
#endif
#else
                    vfloat16m1_t _val0 = __riscv_vle16_v_f16m1(img0, vl);
                    vfloat16m1_t _val1 = __riscv_vle16_v_f16m1(img0 + packn, vl);
                    vfloat16m1_t _val2 = __riscv_vle16_v_f16m1(img0 + packn * 2, vl);
                    vfloat16m1_t _val3 = __riscv_vle16_v_f16m1(img0 + packn * 3, vl);
                    __riscv_vsseg4e16_v_f16m1x4(tmpptr, __riscv_vcreate_v_f16m1x4(_val0, _val1, _val2, _val3), vl);

                    img0 += size * packn;
                    tmpptr += packn * 4;
#endif
                }
            }
        }

        remain_size_start += nn_size << 2;

        nn_size = (size - remain_size_start) >> 1;

        #pragma omp parallel for num_threads(opt.num_threads)
        for (int ii = 0; ii < nn_size; ii++)
        {
            int i = remain_size_start + ii * 2;

            __fp16* tmpptr = tmp.channel(i / 8 + (i % 8) / 4 + (i % 4) / 2);

            for (int q = 0; q < inch; q++)
            {
                const __fp16* img0 = (const __fp16*)bottom_im2col.channel(q) + i * packn;

                for (int k = 0; k < maxk; k++)
                {
#if C906
#ifdef RVV_SPEC_0_7
                    asm volatile(
                        "mv        t3,     %[LEN]  \n\t"
                        "mv        t1,     %[SRC]  \n\t"
                        "mv        t2,     %[TMP]  \n\t"
                        "slli      t3,     t3,     1       \n\t"
                        "vle.v     v0,     (t1)    \n\t"
                        "add       t1,     t1,     t3      \n\t"
                        "vle.v     v1,     (t1)    \n\t"
                        "add       t1,     t1,     t3      \n\t"
                        :
                        : [LEN] "r"(packn), [SRC] "r"(img0), [TMP] "r"(tmpptr)
                        : "cc", "memory", "v0", "v1", "t1", "t2", "t3");

                    img0 += size * packn;
                    tmpptr += packn * 2;
#else
                    for (int l = 0; l < packn; l++)
                    {
                        tmpptr[0] = img0[l];
                        tmpptr[1] = img0[l + packn];
                        tmpptr += 2;
                    }

                    img0 += size * packn;
#endif
#else
                    vfloat16m1_t _val0 = __riscv_vle16_v_f16m1(img0, vl);
                    vfloat16m1_t _val1 = __riscv_vle16_v_f16m1(img0 + packn, vl);
                    __riscv_vsseg2e16_v_f16m1x2(tmpptr, __riscv_vcreate_v_f16m1x2(_val0, _val1), vl);

                    img0 += size * packn;
                    tmpptr += packn * 2;
#endif
                }
            }
        }

        remain_size_start += nn_size << 1;

        #pragma omp parallel for num_threads(opt.num_threads)
        for (int i = remain_size_start; i < size; i++)
        {
            __fp16* tmpptr = tmp.channel(i / 8 + (i % 8) / 4 + (i % 4) / 2 + i % 2);

            for (int q = 0; q < inch; q++)
            {
                const __fp16* img0 = (const __fp16*)bottom_im2col.channel(q) + i * packn;

                for (int k = 0; k < maxk; k++)
                {
                    vfloat16m1_t _val = __riscv_vle16_v_f16m1(img0, vl);
                    __riscv_vse16_v_f16m1(tmpptr, _val, vl);

                    img0 += size * packn;
                    tmpptr += packn;
                }
            }
        }
    }

    #pragma omp parallel for num_threads(opt.num_threads)
    for (int p = 0; p < outch; p++)
    {
        __fp16* outptr0 = top_blob.channel(p);

        int i = 0;
        for (; i + 7 < size; i += 8)
        {
            const __fp16* tmpptr = tmp.channel(i / 8);
            const __fp16* kptr0 = kernel.channel(p);

            int nn = inch * maxk * packn; // inch always > 0

            vfloat16m1_t _sum0 = __riscv_vfmv_v_f_f16m1(0.f, vl);
            vfloat16m1_t _sum1 = __riscv_vfmv_v_f_f16m1(0.f, vl);
            vfloat16m1_t _sum2 = __riscv_vfmv_v_f_f16m1(0.f, vl);
            vfloat16m1_t _sum3 = __riscv_vfmv_v_f_f16m1(0.f, vl);
            vfloat16m1_t _sum4 = __riscv_vfmv_v_f_f16m1(0.f, vl);
            vfloat16m1_t _sum5 = __riscv_vfmv_v_f_f16m1(0.f, vl);
            vfloat16m1_t _sum6 = __riscv_vfmv_v_f_f16m1(0.f, vl);
            vfloat16m1_t _sum7 = __riscv_vfmv_v_f_f16m1(0.f, vl);

            if (bias)
            {
                _sum0 = __riscv_vle16_v_f16m1(bias + p * packn, vl);
                _sum1 = __riscv_vle16_v_f16m1(bias + p * packn, vl);
                _sum2 = __riscv_vle16_v_f16m1(bias + p * packn, vl);
                _sum3 = __riscv_vle16_v_f16m1(bias + p * packn, vl);
                _sum4 = __riscv_vle16_v_f16m1(bias + p * packn, vl);
                _sum5 = __riscv_vle16_v_f16m1(bias + p * packn, vl);
                _sum6 = __riscv_vle16_v_f16m1(bias + p * packn, vl);
                _sum7 = __riscv_vle16_v_f16m1(bias + p * packn, vl);
            }

            for (int j = 0; j < nn; j++)
            {
                __fp16 val0 = *tmpptr++;
                __fp16 val1 = *tmpptr++;
                __fp16 val2 = *tmpptr++;
                __fp16 val3 = *tmpptr++;
                __fp16 val4 = *tmpptr++;
                __fp16 val5 = *tmpptr++;
                __fp16 val6 = *tmpptr++;
                __fp16 val7 = *tmpptr++;
                vfloat16m1_t _w0 = __riscv_vle16_v_f16m1(kptr0, vl);
                _sum0 = __riscv_vfmacc_vf_f16m1(_sum0, val0, _w0, vl);
                _sum1 = __riscv_vfmacc_vf_f16m1(_sum1, val1, _w0, vl);
                _sum2 = __riscv_vfmacc_vf_f16m1(_sum2, val2, _w0, vl);
                _sum3 = __riscv_vfmacc_vf_f16m1(_sum3, val3, _w0, vl);
                _sum4 = __riscv_vfmacc_vf_f16m1(_sum4, val4, _w0, vl);
                _sum5 = __riscv_vfmacc_vf_f16m1(_sum5, val5, _w0, vl);
                _sum6 = __riscv_vfmacc_vf_f16m1(_sum6, val6, _w0, vl);
                _sum7 = __riscv_vfmacc_vf_f16m1(_sum7, val7, _w0, vl);

                kptr0 += packn;
            }

            __riscv_vse16_v_f16m1(outptr0, _sum0, vl);
            __riscv_vse16_v_f16m1(outptr0 + packn, _sum1, vl);
            __riscv_vse16_v_f16m1(outptr0 + packn * 2, _sum2, vl);
            __riscv_vse16_v_f16m1(outptr0 + packn * 3, _sum3, vl);
            __riscv_vse16_v_f16m1(outptr0 + packn * 4, _sum4, vl);
            __riscv_vse16_v_f16m1(outptr0 + packn * 5, _sum5, vl);
            __riscv_vse16_v_f16m1(outptr0 + packn * 6, _sum6, vl);
            __riscv_vse16_v_f16m1(outptr0 + packn * 7, _sum7, vl);

            outptr0 += packn * 8;
        }
        for (; i + 3 < size; i += 4)
        {
            const __fp16* tmpptr = tmp.channel(i / 8 + (i % 8) / 4);
            const __fp16* kptr0 = kernel.channel(p);

            int nn = inch * maxk * packn; // inch always > 0

            vfloat16m1_t _sum0 = __riscv_vfmv_v_f_f16m1(0.f, vl);
            vfloat16m1_t _sum1 = __riscv_vfmv_v_f_f16m1(0.f, vl);
            vfloat16m1_t _sum2 = __riscv_vfmv_v_f_f16m1(0.f, vl);
            vfloat16m1_t _sum3 = __riscv_vfmv_v_f_f16m1(0.f, vl);

            if (bias)
            {
                _sum0 = __riscv_vle16_v_f16m1(bias + p * packn, vl);
                _sum1 = __riscv_vle16_v_f16m1(bias + p * packn, vl);
                _sum2 = __riscv_vle16_v_f16m1(bias + p * packn, vl);
                _sum3 = __riscv_vle16_v_f16m1(bias + p * packn, vl);
            }

            for (int j = 0; j < nn; j++)
            {
                __fp16 val0 = *tmpptr++;
                __fp16 val1 = *tmpptr++;
                __fp16 val2 = *tmpptr++;
                __fp16 val3 = *tmpptr++;
                vfloat16m1_t _w0 = __riscv_vle16_v_f16m1(kptr0, vl);
                _sum0 = __riscv_vfmacc_vf_f16m1(_sum0, val0, _w0, vl);
                _sum1 = __riscv_vfmacc_vf_f16m1(_sum1, val1, _w0, vl);
                _sum2 = __riscv_vfmacc_vf_f16m1(_sum2, val2, _w0, vl);
                _sum3 = __riscv_vfmacc_vf_f16m1(_sum3, val3, _w0, vl);

                kptr0 += packn;
            }

            __riscv_vse16_v_f16m1(outptr0, _sum0, vl);
            __riscv_vse16_v_f16m1(outptr0 + packn, _sum1, vl);
            __riscv_vse16_v_f16m1(outptr0 + packn * 2, _sum2, vl);
            __riscv_vse16_v_f16m1(outptr0 + packn * 3, _sum3, vl);

            outptr0 += packn * 4;
        }
        for (; i + 1 < size; i += 2)
        {
            const __fp16* tmpptr = tmp.channel(i / 8 + (i % 8) / 4 + (i % 4) / 2);
            const __fp16* kptr0 = kernel.channel(p);

            int nn = inch * maxk * packn; // inch always > 0

            vfloat16m1_t _sum0 = __riscv_vfmv_v_f_f16m1(0.f, vl);
            vfloat16m1_t _sum1 = __riscv_vfmv_v_f_f16m1(0.f, vl);

            if (bias)
            {
                _sum0 = __riscv_vle16_v_f16m1(bias + p * packn, vl);
                _sum1 = __riscv_vle16_v_f16m1(bias + p * packn, vl);
            }

            for (int j = 0; j < nn; j++)
            {
                __fp16 val0 = *tmpptr++;
                __fp16 val1 = *tmpptr++;
                vfloat16m1_t _w0 = __riscv_vle16_v_f16m1(kptr0, vl);
                _sum0 = __riscv_vfmacc_vf_f16m1(_sum0, val0, _w0, vl);
                _sum1 = __riscv_vfmacc_vf_f16m1(_sum1, val1, _w0, vl);

                kptr0 += packn;
            }

            __riscv_vse16_v_f16m1(outptr0, _sum0, vl);
            __riscv_vse16_v_f16m1(outptr0 + packn, _sum1, vl);

            outptr0 += packn * 2;
        }
        for (; i < size; i++)
        {
            const __fp16* tmpptr = tmp.channel(i / 8 + (i % 8) / 4 + (i % 4) / 2 + i % 2);
            const __fp16* kptr0 = kernel.channel(p);

            int nn = inch * maxk * packn; // inch always > 0

            vfloat16m1_t _sum = __riscv_vfmv_v_f_f16m1(0.f, vl);

            if (bias)
            {
                _sum = __riscv_vle16_v_f16m1(bias + p * packn, vl);
            }

            for (int j = 0; j < nn; j++)
            {
                __fp16 val = *tmpptr++;
                vfloat16m1_t _w0 = __riscv_vle16_v_f16m1(kptr0, vl);
                _sum = __riscv_vfmacc_vf_f16m1(_sum, val, _w0, vl);

                kptr0 += packn;
            }

            __riscv_vse16_v_f16m1(outptr0, _sum, vl);

            outptr0 += packn;
        }
    }
}

static void convolution_im2col_sgemm_packn_fp16sa_rvv(const Mat& bottom_blob, Mat& top_blob, const Mat& kernel, const Mat& _bias, int kernel_w, int kernel_h, int dilation_w, int dilation_h, int stride_w, int stride_h, const Option& opt)
{
    const int packn = csrr_vlenb() / 2;
    const size_t vl = __riscv_vsetvl_e16m1(packn);

    int w = bottom_blob.w;
    int inch = bottom_blob.c;

    int outw = top_blob.w;
    int outh = top_blob.h;
    const int size = outw * outh;

    const int maxk = kernel_w * kernel_h;

    // im2col
    Mat bottom_im2col(size, maxk, inch, 2u * packn, packn, opt.workspace_allocator);
    {
        const int gap = (w * stride_h - outw * stride_w) * packn;

        #pragma omp parallel for num_threads(opt.num_threads)
        for (int p = 0; p < inch; p++)
        {
            const Mat img = bottom_blob.channel(p);
            __fp16* ptr = bottom_im2col.channel(p);

            for (int u = 0; u < kernel_h; u++)
            {
                for (int v = 0; v < kernel_w; v++)
                {
                    const __fp16* sptr = img.row<const __fp16>(dilation_h * u) + dilation_w * v * packn;

                    for (int i = 0; i < outh; i++)
                    {
                        int j = 0;
                        for (; j < outw; j++)
                        {
                            vfloat16m1_t _val = __riscv_vle16_v_f16m1(sptr, vl);
                            __riscv_vse16_v_f16m1(ptr, _val, vl);

                            sptr += stride_w * packn;
                            ptr += packn;
                        }

                        sptr += gap;
                    }
                }
            }
        }
    }

    im2col_sgemm_packn_fp16sa_rvv(bottom_im2col, top_blob, kernel, _bias, opt);
}
