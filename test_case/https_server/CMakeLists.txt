cmake_minimum_required(VERSION 3.16)

# Tên project
set(PROJECT_NAME "https_server")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Trỏ đến thư mục components chung
set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/../../components)

project(${PROJECT_NAME})
