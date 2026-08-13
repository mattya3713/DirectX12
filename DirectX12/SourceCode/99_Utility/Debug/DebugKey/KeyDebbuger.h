#pragma once
#include "99_Utility/Singleton/SingletonTemplate.h"

class KeyDebbuger : public Singleton<KeyDebbuger>
{
	friend class Singleton<KeyDebbuger>;
private:
	KeyDebbuger() {}
public:
};

