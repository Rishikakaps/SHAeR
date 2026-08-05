#include "pi_hal.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <linux/i2c-dev.h>
#include <linux/spi/spidev.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace shaer {

namespace {

constexpr int kMax17048Address = 0x36;
constexpr const char* kI2cBus = "/dev/i2c-1";

bool write_text_file(const std::string& path, const std::string& value) {
    errno = 0;
    std::ofstream file(path);
    if (file) {
        file << value;
        file.flush();
    }
    const bool ok = static_cast<bool>(file);
    if (!ok) {
        std::cerr << "[PiGPIO] write FAIL path=" << path
                  << " value=" << value
                  << " errno=" << errno
                  << " error=" << std::strerror(errno) << "\n";
    }
    return ok;
}

std::string read_text_file(const std::string& path) {
    std::ifstream file(path);
    std::string value;
    if (file) {
        std::getline(file, value);
    }
    return value;
}

std::string shell_quote(const std::string& value) {
    std::string quoted = "'";
    for (char c : value) {
        if (c == '\'') {
            quoted += "'\\''";
        } else {
            quoted.push_back(c);
        }
    }
    quoted.push_back('\'');
    return quoted;
}

bool path_exists(const std::string& path) {
    return access(path.c_str(), F_OK) == 0;
}

volatile uint32_t* gpio_registers() {
    static volatile uint32_t* registers = nullptr;
    static bool attempted = false;
    if (attempted) {
        return registers;
    }
    attempted = true;

    int fd = open("/dev/gpiomem", O_RDWR | O_SYNC);
    if (fd < 0) {
        std::cerr << "[PiGPIO] /dev/gpiomem open failed errno=" << errno
                  << " error=" << std::strerror(errno) << "\n";
        return nullptr;
    }

    void* mapped = mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (mapped == MAP_FAILED) {
        std::cerr << "[PiGPIO] /dev/gpiomem mmap failed errno=" << errno
                  << " error=" << std::strerror(errno) << "\n";
        return nullptr;
    }

    registers = static_cast<volatile uint32_t*>(mapped);
    std::cerr << "[PiGPIO] /dev/gpiomem mapped for fast GPIO control\n";
    return registers;
}

bool gpio_mem_set_output(int gpio) {
    volatile uint32_t* regs = gpio_registers();
    if (regs == nullptr || gpio < 0 || gpio >= 54) {
        return false;
    }
    const int reg = gpio / 10;
    const int shift = (gpio % 10) * 3;
    uint32_t value = regs[reg];
    value &= ~(0x7u << shift);
    value |= (0x1u << shift);
    regs[reg] = value;
    return true;
}

bool gpio_mem_set_input(int gpio) {
    volatile uint32_t* regs = gpio_registers();
    if (regs == nullptr || gpio < 0 || gpio >= 54) {
        return false;
    }
    const int reg = gpio / 10;
    const int shift = (gpio % 10) * 3;
    uint32_t value = regs[reg];
    value &= ~(0x7u << shift);
    regs[reg] = value;
    return true;
}

bool gpio_mem_write(int gpio, int value) {
    volatile uint32_t* regs = gpio_registers();
    if (regs == nullptr || gpio < 0 || gpio >= 54) {
        return false;
    }
    const int bank_offset = gpio >= 32 ? 1 : 0;
    const int bit = gpio >= 32 ? gpio - 32 : gpio;
    if (value) {
        regs[7 + bank_offset] = (1u << bit);
    } else {
        regs[10 + bank_offset] = (1u << bit);
    }
    return true;
}

bool gpio_mem_read(int gpio, int* out) {
    volatile uint32_t* regs = gpio_registers();
    if (regs == nullptr || gpio < 0 || gpio >= 54 || out == nullptr) {
        return false;
    }
    const int bank_offset = gpio >= 32 ? 1 : 0;
    const int bit = gpio >= 32 ? gpio - 32 : gpio;
    *out = (regs[13 + bank_offset] & (1u << bit)) ? 1 : 0;
    return true;
}

bool run_pinctrl(int gpio, const std::string& mode) {
    const std::string command = "pinctrl set " + std::to_string(gpio) + " " + mode;
    const int rc = std::system(command.c_str());
    std::cerr << "[PiGPIO] pinctrl gpio=" << gpio
              << " mode=" << mode
              << " rc=" << rc << "\n";
    return rc == 0;
}

std::string gpio_path(int gpio, const std::string& leaf) {
    return "/sys/class/gpio/gpio" + std::to_string(gpio) + "/" + leaf;
}

void setup_input(int gpio) {
    pi_export_gpio(gpio);
    pi_set_gpio_direction(gpio, "in");
    if (path_exists(gpio_path(gpio, "active_low"))) {
        write_text_file(gpio_path(gpio, "active_low"), "0");
    }
}

void setup_output(int gpio, int initial, const std::string& label = "GPIO") {
    pi_export_gpio(gpio);
    pi_set_gpio_direction(gpio, "out");
    pi_write_gpio(gpio, initial);
    std::cerr << "[PiGPIO] setup " << label
              << " gpio=" << gpio
              << " direction=" << read_text_file(gpio_path(gpio, "direction"))
              << " value=" << read_text_file(gpio_path(gpio, "value"))
              << "\n";
}

int open_i2c_max17048() {
    int fd = open(kI2cBus, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    if (ioctl(fd, I2C_SLAVE, kMax17048Address) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

bool read_max17048_register(uint8_t reg, std::array<uint8_t, 2>* out) {
    int fd = open_i2c_max17048();
    if (fd < 0) {
        return false;
    }
    if (write(fd, &reg, 1) != 1) {
        close(fd);
        return false;
    }
    const ssize_t count = read(fd, out->data(), out->size());
    close(fd);
    return count == static_cast<ssize_t>(out->size());
}

std::array<uint8_t, 5> glyph_for(char raw) {
    const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(raw)));
    switch (c) {
        case 'A': return {0x7E, 0x11, 0x11, 0x11, 0x7E};
        case 'B': return {0x7F, 0x49, 0x49, 0x49, 0x36};
        case 'C': return {0x3E, 0x41, 0x41, 0x41, 0x22};
        case 'D': return {0x7F, 0x41, 0x41, 0x22, 0x1C};
        case 'E': return {0x7F, 0x49, 0x49, 0x49, 0x41};
        case 'F': return {0x7F, 0x09, 0x09, 0x09, 0x01};
        case 'G': return {0x3E, 0x41, 0x49, 0x49, 0x7A};
        case 'H': return {0x7F, 0x08, 0x08, 0x08, 0x7F};
        case 'I': return {0x00, 0x41, 0x7F, 0x41, 0x00};
        case 'J': return {0x20, 0x40, 0x41, 0x3F, 0x01};
        case 'K': return {0x7F, 0x08, 0x14, 0x22, 0x41};
        case 'L': return {0x7F, 0x40, 0x40, 0x40, 0x40};
        case 'M': return {0x7F, 0x02, 0x0C, 0x02, 0x7F};
        case 'N': return {0x7F, 0x04, 0x08, 0x10, 0x7F};
        case 'O': return {0x3E, 0x41, 0x41, 0x41, 0x3E};
        case 'P': return {0x7F, 0x09, 0x09, 0x09, 0x06};
        case 'Q': return {0x3E, 0x41, 0x51, 0x21, 0x5E};
        case 'R': return {0x7F, 0x09, 0x19, 0x29, 0x46};
        case 'S': return {0x46, 0x49, 0x49, 0x49, 0x31};
        case 'T': return {0x01, 0x01, 0x7F, 0x01, 0x01};
        case 'U': return {0x3F, 0x40, 0x40, 0x40, 0x3F};
        case 'V': return {0x1F, 0x20, 0x40, 0x20, 0x1F};
        case 'W': return {0x7F, 0x20, 0x18, 0x20, 0x7F};
        case 'X': return {0x63, 0x14, 0x08, 0x14, 0x63};
        case 'Y': return {0x07, 0x08, 0x70, 0x08, 0x07};
        case 'Z': return {0x61, 0x51, 0x49, 0x45, 0x43};
        case '0': return {0x3E, 0x51, 0x49, 0x45, 0x3E};
        case '1': return {0x00, 0x42, 0x7F, 0x40, 0x00};
        case '2': return {0x42, 0x61, 0x51, 0x49, 0x46};
        case '3': return {0x21, 0x41, 0x45, 0x4B, 0x31};
        case '4': return {0x18, 0x14, 0x12, 0x7F, 0x10};
        case '5': return {0x27, 0x45, 0x45, 0x45, 0x39};
        case '6': return {0x3C, 0x4A, 0x49, 0x49, 0x30};
        case '7': return {0x01, 0x71, 0x09, 0x05, 0x03};
        case '8': return {0x36, 0x49, 0x49, 0x49, 0x36};
        case '9': return {0x06, 0x49, 0x49, 0x29, 0x1E};
        case ':': return {0x00, 0x36, 0x36, 0x00, 0x00};
        case '-': return {0x08, 0x08, 0x08, 0x08, 0x08};
        case '/': return {0x20, 0x10, 0x08, 0x04, 0x02};
        case '.': return {0x00, 0x60, 0x60, 0x00, 0x00};
        case '%': return {0x23, 0x13, 0x08, 0x64, 0x62};
        case '>': return {0x41, 0x22, 0x14, 0x08, 0x00};
        case '<': return {0x08, 0x14, 0x22, 0x41, 0x00};
        case ' ': return {0x00, 0x00, 0x00, 0x00, 0x00};
        default: return {0x7F, 0x41, 0x5D, 0x41, 0x7F};
    }
}

}  // namespace

void pi_export_gpio(int gpio) {
    const std::string base = "/sys/class/gpio/gpio" + std::to_string(gpio);
    if (!path_exists(base)) {
        const bool ok = write_text_file("/sys/class/gpio/export", std::to_string(gpio));
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        std::cerr << "[PiGPIO] export gpio=" << gpio
                  << " result=" << (ok || path_exists(base) ? "ok" : "sysfs-unavailable-pinctrl-fallback")
                  << "\n";
    } else {
        std::cerr << "[PiGPIO] export gpio=" << gpio << " result=already-exported\n";
    }
}

void pi_set_gpio_direction(int gpio, const std::string& direction) {
    const std::string path = gpio_path(gpio, "direction");
    bool ok = false;
    if (path_exists(path)) {
        ok = write_text_file(path, direction);
    }
    if (!ok && direction == "out") {
        ok = gpio_mem_set_output(gpio);
        std::cerr << "[PiGPIO] gpiomem direction gpio=" << gpio
                  << " requested=out result=" << (ok ? "ok" : "fail") << "\n";
    }
    if (!ok && direction == "in") {
        ok = gpio_mem_set_input(gpio);
        std::cerr << "[PiGPIO] gpiomem direction gpio=" << gpio
                  << " requested=in result=" << (ok ? "ok" : "fail") << "\n";
    }
    if (!ok && direction == "out") {
        ok = run_pinctrl(gpio, "op");
    }
    if (direction == "in") {
        run_pinctrl(gpio, "ip pu");
    }
    std::cerr << "[PiGPIO] direction gpio=" << gpio
              << " requested=" << direction
              << " readback=" << read_text_file(gpio_path(gpio, "direction"))
              << " result=" << (ok ? "ok" : "fail") << "\n";
}

void pi_write_gpio(int gpio, int value) {
    const std::string path = gpio_path(gpio, "value");
    bool ok = false;
    if (path_exists(path)) {
        ok = write_text_file(path, value ? "1" : "0");
    }
    if (!ok) {
        ok = gpio_mem_write(gpio, value);
    }
    if (!ok) {
        run_pinctrl(gpio, value ? "dh" : "dl");
    }
}

int pi_read_gpio(int gpio, int fallback) {
    const std::string value = read_text_file(gpio_path(gpio, "value"));
    if (value == "0") return 0;
    if (value == "1") return 1;
    int mem_value = fallback;
    if (gpio_mem_read(gpio, &mem_value)) {
        return mem_value;
    }
    return fallback;
}

PiDisplay::PiDisplay(PiDisplayConfig config) : config_(std::move(config)) {
    initialize();
}

PiDisplay::~PiDisplay() {
    if (ready_) {
        fill_screen(rgb565(0, 0, 0));
        pi_write_gpio(config_.pins.display_backlight, 0);
    }
    if (spi_fd_ >= 0) {
        close(spi_fd_);
    }
}

bool PiDisplay::ready() const {
    return ready_;
}

void PiDisplay::initialize() {
    verbose_spi_ = true;
    std::cerr << "[PiDisplay] config spi_device=" << config_.spi_device
              << " width=" << config_.width
              << " height=" << config_.height
              << " speed_hz=" << config_.speed_hz
              << " cs_gpio=" << config_.pins.display_cs
              << " dc_gpio=" << config_.pins.display_dc
              << " rst_gpio=" << config_.pins.display_reset
              << " bl_gpio=" << config_.pins.display_backlight
              << "\n";
    setup_output(config_.pins.display_dc, 0, "display_dc");
    setup_output(config_.pins.display_reset, 1, "display_reset");
    setup_output(config_.pins.display_backlight, 1, "display_backlight");

    spi_fd_ = open(config_.spi_device.c_str(), O_WRONLY);
    if (spi_fd_ < 0) {
        std::cerr << "[PiDisplay] Could not open " << config_.spi_device << ". Enable SPI first.\n";
        return;
    }

    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = static_cast<uint32_t>(config_.speed_hz);
    const int mode_rc = ioctl(spi_fd_, SPI_IOC_WR_MODE, &mode);
    const int bits_rc = ioctl(spi_fd_, SPI_IOC_WR_BITS_PER_WORD, &bits);
    const int speed_rc = ioctl(spi_fd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    uint8_t mode_read = 0;
    uint8_t bits_read = 0;
    uint32_t speed_read = 0;
    ioctl(spi_fd_, SPI_IOC_RD_MODE, &mode_read);
    ioctl(spi_fd_, SPI_IOC_RD_BITS_PER_WORD, &bits_read);
    ioctl(spi_fd_, SPI_IOC_RD_MAX_SPEED_HZ, &speed_read);
    std::cerr << "[PiDisplay] spi setup mode=0 rc=" << mode_rc
              << " bits=8 rc=" << bits_rc
              << " speed_hz=" << speed
              << " rc=" << speed_rc
              << " readback_mode=" << static_cast<int>(mode_read)
              << " readback_bits=" << static_cast<int>(bits_read)
              << " readback_speed_hz=" << speed_read
              << "\n";

    reset_panel();

    std::cerr << "[PiDisplay] init software reset 0x01\n";
    command(0x01);
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    command(0x28);
    command(0xCF); { const uint8_t v[] = {0x00, 0x83, 0x30}; data(v, sizeof(v)); }
    command(0xED); { const uint8_t v[] = {0x64, 0x03, 0x12, 0x81}; data(v, sizeof(v)); }
    command(0xE8); { const uint8_t v[] = {0x85, 0x01, 0x79}; data(v, sizeof(v)); }
    command(0xCB); { const uint8_t v[] = {0x39, 0x2C, 0x00, 0x34, 0x02}; data(v, sizeof(v)); }
    command(0xF7); data(0x20);
    command(0xEA); { const uint8_t v[] = {0x00, 0x00}; data(v, sizeof(v)); }
    command(0xC0); data(0x26);
    command(0xC1); data(0x11);
    command(0xC5); { const uint8_t v[] = {0x35, 0x3E}; data(v, sizeof(v)); }
    command(0xC7); data(0xBE);
    std::cerr << "[PiDisplay] init memory access control 0x36=0x48\n";
    command(0x36); data(0x48);
    std::cerr << "[PiDisplay] init pixel format 0x3A=0x55\n";
    command(0x3A); data(0x55);
    command(0xB1); { const uint8_t v[] = {0x00, 0x1B}; data(v, sizeof(v)); }
    command(0xF2); data(0x08);
    command(0x26); data(0x01);
    std::cerr << "[PiDisplay] init sleep out 0x11\n";
    command(0x11);
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    std::cerr << "[PiDisplay] init display on 0x29\n";
    command(0x29);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    ready_ = true;
    verbose_spi_ = false;
    fill_screen(rgb565(8, 10, 24));
}

void PiDisplay::reset_panel() {
    std::cerr << "[PiDisplay] hardware reset: high 100ms, low 100ms, high 150ms\n";
    pi_write_gpio(config_.pins.display_reset, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pi_write_gpio(config_.pins.display_reset, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pi_write_gpio(config_.pins.display_reset, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
}

void PiDisplay::command(uint8_t value) {
    if (spi_fd_ < 0) return;
    pi_write_gpio(config_.pins.display_dc, 0);
    const ssize_t written = write(spi_fd_, &value, 1);
    if (verbose_spi_) {
        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "0x%02X", value);
        std::cerr << "[PiDisplay] command " << buffer << " written=" << written << "\n";
    }
}

void PiDisplay::data(const uint8_t* values, size_t length) {
    if (spi_fd_ < 0 || length == 0) return;
    pi_write_gpio(config_.pins.display_dc, 1);
    const ssize_t written = write(spi_fd_, values, length);
    if (verbose_spi_) {
        std::cerr << "[PiDisplay] data length=" << length << " written=" << written;
        const size_t preview = std::min<size_t>(length, 8);
        for (size_t i = 0; i < preview; ++i) {
            char buffer[16];
            std::snprintf(buffer, sizeof(buffer), " 0x%02X", values[i]);
            std::cerr << buffer;
        }
        if (length > preview) {
            std::cerr << " ...";
        }
        std::cerr << "\n";
    }
}

void PiDisplay::data(uint8_t value) {
    data(&value, 1);
}

void PiDisplay::set_window(int x0, int y0, int x1, int y1) {
    x0 = std::clamp(x0, 0, config_.width - 1);
    x1 = std::clamp(x1, 0, config_.width - 1);
    y0 = std::clamp(y0, 0, config_.height - 1);
    y1 = std::clamp(y1, 0, config_.height - 1);

    command(0x2A);
    const uint8_t col[] = {
        static_cast<uint8_t>(x0 >> 8), static_cast<uint8_t>(x0 & 0xFF),
        static_cast<uint8_t>(x1 >> 8), static_cast<uint8_t>(x1 & 0xFF),
    };
    data(col, sizeof(col));

    command(0x2B);
    const uint8_t row[] = {
        static_cast<uint8_t>(y0 >> 8), static_cast<uint8_t>(y0 & 0xFF),
        static_cast<uint8_t>(y1 >> 8), static_cast<uint8_t>(y1 & 0xFF),
    };
    data(row, sizeof(row));
    command(0x2C);
}

void PiDisplay::fill_rect(int x, int y, int w, int h, uint16_t color) {
    if (!ready_ || w <= 0 || h <= 0) return;
    if (verbose_spi_) {
        char color_buffer[16];
        std::snprintf(color_buffer, sizeof(color_buffer), "0x%04X", color);
        std::cerr << "[PiDisplay] fill_rect x=" << x
                  << " y=" << y
                  << " w=" << w
                  << " h=" << h
                  << " color=" << color_buffer
                  << "\n";
    }
    set_window(x, y, x + w - 1, y + h - 1);
    const uint8_t hi = static_cast<uint8_t>(color >> 8);
    const uint8_t lo = static_cast<uint8_t>(color & 0xFF);
    std::vector<uint8_t> chunk(4096);
    for (size_t i = 0; i < chunk.size(); i += 2) {
        chunk[i] = hi;
        chunk[i + 1] = lo;
    }
    pi_write_gpio(config_.pins.display_dc, 1);
    int pixels = w * h;
    while (pixels > 0) {
        const int send_pixels = std::min<int>(pixels, static_cast<int>(chunk.size() / 2));
        const ssize_t written = write(spi_fd_, chunk.data(), send_pixels * 2);
        if (verbose_spi_) {
            std::cerr << "[PiDisplay] pixel_data bytes=" << send_pixels * 2
                      << " written=" << written << "\n";
        }
        pixels -= send_pixels;
    }
}

void PiDisplay::fill_screen(uint16_t color) {
    fill_rect(0, 0, config_.width, config_.height, color);
}

uint16_t PiDisplay::rgb565(uint8_t r, uint8_t g, uint8_t b) const {
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

void PiDisplay::draw_char(int x, int y, char c, uint16_t color, int scale) {
    const auto glyph = glyph_for(c);
    for (size_t col = 0; col < glyph.size(); ++col) {
        for (int row = 0; row < 7; ++row) {
            if ((glyph[col] >> row) & 0x01) {
                fill_rect(x + static_cast<int>(col) * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

void PiDisplay::draw_text(int x, int y, const std::string& text, uint16_t color, int scale) {
    int cursor = x;
    for (char c : text) {
        if (cursor + 6 * scale >= config_.width) {
            break;
        }
        draw_char(cursor, y, c, color, scale);
        cursor += 6 * scale;
    }
}

void PiDisplay::draw_line(int x0, int y0, int x1, int y1, uint16_t color) {
    const int dx = std::abs(x1 - x0);
    const int sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0);
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        fill_rect(x0, y0, 2, 2, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void PiDisplay::draw_checkerboard(int cell) {
    const uint16_t dark = rgb565(8, 10, 24);
    const uint16_t light = rgb565(232, 232, 220);
    for (int y = 0; y < config_.height; y += cell) {
        for (int x = 0; x < config_.width; x += cell) {
            const bool odd = ((x / cell) + (y / cell)) % 2 != 0;
            fill_rect(x, y, cell, cell, odd ? light : dark);
        }
    }
}

void PiDisplay::run_solid_color_loop() {
    if (!ready_) {
        std::cerr << "[PiDisplay] solid color loop failed: display not ready\n";
        return;
    }

    verbose_spi_ = true;
    std::cerr << "[PiDisplay] entering infinite solid color loop: red 5s, green 5s, blue 5s\n";
    while (true) {
        std::cerr << "[PiDisplay] solid red begin\n";
        fill_screen(rgb565(255, 0, 0));
        std::this_thread::sleep_for(std::chrono::seconds(5));

        std::cerr << "[PiDisplay] solid green begin\n";
        fill_screen(rgb565(0, 255, 0));
        std::this_thread::sleep_for(std::chrono::seconds(5));

        std::cerr << "[PiDisplay] solid blue begin\n";
        fill_screen(rgb565(0, 0, 255));
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void PiDisplay::run_diagnostic_pattern() {
    if (!ready_) {
        std::cerr << "[PiDisplay] diagnostic pattern failed: display not ready\n";
        return;
    }

    const auto hold = [](int milliseconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    };
    const uint16_t white = rgb565(255, 255, 255);
    const uint16_t black = rgb565(0, 0, 0);
    const uint16_t indigo = rgb565(2, 1, 38);
    const uint16_t lavender = rgb565(91, 73, 138);
    const uint16_t sienna = rgb565(89, 32, 4);

    fill_screen(rgb565(255, 0, 0));
    std::cout << "[PiDisplay] pattern solid red\n";
    hold(700);
    fill_screen(rgb565(0, 255, 0));
    std::cout << "[PiDisplay] pattern solid green\n";
    hold(700);
    fill_screen(rgb565(0, 0, 255));
    std::cout << "[PiDisplay] pattern solid blue\n";
    hold(700);
    fill_screen(white);
    std::cout << "[PiDisplay] pattern solid white\n";
    hold(700);
    fill_screen(black);
    std::cout << "[PiDisplay] pattern solid black\n";
    hold(700);

    draw_checkerboard(16);
    draw_text(12, 144, "CHECKERBOARD", sienna, 2);
    std::cout << "[PiDisplay] pattern checkerboard\n";
    hold(1200);

    fill_screen(indigo);
    for (int offset = -config_.height; offset < config_.width; offset += 24) {
        draw_line(std::max(0, offset), std::max(0, -offset),
                  std::min(config_.width - 1, offset + config_.height - 1),
                  std::min(config_.height - 1, config_.height - 1 - std::max(0, offset - config_.width + 1)),
                  lavender);
    }
    draw_line(0, 0, config_.width - 1, config_.height - 1, sienna);
    draw_line(config_.width - 1, 0, 0, config_.height - 1, sienna);
    draw_text(14, 18, "DIAGONAL LINES", white, 2);
    std::cout << "[PiDisplay] pattern diagonal lines\n";
    hold(1400);

    fill_screen(indigo);
    draw_text(18, 28, "SHAER TFT TEST", white, 2);
    draw_text(18, 58, "ILI9341 SPI", lavender, 2);
    draw_text(18, 88, "TEXT RENDERING", sienna, 2);
    draw_text(18, 118, "240 X 320", white, 2);
    std::cout << "[PiDisplay] pattern text rendering\n";
    hold(1400);

    const auto started = std::chrono::steady_clock::now();
    int frames = 0;
    while (std::chrono::steady_clock::now() - started < std::chrono::seconds(3)) {
        fill_rect(0, 0, config_.width, 32, indigo);
        fill_rect((frames * 11) % config_.width, 44, 20, 20, frames % 2 == 0 ? sienna : lavender);
        draw_text(8, 8, "FPS " + std::to_string(frames), white, 2);
        ++frames;
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count();
    const int fps = elapsed > 0 ? static_cast<int>((frames * 1000LL) / elapsed) : 0;
    fill_screen(indigo);
    draw_text(16, 40, "FPS COUNTER", white, 2);
    draw_text(16, 76, "FPS " + std::to_string(fps), lavender, 3);
    std::cout << "[PiDisplay] pattern fps=" << fps << "\n";
    hold(1400);

    fill_screen(indigo);
    fill_rect(24, 48, config_.width - 48, 104, lavender);
    fill_rect(30, 54, config_.width - 60, 92, indigo);
    draw_text(47, 78, "SHAER", white, 4);
    draw_text(35, 166, "ADI VASI OS", sienna, 2);
    draw_text(30, 198, "DISPLAY TEST PASS", white, 2);
    draw_text(42, 232, "SPI TFT ONLINE", lavender, 2);
    std::cout << "[PiDisplay] pattern SHAeR logo\n";
    hold(8000);
}

uint16_t PiDisplay::color_for(UiColor color) const {
    return rgb565(color.r, color.g, color.b);
}

void PiDisplay::draw_icon(const UiCommand& command) {
    const uint16_t fg = color_for(command.fg);
    if (command.selected) {
        fill_rect(command.rect.x - 3, command.rect.y - 3, command.rect.w + 6, command.rect.h + 6, fg);
        draw_text(command.rect.x, command.rect.y + 4, command.text, rgb565(5, 8, 18), 1);
        return;
    }
    fill_rect(command.rect.x, command.rect.y, command.rect.w, command.rect.h, rgb565(16, 22, 36));
    fill_rect(command.rect.x, command.rect.y, command.rect.w, 1, fg);
    fill_rect(command.rect.x, command.rect.y + command.rect.h - 1, command.rect.w, 1, fg);
    fill_rect(command.rect.x, command.rect.y, 1, command.rect.h, fg);
    fill_rect(command.rect.x + command.rect.w - 1, command.rect.y, 1, command.rect.h, fg);
    draw_text(command.rect.x + 3, command.rect.y + 5, command.text, fg, 1);
}

void PiDisplay::draw_image(const UiCommand& command) {
    if (!ready_ || command.rect.w <= 0 || command.rect.h <= 0 || command.text.empty()) {
        return;
    }

    std::ifstream file(command.text, std::ios::binary);
    if (!file) {
        std::cerr << "[PiDisplay] image missing path=" << command.text << "\n";
        return;
    }

    const int x = std::clamp(command.rect.x, 0, config_.width - 1);
    const int y = std::clamp(command.rect.y, 0, config_.height - 1);
    const int w = std::clamp(command.rect.w, 1, config_.width - x);
    const int h = std::clamp(command.rect.h, 1, config_.height - y);
    const size_t expected_bytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 2;

    file.seekg(0, std::ios::end);
    const std::streamoff file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (file_size < static_cast<std::streamoff>(expected_bytes)) {
        std::cerr << "[PiDisplay] image too small path=" << command.text
                  << " bytes=" << file_size
                  << " expected=" << expected_bytes << "\n";
        return;
    }

    set_window(x, y, x + w - 1, y + h - 1);
    pi_write_gpio(config_.pins.display_dc, 1);

    std::vector<uint8_t> chunk(4096);
    size_t remaining = expected_bytes;
    while (remaining > 0 && file) {
        const size_t wanted = std::min(remaining, chunk.size());
        file.read(reinterpret_cast<char*>(chunk.data()), static_cast<std::streamsize>(wanted));
        const std::streamsize got = file.gcount();
        if (got <= 0) {
            break;
        }
        const ssize_t written = write(spi_fd_, chunk.data(), static_cast<size_t>(got));
        if (written < 0) {
            std::cerr << "[PiDisplay] image write failed path=" << command.text
                      << " errno=" << errno
                      << " error=" << std::strerror(errno) << "\n";
            return;
        }
        remaining -= static_cast<size_t>(got);
    }

    if (remaining != 0) {
        std::cerr << "[PiDisplay] image short read path=" << command.text
                  << " remaining=" << remaining << "\n";
    }
}

void PiDisplay::draw_progress(const UiCommand& command) {
    fill_rect(command.rect.x, command.rect.y, command.rect.w, command.rect.h, color_for(command.bg));
    const int max_value = std::max(1, command.max_value);
    const int filled = std::clamp(command.rect.w * std::clamp(command.value, 0, max_value) / max_value, 0, command.rect.w);
    fill_rect(command.rect.x, command.rect.y, filled, command.rect.h, color_for(command.fg));
}

void PiDisplay::draw_command(const UiCommand& command) {
    switch (command.type) {
        case UiCommandType::Rect:
            fill_rect(command.rect.x, command.rect.y, command.rect.w, command.rect.h, color_for(command.fg));
            break;
        case UiCommandType::Text:
            draw_text(command.rect.x, command.rect.y, command.text, color_for(command.fg), command.scale);
            break;
        case UiCommandType::Icon:
            draw_icon(command);
            break;
        case UiCommandType::Progress:
            draw_progress(command);
            break;
        case UiCommandType::Transition:
            fill_rect(command.rect.x, command.rect.y, command.rect.w, command.rect.h, color_for(command.fg));
            break;
        case UiCommandType::Image:
            draw_image(command);
            break;
    }
}

void PiDisplay::render(const RenderModel& model) {
    if (!ready_) {
        if (!warned_) {
            std::cerr << "[PiDisplay] Display not ready; continuing headless.\n";
            warned_ = true;
        }
        return;
    }

    const UiFrame frame = ui_.build_frame(model);
    for (const auto& command : frame.commands) {
        draw_command(command);
    }

    std::cout << "[PiDisplay] ui-frame " << frame.commands.size() << " commands / "
              << to_string(model.firmware_state)
              << " / " << to_string(model.screen)
              << " / battery " << model.battery_percent << "%\n";
}

void PiAudioOutput::play_local(const Track& track) {
    if (track.file_path.empty()) {
        return;
    }
    stop();
    paused_ = false;
    const std::string path = shell_quote(track.file_path);
    const std::string command =
        "(mpg123 -q " + path +
        " || flac -cd " + path + " | aplay -q" +
        " || aplay -q " + path +
        ") >/tmp/shaer_audio.log 2>&1 &";
    std::system(command.c_str());
}

void PiAudioOutput::play_spotify(const Track&) {
    std::system("systemctl start shaer-spotify.service >/tmp/shaer_spotify_start.log 2>&1 || true");
}

void PiAudioOutput::set_volume(int volume) {
    volume_ = std::clamp(volume, 0, 100);
    const std::string value = std::to_string(volume_) + "%";
    const std::string command =
        "amixer -q sset Master " + value + " >/var/log/shaer/audio.log 2>&1" +
        " || amixer -q sset PCM " + value + " >>/var/log/shaer/audio.log 2>&1" +
        " || amixer -q sset Digital " + value + " >>/var/log/shaer/audio.log 2>&1" +
        " || true";
    std::system(command.c_str());
}

void PiAudioOutput::stop() {
    std::system("pkill -f 'ffplay -nodisp -autoexit' >/dev/null 2>&1 || true");
    std::system("pkill -f 'mpg123 -q' >/dev/null 2>&1 || true");
    std::system("pkill -f 'flac -cd' >/dev/null 2>&1 || true");
    std::system("pkill -f 'aplay -q' >/dev/null 2>&1 || true");
}

void PiAudioOutput::pause() {
    stop();
    paused_ = true;
}

void PiAudioOutput::resume() {
    paused_ = false;
}

void PiAudioOutput::seek_seconds(int seconds) {
    std::ofstream log("/tmp/shaer_audio.log", std::ios::app);
    log << "seek_seconds requested=" << seconds << " decoder=process-restart-required\n";
}

void PiAudioOutput::prebuffer_local(const Track& track) {
    std::ofstream log("/tmp/shaer_audio.log", std::ios::app);
    log << "prebuffer requested path=" << track.file_path
        << " crossfade_seconds=" << crossfade_seconds_ << "\n";
}

void PiAudioOutput::set_crossfade_seconds(int seconds) {
    crossfade_seconds_ = std::clamp(seconds, 0, 5);
}

int PiBattery::percent() const {
    std::array<uint8_t, 2> soc{};
    if (!read_max17048_register(0x04, &soc)) {
        return 87;
    }
    const double percent = static_cast<double>(soc[0]) + static_cast<double>(soc[1]) / 256.0;
    return std::clamp(static_cast<int>(percent + 0.5), 0, 100);
}

bool PiBattery::is_charging() const {
    DIR* dir = opendir("/sys/class/power_supply");
    if (!dir) {
        return false;
    }
    bool charging = false;
    while (dirent* entry = readdir(dir)) {
        if (entry->d_name[0] == '.') continue;
        const std::string status = read_text_file(std::string("/sys/class/power_supply/") + entry->d_name + "/status");
        if (status == "Charging") {
            charging = true;
            break;
        }
    }
    closedir(dir);
    return charging;
}

bool PiBluetooth::connected() const {
    return std::system("bluetoothctl info 2>/dev/null | grep -q 'Connected: yes'") == 0;
}

PiInput::PiInput(PiPinMap pins) : pins_(pins) {
    setup_input(pins_.power_button);
    setup_input(pins_.encoder_a);
    setup_input(pins_.encoder_b);
    setup_input(pins_.encoder_push);
    setup_input(pins_.button_back);
    setup_input(pins_.button_play);
    setup_input(pins_.button_options);
    last_a_ = pi_read_gpio(pins_.encoder_a, 1);
    last_b_ = pi_read_gpio(pins_.encoder_b, 1);
}

bool PiInput::pressed(int gpio) const {
    return pi_read_gpio(gpio, 1) == 0;
}

InputAction PiInput::next_action() {
    while (true) {
        if (auto action = poll_action(); action != InputAction::None) return action;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

InputAction PiInput::poll_action() {
    if (auto action = scan_power(); action != InputAction::None) return action;
    if (auto action = scan_buttons(); action != InputAction::None) return action;
    if (auto action = scan_encoder(); action != InputAction::None) return action;
    return InputAction::None;
}

InputAction PiInput::scan_buttons() {
    const bool confirm = pressed(pins_.encoder_push);
    const bool back = pressed(pins_.button_back);
    const bool play = pressed(pins_.button_play);
    const bool options = pressed(pins_.button_options);
    const auto now = std::chrono::steady_clock::now();

    if (confirm && !confirm_latched_) {
        confirm_latched_ = true;
        confirm_long_sent_ = false;
        confirm_down_at_ = now;
        return InputAction::None;
    }
    if (confirm && confirm_latched_ && !confirm_long_sent_) {
        const auto held = std::chrono::duration_cast<std::chrono::milliseconds>(now - confirm_down_at_).count();
        if (held >= 700) {
            confirm_long_sent_ = true;
            return InputAction::Back;
        }
    }
    if (!confirm && confirm_latched_) {
        confirm_latched_ = false;
        if (!confirm_long_sent_) {
            return InputAction::Confirm;
        }
    }
    if (back && !back_latched_) { back_latched_ = true; return InputAction::Back; }
    if (!back) back_latched_ = false;
    if (play && !play_latched_) { play_latched_ = true; return InputAction::PlayPause; }
    if (!play) play_latched_ = false;
    if (options && !options_latched_) { options_latched_ = true; return InputAction::OpenSettings; }
    if (!options) options_latched_ = false;
    return InputAction::None;
}

InputAction PiInput::scan_encoder() {
    const int a = pi_read_gpio(pins_.encoder_a, last_a_);
    const int b = pi_read_gpio(pins_.encoder_b, last_b_);
    InputAction action = InputAction::None;
    if (a != last_a_ && a == 0) {
        action = (b != a) ? InputAction::Down : InputAction::Up;
    }
    last_a_ = a;
    last_b_ = b;
    return action;
}

InputAction PiInput::scan_power() {
    const bool down = pressed(pins_.power_button);
    const auto now = std::chrono::steady_clock::now();

    if (down && !power_latched_) {
        power_latched_ = true;
        power_down_at_ = now;
        return InputAction::None;
    }

    if (!down && power_latched_) {
        power_latched_ = false;
        const auto held = std::chrono::duration_cast<std::chrono::milliseconds>(now - power_down_at_).count();
        if (held >= 1500) {
            return InputAction::BeginShutdown;
        }
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_power_release_).count() < 500) {
            power_press_count_++;
        } else {
            power_press_count_ = 1;
        }
        last_power_release_ = now;
        if (power_press_count_ >= 2) {
            power_press_count_ = 0;
            return InputAction::EnterSleep;
        }
        return InputAction::ToggleBatterySaver;
    }

    return InputAction::None;
}

}  // namespace shaer
