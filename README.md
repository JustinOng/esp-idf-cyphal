# ESP-IDF-Cyphal

Wrapper for [107-Arduino-Cyphal](https://github.com/107-systems/107-Arduino-Cyphal) for use with ESP-IDF providing thread-safety.

This should be compatible with ESP-IDF 4.4, but the Registry API requires ESP-IDF 5+ due to the reliance on features only present in gcc 9 (ESP-IDF 5 uses gcc 11).

# Usage

Use `IDFNode` instead of `Node`. There's no need to call `spinSome` because its handled with the canard tx task automatically when a Canard frame is queued.

```cpp
#include "IDFNode.hpp"

bool sendCanardFrame(CanardFrame const &frame);

IDFNode::Heap<IDFNode::DEFAULT_O1HEAP_SIZE>
    node_heap;
IDFNode node(node_heap.data(), node_heap.size(), esp_timer_get_time, sendCanardFrame, 10);

bool sendCanardFrame(CanardFrame const &frame) {
  ...
}

/*
received CAN frames should be passed to `node.onCanFrameReceived(rx_frame);`
*/

extern "C" {

void app_main() {
  node.initialize(); // Start task handling canard tx
}

}
```

# Usage Examples
- [examples/basic](https://github.com/JustinOng/esp-idf-cyphal/blob/master/examples/basic/main/main.cpp)
- [examples/distance-sensor](https://github.com/JustinOng/esp-idf-cyphal/blob/master/examples/distance-sensor/main/main.cpp): Distance sensor with a VL53L0X
- [107-Arduino-Cyphal#examples](https://github.com/107-systems/107-Arduino-Cyphal#examples)
