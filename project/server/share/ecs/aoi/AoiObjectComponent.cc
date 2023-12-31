#include "AoiObjectComponent.hpp"
#include "share/ecs/aoi/AoiDefine.hpp"
#include "share/ecs/Define.hpp"

namespace share::ecs::aoi
{

AoiObjectComponent::AoiObjectComponent(share::ecs::aoi::AoiEntityFlag mode)
    :Component(share::ecs::emComponentType::EM_COMPONENT_TYPE_AOI_OBJECT),
    m_id(aoi::GenAoiObjectId()),
    m_mode(mode)
{
}

AoiObjectComponent::~AoiObjectComponent()
{
}

ecs::aoi::AoiEntityFlag AoiObjectComponent::GetMode()
{
    return m_mode;
}

void AoiObjectComponent::OnMove(util::pos::Point3 vec)
{
    m_prev_pos = m_curr_pos;
    m_curr_pos = vec;
}

void AoiObjectComponent::OnMove(Tower* tower)
{
    m_curr_tower = tower;
}

util::pos::Point3 AoiObjectComponent::GetCurrentPos()
{
    return m_curr_pos;
}

AoiObjectId AoiObjectComponent::GetObjId()
{
    return m_id;
}

void AoiObjectComponent::OnEnterAoi(engine::ecs::GameObjectId aoi_gameobject_id)
{
    m_prev_aoi_gameobj_id = m_curr_aoi_gameobj_id;
    m_curr_aoi_gameobj_id = aoi_gameobject_id;
}

void AoiObjectComponent::OnLeaveAoi(engine::ecs::GameObjectId aoi_gameobject_id)
{
    AssertWithInfo(m_curr_aoi_gameobj_id == aoi_gameobject_id, "history list is bad!");
    m_curr_aoi_gameobj_id = m_prev_aoi_gameobj_id;
    m_prev_aoi_gameobj_id = engine::ecs::InvalidGameObjectId;
}

engine::ecs::GameObjectSPtr AoiObjectComponent::GetCurrentAoi()
{
    if (engine::ecs::GameObjectIDInvalid(m_curr_aoi_gameobj_id))
        return nullptr;

    auto [gameobject, isok] = G_GameObjectMgr()->Search(m_curr_aoi_gameobj_id);
    if (!isok)
        return nullptr;

    return gameobject;
}


}