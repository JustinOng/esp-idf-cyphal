#pragma once

// #if !defined(__GNUC__) || (__GNUC__ >= 11)

#include "esp_err.h"
#include "nvs_flash.h"
#include "util/storage/KeyValueStorage.hpp"

using namespace cyphal::support::platform::storage;

class KeyValueStorage : public interface::KeyValueStorage {
 public:
  [[nodiscard]] esp_err_t initialize(const char* partition_name = "cyphal-kv-store");
  /// The return value is the number of bytes read into the buffer or the error.
  [[nodiscard]] auto get(const std::string_view key, const std::size_t size, void* const data) const -> std::variant<Error, std::size_t> override;

  /// Existing data, if any, is replaced entirely. New file and its parent directories created implicitly.
  /// Either all or none of the data bytes are written.
  [[nodiscard]] auto put(const std::string_view key, const std::size_t size, const void* const data) -> std::optional<Error> override;

  /// Remove key. If the key does not exist, the existence error is returned.
  [[nodiscard]] auto drop(const std::string_view key) -> std::optional<Error> override;

 private:
  nvs_handle_t _nvs_handle;

  static constexpr int LEN_NVS_KEY = 9;
  static_assert(LEN_NVS_KEY < NVS_KEY_NAME_MAX_SIZE);

  // Hash key because nvs supports a relatively short key length. Output is null terminated.
  static void hash(const std::string_view key, char out[LEN_NVS_KEY]);
};

// #endif
