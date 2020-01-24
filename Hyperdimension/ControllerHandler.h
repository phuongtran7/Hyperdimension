#pragma once
#include <windows.h>
#include "fmt/format.h"
#include <vector>
#include <ppl.h>
#include <agents.h>
#include <iostream>
#include <atomic>
#include <thread>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Gaming.Input.h>

using namespace winrt::Windows::Gaming::Input;
using namespace winrt::Windows::Foundation;
using namespace concurrency;

class ControllerHandler
{
private:
	std::vector<RawGameController> controllers_;
	critical_section lock_;
	RawGameController::RawGameControllerAdded_revoker addedRevoker_;
	RawGameController::RawGameControllerRemoved_revoker removedRevoker_;
	std::atomic<bool> running_;
	std::thread update_thread;
public:
	ControllerHandler();
	~ControllerHandler();
	void StartUpdate();
	void StopUpdate();
private:
	void OnControllerAdded(const IInspectable& /* sender */, const RawGameController& controller);
	void OnControllerRemoved(const IInspectable& /* sender */, const RawGameController& controller);
	void SetCursorPosition(int x, int y);
	void DisableBlinkingCursor();
};