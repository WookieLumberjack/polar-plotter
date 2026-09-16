#include "ppm_writer.hpp"

#include <fstream>
#include <ios>

namespace app {

bool write_ppm(const std::filesystem::path& path, int width, int height,
               const std::vector<std::uint8_t>& rgb) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }
    out << "P6\n" << width << ' ' << height << "\n255\n";
    out.write(reinterpret_cast<const char*>(rgb.data()), static_cast<std::streamsize>(rgb.size()));
    return out.good();
}

}  // namespace app
