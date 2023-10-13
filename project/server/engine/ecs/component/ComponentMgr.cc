#include "engine/ecs/component/ComponentMgr.hpp"
#include "util/other/ConstString.hpp"

namespace engine::ecs
{

ComponentMgr::ComponentMgr()
    :m_component_info_map(
        [](const ComponentTemplateId& key){ return (size_t)(key%ComponentHashBucketNum);},
        ComponentInfo("", -1))
{
}

ComponentMgr::~ComponentMgr()
{
    m_component_info_map.Clear();
}

const std::unique_ptr<ComponentMgr>& ComponentMgr::GetInstance()
{
    static std::unique_ptr<ComponentMgr> _instance = nullptr;
    if(_instance == nullptr)
        _instance = std::unique_ptr<ComponentMgr>(new ComponentMgr());
    return _instance;
}


const std::string& ComponentMgr::GetComponentName(ComponentTemplateId id)
{
    auto [obj, isok] = m_component_info_map.Find(id);
    if(!isok)
        return util::other::string::EmptyString;

    auto& name = std::get<0>(obj);
    return name;
}

bool ComponentMgr::OnInitComponent(KeyType key, ValueType value)
{
    auto [_,isok] = m_component_map.insert(std::make_pair(key, value));
    return isok;
}

bool ComponentMgr::OnDestoryComponent(KeyType key)
{
    auto it = m_component_map.find(key);
    if(it == m_component_map.end())
    {
        return false;
    }

    m_component_map.erase(it);
    return true;
}

ComponentMgr::Result ComponentMgr::Search(KeyType key)
{
    auto it_obj = m_component_map.find(key);
    if(it_obj == m_component_map.end())
        return {nullptr, false};
    
    return {it_obj->second, true};
}

bool ComponentMgr::IsExist(KeyType key)
{
    return !(m_component_map.find(key) == m_component_map.end());
}

bool ComponentMgr::InitTemplateInfo(std::initializer_list<ComponentInfo> list)
{
    static bool is_inited = false;
    if( is_inited )
        return false;

    is_inited = true;

    for(auto&& it : list)
    {
        std::string info = std::get<0>(it);
        ComponentId id = std::get<1>(it);
        AssertWithInfo(m_component_info_map.Insert(id, it), "check component info register is right!"); // 请检查组件注册时是否正确
    }

    return true;
}

}