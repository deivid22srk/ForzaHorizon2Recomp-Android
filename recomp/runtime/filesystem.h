// filesystem.h — game-data I/O rooted at the SAF-picked folder.
#pragma once
#include <string>

namespace fh2 {

// Set once from JNI (SAF tree URI string or raw POSIX path).
void FS_SetGameRoot(const std::string& root);
std::string FS_GameRoot();
// Resolve "media/cars/..." under the game root. Returns "" if root unset.
std::string FS_Resolve(const std::string& relative);
// True if the folder looks like an FH2 dump (has media/ or default.xex nearby).
bool FS_ValidateGameFolder(std::string* why_not);

} // namespace fh2
