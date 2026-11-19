// Include the implementation to inspect nesting and force controller updates
// without exposing test-only state through the runtime ABI.
#include "../runtime/msem_mock.cc"

#include <cstdio>
#include <thread>

namespace {
void check(bool condition) {
    if (!condition)
        std::abort();
}
}

int main() {
    dc_msem_mock_init();
    DcFunction outer{}, inner{};
    check(!dc_has_memory_permit());

    mock.m_last_time = 0;
    dc_acquire(&outer);
    check(dc_has_memory_permit() && permit_depth == 1);
    check(mock.m_last_time != 0);

    mock.m_last_time = 0;
    dc_acquire(&inner);
    dc_acquire(&inner); // Recursive entry.
    check(permit_depth == 3 && mock.m_last_time == 0);
    dc_release(&inner);
    dc_release(&inner);
    check(dc_has_memory_permit() && permit_depth == 1);
    check(mock.m_last_time == 0); // Inner releases did not run the controller.

    // The parent is idle while the worker uses the single-threaded mock.
    std::thread worker([&] {
        check(!dc_has_memory_permit());
        dc_acquire(&inner);
        check(dc_has_memory_permit() && permit_depth == 1);
        check(mock.m_last_time != 0); // Independent outermost acquisition.
        dc_release(&inner);
        check(!dc_has_memory_permit());
    });
    worker.join();
    check(dc_has_memory_permit() && permit_depth == 1);

    mock.m_last_time = 0;
    dc_release(&outer);
    check(!dc_has_memory_permit() && permit_depth == 0);
    check(mock.m_last_time != 0);

    dc_acquire(&outer);
    dc_release(&outer);
    check(!dc_has_memory_permit());
    std::puts("msem mock: nesting, recursion, outermost hooks, thread isolation passed");
}
