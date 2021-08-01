#ifdef RINGBUFFER

template <typename T>
RingBuffer<T>::RingBuffer(int capacity) : m_capacity(capacity), m_num_datas(0), m_buffer(capacity)
{

}

template <typename T>
bool RingBuffer<T>::push(const T &data) 
{ 
    return push_data(std::forward<T>(data)); 
} 	

template <typename T>
bool RingBuffer<T>::push(T &&data) 
{ 
    return push_data(data); 
} 

template <typename T>
bool RingBuffer<T>::pop(T &data)
{
    if (m_num_datas > 0) {
        data = std::move(m_buffer[m_get_pos]);
        add(m_get_pos);
        m_num_datas--;
        return true;
    }

    return false;
}	

template <typename T>
bool RingBuffer<T>::is_full()  const 
{ 
    return ((m_num_datas == m_capacity) ? true : false); 
}	

template <typename T>
bool RingBuffer<T>::is_empty() const 
{ 
    return ((m_num_datas == 0) ? true : false); 
}

template <typename T>
int  RingBuffer<T>::size() const 
{ 
    return m_num_datas; 
}

template <typename T>
template <typename F>
bool RingBuffer<T>::push_data(F &&data)
{
    if (m_num_datas < m_capacity)
    {
        m_buffer[m_put_pos] = std::forward<F>(data);
        add(m_put_pos);
        m_num_datas++;
        return true;
    }

    return false;
}

template <typename T>
void RingBuffer<T>::add(int &pos)
{	
    pos = (((pos + 1) == m_capacity) ? 0 : (pos + 1));
}

#endif