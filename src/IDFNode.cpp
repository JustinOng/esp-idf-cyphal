#include "IDFNode.hpp"

#include "esp_timer.h"
#include "sdkconfig.h"

void IDFNode::initialize() {
  _mutex_tx = xSemaphoreCreateMutex();
  _mutex_o1heap = xSemaphoreCreateMutex();

  xTaskCreate(txTask, "cyphal_tx", CONFIG_CYPHAL_TASK_STACK_SIZE, this, CONFIG_CYPHAL_TASK_PRIORITY, &_tx_task);
}

bool IDFNode::enqueue_transfer(CanardMicrosecond const tx_timeout_usec,
                               CanardTransferMetadata const *const transfer_metadata,
                               size_t const payload_buf_size,
                               uint8_t const *const payload_buf) {
  xSemaphoreTake(_mutex_tx, portMAX_DELAY);
  auto ret = NodeBase::enqueue_transfer(tx_timeout_usec, transfer_metadata, payload_buf_size, payload_buf);
  xSemaphoreGive(_mutex_tx);

  xTaskNotify(_tx_task, 0, eNoAction);

  return ret;
}

void IDFNode::_txTask() {
  while (1) {
    xTaskNotifyWait(0, ULONG_MAX, nullptr, pdMS_TO_TICKS(1000));
    xSemaphoreTake(_mutex_tx, portMAX_DELAY);
    processTxQueue();
    xSemaphoreGive(_mutex_tx);
    processPortList();
  }
}

void *IDFNode::_heap_allocate(size_t const amount) {
  xSemaphoreTake(_mutex_o1heap, portMAX_DELAY);
  auto ret = o1heapAllocate(_o1heap_ins, amount);
  xSemaphoreGive(_mutex_o1heap);

  return ret;
}

void IDFNode::_heap_free(void *const pointer) {
  xSemaphoreTake(_mutex_o1heap, portMAX_DELAY);
  o1heapFree(_o1heap_ins, pointer);
  xSemaphoreGive(_mutex_o1heap);
}
