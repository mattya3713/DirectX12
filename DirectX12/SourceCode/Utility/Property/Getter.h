template<typename T>
class Getter {
protected:
    T value;

public:
    Getter() = default;
    Getter(const T& val) : value(val) {}

    const T& get() const { return value; }
};