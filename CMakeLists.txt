idf_component_register(SRCS
                       "lib/107-Arduino-Cyphal/src/NodeBase.cpp"
                       "lib/107-Arduino-Cyphal/src/Node.cpp"
                       "lib/107-Arduino-Cyphal/src/libcanard/canard.c"
                       "lib/107-Arduino-Cyphal/src/libo1heap/o1heap.c"
                       "src/IDFNode.cpp"
                       "src/NVSKeyValueStorage.cpp"
                       INCLUDE_DIRS "lib/107-Arduino-Cyphal/src" "src"
                       REQUIRES esp_timer nvs_flash
                      )

target_compile_options(${COMPONENT_LIB} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Werror>)
target_compile_options(${COMPONENT_LIB} PUBLIC -std=gnu++17)
