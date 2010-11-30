#ifndef INCLUDED_ARRAY_H
#define INCLUDED_ARRAY_H

template<class T> class Array : Uncopyable
{
public:
    Array() : _data(0) { }
    void allocate(int n) { release(); _data = new T[n]; _n = n; }
    ~Array() { release(); }
    T& operator[](int i) { return _data[i]; }
    const T& operator[](int i) const { return _data[i]; }
    int count() const { return _n; }
    void swap(Array<T>& other)
    {
        T* d = other._data;
        other._data = _data;
        _data = d;
        int n = other._n;
        other._n = _n;
        _n = n;
    }
    bool operator==(const Array& other)
    {
        if (_n != other._n)
            return false;
        for (int i = 0; i < _n; ++i)
            if (_data[i] != other._data[i])
                return false;
        return true;
    }
    bool operator!=(const Array& other)
    {
        return !operator==(other);
    }
private:
    void release() { if (_data != 0) delete[] _data; }

    T* _data;
    int _n;
};

#endif // INCLUDED_ARRAY_H
