#include "huynity/adapters/window/window_factory.h"

#include "huynity/adapters/window/headless_window.h"

#if defined(HUYNITY_HAS_NATIVE_WINDOW)
#include "native_window.h"
#endif

#include <exception>
#include <iostream>

namespace huynity::adapters::window {

bool hasNativeWindowSupport() noexcept {
#if defined(HUYNITY_HAS_NATIVE_WINDOW)
    return true;
#else
    return false;
#endif
}

std::unique_ptr<WindowAdapter> createWindow(const WindowConfig& config) {
    if (config.forceHeadless || !hasNativeWindowSupport()) {
        return std::make_unique<HeadlessWindow>(config.size, config.title);
    }

#if defined(HUYNITY_HAS_NATIVE_WINDOW)
    try {
        return std::make_unique<NativeWindow>(config.size, config.title);
    } catch (const std::exception& error) {
        std::cerr << "Native window initialization failed: " << error.what() << "; using HeadlessWindow fallback\n";
    }
#endif

    return std::make_unique<HeadlessWindow>(config.size, config.title);
}

} // namespace huynity::adapters::window
