#pragma once

template<typename T, size_t MaxSize = 8>
class Queue{

    T m_data[MaxSize];
    size_t m_head = 0;
    size_t m_size = 0;

public:
    auto size() {return m_size;}
    auto isempty() {return m_size == 0;}
    auto isfull() {return m_size == MaxSize;}
    auto& top() {return m_data[m_head];}
    void pop() {
        if(m_size == 0) return;
        --m_size;
        m_head = (m_head + 1) % MaxSize;
    }
    bool enqueue(T t) {
        if(m_size == MaxSize)
            return false;
        m_data[(m_head + m_size) % MaxSize] = t;
        ++m_size;
        return true;
    }

    void show(){
        Serial.printf("h: %d, s: %d\n", m_head, m_size);
    }
};