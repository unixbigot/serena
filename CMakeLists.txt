cmake_minimum_required(VERSION 3.5)

# this presumes you have esp-skainet source in the same directory as your esp-idf
set(EXTRA_COMPONENT_DIRS
    $ENV{IDF_PATH}/../esp-skainet/components
    )

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(serena)
