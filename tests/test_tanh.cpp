// Copyright 2020 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#include "testutil.h"

static int test_tanh(const ncnn::Mat& a)
{
    ncnn::ParamDict pd;

    std::vector<ncnn::Mat> weights(0);

    int ret = test_layer("TanH", pd, weights, a);
    if (ret != 0)
    {
        fprintf(stderr, "test_tanh failed a.dims=%d a=(%d %d %d %d)\n", a.dims, a.w, a.h, a.d, a.c);
    }

    return ret;
}

static int test_tanh_0()
{
    return 0
           || test_tanh(RandomMat(5, 6, 7, 24))
           || test_tanh(RandomMat(7, 8, 9, 12))
           || test_tanh(RandomMat(3, 4, 5, 13));
}

static int test_tanh_1()
{
    return 0
           || test_tanh(RandomMat(5, 7, 24))
           || test_tanh(RandomMat(7, 9, 12))
           || test_tanh(RandomMat(3, 5, 13));
}

static int test_tanh_2()
{
    return 0
           || test_tanh(RandomMat(15, 24))
           || test_tanh(RandomMat(17, 12))
           || test_tanh(RandomMat(19, 15));
}

static int test_tanh_3()
{
    return 0
           || test_tanh(RandomMat(128))
           || test_tanh(RandomMat(124))
           || test_tanh(RandomMat(127));
}

int main()
{
    SRAND(7767517);

    return 0
           || test_tanh_0()
           || test_tanh_1()
           || test_tanh_2()
           || test_tanh_3();
}
