#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>

#include <engine/ecs/component/ComponentMgr.hpp>
#include <share/ecs/gameobject/GameObject.hpp>
// using namespace share::ecs::entity;

// ecs 的 component mgr 基准测试
BOOST_AUTO_TEST_SUITE(ComponentMgrTest)

// component 基础功能测试
BOOST_AUTO_TEST_CASE(t_basic_operation)
{
    auto obj = engine::ecs::GameObjectMgr::GetInstance()->Create<share::ecs::gameobject::GameObject>();

    BOOST_CHECK( obj != nullptr );
    
    // 在游戏对象上挂载组件
}

BOOST_AUTO_TEST_SUITE_END()