#pragma once

#include "Core.h"
#include "KeyCodes.h"
#include "MouseButtonCodes.h"

namespace GLCore
{
	class Input
	{
	public:
		static bool IsKeyPressed (KeyCode keycode);
		static bool IsMouseButtonPressed (MouseCode button);

		static std::pair<float, float> GetMousePosn ();
		static float GetMouseX ();
		static float GetMouseY ();
	};
}