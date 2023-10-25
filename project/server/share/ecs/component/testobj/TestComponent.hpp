#pragma once
#include "engine/ecs/component/Component.hpp"
#include <functional>

namespace share::ecs::component
{

class TestComponent:
    engine::ecs::Component
{
    ComponentDeriveClassDef;
public:
    typedef std::function<void(engine::ecs::GameObjectSPtr)> MyCallback; 

    ~TestComponent();    
protected:
    TestComponent(const MyCallback& OnAdd, const MyCallback& OnDel);
private:

};

}