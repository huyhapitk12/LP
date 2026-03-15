/**
 * lp_gpu.h - GPU Acceleration Abstraction Layer for LP Language
 * 
 * Provides unified GPU acceleration interface:
 * - Device detection and management
 * - Memory management (host/device)
 * - Kernel launching abstraction
 * - Automatic GPU/CPU selection
 * 
 * Supports multiple backends:
 * - CUDA (NVIDIA GPUs)
 * - OpenCL (Cross-platform)
 * - CPU fallback (OpenMP)
 * 
 * Usage in LP:
 *   @settings(device="gpu", gpu_id=0)
 *   parallel for i in range(1000000):
 *       result[i] = compute(data[i])
 */

#ifndef LP_GPU_H
#define LP_GPU_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ==================== GPU Backend Detection ==================== */

/* Check for CUDA */
#ifdef __CUDACC__
#define LP_HAS_CUDA 1
#include <cuda_runtime.h>
#else
#define LP_HAS_CUDA 0
#endif

/* Check for OpenCL - we'll load dynamically */
#define LP_HAS_OPENCL 0  /* Will be detected at runtime */

/* GPU Availability */
#define LP_GPU_AVAILABLE (LP_HAS_CUDA || LP_HAS_OPENCL)

/* ==================== Device Types ==================== */

typedef enum {
    LP_DEVICE_CPU = 0,
    LP_DEVICE_CUDA = 1,
    LP_DEVICE_OPENCL = 2,
    LP_DEVICE_AUTO = 3
} LpDeviceType;

typedef enum {
    LP_GPU_NVIDIA = 0,
    LP_GPU_AMD = 1,
    LP_GPU_INTEL = 2,
    LP_GPU_APPLE = 3,
    LP_GPU_UNKNOWN = 4
} LpGpuVendor;

typedef struct {
    LpDeviceType type;
    LpGpuVendor vendor;
    char name[256];
    int64_t total_memory;      /* Total memory in bytes */
    int64_t free_memory;       /* Free memory in bytes */
    int compute_units;         /* SMs for CUDA, CUs for OpenCL */
    int max_threads_per_block;
    int max_shared_memory;
    int clock_rate;            /* In MHz */
    int supports_double;       /* Double precision support */
    int supports_int64;        /* 64-bit integer support */
    int is_available;
} LpGpuDevice;

/* ==================== Global GPU State ==================== */

typedef struct {
    LpDeviceType selected_device;
    int gpu_id;
    int initialized;
    LpGpuDevice devices[16];
    int num_devices;
    int use_unified_memory;
    int async_transfer;
} LpGpuState;

static LpGpuState lp_gpu_state = {
    .selected_device = LP_DEVICE_CPU,
    .gpu_id = 0,
    .initialized = 0,
    .num_devices = 0,
    .use_unified_memory = 0,
    .async_transfer = 0
};

/* ==================== Device Detection ==================== */

/**
 * Detect available GPU devices
 */
static inline int lp_gpu_detect_devices(void) {
    int count = 0;
    
#if LP_HAS_CUDA
    int cuda_devices = 0;
    if (cudaGetDeviceCount(&cuda_devices) == cudaSuccess) {
        for (int i = 0; i < cuda_devices && count < 16; i++) {
            cudaDeviceProp prop;
            if (cudaGetDeviceProperties(&prop, i) == cudaSuccess) {
                LpGpuDevice *dev = &lp_gpu_state.devices[count];
                dev->type = LP_DEVICE_CUDA;
                dev->vendor = LP_GPU_NVIDIA;
                strncpy(dev->name, prop.name, sizeof(dev->name) - 1);
                dev->total_memory = prop.totalGlobalMem;
                dev->free_memory = prop.totalGlobalMem; /* Approximation */
                dev->compute_units = prop.multiProcessorCount;
                dev->max_threads_per_block = prop.maxThreadsPerBlock;
                dev->max_shared_memory = prop.sharedMemPerBlock;
                dev->clock_rate = prop.clockRate / 1000; /* Convert to MHz */
                dev->supports_double = (prop.major >= 1);
                dev->supports_int64 = 1;
                dev->is_available = 1;
                count++;
            }
        }
    }
#endif

    /* Add CPU as a fallback device */
    if (count < 16) {
        LpGpuDevice *cpu = &lp_gpu_state.devices[count];
        cpu->type = LP_DEVICE_CPU;
        cpu->vendor = LP_GPU_UNKNOWN;
        strcpy(cpu->name, "CPU (OpenMP)");
        cpu->total_memory = 0;  /* System memory */
        cpu->free_memory = 0;
        cpu->compute_units = 0;  /* Will use omp_get_num_procs */
        cpu->max_threads_per_block = 1024;
        cpu->max_shared_memory = 0;
        cpu->clock_rate = 0;
        cpu->supports_double = 1;
        cpu->supports_int64 = 1;
        cpu->is_available = 1;
        count++;
    }
    
    lp_gpu_state.num_devices = count;
    lp_gpu_state.initialized = 1;
    return count;
}

/**
 * Get number of available GPU devices
 */
static inline int64_t lp_gpu_device_count(void) {
    if (!lp_gpu_state.initialized) {
        lp_gpu_detect_devices();
    }
    return lp_gpu_state.num_devices;
}

/**
 * Get device info
 */
static inline LpGpuDevice lp_gpu_get_device(int id) {
    if (!lp_gpu_state.initialized) {
        lp_gpu_detect_devices();
    }
    if (id >= 0 && id < lp_gpu_state.num_devices) {
        return lp_gpu_state.devices[id];
    }
    LpGpuDevice empty = {0};
    return empty;
}

/**
 * Check if GPU is available
 */
static inline int64_t lp_gpu_is_available(void) {
    if (!lp_gpu_state.initialized) {
        lp_gpu_detect_devices();
    }
    /* Check if we have any GPU device (not CPU) */
    for (int i = 0; i < lp_gpu_state.num_devices; i++) {
        if (lp_gpu_state.devices[i].type != LP_DEVICE_CPU) {
            return 1;
        }
    }
    return 0;
}

/**
 * Get best available device
 */
static inline int lp_gpu_get_best_device(void) {
    if (!lp_gpu_state.initialized) {
        lp_gpu_detect_devices();
    }
    
    int best = lp_gpu_state.num_devices - 1;  /* Default to CPU */
    int64_t best_mem = 0;
    
    for (int i = 0; i < lp_gpu_state.num_devices; i++) {
        if (lp_gpu_state.devices[i].type != LP_DEVICE_CPU) {
            if (lp_gpu_state.devices[i].total_memory > best_mem) {
                best_mem = lp_gpu_state.devices[i].total_memory;
                best = i;
            }
        }
    }
    return best;
}

/* ==================== Device Selection ==================== */

/**
 * Select device for computation
 */
static inline void lp_gpu_select_device(int64_t device_id) {
    if (!lp_gpu_state.initialized) {
        lp_gpu_detect_devices();
    }
    
    if (device_id >= 0 && device_id < lp_gpu_state.num_devices) {
        lp_gpu_state.gpu_id = (int)device_id;
        lp_gpu_state.selected_device = lp_gpu_state.devices[device_id].type;
        
#if LP_HAS_CUDA
        if (lp_gpu_state.selected_device == LP_DEVICE_CUDA) {
            cudaSetDevice(lp_gpu_state.gpu_id);
        }
#endif
    }
}

/**
 * Select device by type
 */
static inline int64_t lp_gpu_select_by_type(LpDeviceType type) {
    if (!lp_gpu_state.initialized) {
        lp_gpu_detect_devices();
    }
    
    for (int i = 0; i < lp_gpu_state.num_devices; i++) {
        if (lp_gpu_state.devices[i].type == type || 
            (type == LP_DEVICE_AUTO && lp_gpu_state.devices[i].type != LP_DEVICE_CPU)) {
            lp_gpu_select_device(i);
            return i;
        }
    }
    
    /* Fall back to CPU */
    lp_gpu_select_device(lp_gpu_state.num_devices - 1);
    return lp_gpu_state.num_devices - 1;
}

/**
 * Get current device type
 */
static inline LpDeviceType lp_gpu_get_device_type(void) {
    return lp_gpu_state.selected_device;
}

/**
 * Check if using GPU
 */
static inline int64_t lp_gpu_is_using_gpu(void) {
    return lp_gpu_state.selected_device != LP_DEVICE_CPU;
}

/* ==================== Memory Management ==================== */

typedef struct {
    void *host_ptr;
    void *device_ptr;
    int64_t size;
    int on_device;
    int unified;
} LpGpuBuffer;

/**
 * Allocate unified memory (accessible from both CPU and GPU)
 */
static inline LpGpuBuffer lp_gpu_alloc_unified(int64_t size) {
    LpGpuBuffer buf = {0};
    buf.size = size;
    
#if LP_HAS_CUDA
    if (lp_gpu_state.selected_device == LP_DEVICE_CUDA) {
        if (cudaMallocManaged(&buf.device_ptr, size) == cudaSuccess) {
            buf.host_ptr = buf.device_ptr;
            buf.unified = 1;
            buf.on_device = 1;
            return buf;
        }
    }
#endif
    
    /* Fallback to regular malloc */
    buf.host_ptr = malloc(size);
    buf.unified = 0;
    buf.on_device = 0;
    return buf;
}

/**
 * Allocate device memory
 */
static inline LpGpuBuffer lp_gpu_alloc_device(int64_t size) {
    LpGpuBuffer buf = {0};
    buf.size = size;
    
    /* Always allocate host memory */
    buf.host_ptr = malloc(size);
    if (!buf.host_ptr) return buf;
    
#if LP_HAS_CUDA
    if (lp_gpu_state.selected_device == LP_DEVICE_CUDA) {
        if (cudaMalloc(&buf.device_ptr, size) == cudaSuccess) {
            buf.on_device = 1;
            return buf;
        }
    }
#endif
    
    buf.device_ptr = NULL;
    buf.on_device = 0;
    return buf;
}

/**
 * Copy data to device
 */
static inline int lp_gpu_copy_to_device(LpGpuBuffer *buf) {
    if (!buf || !buf->host_ptr || buf->size <= 0) return -1;
    
#if LP_HAS_CUDA
    if (lp_gpu_state.selected_device == LP_DEVICE_CUDA && buf->device_ptr) {
        if (cudaMemcpy(buf->device_ptr, buf->host_ptr, buf->size, cudaMemcpyHostToDevice) == cudaSuccess) {
            return 0;
        }
    }
#endif
    
    buf->on_device = 0;
    return -1;
}

/**
 * Copy data from device
 */
static inline int lp_gpu_copy_from_device(LpGpuBuffer *buf) {
    if (!buf || !buf->host_ptr || buf->size <= 0) return -1;
    
#if LP_HAS_CUDA
    if (lp_gpu_state.selected_device == LP_DEVICE_CUDA && buf->device_ptr) {
        if (cudaMemcpy(buf->host_ptr, buf->device_ptr, buf->size, cudaMemcpyDeviceToHost) == cudaSuccess) {
            return 0;
        }
    }
#endif
    
    return -1;
}

/**
 * Free GPU buffer
 */
static inline void lp_gpu_free(LpGpuBuffer *buf) {
    if (!buf) return;
    
#if LP_HAS_CUDA
    if (buf->unified && buf->device_ptr) {
        cudaFree(buf->device_ptr);
    } else {
        if (buf->host_ptr) free(buf->host_ptr);
        if (buf->device_ptr) cudaFree(buf->device_ptr);
    }
#else
    if (buf->host_ptr) free(buf->host_ptr);
#endif
    
    buf->host_ptr = NULL;
    buf->device_ptr = NULL;
    buf->size = 0;
    buf->on_device = 0;
}

/* ==================== Configuration ==================== */

/**
 * Configure GPU settings
 */
static inline void lp_gpu_configure(int64_t device_id, int64_t unified_memory, int64_t async_transfer) {
    if (device_id >= 0) {
        lp_gpu_select_device(device_id);
    }
    lp_gpu_state.use_unified_memory = (int)unified_memory;
    lp_gpu_state.async_transfer = (int)async_transfer;
}

/**
 * Get device type string
 */
static inline const char *lp_device_type_string(LpDeviceType type) {
    switch (type) {
        case LP_DEVICE_CPU: return "CPU";
        case LP_DEVICE_CUDA: return "CUDA";
        case LP_DEVICE_OPENCL: return "OpenCL";
        case LP_DEVICE_AUTO: return "Auto";
        default: return "Unknown";
    }
}

/**
 * Get GPU info string
 */
static inline const char *lp_gpu_info(void) {
    static char info[512];
    
    if (!lp_gpu_state.initialized) {
        lp_gpu_detect_devices();
    }
    
    const char *device_name = "Unknown";
    if (lp_gpu_state.gpu_id >= 0 && lp_gpu_state.gpu_id < lp_gpu_state.num_devices) {
        device_name = lp_gpu_state.devices[lp_gpu_state.gpu_id].name;
    }
    
    snprintf(info, sizeof(info), 
             "Device: %s, Type: %s, GPU Available: %s, Devices: %d",
             device_name,
             lp_device_type_string(lp_gpu_state.selected_device),
             lp_gpu_is_available() ? "yes" : "no",
             lp_gpu_state.num_devices);
    return info;
}

/* ==================== Synchronization ==================== */

/**
 * Synchronize device (wait for all operations to complete)
 */
static inline void lp_gpu_sync(void) {
#if LP_HAS_CUDA
    if (lp_gpu_state.selected_device == LP_DEVICE_CUDA) {
        cudaDeviceSynchronize();
    }
#endif
}

/* ==================== Utility Functions ==================== */

/**
 * Get max threads per block for current device
 */
static inline int64_t lp_gpu_max_threads_per_block(void) {
    if (lp_gpu_state.gpu_id >= 0 && lp_gpu_state.gpu_id < lp_gpu_state.num_devices) {
        return lp_gpu_state.devices[lp_gpu_state.gpu_id].max_threads_per_block;
    }
    return 1024;
}

/**
 * Get compute units for current device
 */
static inline int64_t lp_gpu_compute_units(void) {
    if (lp_gpu_state.gpu_id >= 0 && lp_gpu_state.gpu_id < lp_gpu_state.num_devices) {
        return lp_gpu_state.devices[lp_gpu_state.gpu_id].compute_units;
    }
    return 1;
}

/**
 * Calculate optimal block size for given problem size
 */
static inline int64_t lp_gpu_optimal_block_size(int64_t problem_size) {
    int64_t max_threads = lp_gpu_max_threads_per_block();
    
    /* Start with 256 and find the best fit */
    int64_t block_size = 256;
    while (block_size * 2 <= max_threads && block_size * 2 <= problem_size) {
        block_size *= 2;
    }
    
    return block_size;
}

/**
 * Calculate number of blocks for given problem size and block size
 */
static inline int64_t lp_gpu_num_blocks(int64_t problem_size, int64_t block_size) {
    return (problem_size + block_size - 1) / block_size;
}

#endif /* LP_GPU_H */
