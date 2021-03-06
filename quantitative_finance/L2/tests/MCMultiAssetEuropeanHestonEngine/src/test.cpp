/*
 * Copyright 2019 Xilinx, Inc.
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
#include <iostream>
#include "utils.hpp"
#ifndef HLS_TEST
#include "xcl2.hpp"
#endif
#define KN 1

#include <math.h>
#include "kernel_mceuropeanengine.hpp"

#define XCL_BANK(n) (((unsigned int)(n)) | XCL_MEM_TOPOLOGY)

#define XCL_BANK0 XCL_BANK(0)
#define XCL_BANK1 XCL_BANK(1)
#define XCL_BANK2 XCL_BANK(2)
#define XCL_BANK3 XCL_BANK(3)
#define XCL_BANK4 XCL_BANK(4)
#define XCL_BANK5 XCL_BANK(5)
#define XCL_BANK6 XCL_BANK(6)
#define XCL_BANK7 XCL_BANK(7)
#define XCL_BANK8 XCL_BANK(8)
#define XCL_BANK9 XCL_BANK(9)
#define XCL_BANK10 XCL_BANK(10)
#define XCL_BANK11 XCL_BANK(11)
#define XCL_BANK12 XCL_BANK(12)
#define XCL_BANK13 XCL_BANK(13)
#define XCL_BANK14 XCL_BANK(14)
#define XCL_BANK15 XCL_BANK(15)
class ArgParser {
   public:
    ArgParser(int& argc, const char** argv) {
        for (int i = 1; i < argc; ++i) mTokens.push_back(std::string(argv[i]));
    }
    bool getCmdOption(const std::string option, std::string& value) const {
        std::vector<std::string>::const_iterator itr;
        itr = std::find(this->mTokens.begin(), this->mTokens.end(), option);
        if (itr != this->mTokens.end() && ++itr != this->mTokens.end()) {
            value = *itr;
            return true;
        }
        return false;
    }

   private:
    std::vector<std::string> mTokens;
};
bool print_result(double* out1, double golden, double tol, int loop_nm) {
    bool passed = true;
    for (int i = 0; i < loop_nm; ++i) {
        std::cout << "loop_nm: " << loop_nm << ", Expected value: " << golden << ::std::endl;
        std::cout << "FPGA result: " << out1[0] << std::endl;
        if (std::fabs(out1[i] - golden) > tol) {
            passed = false;
        }
    }
    return passed;
}

int main(int argc, const char* argv[]) {
    std::cout << "\n----------------------MC(European) Engine-----------------\n";
    // cmd parser
    ArgParser parser(argc, argv);
    std::string xclbin_path;

    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR:xclbin path is not set!\n";
        return 1;
    }

    struct timeval st_time, end_time;
    DtUsed* out0_a = aligned_alloc<DtUsed>(OUTDEP);
    DtUsed* out1_a = aligned_alloc<DtUsed>(OUTDEP);
    DtUsed* out2_a = aligned_alloc<DtUsed>(OUTDEP);
    DtUsed* out3_a = aligned_alloc<DtUsed>(OUTDEP);

    DtUsed* out0_b = aligned_alloc<DtUsed>(OUTDEP);
    DtUsed* out1_b = aligned_alloc<DtUsed>(OUTDEP);
    DtUsed* out2_b = aligned_alloc<DtUsed>(OUTDEP);
    DtUsed* out3_b = aligned_alloc<DtUsed>(OUTDEP);
    // test data
    bool optionType = 1;
    DtUsed riskFreeRate = 0.06;
    DtUsed strike = 40;
    DtUsed timeLength = 1;
    DtUsed requiredTolerance = 0.2;
    unsigned int requiredSamples = 512; // 0;//1024;//0;
    unsigned int timeSteps = 12;
    unsigned int maxSamples = 2147483648;

    DtUsed* underlying = aligned_alloc<DtUsed>(asset_nm);
    DtUsed* sigma = aligned_alloc<DtUsed>(asset_nm);
    DtUsed* v0 = aligned_alloc<DtUsed>(asset_nm);
    DtUsed* theta = aligned_alloc<DtUsed>(asset_nm);
    DtUsed* kappa = aligned_alloc<DtUsed>(asset_nm);
    DtUsed* rho = aligned_alloc<DtUsed>(asset_nm);
    DtUsed* dividendYield = aligned_alloc<DtUsed>(asset_nm);
    for (int i = 0; i < asset_nm; i++) {
        if (i == 0) {
            underlying[i] = 36;
        } else {
            underlying[i] = 0;
        }
        sigma[i] = 0.001;
        v0[i] = 0.04;
        theta[i] = 0.04;
        kappa[i] = 1.0;
        rho[i] = 0.0;
        dividendYield[i] = 0.0;
    }

    //
    unsigned int loop_nm = 1; // 1000;
    DtUsed goleden = 3.9;
    DtUsed tol = 0.2;
#ifdef HLS_TEST
    int num_rep = 1;
#else
    int num_rep = 1;
    std::string num_str;
    if (parser.getCmdOption("-rep", num_str)) {
        try {
            num_rep = std::stoi(num_str);
        } catch (...) {
            num_rep = 1;
        }
    }
    if (num_rep > 20) {
        num_rep = 20;
        std::cout << "WARNING: limited repeat to " << num_rep << " times\n.";
    }
    if (parser.getCmdOption("-p", num_str)) {
        try {
            requiredSamples = std::stoi(num_str);
        } catch (...) {
            requiredSamples = 512;
        }
    }
#endif

#ifdef HLS_TEST
    kernel_mc_0(loop_nm, underlying, riskFreeRate, sigma, v0, theta, kappa, rho, dividendYield, optionType, strike,
                timeLength, out0_b, requiredSamples, timeSteps, maxSamples);
    print_result(out0_b, goleden, tol, 1);
#else

    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];
    cl::Context context(device);
#ifdef SW_EMU_TEST
    // hls::exp and hls::log have bug in multi-thread.
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE); // | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
#else
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
#endif
    std::string devName = device.getInfo<CL_DEVICE_NAME>();

    std::cout << "Selected Device " << devName << "\n";

    cl::Program::Binaries xclbins = xcl::import_binary_file(xclbin_path);
    devices.resize(1);

    cl::Program program(context, devices, xclbins);

    cl::Kernel kernel0[2];
    for (int i = 0; i < 2; ++i) {
        kernel0[i] = cl::Kernel(program, "kernel_mc_0");
    }
    std::cout << "Kernel has been created\n";

    cl_mem_ext_ptr_t mext_underlying[KN];
    cl_mem_ext_ptr_t mext_sigma[KN];
    cl_mem_ext_ptr_t mext_v0[KN];
    cl_mem_ext_ptr_t mext_theta[KN];
    cl_mem_ext_ptr_t mext_kappa[KN];
    cl_mem_ext_ptr_t mext_rho[KN];
    cl_mem_ext_ptr_t mext_dividendYield[KN];
#ifndef USE_HBM
    mext_underlying[0] = {XCL_MEM_DDR_BANK0, underlying, 0};
    mext_sigma[0] = {XCL_MEM_DDR_BANK0, sigma, 0};
    mext_v0[0] = {XCL_MEM_DDR_BANK0, v0, 0};
    mext_theta[0] = {XCL_MEM_DDR_BANK0, theta, 0};
    mext_kappa[0] = {XCL_MEM_DDR_BANK0, kappa, 0};
    mext_rho[0] = {XCL_MEM_DDR_BANK0, rho, 0};
    mext_dividendYield[0] = {XCL_MEM_DDR_BANK0, dividendYield, 0};
#else
    mext_underlying[0] = {XCL_BANK0, underlying, 0};
    mext_sigma[0] = {XCL_BANK0, sigma, 0};
    mext_v0[0] = {XCL_BANK0, v0, 0};
    mext_theta[0] = {XCL_BANK0, theta, 0};
    mext_kappa[0] = {XCL_BANK0, kappa, 0};
    mext_rho[0] = {XCL_BANK0, rho, 0};
    mext_dividendYield[0] = {XCL_BANK0, dividendYield, 0};
#endif
    cl::Buffer in_buff_underlying[KN];
    cl::Buffer in_buff_sigma[KN];
    cl::Buffer in_buff_v0[KN];
    cl::Buffer in_buff_theta[KN];
    cl::Buffer in_buff_kappa[KN];
    cl::Buffer in_buff_rho[KN];
    cl::Buffer in_buff_dividendYield[KN];

    cl_mem_ext_ptr_t mext_out_a[KN];
    cl_mem_ext_ptr_t mext_out_b[KN];
#ifndef USE_HBM
    mext_out_a[0] = {XCL_MEM_DDR_BANK0, out0_a, 0};

    mext_out_b[0] = {XCL_MEM_DDR_BANK0, out0_b, 0};
#else
    mext_out_a[0] = {XCL_BANK0, out0_a, 0};

    mext_out_b[0] = {XCL_BANK0, out0_b, 0};
#endif
    cl::Buffer out_buff_a[KN];
    cl::Buffer out_buff_b[KN];
    for (int i = 0; i < KN; i++) {
        out_buff_a[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                   (size_t)(OUTDEP * sizeof(DtUsed)), &mext_out_a[i]);
        out_buff_b[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                   (size_t)(OUTDEP * sizeof(DtUsed)), &mext_out_b[i]);
        in_buff_underlying[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                           (size_t)(asset_nm * sizeof(DtUsed)), &mext_underlying[i]);
        in_buff_sigma[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                      (size_t)(asset_nm * sizeof(DtUsed)), &mext_sigma[i]);
        in_buff_v0[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                   (size_t)(asset_nm * sizeof(DtUsed)), &mext_v0[i]);
        in_buff_theta[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                      (size_t)(asset_nm * sizeof(DtUsed)), &mext_theta[i]);
        in_buff_kappa[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                      (size_t)(asset_nm * sizeof(DtUsed)), &mext_kappa[i]);
        in_buff_rho[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    (size_t)(asset_nm * sizeof(DtUsed)), &mext_rho[i]);
        in_buff_dividendYield[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                              (size_t)(asset_nm * sizeof(DtUsed)), &mext_dividendYield[i]);
    }
    std::vector<std::vector<cl::Event> > kernel_events(num_rep);
    std::vector<std::vector<cl::Event> > read_events(num_rep);
    for (int i = 0; i < num_rep; ++i) {
        kernel_events[i].resize(KN);
        read_events[i].resize(1);
    }
    int j = 0;
    kernel0[0].setArg(j++, loop_nm);
    kernel0[0].setArg(j++, in_buff_underlying[0]);
    kernel0[0].setArg(j++, riskFreeRate);
    kernel0[0].setArg(j++, in_buff_sigma[0]);
    kernel0[0].setArg(j++, in_buff_v0[0]);
    kernel0[0].setArg(j++, in_buff_theta[0]);
    kernel0[0].setArg(j++, in_buff_kappa[0]);
    kernel0[0].setArg(j++, in_buff_rho[0]);
    kernel0[0].setArg(j++, in_buff_dividendYield[0]);
    kernel0[0].setArg(j++, optionType);
    kernel0[0].setArg(j++, strike);
    kernel0[0].setArg(j++, timeLength);
    kernel0[0].setArg(j++, out_buff_a[0]);
    kernel0[0].setArg(j++, requiredTolerance);
    kernel0[0].setArg(j++, requiredSamples);
    kernel0[0].setArg(j++, timeSteps);
    kernel0[0].setArg(j++, maxSamples);
    j = 0;
    kernel0[1].setArg(j++, loop_nm);
    kernel0[1].setArg(j++, in_buff_underlying[0]);
    kernel0[1].setArg(j++, riskFreeRate);
    kernel0[1].setArg(j++, in_buff_sigma[0]);
    kernel0[1].setArg(j++, in_buff_v0[0]);
    kernel0[1].setArg(j++, in_buff_theta[0]);
    kernel0[1].setArg(j++, in_buff_kappa[0]);
    kernel0[1].setArg(j++, in_buff_rho[0]);
    kernel0[1].setArg(j++, in_buff_dividendYield[0]);
    kernel0[1].setArg(j++, optionType);
    kernel0[1].setArg(j++, strike);
    kernel0[1].setArg(j++, timeLength);
    kernel0[1].setArg(j++, out_buff_b[0]);
    kernel0[1].setArg(j++, requiredTolerance);
    kernel0[1].setArg(j++, requiredSamples);
    kernel0[1].setArg(j++, timeSteps);
    kernel0[1].setArg(j++, maxSamples);

    std::vector<cl::Memory> out_vec[2]; //{out_buff[0]};
    for (int i = 0; i < KN; ++i) {
        out_vec[0].push_back(out_buff_a[i]);
        out_vec[1].push_back(out_buff_b[i]);
    }

    std::vector<cl::Memory> in_vec[7];
    for (int i = 0; i < KN; i++) {
        in_vec[0].push_back(in_buff_underlying[i]);
        in_vec[1].push_back(in_buff_sigma[i]);
        in_vec[2].push_back(in_buff_v0[i]);
        in_vec[3].push_back(in_buff_theta[i]);
        in_vec[4].push_back(in_buff_kappa[i]);
        in_vec[5].push_back(in_buff_rho[i]);
        in_vec[6].push_back(in_buff_dividendYield[i]);
    }

    // XXX:
    for (int i = 0; i < 7; i++) {
        q.enqueueMigrateMemObjects(in_vec[i], 0);
    }

    q.finish();
    gettimeofday(&st_time, 0);
    for (int i = 0; i < num_rep; ++i) {
        int use_a = i & 1;
        if (use_a) {
            if (i > 1) {
                q.enqueueTask(kernel0[0], &read_events[i - 2], &kernel_events[i][0]);
            }
        } else {
            if (i > 1) {
                q.enqueueTask(kernel0[1], &read_events[i - 2], &kernel_events[i][0]);
            } else {
                q.enqueueTask(kernel0[1], nullptr, &kernel_events[i][0]);
            }
        }
        if (use_a) {
            q.enqueueMigrateMemObjects(out_vec[0], CL_MIGRATE_MEM_OBJECT_HOST, &kernel_events[i], &read_events[i][0]);
        } else {
            q.enqueueMigrateMemObjects(out_vec[1], CL_MIGRATE_MEM_OBJECT_HOST, &kernel_events[i], &read_events[i][0]);
        }
    }

    q.flush();
    q.finish();
    gettimeofday(&end_time, 0);
    int exec_time = tvdiff(&st_time, &end_time);
    std::cout << "FPGA execution time of " << num_rep << " runs:" << exec_time / 1000 << " ms\n"
              << "Average executiom per run: " << exec_time / num_rep / 1000 << " ms\n";

    if (num_rep > 1) {
        bool passed = print_result(out0_a, goleden, tol, loop_nm);
        if (!passed) {
            return -1;
        }
    }
    bool passed = print_result(out0_b, goleden, tol, loop_nm);
    if (!passed) {
        return -1;
    }

    std::cout << "Execution time " << tvdiff(&st_time, &end_time) << std::endl;
#endif

    return 0;
}
