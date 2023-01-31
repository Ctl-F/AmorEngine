#include "pch.h"
#include "Util.h"

namespace amor {
	namespace util {

		Timer::Timer() : m_IsStarted(false), m_StartTimestamp(0), m_LastTimestamp(0) {}
		Timer::~Timer() {}

		void Timer::start() {
			m_IsStarted = true;
			m_StartTimestamp = get_timestamp_ms();
			m_LastTimestamp = m_StartTimestamp;
		}


		void Timer::stop() {
			m_IsStarted = false;
			m_StartTimestamp = 0u;
			m_LastTimestamp = 0u;
		}

		u64 Timer::delta() {
			u64 now = get_timestamp_ms();
			u64 delta = now - m_LastTimestamp;
			m_LastTimestamp = now;
			return delta;
		}

		double Timer::delta_seconds() {
			return (double)(delta()) / 1000.0;
		}

		u64 Timer::elapsed() {
			return get_timestamp_ms() - m_StartTimestamp;
		}

		double Timer::elapsed_seconds() {
			return (double)(elapsed()) / 1000.0;
		}

		u64 Timer::get_timestamp_ms() const {
			return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
		}
	}
}

