#include <iostream>
#include <queue>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <netinet/in.h>
#include <unistd.h>
#include <atomic>
#include <random>
#include <sstream>
#include <iomanip>
#include <memory>
#include <ctime>
#include <fstream>
#include <string>
#include <algorithm>

// Вспомогательные функции для преобразования IP и портов
std::string ip_to_string(in_addr_t ip) {
    ip = ntohl(ip);
    std::stringstream ss;
    ss << ((ip >> 24) & 0xFF) << "."
       << ((ip >> 16) & 0xFF) << "."
       << ((ip >> 8) & 0xFF) << "."
       << (ip & 0xFF);
    return ss.str();
}

std::string port_to_string(in_port_t port) {
    port = ntohs(port);
    return std::to_string(port);
}

class LoggerBuilder;

class Logger {
private:
    friend class LoggerBuilder;
    std::vector<std::ostream*> handlers;
    std::vector<std::unique_ptr<std::ostream>> own_handlers;
    unsigned int log_level;

    Logger() {
        log_level = CRITICAL;
    }

    std::string timestamp() {
        std::time_t now = std::time(nullptr);
        std::tm* tm_ptr = std::localtime(&now);

        std::ostringstream oss;
        oss << std::put_time(tm_ptr, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

public:
    enum {
        CRITICAL = 0,
        ERROR = 1,
        WARNING = 2,
        INFO = 3,
        DEBUG = 4
    };

    ~Logger() {
        debug("Logger destroyed. All own handlers closed.");
    }

    void log(unsigned int level, const std::string& label, const std::string& msg) {
        if (level > log_level) {
            return;
        }
        std::string log_line = timestamp() + " --> " + label + ": " + msg + "\n";
        for (auto&& out : handlers) {
            (*out) << log_line;
        }
    }

    void critical(const std::string& msg) {
        log(CRITICAL, "CRITICAL", msg);
    }
    void error(const std::string& msg) {
        log(ERROR, "ERROR", msg);
    }
    void warning(const std::string& msg) {
        log(WARNING, "WARNING", msg);
    }
    void info(const std::string& msg) {
        log(INFO, "INFO", msg);
    }
    void debug(const std::string& msg) {
        log(DEBUG, "DEBUG", msg);
    }
};

class LoggerBuilder {
private:
    Logger* logger = nullptr;

public:
    LoggerBuilder() { this->logger = new Logger(); }

    LoggerBuilder& set_level(unsigned int logging_level) {
        this->logger->log_level = logging_level;
        return *this;
    }

    LoggerBuilder& add_handler(std::ostream& stream) {
        this->logger->handlers.push_back(&stream);
        return *this;
    }

    LoggerBuilder& add_handler(std::unique_ptr<std::ostream> stream) {
        this->logger->handlers.push_back(stream.get());
        this->logger->own_handlers.push_back(std::move(stream));
        return *this;
    }

    [[nodiscard]] Logger* make_object() const { return this->logger; }
};

struct tcp_traffic_pkg {
    in_addr_t src_addr;
    in_port_t src_port;
    in_addr_t dst_addr;
    in_port_t dst_port;
    size_t sz;  // Убрано const для возможности копирования

    tcp_traffic_pkg(in_addr_t src_ip, in_port_t src_p, in_addr_t dst_ip, in_port_t dst_p, size_t size)
        : src_addr(src_ip), src_port(src_p), dst_addr(dst_ip), dst_port(dst_p), sz(size) {}

    // Добавляем конструктор копирования
    tcp_traffic_pkg(const tcp_traffic_pkg& other)
        : src_addr(other.src_addr), src_port(other.src_port),
          dst_addr(other.dst_addr), dst_port(other.dst_port), sz(other.sz) {}
};

struct ip_stats {
    size_t total_sent = 0;
    size_t total_received = 0;
    size_t connections_established = 0;
    std::map<in_addr_t, std::map<in_port_t, size_t>> connected_ips;
};

template<typename T>
class SynchronizedQueue {
private:
    std::queue<T> queue;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> shutdown_flag{false};

public:
    void push(const T& item) {
        std::unique_lock<std::mutex> lock(mtx);
        queue.push(item);
        cv.notify_one();
    }

    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return !queue.empty() || shutdown_flag.load(); });

        if (shutdown_flag.load() && queue.empty()) {
            return false;
        }

        item = queue.front();
        queue.pop();
        return true;
    }

    void shutdown() {
        shutdown_flag.store(true);
        cv.notify_all();
    }

    bool is_shutdown() const {
        return shutdown_flag.load();
    }
};

class LogGenerator {
private:
    SynchronizedQueue<tcp_traffic_pkg>& queue;
    std::atomic<bool> running{false};
    std::vector<std::thread> threads;
    std::shared_ptr<Logger> logger;
    int thread_count;

    void generate_logs(int thread_id) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> size_dist(64, 4096);
        std::uniform_int_distribution<uint16_t> port_dist(1024, 65535);
        std::uniform_int_distribution<uint32_t> ip_dist(0x0A000001, 0x0A0000FF);

        logger->info("Generator thread " + std::to_string(thread_id) + " started");

        while (running.load()) {
            in_addr_t src_ip = htonl(ip_dist(gen));
            in_addr_t dst_ip = htonl(ip_dist(gen));
            in_port_t src_port = htons(port_dist(gen));
            in_port_t dst_port = htons(port_dist(gen));
            size_t size = size_dist(gen);

            tcp_traffic_pkg pkg(src_ip, src_port, dst_ip, dst_port, size);
            queue.push(pkg);

            std::ostringstream oss;
            oss << "Generated package: " << ip_to_string(src_ip) << ":" << ntohs(src_port)
                << " -> " << ip_to_string(dst_ip) << ":" << ntohs(dst_port)
                << " size = " << size;
            logger->debug(oss.str());

            std::this_thread::sleep_for(std::chrono::milliseconds(10 + thread_id * 5));
        }

        logger->info("Generator thread " + std::to_string(thread_id) + " stopped");
    }

public:
    LogGenerator(SynchronizedQueue<tcp_traffic_pkg>& q, std::shared_ptr<Logger> log, int count = 4)
        : queue(q), logger(log), thread_count(count) {}

    void start() {
        running.store(true);
        for (int i = 0; i < thread_count; ++i) {
            threads.emplace_back(&LogGenerator::generate_logs, this, i);
        }
        logger->info("LogGenerator started with " + std::to_string(thread_count) + " threads");
    }

    void stop() {
        if (running.exchange(false)) {
            for (auto& t : threads) {
                if (t.joinable()) t.join();
            }
            threads.clear();
            logger->info("LogGenerator stopped");
        }
    }
};

class LogAnalyzer {
private:
    SynchronizedQueue<tcp_traffic_pkg>& queue;
    std::atomic<bool> running{false};
    std::vector<std::thread> threads;
    std::vector<std::map<in_addr_t, ip_stats>> local_stats;
    std::mutex stats_mtx;
    std::shared_ptr<Logger> logger;
    int thread_count;

    void analyze_logs(int thread_id) {
        tcp_traffic_pkg pkg(0, 0, 0, 0, 0);
        logger->info("Analyzer thread " + std::to_string(thread_id) + " started");

        while (running.load()) {
            if (!queue.pop(pkg)) {
                break;
            }

            {
                std::lock_guard<std::mutex> lock(stats_mtx);
                ip_stats& src_stats = local_stats[thread_id][pkg.src_addr];
                ip_stats& dst_stats = local_stats[thread_id][pkg.dst_addr];

                src_stats.total_sent += pkg.sz;
                src_stats.connected_ips[pkg.dst_addr][pkg.dst_port] += pkg.sz;

                dst_stats.total_received += pkg.sz;
                dst_stats.connected_ips[pkg.src_addr][pkg.src_port] += pkg.sz;

                src_stats.connections_established++;
            }

            std::ostringstream oss;
            oss << "Analyzed package: " << ip_to_string(pkg.src_addr) << ":" << ntohs(pkg.src_port)
                << " -> " << ip_to_string(pkg.dst_addr) << ":" << ntohs(pkg.dst_port)
                << " size=" << pkg.sz;
            logger->debug(oss.str());
        }

        logger->info("Analyzer thread " + std::to_string(thread_id) + " stopped");
    }

public:
    LogAnalyzer(SynchronizedQueue<tcp_traffic_pkg>& q, std::shared_ptr<Logger> log, int count = 4)
        : queue(q), logger(log), thread_count(count) {
        local_stats.resize(count);
    }

    void start() {
        running.store(true);
        for (int i = 0; i < thread_count; ++i) {
            threads.emplace_back(&LogAnalyzer::analyze_logs, this, i);
        }
        logger->info("LogAnalyzer started with " + std::to_string(thread_count) + " threads");
    }

    void stop() {
        if (running.exchange(false)) {
            for (auto& t : threads) {
                if (t.joinable()) t.join();
            }
            threads.clear();
            logger->info("LogAnalyzer stopped");
        }
    }

    ip_stats get_stats_for_ip(in_addr_t ip) {
        ip_stats result;
        std::lock_guard<std::mutex> lock(stats_mtx);

        for (auto& stats_map : local_stats) {
            auto it = stats_map.find(ip);
            if (it != stats_map.end()) {
                const ip_stats& stats = it->second;

                result.total_sent += stats.total_sent;
                result.total_received += stats.total_received;
                result.connections_established += stats.connections_established;

                for (const auto& conn : stats.connected_ips) {
                    for (const auto& port_data : conn.second) {
                        result.connected_ips[conn.first][port_data.first] += port_data.second;
                    }
                }
            }
        }

        return result;
    }

    std::map<in_addr_t, ip_stats> get_global_stats() {
        std::map<in_addr_t, ip_stats> result;
        std::lock_guard<std::mutex> lock(stats_mtx);

        for (auto& stats_map : local_stats) {
            for (auto& entry : stats_map) {
                ip_stats& global = result[entry.first];
                ip_stats& local = entry.second;

                global.total_sent += local.total_sent;
                global.total_received += local.total_received;
                global.connections_established += local.connections_established;

                for (const auto& conn : local.connected_ips) {
                    for (const auto& port_data : conn.second) {
                        global.connected_ips[conn.first][port_data.first] += port_data.second;
                    }
                }
            }
        }

        return result;
    }
};

void print_stats(const ip_stats& stats, in_addr_t ip, std::shared_ptr<Logger> logger) {
    std::ostringstream oss;
    oss << "Statistics for IP: " << ip_to_string(ip) << "\n";
    oss << "  Total sent: " << stats.total_sent << " bytes\n";
    oss << "  Total received: " << stats.total_received << " bytes\n";
    oss << "  Connections established: " << stats.connections_established << "\n";

    if (!stats.connected_ips.empty()) {
        oss << "  Connected IPs:\n";
        for (const auto& conn : stats.connected_ips) {
            oss << "    " << ip_to_string(conn.first) << ":\n";
            for (const auto& port_data : conn.second) {
                oss << "      Port " << port_to_string(port_data.first)
                    << ": " << port_data.second << " bytes\n";
            }
        }
    }

    logger->info(oss.str());
}

int main() {
    auto log_file = std::make_unique<std::ofstream>("logs.txt");
    if (!log_file->is_open()) {
        std::cerr << "Failed to open log file!" << std::endl;
        return 1;
    }

    auto logger_builder = LoggerBuilder()
        .set_level(Logger::DEBUG)
        .add_handler(std::cout)
        .add_handler(std::move(log_file));

    std::shared_ptr<Logger> logger(logger_builder.make_object());
    SynchronizedQueue<tcp_traffic_pkg> queue;
    LogGenerator generator(queue, logger, 4);
    LogAnalyzer analyzer(queue, logger, 4);

    bool is_running = false;
    std::string command;

    std::cout << "TCP Traffic Monitor - Interactive Mode\n";
    std::cout << "Commands: start, pause, exit\n";

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, command);
        std::transform(command.begin(), command.end(), command.begin(), ::tolower);

        if (command == "start") {
            if (!is_running) {
                logger->info("Starting traffic generation and analysis...");
                generator.start();
                analyzer.start();
                is_running = true;
            } else {
                logger->warning("System is already running");
            }
        }
        else if (command == "pause") {
            if (is_running) {
                logger->info("Pausing traffic generation...");
                generator.stop();
                is_running = false;
            } else {
                logger->warning("System is not running");
            }
            auto global_stats = analyzer.get_global_stats();
            logger->info("\nStatistics 1 el:");
            auto entry = global_stats.begin();
            print_stats(entry->second, entry->first, logger);
        }
        else if (command == "exit") {
            logger->info("Shutting down system...");
            if (is_running) {
                generator.stop();
            }
            queue.shutdown();
            analyzer.stop();
            break;
        }
        else {
            logger->warning("Unknown command. Available: start, pause, exit");
        }
    }

    auto global_stats = analyzer.get_global_stats();
    logger->info("\nFinal global statistics:");
    for (const auto& entry : global_stats) {
        print_stats(entry.second, entry.first, logger);
    }

    logger->info("System shutdown complete");
    return 0;
}