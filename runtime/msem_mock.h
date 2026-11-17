#ifndef DC_MSEM_MOCK_H
#define DC_MSEM_MOCK_H

// Seed the synthetic controller before enabling hooks. Call once, outside timing.
extern "C" void dc_msem_mock_init() noexcept;

#endif
