# Copyright 2021 Tencent
# SPDX-License-Identifier: BSD-3-Clause

import torch
import torch.nn as nn
import torch.nn.functional as F

class Model(nn.Module):
    def __init__(self):
        super(Model, self).__init__()

        self.up_0 = nn.UpsamplingBilinear2d(size=16)
        self.up_1 = nn.UpsamplingBilinear2d(scale_factor=2)
        self.up_2 = nn.UpsamplingBilinear2d(size=(20,20))
        self.up_3 = nn.UpsamplingBilinear2d(scale_factor=(4,4))
        self.up_4 = nn.UpsamplingBilinear2d(size=(16,24))
        self.up_5 = nn.UpsamplingBilinear2d(scale_factor=(2,3))

    def forward(self, x):
        x = self.up_0(x)
        x = self.up_1(x)
        x = self.up_2(x)
        x = self.up_3(x)
        x = self.up_4(x)
        x = self.up_5(x)
        return x

def test():
    net = Model()
    net.eval()

    torch.manual_seed(0)
    x = torch.rand(1, 3, 32, 32)

    a = net(x)

    # export torchscript
    mod = torch.jit.trace(net, x)
    mod.save("test_nn_UpsamplingBilinear2d.pt")

    # torchscript to pnnx
    import os
    os.system("../src/pnnx test_nn_UpsamplingBilinear2d.pt inputshape=[1,3,32,32]")

    # pnnx inference
    import test_nn_UpsamplingBilinear2d_pnnx
    b = test_nn_UpsamplingBilinear2d_pnnx.test_inference()

    return torch.equal(a, b)

if __name__ == "__main__":
    if test():
        exit(0)
    else:
        exit(1)
