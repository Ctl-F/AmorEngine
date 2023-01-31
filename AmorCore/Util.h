#pragma once
#include "Common.h"
#include <chrono>

namespace amor {

	namespace util {


		class Timer {
		public:
			Timer();
			~Timer();

			// starts the timer
			void start();

			// stops and resets the timer
			void stop();
			
			// returns the elapsed time since start or the previous elapsed call
			// in milliseconds
			u64 delta();
			
			double delta_seconds();

			// returns the elapsed time since start in milliseconds
			u64 elapsed();

			double elapsed_seconds();

		private:
			u64 get_timestamp_ms() const;

		private:
			bool m_IsStarted;
			u64 m_StartTimestamp;
			u64 m_LastTimestamp;
		};


	}

}