#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <ctime>

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
        std::cout << "Logger destroyed." << std::endl << "All own handlers closed." << std::endl;
    }

    void log(unsigned int level, const std::string& label, const std::string& msg) {
        if (level > log_level) {
            return;
        }
        for (auto&& out : handlers) {
            (*out) << timestamp() + " --> " << label << ": " << msg << std::endl;
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

class Lift {
private:
    bool CLOSED_DOORS = true;
    unsigned int capacity = 2;
    unsigned int inside = 0;
    unsigned int floor = 1;

public:
    [[nodiscard]] unsigned int cur_cap() const {
        return this->capacity;
    }
    [[nodiscard]] unsigned int cur_inside() const {
        return this->inside;
    }
    [[nodiscard]] unsigned int cur_floor() const {
        return this->floor;
    }
    [[nodiscard]] bool cur_doors() const {
        return this->CLOSED_DOORS;
    }
    void change_doors(bool FUTURE_CLOSED_DOORS, Logger* my_logger) {
        if (CLOSED_DOORS != FUTURE_CLOSED_DOORS) {
            CLOSED_DOORS = FUTURE_CLOSED_DOORS;
            if (CLOSED_DOORS == true) {
                my_logger->info("Doors has been closed");
            }
            else {
                my_logger->info("Doors has been opened");
            }
            return;
        }
        if (CLOSED_DOORS == true) {
            my_logger->info("Doors is already closed");
        }
        else {
            my_logger->info("Doors is already opened");
        }
    }
    int change_cap(int future_cap, Logger* my_logger) {
        if (future_cap < 2 || future_cap > 8) {
            my_logger->critical("Out of capacity");
            return -1;//возвращает ошибку входных данных
        }
        if (future_cap != capacity) {
            capacity = future_cap;
        }
        my_logger->info("Capacity has been changed");
        return 0;
    }
    int change_inside(int came_in, int came_out, Logger* my_logger) {
        if (inside - came_out + came_in > capacity || inside < came_out) {
            my_logger->critical("Out of capacity");
            return -1;//возвращает ошибку входных данных
        }
        inside += came_in - came_out;
        my_logger->info("People inside has been changed");
        return 0;
    }
    int change_floor(int future_floor, Logger* my_logger) {
        if (future_floor <= 0) {
            my_logger->critical("Out of floors");
            return -1;//возвращает ошибку входных данных
        }
        if (future_floor != floor) {
            floor = future_floor;
            my_logger->info("Floor has been changed");
        }
        return 0;
    }
};

class Person {
private:
    unsigned int direction = 1;
};

class People {
private:
    std::vector<std::vector<Person>> people;
public:
    std::vector<std::vector<Person>> get_people() {
        return this->people;
    }
};

void Generate_elevator_calls(Lift *lift) {
    std::thread a[3];
    std::srand(time(0));

    int random_floor = rand() % 8 + 1;
    if (random_floor == lift->cur_floor()) {
        random_floor = (random_floor + 1) % 8 + 1;
    }

    int random_inside = lift->cur_inside()
}

int main() {
    Logger* my_logger = LoggerBuilder()
                            .add_handler(std::cout)
                            //.add_handler(std::make_unique<std::ofstream>("log.txt"))
                            .make_object();

}