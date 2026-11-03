// push_back vs emplace_back, and why the move constructor should be noexcept.
#include <iostream>
#include <vector>

struct Point {
    float x, y;

    Point(float x_, float y_) : x(x_), y(y_) { std::cout << "  constructed\n"; }
    Point(const Point& other) : x(other.x), y(other.y) { std::cout << "  copied\n"; }
    // A real move constructor takes a non-const rvalue reference.
    // `Point(const Point&&)` would bind rvalues but could not steal anything:
    // it would be a copy with a misleading name.
    Point(Point&& other) noexcept : x(other.x), y(other.y) { std::cout << "  moved\n"; }
};

int main() {
    std::vector<Point> points;
    points.reserve(4);  // no reallocation below; Point has no default ctor,
                        // so `std::vector<Point> points(4)` would not compile

    std::cout << "push_back(Point(1, 2)):\n";
    points.push_back(Point(1, 2));  // construct a temporary, then move it in

    std::cout << "emplace_back(1, 2):\n";
    points.emplace_back(1, 2);      // construct in place from the arguments

    std::cout << "reallocation (capacity 1 -> 2):\n";
    std::vector<Point> small;
    small.reserve(1);
    small.emplace_back(3, 4);
    small.emplace_back(5, 6);  // old element is moved because the move ctor is
                               // noexcept; otherwise vector would copy it

    // emplace_back can call explicit constructors; push_back cannot:
    std::vector<std::vector<int>> nested;
    // nested.push_back(1000000);  // error: vector(size_type) is explicit
    nested.emplace_back(1000000);  // compiles: a vector of 1,000,000 zeros
    std::cout << "nested[0].size() = " << nested[0].size() << '\n';
}
