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

    RWField(int, Speed, "/*‚±‚±‚É•Ï”–¼‚Ìà–¾‚ª“ü‚é*/")
        RWField(int, Speed, "/*‚±‚±‚É•Ï”–¼‚Ìà–¾‚ª“ü‚é*/")

        int m_Speed; // aaa.

public:
        MyClass() 
        {
             Speed;
        }
};

int main() {
    MyClass obj;

    // “Ç‚İæ‚èê—p
    std::cout << "readOnlyInt: " << obj.GetSpeed() << std::endl;



    return 0;
}