#pragma once
#include "Common.h"
#include "Core.h"
#include <unordered_set>
#include <unordered_map>
#include <initializer_list>

namespace amor {
	namespace graphics {
		class WindowBase;
	}


	namespace input {
		constexpr i32 STATE_RELEASED = 0;
		constexpr i32 STATE_PRESSED = 1;

		enum class Key {
			Space = 32,
			Apostrophe = 39,
			Comma = 44,
			Minus = 45,
			Period = 46,
			Slash = 47,
			K0 = 48, K1, K2, K3, K4, K5, K6, K7, K8, K9,
			Semicolon = 59,
			Equal = 61,
			A = 65, B, C, D, E, F, G, H,
			I, J, K, L, M, N, O, P,
			Q, R, S, T, U, V, W, X, Y, Z,
			LeftBracket = 91,
			Backslash,
			RightBracket,
			Backtick = 96,
			Escape = 256,
			Enter = 257,
			Tab,
			Backspace,
			Insert,
			Delete,
			Right,
			Left,
			Down,
			Up,
			PageUp,
			PageDown,
			Home,
			End,
			CapsLock = 280,
			ScrollLock,
			NumLock,
			PrintScreen,
			Pause,
			F1 = 290, F2, F3, F4, F5, F6, F7, F8,
			F9, F10, F11, F12, F13, F14, F15, F16, F17,
			F18, F19, F20, F21, F22, F23, F24, F25,
			KP0 = 320, KP1, KP2, KP3, KP4, KP5, KP6, KP7, KP8, KP9,
			KPDecimal,
			KPDivide,
			KPMultiply,
			KPSubtract,
			KPAdd,
			KPEnter,
			KPEqual,
			LeftShift = 340,
			LeftControl,
			LeftAlt,
			LeftSuper,
			RightShift,
			RightControl,
			RightAlt,
			RightSuper,
			Menu,
			FIMKEY
		};

		enum class MouseButton {
			Left = 0,
			Right = 1,
			Middle = 2,
			Button4,
			Button5,
			Button6,
			Button7,
			Button8,
			FIMKEY
		};

		enum class Joysticks {
			Joy0, Joy1, Joy2, Joy3, Joy4, Joy5,
			Joy6, Joy7, Joy8, Joy9, Joy10,
			Joy11, Joy12, Joy13, Joy14, Joy15, Joy16,
			FIMKEY
		};

		enum class GamepadButton {
			A, B, X, Y,
			LeftBumper, RightBumper,
			Back, Start, Guide, LeftThumb, RightThumb,
			DPadUp, DPadRight, DPadDown, DPadLeft,
			FIMKEY
		};

		enum class GamepadAxis {
			LeftX, LeftY, RightX, RightY,
			LeftTrigger, RightTrigger,
			FIMKEY
		};

		class Input {
			friend class graphics::WindowBase;
		public:
			Input(amor::graphics::WindowBase* win);
			~Input();

			bool mouse_check_pressed(MouseButton button) const;
			bool mouse_check_released(MouseButton button) const;

			bool key_check_pressed(Key key) const;
			bool key_check_released(Key key) const;

			bool key_check_just_pressed(Key key) const;
			bool key_check_just_released(Key key) const;

			bool mouse_check_just_pressed(MouseButton button) const;
			bool mouse_check_just_released(MouseButton button) const;

			/*bool gamepad_check_pressed(GamepadButton button) const;
			bool gamepad_check_released(GamepadButton button) const;

			int gamepad_check_axis(GamepadAxis axis) const;*/

			int mouse_wheel() const;
			math::Vec3f mouse_position() const;

			// TODO: Actions and Config

		protected:
			void Update(amor::graphics::WindowBase* window);

		private:
			void populate_keyenum();

		private:
			amor::graphics::WindowBase* m_Handle;
			double m_WheelState;
			bool m_CurrentKeyFrame[(u32)Key::FIMKEY];
			bool m_LastKeyFrame[(u32)Key::FIMKEY];

			bool m_CurrentMouseFrame[(u32)MouseButton::FIMKEY];
			bool m_LastMouseFrame[(u32)MouseButton::FIMKEY];

			static std::unordered_set<u32> KeyEnumValues;
		};
	

		enum class ActionStateType {
			Pressed,
			JustPressed,
			Released,
			JustReleased
		};


		class Action {
			friend class ActionsManager;
		public:
			Action();
			~Action();

			inline bool is_activated() const { return m_isActive; }
			inline bool is_just_activated() const { return m_isActive && !m_wasActive; }
			inline bool is_deactivated() const { return !m_isActive; }
			inline bool is_just_deactivated() const { return !m_isActive && m_wasActive; }

			void add_key_option(Key key, ActionStateType type = ActionStateType::Pressed);
			void begin_key_combo_option();
			void end_key_combo_option();

		protected:
			bool evaluate(Input&);

		private:
			bool m_isCombo;
			std::vector<std::vector<std::pair<Key, bool(Input::*)(Key)const>>> m_Options;
			bool m_isActive, m_wasActive;
		};

		class ActionsManager {
		public:
			ActionsManager();
			~ActionsManager();

			void add_action(const std::string& name, const Action& action);
			void add_action(const std::string& name, Key key, ActionStateType type = ActionStateType::Pressed);
			void add_action(const std::string& name, std::initializer_list<std::pair<Key, ActionStateType>> list);

			void update(Input& input);

			bool is_activated(const std::string& name) const;
			bool is_just_activated(const std::string& name) const;
			bool is_deactivated(const std::string& name) const;
			bool is_just_deactivated(const std::string& name) const;

		private:
			std::unordered_map<std::string, Action> m_Actions;
		};

	}
}