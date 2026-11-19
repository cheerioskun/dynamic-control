#include "dynamic_control.h"
#include "msem_mock.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <random>

// Adapted from svalinn-shenango/m-semaphore/bench/msem_bench.cc via exp-15.
// See msem_mock.md for source revisions and the retained synthetic behavior.
// Always-allow fixture; nesting is tracked per thread, with no capacity
// enforcement or locking. The synthetic controller remains single-threaded.
namespace {
constexpr std::uint32_t MAX_CORES = 45;
constexpr std::uint64_t CTL_DELAY_US = 3000;
constexpr std::uint32_t NUM_STD_NORM_SAMPLES = 1 << 18;
constexpr int WINDOW_SZ = 10;
constexpr double ALPHA = 0.8;
constexpr double TARGET_NORM_MEMBW = 1.0;
constexpr double MAGNIFYER = 100.0;
constexpr double EXPLR_PROB = 0.3;
constexpr double TAU = 30.0;

std::uint64_t microtime() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

double RuntimeMemBwUsage() {
    return 175000.0;
}
} // namespace

struct MemSemaphoreHandle {
    std::uint32_t m_cap = 12;
    std::uint32_t m_max_cap = MAX_CORES;
    double m_max_membw = 0;
    double m_samples[MAX_CORES + 1][WINDOW_SZ] = {};
    int m_samples_idx[MAX_CORES + 1] = {};
    std::uint64_t m_counts[MAX_CORES + 1] = {};
    double m_sums[MAX_CORES + 1] = {};
    double m_means[MAX_CORES + 1] = {};
    double m_std_devs[MAX_CORES + 1] = {};
    double m_softmax_probs[MAX_CORES + 1] = {};
    std::array<double, NUM_STD_NORM_SAMPLES> m_std_normals;
    std::uint64_t m_std_normals_idx = 0;
    std::uint64_t m_last_time = 0;

    void update_capacity();
};

void MemSemaphoreHandle::update_capacity() {
    const std::uint64_t now_time = microtime();
    if (now_time - m_last_time < CTL_DELAY_US) {
        return;
    }
    // Preserve the mock's calculations, including its fixed posterior values.
    double membw = RuntimeMemBwUsage();

    if (m_max_membw < membw) {
        m_max_membw = membw;
    }

    double norm_membw = membw / m_max_membw;
    norm_membw = std::min(norm_membw, TARGET_NORM_MEMBW) / TARGET_NORM_MEMBW;
    double norm_cap = (double)m_cap / (double)m_max_cap;
    double reward = ALPHA * norm_membw - (1 - ALPHA) * norm_cap;
    reward = MAGNIFYER * reward;

    double old_reward = m_samples[m_cap][m_samples_idx[m_cap]];
    m_samples[m_cap][m_samples_idx[m_cap]] = reward;
    m_samples_idx[m_cap] = (m_samples_idx[m_cap] + 1) % WINDOW_SZ;

    ++m_counts[m_cap];
    if (m_counts[m_cap] > WINDOW_SZ) {
        m_counts[m_cap] = WINDOW_SZ;
    }
    m_sums[m_cap] = m_sums[m_cap] + reward - old_reward;

    double explr_prob = ((double)(rand() % 1001)) / 1000.0;
    if (explr_prob < EXPLR_PROB) {
        double max_mean = -100000000.0;
        for (uint32_t i = 1; i <= m_cap; ++i) {
            if (max_mean < m_means[i]) {
                max_mean = m_means[i];
            }
        }
        double sum = 0.0;
        for (uint32_t i = 1; i <= m_cap; ++i) {
            m_softmax_probs[i] = std::exp((m_means[i] - max_mean) / TAU);
            sum += m_softmax_probs[i];
        }
        for (uint32_t i = 1; i <= m_cap; ++i) {
            m_softmax_probs[i] = m_softmax_probs[i] / sum;
        }
        double arm_prob = ((double)(rand() % 1001)) / 1000.0;
        for (uint32_t i = 1; i <= m_cap; ++i) {
            if (arm_prob < m_softmax_probs[i]) {
                m_cap = i;
                break;
            }
            arm_prob = arm_prob - m_softmax_probs[i];
        }
    } else {
        double max_mean_reward = -100000000.0;
        for (uint32_t i = 1; i <= m_max_cap; ++i) {
            double z = m_std_normals[m_std_normals_idx++];
            m_std_normals_idx = m_std_normals_idx & (NUM_STD_NORM_SAMPLES - 1);
            double mean_reward = m_means[i] + m_std_devs[i] * z;
            if (mean_reward > max_mean_reward) {
                max_mean_reward = mean_reward;
                m_cap = i;
            }
        }
    }

    m_last_time = now_time;
}

namespace {
MemSemaphoreHandle mock;
thread_local unsigned permit_depth = 0;
}

extern "C" void dc_msem_mock_init() noexcept {
    std::mt19937 generator(0);
    std::normal_distribution<double> normal(0.0, 1.0);
    for (double& sample : mock.m_std_normals) {
        sample = normal(generator);
    }
    std::uniform_real_distribution<double> dist(0.0, 80.0);
    for (std::uint32_t i = 1; i <= MAX_CORES; ++i) {
        mock.m_means[i] = dist(generator);
        mock.m_std_devs[i] = std::sqrt(dist(generator));
    }
    std::srand(0);
    mock.m_last_time = microtime();
}

extern "C" __attribute__((noinline))
void dc_acquire(const DcFunction *) noexcept {
    if (permit_depth == 0) {
        mock.update_capacity();
        // Keep the controller's state observable across optimization.
        asm volatile("" : : "r"(&mock) : "memory");
    }
    ++permit_depth;
}

extern "C" __attribute__((noinline))
void dc_release(const DcFunction *) noexcept {
    assert(permit_depth != 0);
    if (permit_depth == 1) {
        mock.update_capacity();
        asm volatile("" : : "r"(&mock) : "memory");
    }
    --permit_depth;
}

extern "C" bool dc_has_memory_permit() noexcept {
    return permit_depth != 0;
}
