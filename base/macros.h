#ifndef BASE_MACROS_H_
#define BASE_MACROS_H_

#define ALLOW_DEFAULT_COPY(ClassName)    \
  ClassName(const ClassName&) = default; \
  ClassName& operator=(const ClassName&) = default;

#define DISALLOW_COPY(ClassName)        \
  ClassName(const ClassName&) = delete; \
  ClassName& operator=(const ClassName&) = delete;

#define ALLOW_DEFAULT_MOVE(ClassName)        \
  ClassName(ClassName&&) noexcept = default; \
  ClassName& operator=(ClassName&&) noexcept = default;

#define DISALLOW_MOVE(ClassName)            \
  ClassName(ClassName&&) noexcept = delete; \
  ClassName& operator=(ClassName&&) noexcept = delete;

#endif  // #ifndef BASE_MACROS_H_
