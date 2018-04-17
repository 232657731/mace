// Copyright 2018 Xiaomi, Inc.  All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mace/kernels/softmax.h"

namespace mace {
namespace kernels {

void SoftmaxFunctor<DeviceType::NEON, float>::operator()(const Tensor *input,
                                                         Tensor *output,
                                                         StatsFuture *future) {
  const index_t batch = input->dim(0);
  const index_t class_count = input->dim(1);
  const index_t class_size = input->dim(2) * input->dim(3);

  const float *input_data = input->data<float>();
  float *output_data = output->mutable_data<float>();

  for (index_t b = 0; b < batch; ++b) {
    std::vector<float>
      max_val(class_size, std::numeric_limits<float>::lowest());
    std::vector<float> sum_val(class_size, 0.f);

    // calculate max for each class
    for (index_t c = 0; c < class_count; ++c) {
      const float *input_ptr = input_data + (b * class_count + c) * class_size;
      for (index_t k = 0; k < class_size; ++k) {
        max_val[k] = std::max(max_val[k], input_ptr[k]);
      }
    }

    // calculate data - max for each class
#pragma omp parallel for
    for (index_t c = 0; c < class_count; ++c) {
      const float *input_ptr = input_data + (b * class_count + c) * class_size;
      float *output_ptr = output_data + (b * class_count + c) * class_size;
      for (index_t k = 0; k < class_size; ++k) {
        output_ptr[k] = ::exp(input_ptr[k] - max_val[k]);
      }
    }

    // calculate sum for each class
    for (index_t c = 0; c < class_count; ++c) {
      float *output_ptr = output_data + (b * class_count + c) * class_size;
      for (index_t k = 0; k < class_size; ++k) {
        sum_val[k] += output_ptr[k];
      }
    }

    // calculate (data - max) / sum for each class
    for (index_t c = 0; c < class_count; ++c) {
      float *output_ptr = output_data + (b * class_count + c) * class_size;
      for (index_t k = 0; k < class_size; ++k) {
        output_ptr[k] /= sum_val[k];
      }
    }
  }
}

}  // namespace kernels
}  // namespace mace