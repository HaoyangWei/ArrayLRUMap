#ifndef __ARRAYLRUMAP_H__
#define __ARRAYLRUMAP_H__

#include <stdint.h>
#include <time.h>
#include <unordered_map>

template<typename key_t, typename val_t, size_t MAP_MAX_SIZE, time_t EXPIRE_TIME = 0> /*EXPIRE_TIME: data expired time in second*/
class lru_map
{
    enum 
    { 
        LINK_LIST = MAP_MAX_SIZE, 
    };
    typedef std::unordered_map<key_t, size_t> map_t;
    struct data_t
    {
        key_t  _key;
        val_t  _val;
        size_t _prev;
        size_t _next;
        time_t _expire;
    } ;

private:
    time_t _get_current_time() 
    {
        if (EXPIRE_TIME == 0)
        {
            return 0;
        }
        time_t curr_time;
        time(&curr_time);
        return curr_time;
    }
    time_t _get_expired_time() 
    {
        return EXPIRE_TIME != 0 ? _get_current_time() + EXPIRE_TIME : 0;
    }

public:
    void recover()
    {
        size_t curr = _real_data[LINK_LIST]._next;
        while (curr != LINK_LIST)
        {
            size_t next = _real_data[curr]._next;
            new (&(_real_data[curr]._key)) key_t;
            new (&(_real_data[curr]._val)) val_t;
            _key_index.insert(std::make_pair(_real_data[curr]._key, curr));
            curr = next;
        }
    }

public:
    void clear()
    {
        _key_index.clear();
        _free_size = 0;
        for (size_t i = 0; i < MAP_MAX_SIZE; ++i)
        {
            _free_list[_free_size++] = i;
        }
        _real_data[LINK_LIST]._prev = LINK_LIST;
        _real_data[LINK_LIST]._next = LINK_LIST;
    }
    bool empty() const
    {
        return _free_size == (size_t)0;
    }
    size_t size() const
    {
        return MAP_MAX_SIZE - _free_size;
    }
    val_t* put(const key_t& key, const val_t& val)
    {
        _try_free_data(true);
        data_t* data = _put_data(key, val);
        if (data == NULL)
        {
            return NULL;
        }
        return &data->_val;
    }
    val_t* get(const key_t& key)
    {
        _try_free_data(false);
        data_t* data = _get_data_by_key(key);
        if (data == NULL)
        {
            return NULL;
        }
        if (EXPIRE_TIME != 0 && data->_expire < _get_current_time())
        {
            return NULL;
        }
        return &data->_val;
    }

private:
    void _try_free_data(bool force)
    {
        if (force == true && _free_size == (size_t)0)
        {
            size_t front = _real_data[LINK_LIST]._next;
            _del_data_by_key(_real_data[front]._key);
        }
        if (EXPIRE_TIME == 0)
        {
            return;
        }
        size_t front = _real_data[LINK_LIST]._next;
        if (_real_data[front]._expire < _get_current_time())
        {
            _del_data_by_key(_real_data[front]._key);
        }
    }

private:
    data_t* _put_data(const key_t& key, const val_t& val)
    {
        data_t* data = _get_data_by_key(key);
        if (data != NULL)
        {
            data->_val = val;
            return data;
        }
        if (_free_size == 0)
        {
            return NULL;
        }
        size_t index = _free_list[--_free_size];
        _real_data[index]._key = key;
        _real_data[index]._val = val;
        _append_list_node(_real_data[index]);
        _key_index.insert(std::make_pair(key, index));
        return &_real_data[index];
    }
    void _del_data_by_key(const key_t& key)
    {
        auto it = _key_index.find(key);
        if (it == _key_index.end())
        {
            return;
        }
        size_t index = it->second;
        _remove_list_node(_real_data[index]);
        _free_list[_free_size++] = index;
        _key_index.erase(key);
    }
    data_t* _get_data_by_key(const key_t& key)
    {
        auto it = _key_index.find(key);
        if (it == _key_index.end())
        {
            return NULL;
        }
        size_t index = it->second;
        _remove_list_node(_real_data[index]);
        _append_list_node(_real_data[index]);
        return &_real_data[index];
    }

private:
    void _remove_list_node(data_t& data)
    {
        _real_data[data._prev]._next = data._next;
        _real_data[data._next]._prev = data._prev;
    }
    void _append_list_node(data_t& data)
    {
        data._prev = _real_data[LINK_LIST]._prev;
        data._next = LINK_LIST;
        size_t index = &data - _real_data;
        _real_data[_real_data[LINK_LIST]._prev]._next = index;
        _real_data[LINK_LIST]._prev = index;
        data._expire = _get_expired_time();
    }

private:
    map_t  _key_index; // cannot recover directly
    size_t _free_size;
    size_t _free_list[MAP_MAX_SIZE];
    data_t _real_data[MAP_MAX_SIZE + 1];
};

#endif //__ARRAYLRUMAP_H__
