#ifndef _I_ITERATOR_H
#define _I_ITERATOR_H

class IndexIterator {
public:
    IndexIterator(int lower, int upper, bool reverse)
        : m_current((reverse) ? upper : lower),
          m_lower(lower),
          m_upper(upper),
          m_reverse(reverse)
    {

    }
    ~IndexIterator() = default;

    inline void reset() { m_current = (m_reverse) ? m_upper : m_lower; }

    inline bool hasNext() const
    {
        return (m_reverse) ? (m_current >= m_lower) : (m_current <= m_upper);
    }

    inline int next()
    {
        int index = m_current;
        m_current = (m_reverse) ? (m_current - 1) : (m_current + 1);

        return index;
    }

private:
    int m_current;

    int m_lower;
    int m_upper;

    bool m_reverse;
};
    
#endif
