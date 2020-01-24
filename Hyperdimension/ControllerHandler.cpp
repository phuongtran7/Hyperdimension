#include "ControllerHandler.h"

ControllerHandler::ControllerHandler() :
	controllers_{},
	lock_{}
{
	DisableBlinkingCursor();

	addedRevoker_ = RawGameController::RawGameControllerAdded(winrt::auto_revoke, std::bind(&ControllerHandler::OnControllerAdded, this, std::placeholders::_1, std::placeholders::_2));
	removedRevoker_ = RawGameController::RawGameControllerRemoved(winrt::auto_revoke, std::bind(&ControllerHandler::OnControllerRemoved, this, std::placeholders::_1, std::placeholders::_2));

	// Loop through the intially connected controller and add it to vector for retrieving later.
	// For some reason if ConntrollerAdded and ControllerRemoved event are not subscribed to, the program cannot detect any controller
	auto controllers = RawGameController::RawGameControllers();
	for (const auto& controller : controllers)
	{
		OnControllerAdded(nullptr, controller);
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	StartUpdate();
}

ControllerHandler::~ControllerHandler()
{
	StopUpdate();
}

void ControllerHandler::StartUpdate()
{
	running_.store(true);
	while (running_.load()) {
		critical_section::scoped_lock lock{ lock_ };

		if (controllers_.empty()) {
			// Exit if there's currently no controller connected
			return;
		}

		// More info: https://stackoverflow.com/a/57073912/9017481
		auto buttons = controllers_.at(0).ButtonCount();
		auto switches = controllers_.at(0).SwitchCount();
		auto axis = controllers_.at(0).AxisCount();

		std::unique_ptr<bool[]> buttonsArray = std::make_unique<bool[]>(buttons);
		std::vector<GameControllerSwitchPosition> switchesArray(switches);
		std::vector<double> axisArray(axis);

		uint64_t timestamp = controllers_.at(0).GetCurrentReading(winrt::array_view<bool>(buttonsArray.get(), buttonsArray.get() + buttons), switchesArray, axisArray);

		// Move the cursor back to row 2 so the console does not have to scroll to display info
		SetCursorPosition(0, 2);

		fmt::print("Timestamp: {}\n", timestamp);
		fmt::print("\n");

		fmt::print("Axis count: {}\n", axis);
		for (auto i = 0; i < axis; i++) {
			fmt::print("Axis {}: {}  \n", i, fmt::format("{:.{}f}", axisArray[i], 8));
		}
		fmt::print("\n");

		fmt::print("Button count: {}\n", buttons);
		for (auto i = 0; i < buttons; i++) {
			fmt::print("Button {}: {}  \n", i, buttonsArray[i]);
		}
		fmt::print("\n");

		fmt::print("Switch count: {}\n", switches);
		for (auto i = 0; i < switches; i++) {
			fmt::print("Switch {}: {}  \n", i, switchesArray[i]);
		}
	}
}

void ControllerHandler::StopUpdate()
{
	running_.store(false);
	if (update_thread.joinable()) {
		update_thread.join();
	}
}

void ControllerHandler::OnControllerAdded(const IInspectable&, const RawGameController& controller)
{
	// Check if the controller is already in myControllers; if it isn't, add it.
	critical_section::scoped_lock lock{ lock_ };
	auto it = std::find(std::begin(controllers_), std::end(controllers_), controller);

	if (it == end(controllers_))
	{
		controllers_.emplace_back(controller);
		fmt::print("Added \"{}\".\n", winrt::to_string(controller.DisplayName()));
	}
}

void ControllerHandler::OnControllerRemoved(const IInspectable&, const RawGameController& controller)
{
	critical_section::scoped_lock lock{ lock_ };
	auto it = std::find(std::begin(controllers_), std::end(controllers_), controller);

	if (it != end(controllers_))
	{
		fmt::print("Removed \"{}\".\n", winrt::to_string(controller.DisplayName()));
		controllers_.erase(it);
	}
}

// X is the column, Y is the row. The origin (0,0) is top-left.
// More info: https://stackoverflow.com/a/34843392/9017481
void ControllerHandler::SetCursorPosition(int x, int y)
{
	static const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	std::cout.flush();
	COORD coord = { (SHORT)x, (SHORT)y };
	SetConsoleCursorPosition(hOut, coord);
}

void ControllerHandler::DisableBlinkingCursor()
{
	static const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(hOut, &cursorInfo);
	cursorInfo.bVisible = false;
	SetConsoleCursorInfo(hOut, &cursorInfo);
}