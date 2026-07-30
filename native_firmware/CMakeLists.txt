cmake_minimum_required(VERSION 3.16)
project(shaer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SQLite3 REQUIRED)

set(SHAER_INCLUDE_DIRS
    firmware/app
    firmware/audio
    firmware/boot
    firmware/core
    firmware/hal
    firmware/hal/desktop
    firmware/hal/pi
    firmware/logging
    firmware/render
    firmware/storage
    firmware/settings
    firmware/theme
    firmware/ui
)

set(SHAER_CORE_SOURCES
    firmware/app/types.cpp
    firmware/app/shaer_app.cpp
    firmware/audio/playback_engine.cpp
    firmware/theme/theme_engine.cpp
    firmware/core/event_bus.cpp
    firmware/core/app_state.cpp
    firmware/core/input_manager.cpp
    firmware/core/notification_manager.cpp
    firmware/core/service_manager.cpp
    firmware/core/task_scheduler.cpp
    firmware/core/main_scheduler.cpp
    firmware/core/animation_manager.cpp
    firmware/core/resource_manager.cpp
    firmware/core/screen_manager.cpp
    firmware/core/firmware_services.cpp
    firmware/core/firmware_runtime.cpp
    firmware/boot/boot_firmware.cpp
    firmware/boot/boot_recovery.cpp
    firmware/logging/structured_logger.cpp
    firmware/storage/settings_store.cpp
    firmware/storage/music_library_store.cpp
    firmware/storage/notebook_store.cpp
    firmware/settings/settings_catalog.cpp
    firmware/ui/ui_framework.cpp
)

add_library(shaer_firmware_core ${SHAER_CORE_SOURCES})
target_include_directories(shaer_firmware_core PUBLIC ${SHAER_INCLUDE_DIRS})
target_link_libraries(shaer_firmware_core PUBLIC SQLite::SQLite3)

add_executable(shaer_simulator
    firmware/app/main.cpp
    firmware/hal/desktop/desktop_hal.cpp
    firmware/hal/desktop/desktop_system.cpp
    firmware/render/console_renderer.cpp
)
target_link_libraries(shaer_simulator PRIVATE shaer_firmware_core)

add_executable(navigation_tests tests/navigation_tests.cpp firmware/app/types.cpp firmware/app/shaer_app.cpp firmware/theme/theme_engine.cpp firmware/storage/notebook_store.cpp)
target_include_directories(navigation_tests PRIVATE ${SHAER_INCLUDE_DIRS})

add_executable(notebook_store_tests tests/notebook_store_tests.cpp firmware/storage/notebook_store.cpp firmware/app/types.cpp)
target_include_directories(notebook_store_tests PRIVATE ${SHAER_INCLUDE_DIRS})

add_executable(structured_logger_tests tests/structured_logger_tests.cpp firmware/logging/structured_logger.cpp)
target_include_directories(structured_logger_tests PRIVATE ${SHAER_INCLUDE_DIRS})

add_executable(settings_store_tests tests/settings_store_tests.cpp firmware/storage/settings_store.cpp firmware/app/types.cpp)
target_include_directories(settings_store_tests PRIVATE ${SHAER_INCLUDE_DIRS})
target_link_libraries(settings_store_tests PRIVATE SQLite::SQLite3)

add_executable(boot_firmware_tests tests/boot_firmware_tests.cpp firmware/app/types.cpp firmware/boot/boot_firmware.cpp firmware/boot/boot_recovery.cpp firmware/logging/structured_logger.cpp firmware/storage/settings_store.cpp)
target_include_directories(boot_firmware_tests PRIVATE ${SHAER_INCLUDE_DIRS})
target_link_libraries(boot_firmware_tests PRIVATE SQLite::SQLite3)

add_executable(core_runtime_tests tests/core_runtime_tests.cpp)
target_link_libraries(core_runtime_tests PRIVATE shaer_firmware_core)

add_executable(playback_engine_tests tests/playback_engine_tests.cpp firmware/audio/playback_engine.cpp firmware/app/types.cpp)
target_include_directories(playback_engine_tests PRIVATE ${SHAER_INCLUDE_DIRS})

add_executable(theme_engine_tests tests/theme_engine_tests.cpp firmware/theme/theme_engine.cpp firmware/app/types.cpp)
target_include_directories(theme_engine_tests PRIVATE ${SHAER_INCLUDE_DIRS})

add_executable(asset_loader_tests tests/asset_loader_tests.cpp firmware/theme/theme_engine.cpp firmware/app/types.cpp)
target_include_directories(asset_loader_tests PRIVATE ${SHAER_INCLUDE_DIRS})

add_executable(theme_switch_tests tests/theme_switch_tests.cpp firmware/theme/theme_engine.cpp firmware/app/types.cpp)
target_include_directories(theme_switch_tests PRIVATE ${SHAER_INCLUDE_DIRS})

add_executable(renderer_tests tests/renderer_tests.cpp firmware/ui/ui_framework.cpp firmware/app/types.cpp)
target_include_directories(renderer_tests PRIVATE ${SHAER_INCLUDE_DIRS})

add_executable(theme_performance_tests tests/theme_performance_tests.cpp firmware/theme/theme_engine.cpp firmware/ui/ui_framework.cpp firmware/app/types.cpp)
target_include_directories(theme_performance_tests PRIVATE ${SHAER_INCLUDE_DIRS})

add_executable(theme_memory_tests tests/theme_memory_tests.cpp firmware/theme/theme_engine.cpp firmware/app/types.cpp)
target_include_directories(theme_memory_tests PRIVATE ${SHAER_INCLUDE_DIRS})

enable_testing()
add_test(NAME navigation_tests COMMAND navigation_tests)
add_test(NAME notebook_store_tests COMMAND notebook_store_tests)
add_test(NAME structured_logger_tests COMMAND structured_logger_tests)
add_test(NAME settings_store_tests COMMAND settings_store_tests)
add_test(NAME boot_firmware_tests COMMAND boot_firmware_tests)
add_test(NAME core_runtime_tests COMMAND core_runtime_tests)
add_test(NAME playback_engine_tests COMMAND playback_engine_tests)
add_test(NAME theme_engine_tests COMMAND theme_engine_tests)
add_test(NAME asset_loader_tests COMMAND asset_loader_tests)
add_test(NAME theme_switch_tests COMMAND theme_switch_tests)
add_test(NAME renderer_tests COMMAND renderer_tests)
add_test(NAME theme_performance_tests COMMAND theme_performance_tests)
add_test(NAME theme_memory_tests COMMAND theme_memory_tests)
