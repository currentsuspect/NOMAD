#pragma once

/**
 * Compatibility header for NUIWindowWin32
 * 
 * This makes NUIPlatformBridge a drop-in replacement for the old NUIWindowWin32.
 * Existing code using NUIWindowWin32 will automatically use the new NomadPlat-based
 * implementation without any changes.
 */

#include "../NUIPlatformBridge.h"

namespace NomadUI {

// Typedef for backward compatibility
using NUIWindowWin32 = NUIPlatformBridge;

} // namespace NomadUI
