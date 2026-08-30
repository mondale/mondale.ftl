# Capsule: High-Performance Serialization & Configuration Format Specification

## 1. Overview

Capsule is a minimalist binary serialization and configuration IDL for systems targeting low-latency environments like WebAssembly (Wasm) and Linux services. It enforces a strict separation between unmanaged raw memory containers, generated zero-copy accessors, builders, and heap-allocated materialized representations.

---

## 2. The Capsule IDL Language

Files are suffixed with `.capsule` and parsed via `capsule::Parser`.

### Supported Types

* **Primitives:** `u64`, `u32`, `bool`
* **Strings:** `string` (UTF-8 binary data)
* **Containers:** `vector<T>` (homogeneous dynamically sized arrays)
* **Nested Types:** User-defined capsule identifiers

### Attributes & Static Validation

* **`@default(value)`:** Specifies a fallback scalar or string value if omitted in binary payloads.
* **`@retired`:** Marks a field as deprecated; stops builder generation while retaining its hash slot.
* **Hash Collision Checking:** The compiler performs static uniqueness checks on all 4-byte CRC32C symbol hashes across scopes, halting compilation if a collision occurs.

---

## 3. Runtime Architecture & Generated Types

### Runtime Components (Non-Generated)

* **`capsule::Storage`:** Type-erased raw memory container. Never generated. Manages a byte buffer, provides a getter for a `std::string_view` of its contents (`std::string_view data() const`), and maintains a CRC32C checksum trailer. Instantiated exclusively via a subclass of `capsule::StorageFactory`.

### Generated Types

* **`capsule::View` (Generated):** Zero-copy, read-optimized accessor backed by a `std::shared_ptr<Storage>`. At construction time, the View resolves all field offsets via hash lookup against the payload index table once, caching raw pointers into the underlying storage. Getters involve zero lookups, directly dereferencing these lifetime-guarded pointers.
* `bool has_field_name() const`: Returns `true` if the field is present in storage.
* `field_name() const`: Returns the value from storage if present, or evaluates and returns the schema-defined default value if absent.
* `Result set_field_name(ScalarType new_value)`: Permitted exclusively for present scalar fields. Fails if the field is missing or variable-length.
* `std::string ToString() const`: Returns the `.capsule` text in canonical format.


* **`capsule::Builder` (Generated):** Sequential serialization engine. Accepts a `capsule::StorageFactory` and returns a `std::shared_ptr<Storage>` and `View` pair.
* **`capsule::Materialized` (Generated):** Standard C++ heap-allocated struct representation mirroring the capsule schema for heavy mutations, inheriting from `capsule::Materialized`.
* `static ResultOr<MaterializedType> FromView(const ViewType& view)`: Factory method.
* `ResultOr<std::pair<std::shared_ptr<Storage>, ViewType>> Serialize(StorageFactory& factory) const`: Serializes structure into storage.
* `std::string ToString() const override`: Returns canonical format.
* Constructor from a `View` is private.



---

## 4. Proposed Binary Serialization Format

All multi-byte numeric fields, lengths, and hashes are stored as **unsigned little-endian** values. Capsule and field hashes are 4-byte CRC32Cs.

```text
+-----------------------------------+-----------------------------------+
| Capsule ID Hash [4 bytes]         | Total Payload Length [4 bytes]    |
+-----------------------------------+-----------------------------------+
| Offset Table Count [4 bytes]      | Offset Table Pointer [4 bytes]    |
+-----------------------------------+-----------------------------------+
| [Offset Table Entry 0]            | -> Field Hash [4 bytes]           |
|                                   | -> Data Offset [4 bytes]          |
+-----------------------------------+-----------------------------------+
| [Offset Table Entry N...          | -> Field Hash [4 bytes]           |
|                                   | -> Data Offset [4 bytes]          |
+-----------------------------------+-----------------------------------+
| Fixed-Width Data Section          | (Scalars, inline structs)         |
+-----------------------------------+-----------------------------------+
| Variable-Length Heap Region       | (Strings, vectors, nested blobs)  |
+-----------------------------------+-----------------------------------+
| CRC32C Checksum Trailer [4 bytes] | Covers entire buffer up to trailer|
+-----------------------------------+-----------------------------------+

```

### Layout Mechanics

1. **Header:** Contains the 4-byte CRC32C Capsule ID hash, total payload length, entry count, and pointer to the offset table.
2. **Offset Table:** An array of hash-to-offset mappings used at `View` construction time to bind member raw pointers to their memory addresses within the `Storage` buffer in a single pass.
3. **Natural Alignment:** Assuming the `Storage` buffer is 64-byte aligned, the builder automatically reorders and pads the fixed-width data section by natural alignment requirements (descending order of size: 8-byte, 4-byte, 1-byte).
4. **Data Regions:** Aligned fixed-width scalar slots followed by variable-length heap payloads.
5. **Trailer:** A 4-byte little-endian CRC32C checksum evaluated over all preceding bytes in the `Storage` buffer.

---

## 5. Canonical Example Schema

```protobuf
namespace game::net

capsule Vector3 {
    u32 x
    u32 y
    u32 z
}

capsule PlayerConfig {
    u64 id
    string name
    vector<string> tags
    Vector3 position

    @default(100)
    u32 health

    @retired
    u32 old_metric
}

```
