#pragma once
#include <string>
#include <sstream>
#include <unordered_set>

typedef unsigned __int64 u64;
typedef unsigned __int32 u32;
typedef unsigned __int16 u16;
typedef unsigned __int8 u8;
typedef unsigned __int8 byte;

typedef __int64 i64;
typedef __int32 i32;
typedef __int16 i16;
typedef __int8 i8;

#ifdef AMOR_SINGLE_PRECISION
typedef float real;
#else
typedef double real;
#endif

#define CLASS_INVOKE(instance, method, ...) (instance.*(method))(__VA_ARGS__) 

namespace amor {
	namespace logging {

		constexpr u32 LOG_STDOUT = 1;
		constexpr u32 LOG_FILE = 2;

		enum class Level {
			Info = 0,
			Warning = 1,
			Error = 2,
			Failure = 3
		};

		enum class TimestampMode {
			None = 0,
			Date = 1,
			Time = 2,
			DateTime = 3
		};

		class Log {
		public:
			Log();
			~Log();

			void out(Level level, const std::string& message, const std::string& source = "");
			void flush();

			void info(const std::string& message, const std::string& source = "");
			void warn(const std::string& message, const std::string& source = "");
			void error(const std::string& message, const std::string& source = "");
			void fail(const std::string& message, const std::string& source = "");

			void add_allowed_source(const std::string& source);
			void remove_allowed_source(const std::string& source);

			Level& LogLevel();

			u32& OutputMode();

			TimestampMode& TimeMode();

			std::string& LogFile();

			bool& UseBufferedFileOutput();

		private:
			bool should_log(Level level, const std::string& source);

			void write_level(std::stringstream& buffer, Level level, const std::string& source = "");
			void write_timestamp(std::stringstream& buffer);

			void write_to_file(const std::string& str);
			void write_to_stdout(const std::string& str, Level level);

		private:
			Level m_LogLevel = Level::Info;
			u32 m_OutputFlags = LOG_STDOUT;
			TimestampMode m_TimestampMode = TimestampMode::None;
			std::string m_LogFilename = "log.txt";
			bool m_UseBufferedFileOutput = true;
			std::stringstream m_FileBuffer{ "" };
			std::string m_Source = "";
			std::unordered_set<std::string> m_AllowedSources;
		};

		Log* GetInstance();
	}
}