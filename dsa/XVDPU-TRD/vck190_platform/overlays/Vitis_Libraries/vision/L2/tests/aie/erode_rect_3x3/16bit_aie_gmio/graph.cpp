/*
 * Copyright 2021 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// 67d7842dbbe25473c3c32b93c0da8047785f30d78e8a024de1b57352245f9689

#include "graph.h"

GMIO gmioIn[1] = {GMIO("gmioIn1", 256, 1000)};
GMIO gmioOut[1] = {GMIO("gmioOut1", 256, 1000)};

// connect dataflow graph to simulation platform
simulation::platform<1, 1> platform(&gmioIn[0], &gmioOut[0]);

// instantiate adf dataflow graph
erodeGraph ec;

connect<> net0(platform.src[0], ec.in1);
connect<> net1(ec.out, platform.sink[0]);

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)
int main(int argc, char** argv) {
    int BLOCK_SIZE_in_Bytes = TILE_WINDOW_SIZE;

    int16_t* inputData = (int16_t*)GMIO::malloc(BLOCK_SIZE_in_Bytes);
    int16_t* outputData = (int16_t*)GMIO::malloc(BLOCK_SIZE_in_Bytes);

    for (int i = 0; i < SMARTTILE_ELEMENTS; i++) inputData[i] = 0;
    inputData[0] = TILE_WIDTH;
    inputData[4] = TILE_HEIGHT;
    for (int i = SMARTTILE_ELEMENTS; i < (BLOCK_SIZE_in_Bytes / sizeof(int16_t)); i++) {
        inputData[i] = rand() % 256;
    }

    ec.init();
    ec.run(1);

    gmioIn[0].gm2aie_nb(inputData, BLOCK_SIZE_in_Bytes);
    gmioOut[0].aie2gm_nb(outputData, BLOCK_SIZE_in_Bytes);
    gmioOut[0].wait();

    // compare the results
    int window[9];
    int acceptableError = 1;
    int errCount = 0;
    for (int i = 0; i < TILE_HEIGHT * TILE_WIDTH; i++) {
        int row = i / TILE_WIDTH;
        int col = i % TILE_WIDTH;
        for (int j = -1; j <= 1; j++) {
            for (int k = -1; k <= 1; k++) {
                int r = std::max(row + j, 0);
                int c = std::max(col + k, 0);
                r = std::min(r, TILE_HEIGHT - 1);
                c = std::min(c, TILE_WIDTH - 1);
                window[(j + 1) * 3 + (k + 1)] = inputData[r * TILE_WIDTH + c + SMARTTILE_ELEMENTS];
            }
        }
        int cValue = 256;
        for (int j = 0; j < 9; j++) {
            cValue = std::min(window[j], cValue);
        }
        if (abs(cValue - outputData[i + SMARTTILE_ELEMENTS]) > acceptableError) {
            errCount++;
        }
    }
    if (errCount) {
        std::cout << "Test failed!" << std::endl;
        exit(-1);
    }
    std::cout << "Test passed!" << std::endl;

    ec.end();
    return 0;
}
#endif
