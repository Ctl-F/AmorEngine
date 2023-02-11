#pragma once
#include "Common.h"
#include <chrono>
#include <initializer_list>
#include <functional>

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


		template<typename _Ref_Ty, void(*deallocator)(_Ref_Ty&) = [](_Ref_Ty&) {} > class CountedRef {
		public:
			CountedRef(const _Ref_Ty& copy_existing) {
				m_Reference = new std::pair<_Ref_Ty, u64>(copy_existing, 1u);
			}

			CountedRef(const CountedRef<_Ref_Ty, deallocator>& reference_existing) {
				m_Reference = reference_existing.m_Reference;
				m_Reference->second++;

			}

			CountedRef(CountedRef<_Ref_Ty, deallocator>&& mov_existing) noexcept {
				m_Reference = mov_existing.m_Reference;
				mov_existing.m_Reference = nullptr;
			}

			~CountedRef() {
				remove_reference();
			}

			void operator=(const CountedRef<_Ref_Ty, deallocator>& reference_existing) {
				remove_reference();

				m_Reference = reference_existing.m_Reference;
				m_Reference->second++;
			}
			void operator=(CountedRef<_Ref_Ty, deallocator>&& move_existing) noexcept {
				if (move_existing.m_Reference != this->m_Reference) {

					remove_reference();

					m_Reference = move_existing.m_Reference;

					move_existing.m_Reference = nullptr;
				}
			}

			_Ref_Ty *ptr() {
				return &(m_Reference->first);
			}
			
			_Ref_Ty& operator*() {
				return m_Reference->first;
			}
			_Ref_Ty const& operator*() const {
				return m_Reference->first;
			}

			_Ref_Ty* operator->() {
				return &(m_Reference->first);
			}
			const _Ref_Ty* operator->() const {
				return &(m_Reference->first);
			}

		private:
			void remove_reference() {
				if (m_Reference != nullptr) {
					if (--m_Reference->second == 0) {
						deallocator(m_Reference->first);
						delete m_Reference;
					}

					m_Reference = nullptr;
				}
			}

		private:
			std::pair<_Ref_Ty, u64>* m_Reference = nullptr;
		};

	}

}