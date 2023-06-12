#include "IDFNode.hpp"
#include "NVSKeyValueStorage.hpp"
#include "driver/twai.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "pins.h"

constexpr char const *TAG = "main";

bool sendCanardFrame(CanardFrame const &frame);

IDFNode::Heap<IDFNode::DEFAULT_O1HEAP_SIZE>
    node_heap;
IDFNode node(node_heap.data(), node_heap.size(), esp_timer_get_time, sendCanardFrame, 10);

KeyValueStorage kvstore;

static uint16_t node_id = 10;

const auto node_registry = node.create_registry();
const auto reg_ro_cyphal_node_description = node_registry->route("cyphal.node.description", {true}, []() { return "basic-cyphal-node"; });
const auto reg_rw_cyphal_node_id = node_registry->expose("cyphal.node.id", {true}, node_id);

Publisher<uavcan::node::Heartbeat_1_0>
    heartbeat_pub = node.create_publisher<uavcan::node::Heartbeat_1_0>(1 * 1000 * 1000UL /* = 1 sec in usecs. */);

uavcan::node::ExecuteCommand::Response_1_1 onExecuteCommand_1_1_Request_Received(uavcan::node::ExecuteCommand::Request_1_1 const &);
ServiceServer execute_command_srv = node.create_service_server<uavcan::node::ExecuteCommand::Request_1_1, uavcan::node::ExecuteCommand::Response_1_1>(2 * 1000 * 1000UL, onExecuteCommand_1_1_Request_Received);

uavcan::node::ExecuteCommand::Response_1_1 onExecuteCommand_1_1_Request_Received(uavcan::node::ExecuteCommand::Request_1_1 const &req) {
  uavcan::node::ExecuteCommand::Response_1_1 rsp;

  if (req.command == uavcan::node::ExecuteCommand::Request_1_1::COMMAND_RESTART) {
    static TimerHandle_t restart_timer = nullptr;

    if (restart_timer == nullptr) {
      restart_timer = xTimerCreate("restart", pdMS_TO_TICKS(100), pdFALSE, nullptr, [](TimerHandle_t handler) {
        ESP_LOGI(TAG, "executing restart command");
        esp_restart();
      });

      if (xTimerStart(restart_timer, 0) == pdFALSE) {
        rsp.status = uavcan::node::ExecuteCommand::Response_1_1::STATUS_FAILURE;
      }
    }

    rsp.status = uavcan::node::ExecuteCommand::Response_1_1::STATUS_SUCCESS;
  } else if (req.command == uavcan::node::ExecuteCommand::Request_1_1::COMMAND_STORE_PERSISTENT_STATES) {
    auto const rc_save = cyphal::support::save(kvstore, *node_registry);
    if (rc_save.has_value()) {
      ESP_LOGE(TAG, "cyphal::support::save failed with %d", static_cast<int>(rc_save.value()));
      rsp.status = uavcan::node::ExecuteCommand::Response_1_1::STATUS_FAILURE;
      return rsp;
    }
    rsp.status = uavcan::node::ExecuteCommand::Response_1_1::STATUS_SUCCESS;
  } else {
    rsp.status = uavcan::node::ExecuteCommand::Response_1_1::STATUS_BAD_COMMAND;
  }

  return rsp;
}

void heartbeatTask(void *arg) {
  while (1) {
    uavcan::node::Heartbeat_1_0 msg;

    msg.uptime = esp_timer_get_time() / 1000000;
    msg.health.value = uavcan::node::Health_1_0::NOMINAL;
    msg.mode.value = uavcan::node::Mode_1_0::OPERATIONAL;
    msg.vendor_specific_status_code = 0;

    heartbeat_pub->publish(msg);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

esp_err_t canInit() {
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(PIN_CAN_TX, PIN_CAN_RX, TWAI_MODE_NO_ACK);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
  t_config.clk_src = TWAI_CLK_SRC_DEFAULT;
  t_config.brp = 0;
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  ESP_RETURN_ON_ERROR(twai_driver_install(&g_config, &t_config, &f_config), TAG, "twai_driver_install");
  ESP_RETURN_ON_ERROR(twai_reconfigure_alerts(TWAI_ALERT_AND_LOG | 0xFFF8, nullptr), TAG, "twai_reconfigure_alerts");
  ESP_RETURN_ON_ERROR(twai_start(), TAG, "twai_start");

  return ESP_OK;
}

bool sendCanardFrame(CanardFrame const &frame) {
  twai_message_t message;
  memset(&message, 0, sizeof(twai_message_t));

  message.extd = 1;
  message.identifier = frame.extended_can_id;

  assert(frame.payload_size <= TWAI_FRAME_MAX_DLC);
  message.data_length_code = frame.payload_size;
  memcpy(message.data, frame.payload, frame.payload_size);

  esp_err_t err = twai_transmit(&message, pdMS_TO_TICKS(1000));
  if (err != ESP_OK) {
    twai_status_info_t state;
    twai_get_status_info(&state);

    ESP_LOGW(TAG, "twai tx failed: %s state=%d tec=%" PRIu32, esp_err_to_name(err), state.state, state.tx_error_counter);
    return false;
  } else {
    return true;
  }
}

void canRxTask(void *arg) {
  while (1) {
    twai_message_t message;
    if (twai_receive(&message, portMAX_DELAY) != ESP_OK) continue;

    if (!message.extd) {
      ESP_LOGW(TAG, "rx Standard Format frame");
      continue;
    }

    CanardFrame rx_frame;
    rx_frame.extended_can_id = message.identifier;
    rx_frame.payload_size = message.data_length_code;
    rx_frame.payload = message.data;

    node.onCanFrameReceived(rx_frame);
  }
}

extern "C" {
void app_main() {
  vTaskDelay(pdMS_TO_TICKS(50));
  ESP_ERROR_CHECK(canInit());
  ESP_ERROR_CHECK(kvstore.initialize());

  auto const rc_load = cyphal::support::load(kvstore, *node_registry);
  if (rc_load.has_value()) {
    ESP_LOGE(TAG, "cyphal::support::load failed with %d", static_cast<int>(rc_load.value()));
    vTaskDelete(nullptr);
  }

  node.initialize();
  node.setNodeId(static_cast<CanardNodeID>(node_id));

  xTaskCreate(canRxTask, "can", 8192, nullptr, 10, nullptr);
  xTaskCreate(heartbeatTask, "heartbeat", 4096, nullptr, 5, nullptr);

  vTaskDelete(nullptr);
}
}
