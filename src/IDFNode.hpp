#pragma once

#include "107-Arduino-Cyphal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

class IDFNode : public NodeBase {
 public:
  template <size_t SIZE>
  struct alignas(O1HEAP_ALIGNMENT) Heap final : public std::array<uint8_t, SIZE> {};

  static size_t constexpr DEFAULT_O1HEAP_SIZE = 16384UL;

  IDFNode(uint8_t *heap_ptr, size_t const heap_size, MicrosFunc const micros_func, CanFrameTxFunc const tx_func, CanardNodeID const node_id = DEFAULT_NODE_ID, size_t const tx_queue_capacity = DEFAULT_TX_QUEUE_SIZE, size_t const mtu_bytes = DEFAULT_MTU_SIZE)
      : NodeBase(micros_func, tx_func, node_id, tx_queue_capacity, mtu_bytes), _o1heap_ins{o1heapInit(heap_ptr, heap_size)} {}
  void initialize();

  static void txTask(void *arg) {
    IDFNode *node = static_cast<IDFNode *>(arg);

    node->_txTask();
  };

  bool enqueue_transfer(CanardMicrosecond const tx_timeout_usec,
                        CanardTransferMetadata const *const transfer_metadata,
                        size_t const payload_buf_size,
                        uint8_t const *const payload_buf) override final;

 protected:
  TaskHandle_t _tx_task;
  SemaphoreHandle_t _mutex_tx, _mutex_o1heap;

  O1HeapInstance *_o1heap_ins;

  void _txTask();

  void *_heap_allocate(size_t const amount) override final;
  void _heap_free(void *const pointer) override final;
};
