#ifndef CORE_CRC32C_H_
#define CORE_CRC32C_H_

#include <cstddef>
#include <cstdint>

#include "core/hardened_int.h"

namespace core {

HARDENED_INT_TYPE(CRC32C, uint32_t);

CRC32C ComputeCRC32C(const void* p, size_t l, CRC32C v = CRC32C(0));

}  // namespace core

#endif  // CORE_CRC32C_H_
