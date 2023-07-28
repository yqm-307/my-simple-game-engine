#pragma once
#include "Hashmap.hpp" // 为了语法提示
#include "util/assert/Assert.hpp"
#include <bbt/timer/Interval.hpp>
#include <bbt/Define.hpp>

// using namespace
namespace game::util::hashmap
{

template<typename TKey, typename TValue, size_t BucketNum>
Hashmap<TKey, TValue, BucketNum>::Hashmap(const HashFunction& key_hash, const ValueType& default_value)
    :m_key_hash_func(key_hash),
    m_default_value(default_value)
{
    AssertWithInfo(key_hash != nullptr, "hash function not null!");
    AssertWithInfo(m_bucket_size > 0 && m_bucket_size <= MYGAME_HASH_MAX_BUCKET, "bucket num too much!"); /* 桶数量限制 */
    InitHashmap();
}

template<typename TKey, typename TValue, size_t BucketNum>
Hashmap<TKey, TValue, BucketNum>::Hashmap(const HashFunction& key_hash, const ValueType& default_value, std::initializer_list<std::pair<KeyType, ValueType>> args)
    :m_key_hash_func(key_hash),
    m_default_value(default_value),
    m_hash_map(m_bucket_size)
{
    AssertWithInfo(key_hash != nullptr, "hash function not null!");
    AssertWithInfo(m_bucket_size > 0 && m_bucket_size <= MYGAME_HASH_MAX_BUCKET, "bucket num too much!"); /* 桶数量限制 */
    InitHashmap();
    for(auto arg: args)
    {
        AssertWithInfo(Insert(arg.first, arg.second), "Hashmap init failed!");
    }
}

template<typename TKey, typename TValue, size_t BucketNum>
Hashmap<TKey, TValue, BucketNum>::~Hashmap()
{
}

template<typename TKey, typename TValue, size_t BucketNum>
void Hashmap<TKey, TValue, BucketNum>::InitHashmap() const
{
}

#pragma region "增删改查接口实现"

template<typename TKey, typename TValue, size_t BucketNum>
typename Hashmap<TKey, TValue, BucketNum>::Result Hashmap<TKey, TValue, BucketNum>::Find(const KeyType& key) const
{
    auto bucket_index = m_key_hash_func(key);
    if( bbt_unlikely(!CheckIndex(bucket_index)) )
        return {m_default_value, false};

    auto& bucket = m_hash_map[bucket_index];
    auto it = bucket.find(key);
    if( bbt_unlikely(it == bucket.end()))
        return {m_default_value, false};
        
    return {it->second, true};
}

template<typename TKey, typename TValue, size_t BucketNum>
bool Hashmap<TKey, TValue, BucketNum>::Insert(const KeyType& key, ValueType value)
{
    auto [bucket, isok] = GetBucket(key);
    if( bbt_unlikely(!isok) )
        return false;
        
    auto it = bucket->find(key);
    if( bbt_unlikely(it != bucket->end()) )
        return false;

    bucket->insert(std::make_pair(key, value));
    return true;
}

template<typename TKey, typename TValue, size_t BucketNum>
typename Hashmap<TKey, TValue, BucketNum>::Result Hashmap<TKey, TValue, BucketNum>::Earse(const KeyType& key)
{
    auto [bucket, isok] = GetBucket(key);
    if( bbt_unlikely(!isok) )
        return {m_default_value, false};
        
    auto it = bucket->find(key);
    if( bbt_unlikely(it == bucket->end()) )
        return {m_default_value, false};
        
    bucket->erase(it);
    return {it->second, true};
}

#pragma endregion

template<typename TKey, typename TValue, size_t BucketNum>
bool Hashmap<TKey, TValue, BucketNum>::CheckIndex(size_t idx)
{
    return ( idx >= 0 && idx < m_bucket_size );
}

template<typename TKey, typename TValue, size_t BucketNum>
typename Hashmap<TKey, TValue, BucketNum>::BucketResult Hashmap<TKey, TValue, BucketNum>::GetBucket(const KeyType& key)
{
    auto bucket_index = m_key_hash_func(key);
    if( bbt_unlikely(!CheckIndex(bucket_index)) )
        return {nullptr, false};

    return { &(m_hash_map[bucket_index]), true};
}

template<typename TKey, typename TValue, size_t BucketNum>
size_t Hashmap<TKey, TValue, BucketNum>::BucketSize() const
{
    return m_bucket_size;
}

template<typename TKey, typename TValue, size_t BucketNum>
size_t Hashmap<TKey, TValue, BucketNum>::Size() const
{
    size_t count = 0;
    for(auto&& map : m_hash_map)
    {
        count += map.size();
    }
    return count;
}

}