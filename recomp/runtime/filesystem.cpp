#include "filesystem.h"
#include <sys/stat.h>

namespace fh2 {
namespace { std::string g_root; }

void FS_SetGameRoot(const std::string& root) { g_root = root; }
std::string FS_GameRoot() { return g_root; }

std::string FS_Resolve(const std::string& relative) {
    if (g_root.empty()) return "";
    // SAF tree URIs (content://) cannot be opened via POSIX; the Java layer
    // resolves them through ContentResolver and mirrors to cache when needed.
    // Here we handle plain paths; content:// is passed through for Java I/O.
    if (g_root.rfind("content://", 0) == 0) return g_root + "/" + relative;
    std::string r = g_root;
    if (!r.empty() && r.back() != '/') r += '/';
    return r + relative;
}

static bool exists(const std::string& p) {
    struct stat st {};
    return ::stat(p.c_str(), &st) == 0;
}

bool FS_ValidateGameFolder(std::string* why_not) {
    if (g_root.empty()) { if (why_not) *why_not = "no folder selected"; return false; }
    if (g_root.rfind("content://", 0) == 0) return true; // validated in Java
    if (exists(FS_Resolve("media")) || exists(FS_Resolve("Media"))) return true;
    if (exists(FS_Resolve("default.xex"))) return true;
    if (exists(g_root)) { if (why_not) *why_not = "folder lacks media/ (pick the game root)"; return false; }
    if (why_not) *why_not = "path not accessible";
    return false;
}

} // namespace fh2
