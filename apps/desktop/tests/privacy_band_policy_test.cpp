#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "privacy_band_ipc.h"
#include "privacy_window_visibility.h"

namespace {

std::filesystem::path FindRepoRoot() {
  std::filesystem::path current = std::filesystem::current_path();
  while (!current.empty()) {
    if (std::filesystem::exists(current / "xmake.lua") &&
        std::filesystem::exists(
            current / "apps/desktop/src/platform/windows/privacy/"
                      "privacy_backend_win.cpp")) {
      return current;
    }
    current = current.parent_path();
  }
  return {};
}

std::string ReadFile(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) return {};
  std::ostringstream stream;
  stream << file.rdbuf();
  return stream.str();
}

bool Expect(const char* name, bool value) {
  if (value) return true;
  std::cerr << name << " failed\n";
  return false;
}

bool ExpectContains(const char* name, const std::string& value,
                    const std::string& expected) {
  if (value.find(expected) != std::string::npos) return true;
  std::cerr << name << " missing expected text: " << expected << "\n";
  return false;
}

bool ExpectNotContains(const char* name, const std::string& value,
                       const std::string& unexpected) {
  if (value.find(unexpected) == std::string::npos) return true;
  std::cerr << name << " contains unexpected text: " << unexpected << "\n";
  return false;
}

}  // namespace

int main() {
  const std::filesystem::path repo_root = FindRepoRoot();
  if (repo_root.empty()) {
    std::cerr << "failed to locate repository root\n";
    return 1;
  }

  const std::string backend = ReadFile(
      repo_root / "apps/desktop/src/platform/windows/privacy/"
                  "privacy_backend_win.cpp");
  const std::string visibility = ReadFile(
      repo_root / "apps/desktop/src/platform/windows/privacy/"
                  "privacy_window_visibility.h");

  bool ok = true;

  // The cover band is the only threshold that matters. A window below it can
  // never paint above the cover regardless of its place in the HWND list.
  ok &= Expect("band 0 is below the cover",
               !crossdesk::IsBandAbovePrivacyCover(0, crossdesk::kPrivacyWindowBand));
  ok &= Expect("band 1 is below the cover",
               !crossdesk::IsBandAbovePrivacyCover(1, crossdesk::kPrivacyWindowBand));
  // Regression: an IME status bar reported band 2 and tore privacy down.
  ok &= Expect("band 2 is below the cover",
               !crossdesk::IsBandAbovePrivacyCover(2, crossdesk::kPrivacyWindowBand));
  ok &= Expect("band 17 is below the cover",
               !crossdesk::IsBandAbovePrivacyCover(17,
                                                   crossdesk::kPrivacyWindowBand));
  ok &= Expect("the cover band itself counts",
               crossdesk::IsBandAbovePrivacyCover(crossdesk::kPrivacyWindowBand,
                                                  crossdesk::kPrivacyWindowBand));
  ok &= Expect("above the cover band counts",
               crossdesk::IsBandAbovePrivacyCover(
                   crossdesk::kPrivacyWindowBand + 1, crossdesk::kPrivacyWindowBand));

  // The backend must use the shared predicate, not its own band literals.
  ok &= ExpectContains("privacy_backend_win.cpp", backend,
                       "IsBandAbovePrivacyCover(above_band, kPrivacyWindowBand)");
  ok &= ExpectNotContains("privacy_backend_win.cpp", backend,
                          "above_band == 0 || above_band == 1");
  ok &= ExpectContains("privacy_window_visibility.h", visibility,
                       "inline bool IsBandAbovePrivacyCover(");

  // A single compositor frame of a transient overlay must not release privacy
  // for the rest of the session, so any obstruction needs a confirmation delay.
  ok &= ExpectContains("privacy_backend_win.cpp", backend,
                       "kObstructionConfirmMs");
  ok &= ExpectContains("privacy_backend_win.cpp", backend,
                       "if (now - obstruction_since_ < kObstructionConfirmMs)");
  ok &= ExpectContains("privacy_backend_win.cpp", backend,
                       "obstruction_since_ = 0;");

  if (!ok) {
    std::cerr << "privacy band policy test failed\n";
    return 1;
  }
  std::cout << "privacy band policy test passed\n";
  return 0;
}
