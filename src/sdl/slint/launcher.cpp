#include "launcher.h"
#include "launcher_api.h"
#include "launcher_config.h"
#include "launcher_paths.h"

#include <nfd.h>
#include <slint.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>

namespace {

namespace fs = std::filesystem;

using namespace wc1_launcher;

void refresh_window(LauncherWindow &window, bool swcRequested)
{
    const std::string directory(window.get_game_directory());
    const DirectoryStatus status = validate_game_directory(directory, swcRequested);

    window.set_picker_error(false);
    window.set_path_valid(status.valid);
    window.set_swc_demo(status.swcDemo);
    window.set_status_text(slint::SharedString(status.message));
}

void report_picker_error(LauncherWindow &window, std::string message,
                         const char *details)
{
    if (details != nullptr && details[0] != '\0') {
        message += ": ";
        message += details;
    }
    window.set_picker_error(true);
    window.set_status_text(slint::SharedString(message));
}

void browse_for_game_directory(LauncherWindow &window, bool swcRequested)
{
    const std::string currentDirectory(window.get_game_directory());
    std::string defaultDirectory;
    std::error_code error;

    if (!currentDirectory.empty()) {
        const fs::path candidate = path_from_utf8(currentDirectory);
        if (fs::is_directory(candidate, error)) {
            const fs::path absolute = fs::absolute(candidate, error);
            if (!error)
                defaultDirectory = path_to_utf8(absolute);
        }
    }

    nfdu8char_t *selectedDirectory = nullptr;
    const nfdresult_t result = NFD_PickFolderU8(
        &selectedDirectory,
        defaultDirectory.empty() ? nullptr : defaultDirectory.c_str());
    if (result == NFD_CANCEL)
        return;
    if (result != NFD_OKAY || selectedDirectory == nullptr) {
        report_picker_error(window, "The folder picker failed",
                            NFD_GetError());
        return;
    }

    const std::string selection(selectedDirectory);
    NFD_FreePathU8(selectedDirectory);
    window.set_game_directory(slint::SharedString(selection));
    refresh_window(window, swcRequested);
}

} // namespace

int SdlRunLauncherGui(SdlLauncherOptions *options)
{
    bool accepted;
    bool pickerInitialized;

    if (options == nullptr)
        return SDL_LAUNCHER_ERROR;
    const bool swcRequested = options->swcDemo != 0;
    const auto configPath = launcher_config_path();
    load_launcher_config(configPath, *options);

    if (options->gameDirectory[0] == '\0') {
        std::error_code error;
        const auto currentDirectory = fs::current_path(error);
        const std::string initialDirectory =
            error ? "." : path_to_utf8(currentDirectory);
        if (initialDirectory.size() >= sizeof(options->gameDirectory))
            return SDL_LAUNCHER_ERROR;
        std::memcpy(options->gameDirectory, initialDirectory.c_str(),
                    initialDirectory.size() + 1);
    }

    auto window = LauncherWindow::create();
    accepted = false;
    pickerInitialized = false;
    window->set_game_directory(slint::SharedString(options->gameDirectory));
    window->set_enhanced_renderer(options->enhancedRenderer != 0);
    window->set_ega_dither(options->egaDither != 0);
    window->set_cockpitless_view(options->cockpitlessView != 0);
    window->set_joystick_rumble(options->joystickRumble != 0);
    window->set_joystick_mode_index(options->joystickMode);
    window->set_joystick_axes_index(options->joystickAxes);
    refresh_window(*window, swcRequested);

    window->on_configuration_changed([&window, swcRequested]() {
        refresh_window(*window, swcRequested);
    });
    window->on_launch_requested([&window, &accepted, swcRequested]() {
        refresh_window(*window, swcRequested);
        if (!window->get_path_valid())
            return;
        std::error_code error;
        const std::string directory(window->get_game_directory());
        const auto absoluteDirectory = fs::absolute(path_from_utf8(directory), error);
        if (error) {
            report_picker_error(*window, "Cannot resolve the game directory",
                                error.message().c_str());
            return;
        }
        const auto absoluteText = path_to_utf8(absoluteDirectory);
        if (absoluteText.size() >= SDL_LAUNCHER_DIRECTORY_CAPACITY) {
            report_picker_error(*window, "The game directory path is too long", nullptr);
            return;
        }
        fs::current_path(absoluteDirectory, error);
        if (error) {
            report_picker_error(*window, "Cannot open the game directory",
                                error.message().c_str());
            return;
        }
        window->set_game_directory(slint::SharedString(absoluteText));
        accepted = true;
        slint::quit_event_loop();
    });
    window->on_cancel_requested([]() {
        slint::quit_event_loop();
    });

    window->show();
    /* NFD_Init records Cocoa's activation policy. Initialize it from the
     * callback, after winit launches NSApplication, so NFD_Quit cannot restore
     * the temporary pre-launch Prohibited policy before SDL takes over. */
    window->on_browse_requested([&window, &pickerInitialized, swcRequested]() {
        if (!pickerInitialized) {
            if (NFD_Init() != NFD_OKAY) {
                report_picker_error(
                    *window, "The folder picker could not be initialized",
                    NFD_GetError());
                return;
            }
            pickerInitialized = true;
        }
        browse_for_game_directory(*window, swcRequested);
    });
    slint::run_event_loop();
    if (pickerInitialized)
        NFD_Quit();
    window->hide();
    if (!accepted)
        return SDL_LAUNCHER_CANCELLED;

    const std::string gameDirectory(window->get_game_directory());
    if (gameDirectory.size() >= sizeof(options->gameDirectory))
        return SDL_LAUNCHER_ERROR;
    std::memcpy(options->gameDirectory, gameDirectory.c_str(),
                gameDirectory.size() + 1);
    options->enhancedRenderer = window->get_enhanced_renderer();
    options->egaDither = window->get_ega_dither();
    options->cockpitlessView = window->get_cockpitless_view();
    options->joystickRumble = window->get_joystick_rumble();
    options->joystickMode = std::max(0, window->get_joystick_mode_index());
    options->joystickAxes = std::max(0, window->get_joystick_axes_index());
    options->swcDemo = window->get_swc_demo();
    if (!configPath.empty() && !save_launcher_config(configPath, *options))
        std::fprintf(stderr, "Cannot save launcher settings: %s\n",
                     path_to_utf8(configPath).c_str());
    return SDL_LAUNCHER_ACCEPTED;
}
