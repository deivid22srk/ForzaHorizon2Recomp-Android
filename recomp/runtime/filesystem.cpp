#include "filesystem.h"
#include <sys/stat.h>

namespace fh2 {
namespace {
std::string g_root;
std::atomic<bool> g_tree_validated{false};
std::string g_tree_label;
} // namespace

void FS_SetGameRoot(const std::string& root) {
    if (root != g_root) {
        g_root = root;
        // A new root invalidates any previous content:// validation.
        if (root.rfind("content://", 0) != 0) g_tree_validated.store(false);
        g_tree_label.clear();
    }
}
std::string FS_GameRoot() { return g_root; }

void FS_SetTreeValidated(bool ok, const std::string& label) {
    g_tree_validated.store(ok);
    g_tree_label = ok ? label : "";
}
bool FS_TreeValidated() { return g_tree_validated.load(); }
std::string FS_TreeLabel() { return g_tree_label; }

std::string FS_Resolve(const std::string& relative) {
    if (g_root.empty()) return "";
    // content:// trees are served by Java (ContentResolver/DocumentsContract);
    // POSIX open/stat on them is meaningless, so never fabricate a path.
    if (g_root.rfind("content://", 0) == 0) return "";
    std::string r = g_root;
    if (!r.empty() && r.back() != '/') r += '/';
    return r + relative;
}

static bool exists(const std::string& p) {
    if (p.empty()) return false;
    struct stat st {};
    return ::stat(p.c_str(), &st) == 0;
}

bool FS_ValidateGameFolder(std::string* why_not) {
    if (g_root.empty()) { if (why_not) *why_not = "no folder selected"; return false; }
    if (g_root.rfind("content://", 0) == 0) {
        if (g_tree_validated.load()) return true;
        if (why_not) *why_not = "SAF folder not validated yet";
        return false;
    }
    if (exists(FS_Resolve("media")) || exists(FS_Resolve("Media"))) return true;
    if (exists(FS_Resolve("default.xex"))) return true;
    if (exists(g_root)) { if (why_not) *why_not = "folder lacks media/ (pick the game root)"; return false; }
    if (why_not) *why_not = "path not accessible";
    return false;
}

} // namespace fh2
