#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <ranges>
#include <span>
#include <utility>

// U = {u : u.id in N}
struct User {
    int id;
    std::string username;
    std::string email;
    bool is_moderator;
};

// C = {c : c.id in N, c.owner_id in U.id}
struct Course {
    int id;
    int owner_id;
    std::string title;
    std::string description;
};

// L = {l : l.id in N, l.course_id in C.id, l.owner_id in U.id}
struct Lesson {
    int id;
    int course_id;
    int owner_id;
    std::string title;
    std::string description;
    std::string video_url;
};

// S subset U.id x C.id, (u,c) unique
struct Subscription {
    int user_id;
    int course_id;
};

class Platform {
    // M_C : id -> Course, O(log n) lookup
    std::map<int, Course> courses_;
    // M_L : id -> Lesson, O(log n) lookup
    std::map<int, Lesson> lessons_;
    // U : vector<User>
    std::vector<User> users_;
    // S subset U.id x C.id
    std::vector<Subscription> subscriptions_;

    int next_course_id_ = 1;
    int next_lesson_id_ = 1;

public:
    Platform() = default;

    Platform(std::vector<User> users,
             std::map<int, Course> courses,
             std::map<int, Lesson> lessons,
             std::vector<Subscription> subs = {})
        : courses_(std::move(courses)),
          lessons_(std::move(lessons)),
          users_(std::move(users)),
          subscriptions_(std::move(subs)) {}

    // |U| += 1
    void add_user(User&& u) {
        users_.push_back(std::move(u));
    }

    void add_user(const User& u) {
        users_.push_back(u);
    }

    // M_C[c.id] = c, O(log |M_C|)
    void add_course(Course&& c) {
        if (c.id == 0) c.id = next_course_id_++;
        else if (c.id >= next_course_id_) next_course_id_ = c.id + 1;
        int id = c.id;
        courses_[id] = std::move(c);
    }

    void add_course(const Course& c) {
        Course nc = c;
        add_course(std::move(nc));
    }

    // M_L[l.id] = l, O(log |M_L|)
    void add_lesson(Lesson&& l) {
        if (l.id == 0) l.id = next_lesson_id_++;
        else if (l.id >= next_lesson_id_) next_lesson_id_ = l.id + 1;
        int id = l.id;
        lessons_[id] = std::move(l);
    }

    void add_lesson(const Lesson& l) {
        Lesson nl = l;
        add_lesson(std::move(nl));
    }

    // F(C, u) = {c in C | owner(c) = u v is_moderator(u)}, O(|C|)
    [[nodiscard]] std::vector<Course> get_courses_for_user(int user_id) const {
        bool moderator = false;
        auto it = std::ranges::find_if(users_,
            [user_id](const User& u) { return u.id == user_id; });
        if (it != users_.end()) moderator = it->is_moderator;

        std::vector<Course> result;
        for (const auto& [id, c] : courses_) {
            // moderator -> forall c : c in result; not moderator -> c.owner_id = user_id
            if (moderator || c.owner_id == user_id) {
                result.push_back(c);
            }
        }
        return result;
    }

    // L_c = {l in L | l.course_id = c}, O(|L|)
    [[nodiscard]] std::vector<Lesson> get_lessons_for_course(int course_id) const {
        std::vector<Lesson> result;
        for (const auto& [id, l] : lessons_) {
            if (l.course_id == course_id) {
                result.push_back(l);
            }
        }
        return result;
    }

    // S' = S triangle {(u,c)}, triangle = symmetric difference, O(|S|)
    // returns true <=> added, false <=> removed
    bool toggle_subscription(int user_id, int course_id) {
        auto it = std::ranges::find_if(subscriptions_,
            [user_id, course_id](const Subscription& s) {
                return s.user_id == user_id && s.course_id == course_id;
            });
        if (it != subscriptions_.end()) {
            // (u,c) in S -> S' = S \ {(u,c)}
            subscriptions_.erase(it);
            return false;
        }
        // (u,c) not in S -> S' = S union {(u,c)}
        subscriptions_.push_back({user_id, course_id});
        return true;
    }

    // owner(u, c) <-> exists c in C : c.owner_id = u, O(log |C|)
    [[nodiscard]] bool is_owner(int user_id, int course_id) const {
        auto it = courses_.find(course_id);
        if (it == courses_.end()) return false;
        return it->second.owner_id == user_id;
    }

    // has_sub(u, c) <-> (u,c) in S, O(|S|)
    [[nodiscard]] bool has_subscription(int user_id, int course_id) const {
        return std::ranges::any_of(subscriptions_,
            [user_id, course_id](const Subscription& s) {
                return s.user_id == user_id && s.course_id == course_id;
            });
    }

    // G(u) = {c in C | (u, c.id) in S}, O(|S| * log|C|)
    [[nodiscard]] std::vector<Course> get_subscribed_courses(int user_id) const {
        std::vector<Course> result;
        for (const auto& s : subscriptions_) {
            if (s.user_id == user_id) {
                auto it = courses_.find(s.course_id);
                if (it != courses_.end()) {
                    result.push_back(it->second);
                }
            }
        }
        return result;
    }

    // sort C by |{l in L : l.course_id = c.id}| desc, stable_sort O(n log n)
    [[nodiscard]] std::vector<std::pair<Course, int>> courses_by_lesson_count() const {
        // cnt(c) = |{l in L : l.course_id = c}|, O(|L|)
        std::unordered_map<int, int> cnt;
        for (const auto& [id, l] : lessons_) {
            cnt[l.course_id]++;
        }
        // R = [(c, cnt(c)) forall c in C]
        std::vector<std::pair<Course, int>> result;
        result.reserve(courses_.size());
        for (const auto& [id, c] : courses_) {
            auto cit = cnt.find(id);
            result.emplace_back(c, cit != cnt.end() ? cit->second : 0);
        }
        // stable_sort by cnt desc, O(|C| log |C|)
        std::ranges::stable_sort(result,
            [](const auto& a, const auto& b) { return a.second > b.second; });
        return result;
    }

    // f(c) = |{s in S : s.course_id = c}| forall c, O(|S|)
    [[nodiscard]] std::unordered_map<int, int> count_subscribers_per_course() const {
        std::unordered_map<int, int> result;
        for (const auto& [id, c] : courses_) {
            result[id] = 0;
        }
        for (const auto& s : subscriptions_) {
            result[s.course_id]++;
        }
        return result;
    }

    // forall c in C : print(c), O(|C|)
    void print_all_courses() const {
        for (const auto& [id, c] : courses_) {
            std::cout << "  [" << id << "] " << c.title
                      << " (owner_id=" << c.owner_id << ")\n";
        }
    }

    // U_id -> username, O(|U|)
    [[nodiscard]] std::string_view get_username(int user_id) const {
        auto it = std::ranges::find_if(users_,
            [user_id](const User& u) { return u.id == user_id; });
        if (it != users_.end()) return it->username;
        static constexpr std::string_view unknown = "?";
        return unknown;
    }

    // |C|
    [[nodiscard]] int course_count() const { return static_cast<int>(courses_.size()); }

    // exists c in C : c.id = course_id
    [[nodiscard]] bool course_exists(int course_id) const {
        return courses_.count(course_id) > 0;
    }

    // exists u in U : u.id = user_id
    [[nodiscard]] bool user_exists(int user_id) const {
        return std::ranges::any_of(users_,
            [user_id](const User& u) { return u.id == user_id; });
    }

    // U -> stdout, O(|U|)
    void print_users() const {
        for (const auto& u : users_) {
            std::cout << "  [" << u.id << "] " << u.username
                      << (u.is_moderator ? " (moderator)" : "") << "\n";
        }
    }
};

int main() {
    Platform p;

    // U = {u1(mod), u2, u3}, |U| = 3
    p.add_user({1, "admin_ivan", "ivan@example.com", true});
    p.add_user({2, "student_anna", "anna@example.com", false});
    p.add_user({3, "teacher_boris", "boris@example.com", false});

    // C = {c1..c5}, |C| = 5
    p.add_course({1, 3, "Python Backend",       "Django, DRF, PostgreSQL"});
    p.add_course({2, 3, "C++ Algorithms",        "STL, graphs, DP"});
    p.add_course({3, 2, "Data Science",           "pandas, numpy, sklearn"});
    p.add_course({4, 3, "DevOps",                 "Docker, CI/CD, Kubernetes"});
    p.add_course({5, 2, "Frontend React",         "React, TypeScript, Redux"});

    // L = {l1..l12}, |L| = 12
    p.add_lesson({1,  1, 3, "Django Models",        "ORM basics",           "https://vid.io/1"});
    p.add_lesson({2,  1, 3, "DRF Serializers",      "Serialization",        "https://vid.io/2"});
    p.add_lesson({3,  1, 3, "DRF Views",            "ViewSets, APIView",    "https://vid.io/3"});
    p.add_lesson({4,  2, 3, "STL Containers",       "vector, map, set",     "https://vid.io/4"});
    p.add_lesson({5,  2, 3, "Graph BFS/DFS",        "Traversals",           "https://vid.io/5"});
    p.add_lesson({6,  2, 3, "Dynamic Programming",  "Memoization, tabulation","https://vid.io/6"});
    p.add_lesson({7,  2, 3, "Sorting Algorithms",   "merge, quick, radix",  "https://vid.io/7"});
    p.add_lesson({8,  3, 2, "Pandas Intro",         "DataFrame, Series",    "https://vid.io/8"});
    p.add_lesson({9,  3, 2, "NumPy Arrays",         "ndarray operations",   "https://vid.io/9"});
    p.add_lesson({10, 4, 3, "Docker Basics",        "Containers, images",   "https://vid.io/10"});
    p.add_lesson({11, 5, 2, "React Components",     "JSX, props, state",    "https://vid.io/11"});
    p.add_lesson({12, 5, 2, "Redux State",          "Actions, reducers",    "https://vid.io/12"});

    // S_0 = {(2,1),(2,2),(3,1),(3,4),(2,5)}
    p.toggle_subscription(2, 1);
    p.toggle_subscription(2, 2);
    p.toggle_subscription(3, 1);
    p.toggle_subscription(3, 4);
    p.toggle_subscription(2, 5);

    int choice = 0;
    while (true) {
        std::cout << "\n========================================\n";
        std::cout << "       ОБРАЗОВАТЕЛЬНАЯ ПЛАТФОРМА\n";
        std::cout << "========================================\n";
        std::cout << "1. Курсы (список, фильтр по пользователю)\n";
        std::cout << "2. Уроки (список по курсу)\n";
        std::cout << "3. Подписки (переключить, список)\n";
        std::cout << "4. Отчёты (топ курсов по урокам, статистика подписок)\n";
        std::cout << "5. Проверка прав (владелец?, подписан?)\n";
        std::cout << "0. Выход\n";
        std::cout << "========================================\n";
        std::cout << "Выбор: ";
        std::cin >> choice;

        if (choice == 0) break;

        if (choice == 1) {
            std::cout << "\n--- Курсы ---\n";
            std::cout << "a) Все курсы\n";
            std::cout << "b) Курсы пользователя (фильтр)\n";
            std::cout << "Выбор (a/b): ";
            char sub;
            std::cin >> sub;

            if (sub == 'a') {
                // forall c in C : print(c)
                std::cout << "\nВсе курсы:\n";
                p.print_all_courses();
            } else if (sub == 'b') {
                std::cout << "Пользователи:\n";
                p.print_users();
                std::cout << "ID пользователя: ";
                int uid;
                std::cin >> uid;
                if (!p.user_exists(uid)) {
                    std::cout << "Пользователь не найден.\n";
                    continue;
                }
                // F(C, u), O(|C|)
                auto uc = p.get_courses_for_user(uid);
                std::cout << "\nКурсы для " << p.get_username(uid)
                          << " (|R|=" << uc.size() << "):\n";
                for (const auto& c : uc) {
                    std::cout << "  [" << c.id << "] " << c.title << "\n";
                }
            }
        }
        else if (choice == 2) {
            std::cout << "\n--- Уроки ---\n";
            std::cout << "Все курсы:\n";
            p.print_all_courses();
            std::cout << "ID курса: ";
            int cid;
            std::cin >> cid;
            if (!p.course_exists(cid)) {
                std::cout << "Курс не найден.\n";
                continue;
            }
            // L_c = {l in L | l.course_id = c}, O(|L|)
            auto ls = p.get_lessons_for_course(cid);
            std::cout << "\nУроки курса " << cid << " (|L_c|=" << ls.size() << "):\n";
            for (const auto& l : ls) {
                std::cout << "  [" << l.id << "] " << l.title
                          << " | " << l.video_url << "\n";
            }
        }
        else if (choice == 3) {
            std::cout << "\n--- Подписки ---\n";
            std::cout << "a) Переключить подписку\n";
            std::cout << "b) Список подписок пользователя\n";
            std::cout << "Выбор (a/b): ";
            char sub;
            std::cin >> sub;

            if (sub == 'a') {
                std::cout << "Пользователи:\n";
                p.print_users();
                std::cout << "ID пользователя: ";
                int uid;
                std::cin >> uid;
                if (!p.user_exists(uid)) {
                    std::cout << "Пользователь не найден.\n";
                    continue;
                }
                std::cout << "Все курсы:\n";
                p.print_all_courses();
                std::cout << "ID курса: ";
                int cid;
                std::cin >> cid;
                if (!p.course_exists(cid)) {
                    std::cout << "Курс не найден.\n";
                    continue;
                }
                // S' = S triangle {(u,c)}
                bool added = p.toggle_subscription(uid, cid);
                std::cout << (added ? "Подписка добавлена." : "Подписка удалена.") << "\n";
            } else if (sub == 'b') {
                std::cout << "Пользователи:\n";
                p.print_users();
                std::cout << "ID пользователя: ";
                int uid;
                std::cin >> uid;
                if (!p.user_exists(uid)) {
                    std::cout << "Пользователь не найден.\n";
                    continue;
                }
                // G(u) = {c in C | (u, c.id) in S}
                auto sc = p.get_subscribed_courses(uid);
                std::cout << "\nПодписки " << p.get_username(uid)
                          << " (|G|=" << sc.size() << "):\n";
                for (const auto& c : sc) {
                    std::cout << "  [" << c.id << "] " << c.title << "\n";
                }
            }
        }
        else if (choice == 4) {
            std::cout << "\n--- Отчёты ---\n";
            std::cout << "a) Топ курсов по количеству уроков\n";
            std::cout << "b) Статистика подписок\n";
            std::cout << "Выбор (a/b): ";
            char sub;
            std::cin >> sub;

            if (sub == 'a') {
                // sort C by |L_c| desc, stable_sort O(|C| log |C|)
                auto ranked = p.courses_by_lesson_count();
                std::cout << "\nКурсы по убыванию уроков:\n";
                int rank = 1;
                for (const auto& [c, cnt] : ranked) {
                    std::cout << "  #" << rank++ << " [" << c.id << "] "
                              << c.title << " — " << cnt << " уроков\n";
                }
            } else if (sub == 'b') {
                // f(c) = |{s in S : s.course_id = c}|, O(|S|)
                auto stats = p.count_subscribers_per_course();
                std::cout << "\nПодписчики по курсам:\n";
                // sort output by course_id
                std::vector<std::pair<int, int>> sorted_stats(stats.begin(), stats.end());
                std::ranges::sort(sorted_stats);
                for (const auto& [cid, cnt] : sorted_stats) {
                    std::cout << "  Курс " << cid << ": "
                              << cnt << " подписчиков\n";
                }
            }
        }
        else if (choice == 5) {
            std::cout << "\n--- Проверка прав ---\n";
            std::cout << "Пользователи:\n";
            p.print_users();
            std::cout << "ID пользователя: ";
            int uid;
            std::cin >> uid;
            if (!p.user_exists(uid)) {
                std::cout << "Пользователь не найден.\n";
                continue;
            }
            std::cout << "Все курсы:\n";
            p.print_all_courses();
            std::cout << "ID курса: ";
            int cid;
            std::cin >> cid;
            if (!p.course_exists(cid)) {
                std::cout << "Курс не найден.\n";
                continue;
            }
            // owner(u,c) <-> c.owner_id = u, O(log |C|)
            bool owns = p.is_owner(uid, cid);
            // (u,c) in S, O(|S|)
            bool subbed = p.has_subscription(uid, cid);
            std::cout << "\n" << p.get_username(uid) << " -> Курс " << cid << ":\n";
            std::cout << "  Владелец: " << (owns ? "Да" : "Нет") << "\n";
            std::cout << "  Подписан: " << (subbed ? "Да" : "Нет") << "\n";
        }
        else {
            std::cout << "Неверный выбор.\n";
        }
    }

    std::cout << "Выход.\n";
    return 0;
}
