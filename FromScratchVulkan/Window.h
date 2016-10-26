#pragma once

#include "Platform.h"

class Window {
public:
	Window();
	~Window();
	void Close();
	bool Update();
private:
	bool m_running = true;
};