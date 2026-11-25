/*
 * HarmonyOS clipboard backend for the Slint desktop port.
 *
 * Text clipboard operations go through the ArkTS bridge: services.cpp exposes
 * SDL_HasClipboardText / SDL_GetClipboardText / SDL_SetClipboardText, which
 * forward clipboardRead / clipboardWrite / clipboardHas to DesktopHost.ets and
 * therefore to @kit.BasicServicesKit pasteboard. Nothing here talks to a
 * platform clipboard API directly.
 *
 * File clipboard (promise/URI list) has no HarmonyOS pasteboard equivalent in
 * this port: the system pasteboard only carries MIME text and plain entries, so
 * file offers report UnavailableRuntime instead of pretending to publish.
 */

#include "clipboard.h"

#include <algorithm>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "SDL3/SDL.h"
#include "platform/clipboard_backend.h"
#include "rd_log.h"

namespace crossdesk {
namespace {

// Wake granularity of the polling monitor, so StopMonitoring joins quickly.
constexpr int kPollSliceMs = 20;

bool WarnFileClipboardUnsupported(const char* operation) {
  static bool warned = false;
  if (!warned) {
    warned = true;
    LOG_WARN("Clipboard {}: HarmonyOS pasteboard carries text only, file clipboard is unavailable",
             operation);
  }
  return false;
}

}  // namespace

std::string Clipboard::GetText() {
  char* text = SDL_GetClipboardText();
  if (!text) return {};
  std::string result(text);
  SDL_free(text);
  return result;
}

bool Clipboard::SetText(const std::string& text) { return SDL_SetClipboardText(text.c_str()); }

bool Clipboard::HasText() { return SDL_HasClipboardText(); }

std::optional<std::vector<std::string>> Clipboard::GetFiles() {
  WarnFileClipboardUnsupported("GetFiles");
  return std::nullopt;
}

bool Clipboard::SetFiles(const std::vector<std::string>& paths) {
  (void)paths;
  return WarnFileClipboardUnsupported("SetFiles");
}

// The pasteboard bridge exposes no change counter; the polling monitor compares
// text snapshots instead, which is what StartMonitoring already does.
uint64_t Clipboard::ChangeCount() { return 0; }

bool Clipboard::SetFileOffer(std::shared_ptr<ClipboardFilePromise> offer, FileOfferError* error) {
  (void)offer;
  if (error) *error = FileOfferError::UnavailableRuntime;
  WarnFileClipboardUnsupported("SetFileOffer");
  return false;
}

bool Clipboard::OwnsFileOffer(const std::shared_ptr<ClipboardFilePromise>& offer) {
  (void)offer;
  return false;
}

void Clipboard::ClearFileOffer() {}

namespace platform {

void RunClipboardMonitor(int check_interval_ms) {
  const int interval = check_interval_ms > 0 ? check_interval_ms : 100;
  LOG_INFO("Clipboard polling monitor started (interval {} ms)", interval);
  while (ClipboardMonitoring()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
    if (ClipboardMonitoring()) HandleClipboardChange();
  }
  LOG_INFO("Clipboard polling monitor stopped");
}

// The monitor sleeps in full intervals, so a wake is only a best-effort hint:
// StopMonitoring joins after the current interval at worst.
void WakeClipboardMonitor() {}

}  // namespace platform

}  // namespace crossdesk
