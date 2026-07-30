CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic -O2
CPPFLAGS := -Ifirmware/app -Ifirmware/audio -Ifirmware/boot -Ifirmware/core -Ifirmware/hal -Ifirmware/hal/desktop -Ifirmware/hal/pi -Ifirmware/logging -Ifirmware/storage -Ifirmware/theme -Ifirmware/render -Ifirmware/ui -Ifirmware/settings
BUILD_DIR := build
VERSION := $(shell cat VERSION)
RELEASE_DIR := releases
UNAME_S := $(shell uname -s)
CORE := firmware/app/types.cpp firmware/app/shaer_app.cpp firmware/theme/theme_engine.cpp
AUDIO := firmware/audio/playback_engine.cpp
RUNTIME := firmware/core/event_bus.cpp firmware/core/app_state.cpp firmware/core/input_manager.cpp firmware/core/notification_manager.cpp firmware/core/service_manager.cpp firmware/core/task_scheduler.cpp firmware/core/main_scheduler.cpp firmware/core/animation_manager.cpp firmware/core/resource_manager.cpp firmware/core/screen_manager.cpp firmware/core/firmware_services.cpp firmware/core/firmware_runtime.cpp
UI := firmware/ui/ui_framework.cpp
SETTINGS := firmware/settings/settings_catalog.cpp
BOOT := firmware/boot/boot_firmware.cpp firmware/boot/boot_recovery.cpp
LOGGING := firmware/logging/structured_logger.cpp
STORAGE := firmware/storage/settings_store.cpp firmware/storage/music_library_store.cpp firmware/storage/notebook_store.cpp
DESKTOP := firmware/hal/desktop/desktop_hal.cpp firmware/hal/desktop/desktop_system.cpp firmware/render/console_renderer.cpp
PI := firmware/hal/pi/pi_hal.cpp firmware/hal/pi/pi_system.cpp
PI_DIAG := firmware/diagnostics/pi_diagnostics.cpp
LDLIBS := -lsqlite3

.PHONY: all clean check pi simulator preview install package release display_test renderer_test navigation_test ui_navigation_test encoder_test font_test transition_test audio_test playback_test gapless_test crossfade_test buffer_test volume_test seek_test stress_playback_test sdcard_test library_scan_test library_test metadata_test album_art_test performance_test settings_test power_test brightness_test storage_test battery_test wifi_test gpio_test shutdown_test reboot_test developer_mode_test

all: $(BUILD_DIR)/shaer_simulator $(BUILD_DIR)/navigation_tests $(BUILD_DIR)/structured_logger_tests $(BUILD_DIR)/settings_store_tests $(BUILD_DIR)/settings_services_tests $(BUILD_DIR)/music_library_store_tests $(BUILD_DIR)/notebook_store_tests $(BUILD_DIR)/boot_firmware_tests $(BUILD_DIR)/core_runtime_tests $(BUILD_DIR)/playback_engine_tests $(BUILD_DIR)/theme_engine_tests $(BUILD_DIR)/asset_loader_tests $(BUILD_DIR)/theme_switch_tests $(BUILD_DIR)/renderer_tests $(BUILD_DIR)/theme_performance_tests $(BUILD_DIR)/theme_memory_tests $(BUILD_DIR)/ui_framework_tests $(BUILD_DIR)/font_tests $(BUILD_DIR)/transition_tests

check: all
	$(BUILD_DIR)/navigation_tests
	$(BUILD_DIR)/structured_logger_tests
	$(BUILD_DIR)/settings_store_tests
	$(BUILD_DIR)/settings_services_tests
	$(BUILD_DIR)/music_library_store_tests
	$(BUILD_DIR)/notebook_store_tests
	$(BUILD_DIR)/boot_firmware_tests
	$(BUILD_DIR)/core_runtime_tests
	$(BUILD_DIR)/playback_engine_tests
	$(BUILD_DIR)/theme_engine_tests
	$(BUILD_DIR)/asset_loader_tests
	$(BUILD_DIR)/theme_switch_tests
	$(BUILD_DIR)/renderer_tests
	$(BUILD_DIR)/theme_performance_tests
	$(BUILD_DIR)/theme_memory_tests
	$(BUILD_DIR)/ui_framework_tests
	$(BUILD_DIR)/font_tests
	$(BUILD_DIR)/transition_tests
	python3 tests/web_simulator_checks.py
	python3 -m unittest discover -s companion_app/tests
	bash -n install.sh update.sh scripts/*.sh

simulator: $(BUILD_DIR)/shaer_simulator

preview: $(BUILD_DIR)/shaer_preview_backend

$(BUILD_DIR)/shaer_preview_backend: firmware/preview/preview_backend.cpp $(CORE) $(UI) firmware/storage/notebook_store.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/shaer_simulator: firmware/app/main.cpp $(CORE) $(AUDIO) $(RUNTIME) $(UI) $(SETTINGS) $(BOOT) $(LOGGING) $(STORAGE) $(DESKTOP) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/navigation_tests: tests/navigation_tests.cpp $(CORE) firmware/storage/notebook_store.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/structured_logger_tests: tests/structured_logger_tests.cpp $(LOGGING) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/settings_store_tests: tests/settings_store_tests.cpp firmware/storage/settings_store.cpp $(SETTINGS) firmware/app/types.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/settings_services_tests: tests/settings_services_tests.cpp $(AUDIO) $(RUNTIME) $(UI) $(SETTINGS) $(BOOT) $(LOGGING) $(STORAGE) firmware/app/types.cpp firmware/theme/theme_engine.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/music_library_store_tests: tests/music_library_store_tests.cpp firmware/storage/music_library_store.cpp firmware/app/types.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/notebook_store_tests: tests/notebook_store_tests.cpp firmware/storage/notebook_store.cpp firmware/app/types.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/boot_firmware_tests: tests/boot_firmware_tests.cpp $(BOOT) $(LOGGING) $(STORAGE) $(SETTINGS) firmware/app/types.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/core_runtime_tests: tests/core_runtime_tests.cpp $(AUDIO) $(RUNTIME) $(UI) $(SETTINGS) $(BOOT) $(LOGGING) $(STORAGE) firmware/app/types.cpp firmware/theme/theme_engine.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/playback_engine_tests: tests/playback_engine_tests.cpp $(AUDIO) firmware/app/types.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/theme_engine_tests: tests/theme_engine_tests.cpp firmware/theme/theme_engine.cpp firmware/app/types.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/asset_loader_tests: tests/asset_loader_tests.cpp firmware/theme/theme_engine.cpp firmware/app/types.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/theme_switch_tests: tests/theme_switch_tests.cpp firmware/theme/theme_engine.cpp firmware/app/types.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/renderer_tests: tests/renderer_tests.cpp $(UI) firmware/app/types.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/theme_performance_tests: tests/theme_performance_tests.cpp firmware/theme/theme_engine.cpp $(UI) firmware/app/types.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/theme_memory_tests: tests/theme_memory_tests.cpp firmware/theme/theme_engine.cpp firmware/app/types.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/ui_framework_tests: tests/ui_framework_tests.cpp $(UI) firmware/app/types.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/font_tests: tests/font_tests.cpp $(UI) firmware/app/types.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/transition_tests: tests/transition_tests.cpp $(UI) firmware/app/types.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

ifeq ($(UNAME_S),Linux)
pi: $(BUILD_DIR)/shaer_pi_bringup $(BUILD_DIR)/shaer_diag

$(BUILD_DIR)/shaer_pi_bringup: firmware/app/pi_main.cpp $(CORE) $(AUDIO) $(RUNTIME) $(UI) $(SETTINGS) $(BOOT) $(LOGGING) $(STORAGE) $(PI) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/shaer_diag: $(PI_DIAG) firmware/app/types.cpp firmware/core/app_state.cpp $(AUDIO) $(UI) $(SETTINGS) firmware/storage/settings_store.cpp firmware/storage/music_library_store.cpp $(PI) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDLIBS) -o $@

display_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag display

renderer_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag renderer

navigation_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag navigation

ui_navigation_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag ui_navigation

encoder_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag encoder

font_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag font

transition_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag transition

audio_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag audio

playback_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag playback

gapless_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag gapless

crossfade_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag crossfade

buffer_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag buffer

volume_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag volume

seek_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag seek

stress_playback_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag stress_playback

sdcard_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag sdcard

library_scan_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag library_scan_test

library_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag library_test

metadata_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag metadata_test

album_art_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag album_art_test

performance_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag performance_test

settings_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag settings_test

power_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag power_test

brightness_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag brightness_test

storage_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag storage_test

battery_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag battery

wifi_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag wifi

gpio_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag gpio

shutdown_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag shutdown_test

reboot_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag reboot_test

developer_mode_test: $(BUILD_DIR)/shaer_diag
	$(BUILD_DIR)/shaer_diag developer_mode_test

install: pi
	./scripts/install_adi_vasi_os.sh
else
pi:
	@echo "make pi requires Raspberry Pi OS/Linux headers. Use make simulator on this machine, then run make pi on the Raspberry Pi."
	@exit 2

display_test renderer_test navigation_test ui_navigation_test encoder_test font_test transition_test audio_test playback_test gapless_test crossfade_test buffer_test volume_test seek_test stress_playback_test sdcard_test library_scan_test library_test metadata_test album_art_test performance_test settings_test power_test brightness_test storage_test battery_test wifi_test gpio_test shutdown_test reboot_test developer_mode_test:
	@echo "$@ requires Raspberry Pi OS/Linux hardware access."
	@exit 2

install:
	@echo "make install is only for the Raspberry Pi."
	@echo "On this Mac, use: make check"
	@echo "On the Pi, use: cd /opt/shaer && make pi && sudo make install && sudo systemctl restart shaer"
	@exit 2

$(BUILD_DIR)/shaer_pi_bringup:
	@echo "shaer_pi_bringup must be built on Raspberry Pi OS/Linux."
	@exit 2
endif

package: all
	python3 scripts/package_release.py

release: check package

clean:
	rm -rf $(BUILD_DIR)/*
	mkdir -p $(BUILD_DIR)
	touch $(BUILD_DIR)/.gitkeep
