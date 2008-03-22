#ifndef SHAREDDATAPOINTER_H
#define SHAREDDATAPOINTER_H

#include <QtCore/QGlobalStatic>

/**
 * That's a copy of QSharedDataPointer that we had to fork, as
 * we need to change the detach_helper() function for our use
 * case.
 */


template <class T> class SharedDataPointer
{
public:
    inline void detach() { if (d && d->ref != 1) detach_helper(); }
    inline T &operator*() { detach(); return *d; }
    inline const T &operator*() const { return *d; }
    inline T *operator->() { detach(); return d; }
    inline const T *operator->() const { return d; }
    inline operator T *() { detach(); return d; }
    inline operator const T *() const { return d; }
    inline T *data() { detach(); return d; }
    inline const T *data() const { return d; }
    inline const T *constData() const { return d; }

    inline bool operator==(const SharedDataPointer<T> &other) const { return d == other.d; }
    inline bool operator!=(const SharedDataPointer<T> &other) const { return d != other.d; }

    inline SharedDataPointer() { d = 0; }
    inline ~SharedDataPointer() { if (d && !d->ref.deref()) delete d; }

    explicit SharedDataPointer(T *data);
    inline SharedDataPointer(const SharedDataPointer<T> &o) : d(o.d) { if (d) d->ref.ref(); }
    inline SharedDataPointer<T> & operator=(const SharedDataPointer<T> &o) {
        if (o.d != d) {
            if (o.d)
                o.d->ref.ref();
            if (d && !d->ref.deref())
                delete d;
            d = o.d;
        }
        return *this;
    }
    inline SharedDataPointer &operator=(T *o) {
        if (o != d) {
            if (o)
                o->ref.ref();
            if (d && !d->ref.deref())
                delete d;
            d = o;
        }
        return *this;
    }

    inline bool operator!() const { return !d; }

private:
    void detach_helper();

    T *d;
};

template <class T>
Q_INLINE_TEMPLATE SharedDataPointer<T>::SharedDataPointer(T *adata) : d(adata)
{ if (d) d->ref.ref(); }

template <class T>
Q_OUTOFLINE_TEMPLATE void SharedDataPointer<T>::detach_helper()
{
    T *x = d->clone();
    x->ref.ref();
    if (!d->ref.deref())
        delete d;
    d = x;
}

#endif
