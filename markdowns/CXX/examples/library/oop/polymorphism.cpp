// Runtime polymorphism through an abstract base class, plus checked downcasts.
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class Shape {  // abstract: has at least one pure virtual function
public:
    explicit Shape(std::string name) : name_(std::move(name)) {}
    virtual ~Shape() = default;
    virtual double area() const = 0;  // pure virtual
    const std::string& name() const { return name_; }

private:
    std::string name_;
};

class Rectangle : public Shape {
public:
    Rectangle(double w, double h) : Shape("rectangle"), w_(w), h_(h) {}
    double area() const override { return w_ * h_; }
    double width() const { return w_; }

private:
    double w_, h_;
};

class Triangle : public Shape {
public:
    Triangle(double base, double h) : Shape("triangle"), b_(base), h_(h) {}
    double area() const override { return b_ * h_ / 2; }

private:
    double b_, h_;
};

class Circle : public Shape {
public:
    explicit Circle(double r) : Shape("circle"), r_(r) {}
    double area() const override { return 3.14159265 * r_ * r_; }

private:
    double r_;
};

int main() {
    // Shape s("x");  // error: cannot instantiate an abstract class

    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(std::make_unique<Rectangle>(3, 3));
    shapes.push_back(std::make_unique<Triangle>(3, 3));
    shapes.push_back(std::make_unique<Circle>(3));

    for (const auto& s : shapes) {  // one interface, different behavior
        std::cout << s->name() << " area = " << s->area() << '\n';
    }

    // Downcast: dynamic_cast checks the real type at run time.
    for (const auto& s : shapes) {
        if (auto* r = dynamic_cast<Rectangle*>(s.get())) {
            std::cout << "found a Rectangle, width = " << r->width() << '\n';
        } else {
            std::cout << s->name() << " is not a Rectangle (dynamic_cast -> nullptr)\n";
        }
    }
}   // unique_ptr deletes each shape through the virtual destructor
