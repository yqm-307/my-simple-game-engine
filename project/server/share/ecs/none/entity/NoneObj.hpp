#pragma once
#include <engine/ecs/gameobject/GameObject.hpp>

namespace share::ecs::entity::none
{

/**
 * @brief 无实体的空对象，用来挂在component
 */
class NoneObj:
    public engine::ecs::GameObject
{
    GameObjectDeriveClassDef;
public:
    ~NoneObj();

    virtual void OnCreate();
    virtual void OnDestory();
    virtual void OnUpdate() override;
protected:
    explicit NoneObj();

};

};