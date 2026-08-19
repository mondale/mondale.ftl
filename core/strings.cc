#include "core/strings.h"

namespace core::strings {
namespace {

// Check standard feature-test macro along with GCC/Clang and MSVC flags
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
constexpr bool kExceptionsEnabled = true;
#else
constexpr bool kExceptionsEnabled = false;
#endif

// Fail compilation if exceptions are enabled
static_assert(
    !kExceptionsEnabled,
    "This library must be compiled with -fno-exceptions! (std::format)");
}  // namespace

}  // namespace core::strings
