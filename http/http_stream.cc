#include "base/flags.h"
#include "http/http_stream.h"

FLAG_COHORT(http);
FLAG(size_t, standing_buffer_size, 8192).Ge(1).Le(65536);
FLAG(size_t, max_buffer_size, 1024 * 1024).Ge(1);

namespace http {

void HttpStream::Prime() {
  buf_.resize(FLAG_LOOKUP(standing_buffer_size));
  p_->Post(
      {buf_.data(), buf_.size(), [this](size_t bytes) { AddBytes(bytes); }});
}

void HttpStream::AddBytes(size_t bytes) {
  // Upcall of zero indicates that a buffer was popped to deal with an error
  // conditon and we shouldn't be posting anymore.
  if (bytes == 0) return;

  bytes_ += bytes;
  CHECK_LE(bytes_, buf_.size());
  r_ = TryParse();
  ReportSadIfNeeded();
}

Result HttpStream::TryParse() {
  RequestParser rp;
  TRY_ASSIGN(auto n, rp.Parse(std::string_view(buf_.data(), bytes_)));
  if (n > 0) {
    fn_(r_, rp, p_.get());
    TRY(ShiftVector(n));
  }

  TRY(GrowVectorIfNeeded());
  TrimVectorIfNeeded();

  // Post another buffer for consumption.
  const auto avail = buf_.size() - bytes_;
  p_->Post(
      {buf_.data() + bytes_, avail, [this](size_t bytes) { AddBytes(bytes); }});
  return Result::Ok();
}

void HttpStream::TrimVectorIfNeeded() {
  // Possibly shrink the vector's allocation.
  const size_t sbs = FLAG_LOOKUP(standing_buffer_size);
  if (bytes_ <= sbs) {
    buf_.resize(sbs);
    buf_.shrink_to_fit();
  }
}

Result HttpStream::ShiftVector(size_t n) {
  // Trim the leading n bytes from the vector.
  if (n > bytes_) {
    return core::StreamFatalError(
        strings::Format("Overconsumed? [{}] > [{}]", n, bytes_));
  }

  if (n == bytes_) {
    // Easy case - there's no meaningful bytes left in the vector.
    bytes_ = 0;
  } else {
    // Sad case -> need to shift unconsumed bytes down in the buffer.
    const size_t live_bytes = bytes_ - n;
    for (size_t i = 0; i < live_bytes; ++i) {
      buf_[i] = buf_[bytes_ + i];
    }
    bytes_ = live_bytes;
  }
  return Result::Ok();
}

Result HttpStream::GrowVectorIfNeeded() {
  const size_t avail = buf_.size() - bytes_;
  const size_t sbs = FLAG_LOOKUP(standing_buffer_size);
  if (avail < sbs) {
    const size_t max = FLAG_LOOKUP(max_buffer_size);
    const size_t grow_to = std::min(max, buf_.size() * 2);
    if (grow_to == buf_.size()) {
      return core::StreamFatalError(strings::Format(
          "Inbound HTTP stream size exceeds buffer limit of [{}] bytes.", max));
    }
    buf_.resize(grow_to);
  }
  return Result::Ok();
}

void HttpStream::ReportSadIfNeeded() {
  if (r_.IsOk()) return;
  RequestParser rp;
  fn_(r_, rp, p_.get());
  p_->PostClose();
}

}  // namespace http
