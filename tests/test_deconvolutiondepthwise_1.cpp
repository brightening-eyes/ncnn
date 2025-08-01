// Copyright 2019 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#include "testutil.h"

static int test_deconvolutiondepthwise_dynamic(int w, int h, int c, int outch, int kernel, int dilation, int stride, int pad, int bias, int group, int output_pad_right, int output_pad_bottom, int output_w, int output_h)
{
    ncnn::Mat a = RandomMat(w, h, c);

    if (output_w > 0 && output_h > 0 && pad != -233 && pad != -234)
    {
        pad = -233;
    }

    ncnn::ParamDict pd;
    pd.set(0, 0);
    pd.set(1, 0);
    pd.set(2, dilation);
    pd.set(3, stride);
    pd.set(4, pad);
    pd.set(5, bias);
    pd.set(6, 0);
    pd.set(7, group);
    pd.set(28, 1); // dynamic weight

    int activation_type = RAND() % 5; // 0 1 2 3 4
    ncnn::Mat activation_params(2);
    activation_params[0] = RandomFloat(-1, 0); // alpha
    activation_params[1] = RandomFloat(0, 1);  // beta
    pd.set(9, activation_type);
    pd.set(10, activation_params);

    pd.set(18, output_pad_right);
    pd.set(19, output_pad_bottom);
    pd.set(20, output_w);
    pd.set(21, output_h);

    std::vector<ncnn::Mat> as(bias ? 3 : 2);
    as[0] = a;
    as[1] = RandomMat(kernel, kernel, outch / group, c);
    if (bias)
        as[2] = RandomMat(outch);

    std::vector<ncnn::Mat> weights(0);

    int ret = test_layer("DeconvolutionDepthWise", pd, weights, as);
    if (ret != 0)
    {
        fprintf(stderr, "test_deconvolutiondepthwise_dynamic failed w=%d h=%d c=%d outch=%d kernel=%d dilation=%d stride=%d pad=%d bias=%d group=%d act=%d actparams=[%f,%f] output_pad_right=%d output_pad_bottom=%d output_w=%d output_h=%d\n", w, h, c, outch, kernel, dilation, stride, pad, bias, group, activation_type, activation_params[0], activation_params[1], output_pad_right, output_pad_bottom, output_w, output_h);
    }

    return ret;
}

static int test_deconvolutiondepthwise_0()
{
    static const int kdsp[16][4] = {
        {1, 1, 1, 0},
        {1, 1, 2, 0},
        {2, 1, 1, 1},
        {2, 1, 2, -233},
        {3, 1, 1, 1},
        {3, 1, 2, 1},
        {3, 2, 1, 1},
        {4, 1, 1, -233},
        {4, 1, 2, -234},
        {4, 2, 1, -234},
        {5, 1, 1, 2},
        {5, 1, 2, 2},
        {5, 2, 2, 2},
        {7, 1, 1, 3},
        {7, 1, 2, 3},
        {7, 2, 1, -233},
    };

    for (int i = 0; i < 16; i++)
    {
        const int k = kdsp[i][0];
        const int d = kdsp[i][1];
        const int s = kdsp[i][2];
        const int p = kdsp[i][3];

        int ret = 0
                  || test_deconvolutiondepthwise_dynamic(15, 7, 1, 1, k, d, s, p, 1, 1, 0, 0, 0, 0)
                  || test_deconvolutiondepthwise_dynamic(15, 7, 2, 2, k, d, s, p, 0, 1, 1, 1, 7, 5)
                  || test_deconvolutiondepthwise_dynamic(15, 7, 2, 2, k, d, s, p, 1, 2, 1, 0, 0, 0)
                  || test_deconvolutiondepthwise_dynamic(15, 7, 3, 3, k, d, s, p, 0, 3, 0, 1, 0, 0)
                  || test_deconvolutiondepthwise_dynamic(15, 7, 4, 2, k, d, s, p, 1, 2, 0, 0, 7, 5)
                  || test_deconvolutiondepthwise_dynamic(15, 7, 4, 4, k, d, s, p, 0, 4, 2, 2, 0, 0)
                  || test_deconvolutiondepthwise_dynamic(15, 7, 7, 7, k, d, s, p, 1, 7, 2, 0, 0, 0)
                  || test_deconvolutiondepthwise_dynamic(15, 7, 8, 8, k, d, s, p, 0, 2, 0, 2, 7, 5)
                  || test_deconvolutiondepthwise_dynamic(15, 7, 8, 8, k, d, s, p, 1, 8, 0, 0, 0, 0)
                  || test_deconvolutiondepthwise_dynamic(15, 7, 12, 12, k, d, s, p, 0, 4, 3, 3, 0, 0)
                  || test_deconvolutiondepthwise_dynamic(15, 7, 15, 15, k, d, s, p, 1, 15, 3, 0, 7, 5)
                  || test_deconvolutiondepthwise_dynamic(15, 7, 16, 8, k, d, s, p, 0, 2, 0, 3, 0, 0)
                  || test_deconvolutiondepthwise_dynamic(15, 7, 16, 16, k, d, s, p, 1, 16, 0, 0, 0, 0);

        if (ret != 0)
            return -1;
    }

    return 0;
}

int main()
{
    SRAND(7767517);

    return test_deconvolutiondepthwise_0();
}
