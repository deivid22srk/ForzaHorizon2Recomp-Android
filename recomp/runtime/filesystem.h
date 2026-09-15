// filesystem.h — game-data I/O rooted at the SAF-picked folder.
#pragma once
#include <atomic>
#include <string>

namespace fh2 {

// Set once from JNI (SAF tree URI string or raw POSIX path).
void FS_SetGameRoot(const std::string& root);
std::string FS_GameRoot();
// Set from Java after DocumentsContract validation of a content:// tree.
// content:// roots are never trusted blindly (validated flag + label).
void FS_SetTreeValidated(bool ok, const std::string& label);
bool FS_TreeValidated();
std::string FS_TreeLabel();
// Resolve "media/cars/..." under the game root. Returns "" if root unset.
std::string FS_Resolve(const std::string& relative);
// True if the folder looks like an FH2 dump (has media/ or default.xex nearby).
bool FS_ValidateGameFolder(std::string* why_not);

} // namespace fh2
