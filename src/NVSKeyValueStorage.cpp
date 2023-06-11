#include "NVSKeyValueStorage.hpp"

// #if !defined(__GNUC__) || (__GNUC__ >= 11)

#include "esp_check.h"
#include "sdkconfig.h"
#include "util/registry/Registry.hpp"

static const char* TAG = "NVSKVStore";

using namespace cyphal::support::platform::storage;

esp_err_t KeyValueStorage::initialize(const char* partition_name) {
#ifdef CONFIG_CYPHAL_NVSKVSTORE_INIT_NVS
  {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      err = nvs_flash_erase();
      ESP_RETURN_ON_ERROR(err, TAG, "Failed to erase flash");
      err = nvs_flash_init();
    }
    ESP_RETURN_ON_ERROR(err, TAG, "Failed to init flash");
  }
#endif
  esp_err_t err = nvs_open(partition_name, NVS_READWRITE, &_nvs_handle);
  ESP_RETURN_ON_ERROR(err, TAG, "Failed to open nvs partition %s", partition_name);

  return ESP_OK;
}

void KeyValueStorage::hash(const std::string_view key, char out[LEN_NVS_KEY]) {
  uint64_t h = registry::detail::CRC64WE(key.begin(), key.end()).get();
  memcpy(out, (void*)&h, sizeof(h));
  out[sizeof(h)] = '\0';
}

auto KeyValueStorage::get(const std::string_view key, const std::size_t size, void* const data) const
    -> std::variant<Error, std::size_t> {
  char hashed_key[LEN_NVS_KEY];
  hash(key, hashed_key);

  size_t blob_size = size;
  esp_err_t err = nvs_get_blob(_nvs_handle, hashed_key, data, &blob_size);

  assert(err != ESP_ERR_NVS_INVALID_NAME);

  if (err == ESP_OK) {
    return blob_size;
  } else if (err == ESP_ERR_NVS_NOT_FOUND) {
    return Error::Existence;
  } else if (err == ESP_ERR_NVS_INVALID_LENGTH) {
    return Error::API;
  } else if (err == ESP_ERR_NVS_INVALID_HANDLE || err == ESP_ERR_NVS_INVALID_LENGTH) {
    return Error::API;
  } else {
    return Error::Internal;
  }
}

auto KeyValueStorage::put(const std::string_view key, const std::size_t size, const void* const data) -> std::optional<Error> {
  char hashed_key[LEN_NVS_KEY];
  hash(key, hashed_key);

  esp_err_t err = nvs_set_blob(_nvs_handle, hashed_key, data, size);

  assert(err != ESP_ERR_NVS_INVALID_NAME);

  if (err == ESP_OK) {
    nvs_commit(_nvs_handle);
    return std::nullopt;
  } else if (err == ESP_ERR_NVS_INVALID_HANDLE || err == ESP_ERR_NVS_READ_ONLY || err == ESP_ERR_NVS_READ_ONLY || err == ESP_ERR_NVS_REMOVE_FAILED) {
    return Error::API;
  } else if (err == ESP_ERR_NVS_NOT_ENOUGH_SPACE) {
    return Error::Capacity;
  } else {
    return Error::Internal;
  }
}

auto KeyValueStorage::drop(const std::string_view key) -> std::optional<Error> {
  char hashed_key[LEN_NVS_KEY];
  hash(key, hashed_key);

  esp_err_t err = nvs_erase_key(_nvs_handle, hashed_key);

  if (err == ESP_OK) {
    nvs_commit(_nvs_handle);
    return std::nullopt;
  } else if (err == ESP_ERR_NVS_NOT_FOUND) {
    return Error::Existence;
  } else if (err == ESP_ERR_NVS_INVALID_HANDLE || err == ESP_ERR_NVS_READ_ONLY) {
    return Error::API;
  } else {
    return Error::Internal;
  }
}

// #endif
