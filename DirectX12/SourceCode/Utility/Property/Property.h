#define RWField(type, name, comment)                      \
public:                                                   \
    /* Get method for 'name' variable */                  \
    const type& Get##name() const                         \
    {                                                     \
        return name;                                      \
    }                                                     \
    /* Set method for 'name' variable */                  \
    void Set##name(const type& value)                     \
    {                                                     \
        name = value;                                     \
    }                                                     \
private:                                                  \
    /* comment */                                         \
    type name = {};                                                                            

class MyClass {

public:

    RWField(int, Speed, "/*ここに変数名の説明が入る*/")
        RWField(int, Speed, "/*ここに変数名の説明が入る*/")

        int m_Speed; // aaa.

public:
        MyClass() 
        {
             Speed;
        }
};

int main() {
    MyClass obj;

    // 読み取り専用
    std::cout << "readOnlyInt: " << obj.GetSpeed() << std::endl;



    return 0;
}