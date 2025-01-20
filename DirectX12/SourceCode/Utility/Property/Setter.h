template<typename T>
class Setter {
protected:
    T value;

public:
    Setter() = default;
    Setter(const T& val) : value(val) {}

    void set(const T& val) { value = val; }
};