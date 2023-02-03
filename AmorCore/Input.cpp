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
				m_CurrentKeyFrame[i] = glfwGetKey(m_Handle->internal_ptr(), i) == STATE_PRESSED;
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


		Action::Action() : m_isActive(false), m_wasActive(false), m_isCombo(false) {

		}

		Action::~Action() {

		}

		void Action::add_key_option(Key key, ActionStateType type) {
			std::vector<std::pair<Key, bool(Input::*)(Key)const>>* bucket = nullptr;
			std::pair<Key, bool(Input::*)(Key)const> trigger;

			switch (type) {
			case ActionStateType::Pressed:
				trigger = std::make_pair(key, &Input::key_check_pressed);
				break;
			case ActionStateType::JustPressed:
				trigger = std::make_pair(key, &Input::key_check_just_pressed);
				break;
			case ActionStateType::Released:
				trigger = std::make_pair(key, &Input::key_check_released);
				break;
			case ActionStateType::JustReleased:
				trigger = std::make_pair(key, &Input::key_check_just_released);
				break;
			}

			if (m_isCombo) {
				bucket = &(m_Options[m_Options.size() - 1]);
			}
			else {
				bucket = &(m_Options.emplace_back());
			}
			bucket->push_back(trigger);
		}


		void Action::begin_key_combo_option() {
			m_isCombo = true;
			if (m_Options.empty()) {
				m_Options.emplace_back();
			}
		}

		void Action::end_key_combo_option() {
			m_isCombo = false;
		}

		bool Action::evaluate(Input& input) {
			m_wasActive = m_isActive;
			m_isActive = false;

			for (size_t i = 0; i < m_Options.size(); i++) {
				if (m_Options[i].empty()) {
					continue;
				}
				
				bool totalTrue = true;

				for (size_t j = 0; j < m_Options[i].size(); j++) {
					std::pair<Key, bool(Input::*)(Key)const>& entry = m_Options[i][j];

					if(!CLASS_INVOKE(input, entry.second, entry.first)) {
						totalTrue = false;
						break;
					}
				}

				if (totalTrue) {
					m_isActive = true;
					break;
				}
			}
			return m_isActive;
		}

		ActionsManager::ActionsManager() {

		}
		ActionsManager::~ActionsManager() {

		}

		void ActionsManager::update(Input& input) {
			for (auto& action : m_Actions) {
				action.second.evaluate(input);
			}
		}


		void ActionsManager::add_action(const std::string& name, const Action& action) {
			m_Actions[name] = action;
		}
		void ActionsManager::add_action(const std::string& name, Key key, ActionStateType type) {
			Action a;
			a.add_key_option(key, type);

			add_action(name, a);
		}

		void ActionsManager::add_action(const std::string& name, std::initializer_list<std::pair<Key, ActionStateType>> list) {
			Action a;

			for (auto p : list) {
				a.add_key_option(p.first, p.second);
			}

			add_action(name, a);
		}

		bool ActionsManager::is_activated(const std::string& name) const {
			if (m_Actions.contains(name)) {
				return m_Actions.at(name).is_activated();
			}
			return false;
		}
		bool ActionsManager::is_deactivated(const std::string& name) const {
			if (m_Actions.contains(name)) {
				return m_Actions.at(name).is_deactivated();
			}
			return false;
		}
		bool ActionsManager::is_just_activated(const std::string& name) const {
			if (m_Actions.contains(name)) {
				return m_Actions.at(name).is_just_activated();
			}
			return false;
		}
		bool ActionsManager::is_just_deactivated(const std::string& name) const {
			if (m_Actions.contains(name)) {
				return m_Actions.at(name).is_deactivated();
			}
			return false;
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