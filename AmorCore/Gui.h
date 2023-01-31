#pragma once
#include "Common.h"
#include "Core.h"
#include "Graphics.h"
#include "Input.h"

#include <functional>
#include <sstream>

namespace amor {
	namespace graphics {
		namespace ui {

			typedef std::function<void(void* caller)> Action;

			class EventListener {
			protected:
				void foo();
			};

			class UIElement {
			public:
				virtual void update(input::Input&) = 0;
				void focus();
				const math::Rect& get_area() const;
			protected:
				bool m_Focused = false;
			};

			class Label : public UIElement {
			public:
				i32 x, y;
				std::string text;

				void update(input::Input&) override;
			};

			class Button : public UIElement {
			public:
				math::Rect location;
				std::string text;
				Action callback;

				void update(input::Input&) override;
			};

			struct TextBox : public UIElement {
				math::Rect location;
				std::stringstream text;
				
			};

			// Implementation dependant renderer base for ui.
			// this should allow common ui code as long as a UIRenderer is implemented
			// for the given renderer you happen to be using
			class CommonUIRendererBase {
			public:
				virtual void fill_rect(const math::Rect& rect) = 0;


			protected:
				

			};

		}
	}
}
