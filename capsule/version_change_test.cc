#include "capsule/decoder.h"
#include "capsule/encoder.h"
#include "capsule/storage.h"
#include "capsule/storage_factory.h"
#include "capsule/version_change.capsule.h"
#include "core/vocabulary.h"
#include "testing/testing.h"

using testing::IsOk;

namespace {

TEST(VersionChangeThroughSerialization) {
  capsule_version_change::OldCapsuleM o;
  o.unchanged_field = 44;
  o.cleverly_named_field = 55;
  o.retired_field = 66;
  o.long_dead_field = 77;

  const auto size = o.ComputeStorageSize();
  auto fac = capsule::NewHeapStorageFactory().ValueOrDie();
  auto storage = capsule::Storage::Allocate(fac, size).ValueOrDie();
  auto* const base = storage->DataAsPtrTo<void>();
  capsule::Encoder e(base, size,
                     capsule_version_change::OldCapsuleM::kFieldCount);
  o.Encode(&e);
  ASSERT_THAT(e.result(), IsOk());
  EXPECT_EQ(size, e.Seal());

  capsule_version_change::NewCapsuleM n;
  auto d = capsule::Decoder::Build(base, size).ValueOrDie();
  EXPECT_THAT(n.Decode(&d), IsOk());

  EXPECT_EQ(n.unchanged_field, 44);
  EXPECT_EQ(n.renamed_field, 55);
}

}  // namespace
