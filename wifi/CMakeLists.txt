cmake_minimum_required(VERSION 3.16)
include(FetchContent)

FetchContent_Declare(
  esp_aws_iot
  GIT_REPOSITORY https://github.com/espressif/esp-aws-iot.git
  GIT_TAG        release/v3.1.x
)
FetchContent_Populate(esp_aws_iot)

FetchContent_Declare(
  espidflib
  GIT_REPOSITORY https://github.com/UncleRus/esp-idf-lib.git
)
FetchContent_MakeAvailable(espidflib)

set(EXTRA_COMPONENT_DIRS
    ${espidflib_SOURCE_DIR}/components
    ${esp_aws_iot_SOURCE_DIR}
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(wifi)

