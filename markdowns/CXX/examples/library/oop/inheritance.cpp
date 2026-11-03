// Inheritance: derived copy operations, object slicing, and
// static (non-virtual) vs dynamic (virtual) binding.
#include <iostream>
#include <string>

class Shape {
public:
    explicit Shape(int w = 0, int h = 0, std::string name = "shape")
        : name_(std::move(name)), width_(w), height_(h) {}  // same order as declared
    virtual ~Shape() = default;  // deleting through Shape* must reach the derived dtor

    virtual double area() const { return 0; }                         // dynamic binding
    void describe() const { std::cout << "Shape::describe " << name_ << '\n'; }  // static binding

private:
    std::string name_;

protected:
    int width_, height_;  // accessible in derived classes
};

class Rectangle : public Shape {
public:
    explicit Rectangle(int w = 0, int h = 0, std::string name = "rect")
        : Shape(w, h, std::move(name)) {}

    // Usually you write none of these (Rule of Zero). If you do, call the base
    // version explicitly; otherwise the base part is default-constructed.
    Rectangle(const Rectangle& other) : Shape(other), copies_(other.copies_ + 1) {}
    Rectangle& operator=(const Rectangle& other) {
        if (this != &other) {
            Shape::operator=(other);
            copies_ = other.copies_ + 1;
        }
        return *this;
    }

    double area() const override { return width_ * height_; }
    void describe() const {  // hides (does not override) Shape::describe
        std::cout << "Rectangle::describe (copies=" << copies_ << ") -> ";
        Shape::describe();   // call the base version explicitly
    }

private:
    int copies_ = 0;
};

int main() {
    Rectangle rect(3, 4);
    rect.describe();
    std::cout << "rect.area() = " << rect.area() << "\n\n";

    Rectangle copy(rect);
    copy.describe();
    Rectangle assigned;
    assigned = rect;
    assigned.describe();
    std::cout << '\n';

    // Upcast through a pointer/reference: no slicing.
    Shape* base = &rect;
    base->describe();  // non-virtual: chosen by static type -> Shape::describe
    std::cout << "base->area() = " << base->area() << " (virtual: Rectangle::area)\n";
    std::cout << "base->Shape::area() = " << base->Shape::area() << " (qualified call)\n\n";

    // Upcast by value: object slicing. Only the Shape part is copied.
    Shape sliced = rect;
    std::cout << "sliced.area() = " << sliced.area() << " (Rectangle part is gone)\n";
}
