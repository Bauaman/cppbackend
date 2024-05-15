#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
    auto GetTime() const {
        if (manual_ts_) {
            return *manual_ts_;
        }

        return std::chrono::system_clock::now();
    }

    auto GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        return std::put_time(std::localtime(&t_c), "%F %T");
    }

    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&t_c), "%Y_%m_%d");
        return oss.str();
    }

    Logger() = default;
    Logger(const Logger&) = delete;

    template <typename T, typename... Ts>
    void LogInternal(std::string prefix, const T& arg, const Ts&... args) {
        logfile_ << prefix << arg;
        LogInternal(prefix, args...);
    }

    void LogInternal(std::string) {
    }

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    // Выведите в поток все аргументы.
    template<class... Ts>
    void Log(const Ts&... args) {
        const auto time_stamp = GetTimeStamp();
        const auto file_time_stamp = GetFileTimeStamp();

        std::ostringstream oss;
        oss << time_stamp << ": ";
        LogInternal(oss.str(), args...);

        std::lock_guard<std::mutex> lock(mutex_);
        if (current_file_ts_ != file_time_stamp) {
            current_file_ts_ = file_time_stamp;
            logfile_.close();
            logfile_.open("/var/log/sample_log_" + file_time_stamp + ".log", std::ios_base::app);
        }
        logfile_ << time_stamp << ": ";
        ((logfile_ << args), ...);
        logfile_ << std::endl;
    }

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть 
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        std::lock_guard<std::mutex> lock(mutex_);
        manual_ts_ = ts;
    }

private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    mutable std::mutex mutex_;
    std::ofstream logfile_;
    std::string current_file_ts_;
};