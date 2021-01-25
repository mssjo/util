#ifndef REF_PTR_H
#define REF_PTR_H

#ifdef REF_PTR_ASSIGN_FROM_SMART
#    include <memory>
#endif

namespace util {
    
/**
 * @brief A simple wrapper for a non-owning pointer to a const object.
 * 
 * <p>It works like const T&, except that it
 * <ul>
 *    <li>is nullable
 *    <li>is reassignable
 *    <li>has pointer semantics
 *    <li>allows references to itself (so it works better with STL)
 * </ul>
 * 
 * <p>It also works much like const T*, except that it
 * <ul>
 *    <li>provides some syntactic sugar
 *    <li>makes it abolutely clear that you don't claim ownership
 *    <li>avoids the anxiety one gets from raw pointers
 * </ul>
 * 
 * <p>Compared to smart pointers, it
 *    <li>is simpler
 *    <li>never modifies its pointee
 *    <li>doesn't bother with resource management
 * </ul>
 * 
 * <p>It should only be used when referencing objects that
 * are safely stored somewhere else, and whose lifetime
 * exceeds that of the pointer, much like how references
 * are used.
 */
template<typename T>
class cref_ptr {
private:
    const T* ptr;
    
public:
    cref_ptr() : ptr(nullptr) {}
    cref_ptr(nullptr_t) : ptr(nullptr) {}
    cref_ptr(const cref_ptr& p) : ptr(p.ptr) {}
    cref_ptr(cref_ptr&& p) : ptr(p.ptr) {}
    cref_ptr(const T* p) : ptr(p) {}
    cref_ptr(const T& p) : ptr(&p) {}
    
    virtual ~cref_ptr() {}
    
    const T& operator*() const {
        return *ptr;
    }
    const T* operator->() const {
        return ptr;
    }
    
    cref_ptr& operator= (const cref_ptr& p){
        ptr = p.ptr;    return *this;
    }
    cref_ptr& operator= (cref_ptr&& p){
        ptr = p.ptr;    return *this;
    }
    cref_ptr& operator= (const T* t_ptr){
        ptr = t_ptr;    return *this;
    }
    //A bit of syntactic sugar, especially useful if assigning from an iterator
    cref_ptr& operator= (const T& t){
        ptr = &t;       return *this;
    }
    
#ifdef REF_PTR_ASSIGN_FROM_SMART
    template<typename U, class E>
    cref_ptr& operator= (const std::unique_ptr<U, E>& un_ptr){
        ptr = un_ptr.get(); return *this;
    }
    template<typename U>
    cref_ptr& operator= (const std::shared_ptr<U>& sh_ptr){
        ptr = sh_ptr.get(); return *this;
    }
    template<typename U>
    cref_ptr& operator= (const std::weak_ptr<U>& wk_ptr){
        ptr = wk_ptr.get(); return *this;
    }
#endif
    
    operator bool () const {
        return ptr != nullptr;
    }
    
    
    friend bool operator== (const cref_ptr<T>& p, const T* t_ptr){
        return p.ptr == t_ptr;
    }
    friend bool operator!= (const cref_ptr<T>& p, const T* t_ptr){
        return p.ptr != t_ptr;
    }
    friend bool operator== (const cref_ptr<T>& p, const T& t){
        return p.ptr == &t;
    }
    friend bool operator!= (const cref_ptr<T>& p, const T& t){
        return p.ptr != &t;
    }
};


/**
 * @brief A simple wrapper for a non-owning pointer to an object, like a non-const
 * version of @c cref_ptr.
 * 
 * <p>It works like const T&, except that it
 * <ul>
 *    <li>is nullable
 *    <li>has pointer semantics
 *    <li>allows references to itself (so it works better with STL)
 * </ul>
 * 
 * <p>It also works much like const T*, except that it
 * <ul>
 *    <li>provides some syntactic sugar
 *    <li>makes it abolutely clear that you don't claim ownership
 *    <li>avoids the anxiety one gets from raw pointers
 *    <li>doesn't permit deletion
 * </ul>
 * 
 * <p>Compared to smart pointers, it
 *    <li>is simpler
 *    <li>doesn't bother with resource management
 *    <li>again, doesn't permit deletion
 * </ul>
 * 
 * <p>It should only be used when referencing objects that
 * are safely stored somewhere else, and whose lifetime
 * exceeds that of the pointer, much like how references
 * are used.
 */
template<typename T>
class ref_ptr {
private:
    T* ptr;
    
public:
    ref_ptr() : ptr(nullptr) {}
    ref_ptr(nullptr_t) : ptr(nullptr) {}
    ref_ptr(ref_ptr& p) : ptr(p.ptr) {}
    ref_ptr(ref_ptr&& p) : ptr(p.ptr) {}
    ref_ptr(T* p) : ptr(p) {}
    ref_ptr(T& p) : ptr(&p) {}
    
    virtual ~ref_ptr() {}
    
    T& operator*() const {
        return *ptr;
    }
    T* operator->() const {
        return ptr;
    }
    
    ref_ptr& operator= (ref_ptr& p){
        ptr = p.ptr;    return *this;
    }
    ref_ptr& operator= (ref_ptr&& p){
        ptr = p.ptr;    return *this;
    }
    ref_ptr& operator= (T* t_ptr){
        ptr = t_ptr;    return *this;
    }
    //A bit of syntactic sugar, especially useful if assigning from an iterator
    ref_ptr& operator= (T& t){
        ptr = &t;       return *this;
    }

#ifdef REF_PTR_ASSIGN_FROM_SMART
    template<typename U, class E>
    ref_ptr& operator= (const std::unique_ptr<U, E>& un_ptr){
        ptr = un_ptr.get(); return *this;
    }
    template<typename U>
    ref_ptr& operator= (const std::shared_ptr<U>& sh_ptr){
        ptr = sh_ptr.get(); return *this;
    }
    template<typename U>
    ref_ptr& operator= (const std::weak_ptr<U>& wk_ptr){
        ptr = wk_ptr.get(); return *this;
    }
#endif

    operator bool () const {
        return ptr != nullptr;
    }
    
    friend bool operator== (const ref_ptr<T>& p, const T* t_ptr){
        return p.ptr == t_ptr;
    }
    friend bool operator!= (const ref_ptr<T>& p, const T* t_ptr){
        return p.ptr != t_ptr;
    }
    friend bool operator== (const ref_ptr<T>& p, const T& t){
        return p.ptr == &t;
    }
    friend bool operator!= (const ref_ptr<T>& p, const T& t){
        return p.ptr != &t;
    }
};

};

#endif
