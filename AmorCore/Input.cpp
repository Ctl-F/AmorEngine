#include "pch.h"
#include "Input.h"

#include "Graphics.h"

#include <GLFW/glfw3.h>


namespace amor{
	namespace input {
		static double previousYScroll = 0.0;

		std::unordered_set<u32> Input::KeyEnumValues;

		static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
		{
			previousYScroll = yoffset;
		}

		Input::Input(amor::graphics::WindowBase* win) : m_Handle(win), m_WheelState(0.0) {
			glfwSetScrollCallback(win->internal_ptr(), scroll_callback);

			for (u32 i = 0; i < (u32)Key::FIMKEY; i++) {
				m_CurrentKeyFrame[i] = false;
				m_LastKeyFrame[i] = false;
			}
			for (u32 i = 0; i < (u32)MouseButton::FIMKEY; i++) {
				m_CurrentMouseFrame[i] = false;
				m_LastMouseFrame[i] = false;
			}

			populate_keyenum();
		}
		Input::~Input() {}

		void Input::Update(amor::graphics::WindowBase* win) {
			m_WheelState = previousYScroll;
			previousYScroll = 0.0;


			for (u32 i = 0; i < (u32)Key::FIMKEY; i++) {
				if (!KeyEnumValues.contains(i)) continue;

				m_LastKeyFrame[i] = m_CurrentKeyFrame[i];
				m_LastKeyFrame[i] = glfwGetKey(m_Handle->internal_ptr(), i) == STATE_PRESSED;
			}
			for (u32 i = 0; i < (u32)MouseButton::FIMKEY; i++) {
				m_LastMouseFrame[i] = m_CurrentMouseFrame[i];
				m_CurrentMouseFrame[i] = glfwGetMouseButton(m_Handle->internal_ptr(), i) == STATE_PRESSED;
			}
		}

		bool Input::mouse_check_pressed(MouseButton button) const {
			return glfwGetMouseButton(m_Handle->internal_ptr(), (int)button) == STATE_PRESSED;
		}
		bool Input::mouse_check_released(MouseButton button) const {
			return glfwGetMouseButton(m_Handle->internal_ptr(), (int)button) == STATE_RELEASED;
		}

		bool Input::key_check_pressed(Key key) const {
			return glfwGetKey(m_Handle->internal_ptr(), (int)key) == STATE_PRESSED;
		}
		bool Input::key_check_released(Key key) const {
			return glfwGetKey(m_Handle->internal_ptr(), (int)key) == STATE_RELEASED;
		}

		bool Input::key_check_just_pressed(Key key) const {
			return m_CurrentKeyFrame[(u32)key] && !m_LastKeyFrame[(u32)key];
		}
		bool Input::key_check_just_released(Key key) const {
			return !m_CurrentKeyFrame[(u32)key] && m_LastKeyFrame[(u32)key];
		}

		bool Input::mouse_check_just_pressed(MouseButton key) const {
			return m_CurrentMouseFrame[(u32)key] && !m_LastMouseFrame[(u32)key];
		}
		bool Input::mouse_check_just_released(MouseButton key) const {
			return !m_CurrentMouseFrame[(u32)key] && m_LastMouseFrame[(u32)key];
		}

		i32 Input::mouse_wheel() const {
			return (i32) m_WheelState;
		}

		math::Vec3f Input::mouse_position() const {
			math::Vec3f pos;
			glfwGetCursorPos(m_Handle->internal_ptr(), &pos.x, &pos.y);
			return pos;
		}





		// if you ever want to exclude a certain key from being polled then exclude it from the KeyEnumValues 
		void Input::populate_keyenum() {
			static bool hasPopulated = false;
			if (hasPopulated) {
				return;
			}
			hasPopulated = true;

			KeyEnumValues.insert((u32)Key::Space);
			KeyEnumValues.insert((u32)Key::Apostrophe);
			KeyEnumValues.insert((u32)Key::Comma);
			KeyEnumValues.insert((u32)Key::Minus);
			KeyEnumValues.insert((u32)Key::Period);
			KeyEnumValues.insert((u32)Key::Slash);
			KeyEnumValues.insert((u32)Key::K0);
			KeyEnumValues.insert((u32)Key::K1);
			KeyEnumValues.insert((u32)Key::K2);
			KeyEnumValues.insert((u32)Key::K3);
			KeyEnumValues.insert((u32)Key::K4);
			KeyEnumValues.insert((u32)Key::K5);
			KeyEnumValues.insert((u32)Key::K6);
			KeyEnumValues.insert((u32)Key::K7);
			KeyEnumValues.insert((u32)Key::K8);
			KeyEnumValues.insert((u32)Key::K9);
			KeyEnumValues.insert((u32)Key::Semicolon);
			KeyEnumValues.insert((u32)Key::Equal);
			KeyEnumValues.insert((u32)Key::A);
			KeyEnumValues.insert((u32)Key::B);
			KeyEnumValues.insert((u32)Key::C);
			KeyEnumValues.insert((u32)Key::D);
			KeyEnumValues.insert((u32)Key::E);
			KeyEnumValues.insert((u32)Key::F);
			KeyEnumValues.insert((u32)Key::G);
			KeyEnumValues.insert((u32)Key::H);
			KeyEnumValues.insert((u32)Key::I);
			KeyEnumValues.insert((u32)Key::J);
			KeyEnumValues.insert((u32)Key::K);
			KeyEnumValues.insert((u32)Key::L);
			KeyEnumValues.insert((u32)Key::M);
			KeyEnumValues.insert((u32)Key::N);
			KeyEnumValues.insert((u32)Key::O);
			KeyEnumValues.insert((u32)Key::P);
			KeyEnumValues.insert((u32)Key::Q);
			KeyEnumValues.insert((u32)Key::R);
			KeyEnumValues.insert((u32)Key::S);
			KeyEnumValues.insert((u32)Key::T);
			KeyEnumValues.insert((u32)Key::U);
			KeyEnumValues.insert((u32)Key::V);
			KeyEnumValues.insert((u32)Key::W);
			KeyEnumValues.insert((u32)Key::S);
			KeyEnumValues.insert((u32)Key::Y);
			KeyEnumValues.insert((u32)Key::Z);
			KeyEnumValues.insert((u32)Key::LeftBracket);
			KeyEnumValues.insert((u32)Key::Backslash);
			KeyEnumValues.insert((u32)Key::RightBracket);
			KeyEnumValues.insert((u32)Key::Backtick);
			KeyEnumValues.insert((u32)Key::Escape);
			KeyEnumValues.insert((u32)Key::Enter);
			KeyEnumValues.insert((u32)Key::Tab);
			KeyEnumValues.insert((u32)Key::Backspace);
			KeyEnumValues.insert((u32)Key::Insert);
			KeyEnumValues.insert((u32)Key::Delete);
			KeyEnumValues.insert((u32)Key::Right);
			KeyEnumValues.insert((u32)Key::Left);
			KeyEnumValues.insert((u32)Key::Up);
			KeyEnumValues.insert((u32)Key::Down);
			KeyEnumValues.insert((u32)Key::PageUp);
			KeyEnumValues.insert((u32)Key::PageDown);
			KeyEnumValues.insert((u32)Key::Home);
			KeyEnumValues.insert((u32)Key::End);
			KeyEnumValues.insert((u32)Key::CapsLock);
			KeyEnumValues.insert((u32)Key::ScrollLock);
			KeyEnumValues.insert((u32)Key::NumLock);
			KeyEnumValues.insert((u32)Key::PrintScreen);
			KeyEnumValues.insert((u32)Key::Pause);
			KeyEnumValues.insert((u32)Key::F1);
			KeyEnumValues.insert((u32)Key::F2);
			KeyEnumValues.insert((u32)Key::F3);
			KeyEnumValues.insert((u32)Key::F4);
			KeyEnumValues.insert((u32)Key::F5);
			KeyEnumValues.insert((u32)Key::F6);
			KeyEnumValues.insert((u32)Key::F7);
			KeyEnumValues.insert((u32)Key::F8);
			KeyEnumValues.insert((u32)Key::F9);
			KeyEnumValues.insert((u32)Key::F10);
			KeyEnumValues.insert((u32)Key::F11);
			KeyEnumValues.insert((u32)Key::F12);
			KeyEnumValues.insert((u32)Key::F13);
			KeyEnumValues.insert((u32)Key::F14);
			KeyEnumValues.insert((u32)Key::F15);
			KeyEnumValues.insert((u32)Key::F16);
			KeyEnumValues.insert((u32)Key::F17);
			KeyEnumValues.insert((u32)Key::F18);
			KeyEnumValues.insert((u32)Key::F19);
			KeyEnumValues.insert((u32)Key::F20);
			KeyEnumValues.insert((u32)Key::F21);
			KeyEnumValues.insert((u32)Key::F22);
			KeyEnumValues.insert((u32)Key::F23);
			KeyEnumValues.insert((u32)Key::F24);
			KeyEnumValues.insert((u32)Key::F25);
			KeyEnumValues.insert((u32)Key::KP0);
			KeyEnumValues.insert((u32)Key::KP1);
			KeyEnumValues.insert((u32)Key::KP2);
			KeyEnumValues.insert((u32)Key::KP3);
			KeyEnumValues.insert((u32)Key::KP4);
			KeyEnumValues.insert((u32)Key::KP5);
			KeyEnumValues.insert((u32)Key::KP6);
			KeyEnumValues.insert((u32)Key::KP7);
			KeyEnumValues.insert((u32)Key::KP8);
			KeyEnumValues.insert((u32)Key::KP9);
			KeyEnumValues.insert((u32)Key::KPDecimal);
			KeyEnumValues.insert((u32)Key::KPDivide);
			KeyEnumValues.insert((u32)Key::KPMultiply);
			KeyEnumValues.insert((u32)Key::KPSubtract);
			KeyEnumValues.insert((u32)Key::KPAdd);
			KeyEnumValues.insert((u32)Key::KPEnter);
			KeyEnumValues.insert((u32)Key::KPEqual);
			KeyEnumValues.insert((u32)Key::LeftShift);
			KeyEnumValues.insert((u32)Key::LeftControl);
			KeyEnumValues.insert((u32)Key::LeftAlt);
			KeyEnumValues.insert((u32)Key::LeftSuper);
			KeyEnumValues.insert((u32)Key::RightShift);
			KeyEnumValues.insert((u32)Key::RightControl);
			KeyEnumValues.insert((u32)Key::RightAlt);
			KeyEnumValues.insert((u32)Key::RightSuper);
			KeyEnumValues.insert((u32)Key::Menu);

		}
	}
}