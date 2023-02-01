#pragma once
#include "Common.h"
#include "Core.h"
#include "Graphics.h"
#include "Input.h"

#include <functional>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace amor {
	namespace graphics {
		namespace ui {

			// Implementation dependant renderer base for ui.
			// this should allow common ui code as long as a UIRenderer is implemented
			// for the given renderer you happen to be using
			class CommonUIRendererBase {
			public:
				virtual void fill_rect(const math::Rect& rect) = 0;


			protected:


			};

			struct Theme {
				Font* TextFont;
				Color TextColor;
				Color BackgroundColor;
				Color HighlightColor;
				Color LowlightColor;
				math::Rect Margins;
				math::Rect Padding;
			};

			enum class EventType {
				KeyEvent,
				MouseEvent, // todo: window event?
			};

			enum class ModifierType {
				Shift = 0x01,
				Control = 0x02,
				Alt = 0x04
			};

			struct Event {
				EventType type;
				u64 timestamp;
				void* parent_ptr;
			};

			struct KeyEvent : public Event {
				input::Key which;
				bool is_pressed;
				bool is_just_pressed;
				bool is_just_released;
				std::stringstream& text_buffer;
				const bool* key_states;
				u32 modifiers;
			};

			struct MouseEvent : public Event {
				input::MouseButton which;
				bool is_pressed;
				bool is_just_pressed;
				bool is_just_released;
				i32 scroll;
				const bool* mouse_states;
				math::Vec3f position;
				math::Vec3f frame_delta;
				math::Vec3f event_delta;
			};

			typedef std::function<bool(Event*)> Callback;

			class EventDispatcher {
			public:
				EventDispatcher(WindowBase* window);
				~EventDispatcher();

				void set_parent_ptr(void* ptr);
				virtual void update(input::Input&);

			protected:
				void prepare_queue();
				void pump_events(input::Input&);

			protected:
				std::vector<Event*> m_FrameEventQueue;
				std::unordered_map<EventType, Callback> m_Callbacks;
				void* m_Parent;
				std::stringstream m_KeyboardTextBuffer;

				math::Vec3f m_MousePreviousFrame;
				math::Vec3f m_MousePreviousEvent;
			};

			class UIElement {
			public:

				void set_theme(const Theme& theme);
				void focus();

				bool am_focused() const;
			protected:
				virtual void update(input::Input& input) = 0;
				virtual void render(CommonUIRendererBase& renderer) = 0;

			protected:
				Theme m_Theme;

			private:
				bool m_AmFocused;
			};

			
			

		}
	}
}
