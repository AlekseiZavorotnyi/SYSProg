#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <ctime>
#include <thread>
#include <mutex>
#include <random>

class LoggerBuilder;

class Logger {
private:
    friend class LoggerBuilder;
    std::vector<std::ostream*> handlers;
    std::vector<std::unique_ptr<std::ostream>> own_handlers;
    unsigned int log_level;
    std::mutex log_mutex;

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
        std::lock_guard<std::mutex> lock(log_mutex);
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

class Person {
private:
    int direction = 1;
public:
    explicit Person(int dir): direction(dir){}

    [[nodiscard]] int get_dir() const {
        return this->direction;
    }
};

class Lift {
private:
    bool CLOSED_DOORS = true;
    int max_floors = 2;
    int capacity = 2;
    int inside = 0;
    int floor = 1;
    int people_count = 0;
    int floor_count = 0;
    int time_wait = 0;
    int calls_count = 0;
    std::vector<std::vector<Person>> people;

public:
    explicit Lift(int cap = 2, int mx_fl = 2) : capacity(cap), max_floors(mx_fl) {}

    [[nodiscard]] int cur_max_floors() const {
        return this->max_floors;
    }
    [[nodiscard]] int cur_cap() const {
        return this->capacity;
    }
    [[nodiscard]] int cur_inside() const {
        return this->inside;
    }
    [[nodiscard]] int cur_floor() const {
        return this->floor;
    }
    [[nodiscard]] bool cur_doors() const {
        return this->CLOSED_DOORS;
    }
    [[nodiscard]] int get_people_count() const {
        return this->people_count;
    }
    [[nodiscard]] int get_floor_count() const {
        return this->floor_count;
    }
    [[nodiscard]] int get_time_wait() const {
        return calls_count > 0 ? time_wait / calls_count : 0;
    }

    std::vector<std::vector<Person>>& get_people() {
        return this->people;
    }

    void remove_first_group() {
        if (!people.empty()) {
            people.erase(people.begin());
        }
    }
    void change_doors(bool FUTURE_CLOSED_DOORS, Logger* my_logger) {
        if (CLOSED_DOORS != FUTURE_CLOSED_DOORS) {
            time_wait++;
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
    void change_inside(int came_in, int came_out, Logger* my_logger) {
        if (came_out > inside) {
            my_logger->critical("Adjusting came_out from " + std::to_string(came_out) +
                              " to " + std::to_string(inside));
            came_out = inside;
        }
        if (inside - came_out + came_in > capacity) {
            int old_came_in = came_in;
            came_in = capacity - (inside - came_out);
            my_logger->warning("Reducing came_in from " + std::to_string(old_came_in) +
                             " to " + std::to_string(came_in));
        }
        time_wait++;
        people_count += came_in;
        inside += came_in - came_out;
        my_logger->info("People inside changed from " + std::to_string(inside - came_in + came_out) +
                       " to " + std::to_string(inside));
    }
    void change_floor(int future_floor, Logger* my_logger) {
        if (future_floor != floor) {
            int cur_floor_count = abs(floor - future_floor);
            calls_count++;
            time_wait += 2 * cur_floor_count;
            floor_count += cur_floor_count;
            floor = future_floor;
            my_logger->info("Floor has been changed");
        }
    }
    void add_person(const Person &person) {
        for (auto& group : people) {
            if (group[0].get_dir() == person.get_dir()) {
                group.push_back(person);
                return;
            }
        }
        people.push_back({person});
    }
};

void Generate_elevator_calls(int thread_id, Lift *lift, Logger* my_logger) {
    std::string str_thread = "Thread #" + std::to_string(thread_id) + " started\n";
    std::cout << str_thread;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> rand_calls_dist(5, 15);

    int random_calls = rand_calls_dist(gen);
    while (random_calls > 0) {
        int next_floor = lift->cur_floor();
        auto people_inside = lift->get_people();
        if (!people_inside.empty()) {
            next_floor = people_inside.front()[0].get_dir();
        }

        lift->change_floor(next_floor, my_logger);

        lift->change_doors(false, my_logger);
        int came_out = 0;
        people_inside = lift->get_people();
        if (!people_inside.empty() && people_inside.front()[0].get_dir() == lift->cur_floor()) {
            came_out = people_inside.front().size();
            lift->remove_first_group();
        }
        lift->change_inside(0, came_out, my_logger);

        int max_possible = lift->cur_cap() - lift->cur_inside();
        if (max_possible > 0) {
            std::uniform_int_distribution<> came_in_dist(1, max_possible);
            int came_in = came_in_dist(gen);
            for (int i = 0; i < came_in; i++) {
                std::uniform_int_distribution<> person_dir_dist(1, lift->cur_max_floors());
                int person_direction = person_dir_dist(gen);
                Person cur_person(person_direction);
                lift->add_person(cur_person);
            }
            lift->change_inside(came_in, 0, my_logger);
        }

        lift->change_doors(true, my_logger);
        random_calls--;
    }

    // Выпускаем всех оставшихся людей
    while (lift->cur_inside() > 0) {
        auto people_inside = lift->get_people();
        int next_floor = people_inside.front()[0].get_dir();
        lift->change_floor(next_floor, my_logger);
        lift->change_doors(false, my_logger);
        int came_out = people_inside.front().size();
        lift->remove_first_group();
        lift->change_inside(0, came_out, my_logger);
        lift->change_doors(true, my_logger);
    }
    str_thread = "Thread #" + std::to_string(thread_id) + " finished\n";
    std::cout << str_thread;
}

int main() {
    auto log_file = std::make_unique<std::ofstream>("elevator_log.txt");
    if (!log_file->is_open()) {
        std::cerr << "Failed to open log file!" << std::endl;
        return 1;
    }

    Logger* my_logger = LoggerBuilder()
                            .set_level(Logger::DEBUG)
                            //.add_handler(std::cout)
                            .add_handler(std::move(log_file))
                            .make_object();

    const int num_threads = 3;

    std::vector<std::thread> threads;
    std::vector<std::unique_ptr<Lift>> lifts;

    for (int i = 0; i < num_threads; ++i) {
        lifts.push_back(std::make_unique<Lift>(4, 10));
        threads.emplace_back(Generate_elevator_calls, i, lifts.back().get(), my_logger);
    }

    for (auto &thread : threads) {
        thread.join();
    }

    std::cout << "All threads have been finished!" << std::endl;

    std::cout << "Elevator operation statistics:\n\n";

    std::cout << std::left
              << std::setw(8) << "Lift"
              << std::setw(20) << "People Transported"
              << std::setw(18) << "Floors Visited"
              << std::setw(15) << "Avg Wait Time"
              << "\n";

    std::cout << std::setfill('=')
              << std::setw(60) << ""
              << std::setfill(' ')
              << "\n";

    size_t all_people = 0, all_floors = 0;
    for (size_t i = 0; i < lifts.size(); ++i) {
        size_t cur_people = lifts[i]->get_people_count();
        size_t cur_floors = lifts[i]->get_floor_count();
        all_people += cur_people;
        all_floors += cur_floors;
        std::cout << std::left
                  << std::setw(8) << i+1
                  << std::setw(20) << cur_people
                  << std::setw(18) << cur_floors
                  << std::setw(15) << lifts[i]->get_time_wait() << std::endl;
    }

    std::cout << std::setfill('=')
              << std::setw(60) << ""
              << std::setfill(' ')
              << "\n\n";

    std::cout << "All Transported People:  " << all_people << std::endl;
    std::cout << "All Visited Floors: " << all_floors << std::endl;
    return 0;
}