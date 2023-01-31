#include "pch.h"
#include "Common.h"

namespace amor {
	namespace logging {
		Log::Log() {

		}
		Log::~Log() {
			flush();
		}

		Level& Log::LogLevel() {
			return m_LogLevel;
		}

		u32& Log::OutputMode() {
			return m_OutputFlags;
		}

		TimestampMode& Log::TimeMode() {
			return m_TimestampMode;
		}

		bool& Log::UseBufferedFileOutput() {
			return m_UseBufferedFileOutput;
		}

		std::string& Log::LogFile() {
			return m_LogFilename;
		}

		void Log::out(Level level, const std::string& message, const std::string& source) {
			if (!should_log(level, source)) {
				return;
			}

			std::stringstream str;

			write_timestamp(str);
			write_level(str, level, source);
			str << "- " << message << std::endl;

			if (m_OutputFlags & LOG_FILE) {
				write_to_file(str.str());
			}

			if (m_OutputFlags & LOG_STDOUT) {
				write_to_stdout(str.str(), level);
			}
		}

		void Log::flush() {
			if (m_UseBufferedFileOutput) {
				std::ofstream file;

				if (std::filesystem::exists(m_LogFilename)) {
					file.open(m_LogFilename, std::ios_base::app);
				}
				else {
					file.open(m_LogFilename, std::ios_base::out);
				}
				file.write(m_FileBuffer.str().c_str(), m_FileBuffer.str().length());
				file.close();
			}
		}

		void Log::add_allowed_source(const std::string& source) {
			m_AllowedSources.insert(source);
		}
		void Log::remove_allowed_source(const std::string& source) {
			m_AllowedSources.erase(source);
		}
		
		// only log if:
		//		1: The allowed logging level is greater or equal to the current logging level
		//		2: All sources are allowed (the allowed source list is empty)
		//		3: (or) the given source is in the allowed list
		//		4: if the source itself is empty log it
		bool Log::should_log(Level level, const std::string& source) {
			return level >= m_LogLevel && (m_AllowedSources.empty() || source.empty() || m_AllowedSources.contains(source));
		}

		// timestamp [level: source] - message
		void Log::write_level(std::stringstream& buffer, Level level, const std::string& source) {
			buffer << "[";
			switch (level) {
			case Level::Info:
				buffer << "Info";
				break;
			case Level::Warning:
				buffer << "Warning";
				break;
			case Level::Error:
				buffer << "Error";
				break;
			case Level::Failure:
				buffer << "Failure";
				break;
			}

			if (source != "") {
				buffer << ": " << source;
			}
			buffer << "] ";
		}
		void Log::write_timestamp(std::stringstream& buffer) {
			auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

			std::string date(30, '\0');
			std::string time(30, '\0');

			if ((u32)m_TimestampMode & (u32)TimestampMode::Date) {
				tm t;
				localtime_s(&t, &now);
				std::strftime(&date[0], date.size(), "%Y-%m-%d", &t);
			}

			if ((u32)m_TimestampMode & (u32)TimestampMode::Time) {
				tm t;
				localtime_s(&t, &now);
				std::strftime(&time[0], time.size(), "%H:%M:%S", &t);
			}

			buffer << date << (date.empty() ? "" : " ") << time << " ";
		}


		void Log::write_to_stdout(const std::string& str, Level level) {
			switch (level) {
			case Level::Info:
			case Level::Warning:
				std::cout << str << std::endl;
				break;
			case Level::Error:
			case Level::Failure:
				std::cerr << str << std::endl;
				break;
			}
		}

		void Log::write_to_file(const std::string& str) {
			if (m_UseBufferedFileOutput) {
				m_FileBuffer << str << std::endl;
			}
			else {
				std::ofstream file;

				if (std::filesystem::exists(m_LogFilename)) {
					file.open(m_LogFilename, std::ios_base::app);
				}
				else {
					file.open(m_LogFilename, std::ios_base::out);
				}
				file.write(str.c_str(), str.length());
				file.close();
			}
		}


		void Log::info(const std::string& message, const std::string& source) { out(Level::Info, message, source); }
		void Log::warn(const std::string& message, const std::string& source) { out(Level::Warning, message, source); }
		void Log::error(const std::string& message, const std::string& source) { out(Level::Error, message, source); }
		void Log::fail(const std::string& message, const std::string& source) { out(Level::Failure, message, source); }


		Log* GetInstance() {
			static Log __log;
			return &__log;
		}
	}
}