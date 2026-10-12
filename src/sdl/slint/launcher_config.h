#ifndef WC1_SDL_SLINT_LAUNCHER_CONFIG_H
#define WC1_SDL_SLINT_LAUNCHER_CONFIG_H

#include "launcher_api.h"

#include <filesystem>

namespace wc1_launcher {

std::filesystem::path launcher_config_path();
void load_launcher_config(const std::filesystem::path &path,
                          SdlLauncherOptions &options);
bool save_launcher_config(const std::filesystem::path &path,
                          const SdlLauncherOptions &options);

} // namespace wc1_launcher

#endif
