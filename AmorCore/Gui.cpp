#include "pch.h"
#include "Gui.h"

namespace amor {
	namespace graphics {

		namespace ui {

			EventDispatcher::EventDispatcher(WindowBase* window) {

			}
			EventDispatcher::~EventDispatcher() {

			}

			void EventDispatcher::update(input::Input& input) {
				prepare_queue();




			}

			void EventDispatcher::prepare_queue() {
				for (size_t i = 0; i < m_FrameEventQueue.size(); i++) {
					if (m_FrameEventQueue[i] != nullptr) {
						delete m_FrameEventQueue[i];
						m_FrameEventQueue[i] = nullptr;
					}
				}
				m_FrameEventQueue.clear();
			}

			void EventDispatcher::set_parent_ptr(void* ptr) {
				m_Parent = ptr;
			}



		}

	}
}