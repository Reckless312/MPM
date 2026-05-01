#ifndef MPM_METHOD_CUDA_CHECK_H
#define MPM_METHOD_CUDA_CHECK_H

#include <cstdio>
#include <cuda_runtime.h>

#define CUDA_CHECK(expr_to_check) do {                                    \
    cudaError_t result = expr_to_check;                                   \
    if (result != cudaSuccess)                                            \
    {                                                                     \
        fprintf(stderr,                                                   \
                "CUDA Runtime Error: %s:%i:%d = %s\n",                   \
                __FILE__,                                                 \
                __LINE__,                                                 \
                result,                                                   \
                cudaGetErrorString(result));                              \
        exit(1);                                                          \
    }                                                                     \
} while(0)

#endif
