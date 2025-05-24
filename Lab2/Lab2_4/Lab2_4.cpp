#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <ctime>
#include <thread>
#include <mutex>

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
    size_t inside = 0;
    int floor = 1;
    size_t people_count = 0;
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
    [[nodiscard]] size_t cur_inside() const {
        return this->inside;
    }
    [[nodiscard]] int cur_floor() const {
        return this->floor;
    }
    [[nodiscard]] bool cur_doors() const {
        return this->CLOSED_DOORS;
    }
    [[nodiscard]] size_t get_people_count() const {
        return this->people_count;
    }
    [[nodiscard]] int get_floor_count() const {
        return this->floor_count;
    }
    [[nodiscard]] int get_time_wait() const {
        return this->time_wait / this->calls_count;
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
    void change_inside(size_t came_in, size_t came_out, Logger* my_logger) {
        if (inside - came_out + came_in > capacity || inside < came_out) {
            my_logger->critical("Out of capacity");
        }
        time_wait++;
        people_count += came_in;
        inside += came_in - came_out;
        my_logger->info("People inside has been changed");
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
        bool group_exists = false;
        for (std::vector<Person> group : people) {
            if (group[0].get_dir() == person.get_dir()) {
                group.push_back(person);
                group_exists = true;
            }
        }
        if (group_exists == false) {
            std::vector<Person> group = {person};
            people.push_back(group);
        }
    }
};


void Generate_elevator_calls(int thread_id, Lift *lift, Logger* my_logger) {
    std::cout << "Thread #" << thread_id << " started\n";
    std::srand(time(nullptr));

    int random_calls = rand() % 10 + 5;
    while (random_calls > 0) {
        lift->change_doors(false, my_logger);
        size_t came_out = 0;
        std::vector<std::vector<Person>> people_inside = lift->get_people();
        if (!people_inside.empty() && people_inside.front()[0].get_dir() == lift->cur_floor()) {
            came_out = people_inside.front().size();
            lift->remove_first_group();
        }
        size_t came_in = rand() % (lift->cur_cap() - (lift->cur_inside() - came_out) + 1);
        for (int i = 0; i < came_in; i++) {
            int person_direction = rand() % lift->cur_max_floors() + 1;
            Person cur_person(person_direction);
            lift->add_person(cur_person);
        }
        lift->change_inside(came_in, came_out, my_logger);
        lift->change_doors(true, my_logger);
        int next_floor = lift->cur_floor();
        if (!people_inside.empty()) {
            next_floor = people_inside.front()[0].get_dir();
        }
        lift->change_floor(next_floor, my_logger);
        random_calls--;
        if (random_calls == 0) {
            while(lift->cur_inside() != 0) {
                lift->change_doors(false, my_logger);
                people_inside = lift->get_people();
                if (!people_inside.empty() && people_inside.front()[0].get_dir() == lift->cur_floor()) {
                    came_out = people_inside.front().size();
                    lift->remove_first_group();
                }
                lift->change_inside(0, came_out, my_logger);
                lift->change_doors(true, my_logger);
                next_floor = lift->cur_floor();
                if (!people_inside.empty()) {
                    next_floor = people_inside.front()[0].get_dir();
                }
                lift->change_floor(next_floor, my_logger);
            }
        }
    }
    std::cout << "Thread #" << thread_id << " finished\n";
}

int main() {
    auto log_file = std::make_unique<std::ofstream>("elevator_log.txt");
    if (!log_file->is_open()) {
        std::cerr << "Failed to open log file!" << std::endl;
        return 1;
    }

    Logger* my_logger = LoggerBuilder()
                            .set_level(Logger::DEBUG)
                            .add_handler(std::cout)
                            .add_handler(std::move(log_file))
                            .make_object();

    const int num_threads = 5;

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