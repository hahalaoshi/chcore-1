cmake_minimum_required(VERSION 3.11)

set(TEST_LAB4_BINS
    "ipc_send_client" "ipc_recv_server"
)

foreach(bin ${TEST_LAB4_BINS})
  file(GLOB ${bin}_source_files "${bin}.c" "../lib/*.c" "../lib/*.S")
  add_executable(${bin}.bin ${${bin}_source_files})
  set_property(
          TARGET ${bin}.bin
          APPEND_STRING
          PROPERTY
          LINK_FLAGS
          "-e START"
  )
endforeach(bin)

include_directories("../")
