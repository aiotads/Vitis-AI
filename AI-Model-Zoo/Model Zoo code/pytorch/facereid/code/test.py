# Copyright 2019 Xilinx Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import argparse
import logging
import os
import sys

import torch
from torch import nn

import network
from core.config import opt, update_config
from core.loader import get_data_provider
from core.solver import Solver
from ipdb import set_trace

FORMAT = '[%(levelname)s]: %(message)s'
logging.basicConfig(
    level=logging.INFO,
    format=FORMAT,
    stream=sys.stdout
)


def test(args):
    logging.info('======= args ======')
    logging.info(print(args))
    logging.info('======= end ======')

    _, test_data, num_query, num_class = get_data_provider(opt, args.dataset, args.dataset_root)

    net = getattr(network, opt.network.name)(num_class, opt.network.last_stride)
    if args.load_model[-3:] == 'tar':
        checkpoint = torch.load(args.load_model)['state_dict']
        for i in checkpoint:
            if 'classifier' in i or 'fc' in i:
                continue
            net.state_dict()[i].copy_(checkpoint[i])
    else:
        net = net.load_state_dict(torch.load(args.load_model))
    logging.info('Load model checkpoint: {}'.format(args.load_model))
    if opt.device.type == 'cuda':
        net = nn.DataParallel(net)
    net = net.to(opt.device)
    net.eval()

    mod = Solver(opt, net)
    mod.test_func(test_data, num_query)


def main():
    parser = argparse.ArgumentParser(description='reid model testing')
    parser.add_argument('--dataset', type=str, default = 'facereid', 
                       help = 'set the dataset for test')
    parser.add_argument('--dataset_root', type=str, default = '../data/face_reid', 
                       help = 'dataset path')
    parser.add_argument('--config_file', type=str, required=True,
                        help='Optional config file for params')
    parser.add_argument('--load_model', type=str, required=True, 
                        help='load trained model for testing')
    parser.add_argument('--device', type=str, default='gpu', choices=['gpu','cpu'],
                        help='set running device')
    parser.add_argument('--gpu', type=str, default='0', 
                        help='set visible cuda device')

    args = parser.parse_args()
    update_config(args.config_file)

    if args.device=='gpu':
        os.environ["CUDA_VISIBLE_DEVICES"] = args.gpu
        opt.device = torch.device('cuda:{}'.format(args.gpu))
    else:
        opt.device = torch.device('cpu')
    test(args)


if __name__ == '__main__':
    main()