/*
Copyright 2022-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _IR_SHARED_PTR_H_
#define _IR_SHARED_PTR_H_

#include <memory>
#include "lib/log.h"

namespace IR {

template<class T> class shared_ptr;

class shared_ptr_base {
    template<class T> friend class shared_ptr;
    mutable std::atomic<uint32_t>       refcount;
    bool                                not_on_heap;
    static void                         *last_alloc;

 public:
    void *operator new(size_t size) {
        BUG_CHECK(last_alloc == nullptr, "Failed to catch IR::Node heap allocation");
        last_alloc = ::operator new(size);
        return last_alloc; }
    shared_ptr_base() : refcount(0) {
        not_on_heap = last_alloc == nullptr;
        last_alloc = nullptr; }
    shared_ptr_base(const shared_ptr_base &) : shared_ptr_base() {}   // copying ignores refcnt
    shared_ptr_base(shared_ptr_base &&) : shared_ptr_base() {}        // moving ignores refcnt
    shared_ptr_base &operator=(const shared_ptr_base &) { return *this; }  // copying ignores refcnt
    shared_ptr_base &operator=(shared_ptr_base &&) { return *this; }       // moving ignores refcnt

 protected:
    const char *dbheap() const { return not_on_heap ? "" : " (heap)"; }
};

/* Shared (reference counted) pointer type for IR classes -- not using std::shared_ptr as
 * that works poorly with pointers to objects not allocated on the heap (allocated statically
 * or on the stack, or as part of another object.)  We intercept IR::INode::operator new to know
 * when Nodes are allocated on the heap or elsewhere */

template<class T> class shared_ptr {
// FIXME -- static assert fails due to circular uses causing invalid incomplete type errors
// static_assert(std::is_base_of<shared_ptr_base, T>::value,
//               "IR::shared_ptr only usable on subclasses of IR::shared_ptr_base");
    const       T *ptr;

 public:
    shared_ptr() : ptr(nullptr) {}
    shared_ptr(nullptr_t) : ptr(nullptr) {}                             // NOLINT(runtime/explicit)
    shared_ptr(const shared_ptr &a) { if ((ptr = a.ptr)) ptr->refcount++; }
    template<class U, typename = typename std::enable_if<std::is_base_of<T, U>::value>::type>
    shared_ptr(const shared_ptr<U> &a) { if ((ptr = a)) ptr->refcount++; }
    template<class U, typename = typename std::enable_if<std::is_base_of<T, U>::value>::type>
    shared_ptr(const U *a) { if ((ptr = a)) a->refcount++; }            // NOLINT(runtime/explicit)
    shared_ptr<T> &operator=(const shared_ptr<T> &a) {
        if (ptr == a) return *this;
        if (ptr && !ptr->not_on_heap && --ptr->refcount == 0) delete ptr;
        if ((ptr = a)) ptr->refcount++;
        return *this; }
    template<class U, typename = typename std::enable_if<std::is_base_of<T, U>::value>::type>
    shared_ptr<T> &operator=(const shared_ptr<U> &a) {
        if (ptr == a) return *this;
        if (ptr && !ptr->not_on_heap && --ptr->refcount == 0) delete ptr;
        if ((ptr = a)) ptr->refcount++;
        return *this; }
    shared_ptr<T> &operator=(const T *a) {
        if (ptr == a) return *this;
        if (ptr && !ptr->not_on_heap && --ptr->refcount == 0) delete ptr;
        if ((ptr = a)) a->refcount++;
        return *this; }
    template<class U, typename = typename std::enable_if<std::is_base_of<T, U>::value>::type>
    shared_ptr<T> &operator=(const U *a) {
        if (ptr == a) return *this;
        if (ptr && !ptr->not_on_heap && --ptr->refcount == 0) delete ptr;
        if ((ptr = a)) a->refcount++;
        return *this; }
    shared_ptr<T> &operator=(nullptr_t) {
        if (ptr && !ptr->not_on_heap && --ptr->refcount == 0) delete ptr;
        return *this; }
    ~shared_ptr() { if (ptr && !ptr->not_on_heap && --ptr->refcount == 0) delete ptr; }

    const T *operator->() const { return ptr; }
    const T &operator*() const { return *ptr; }
    operator const T *() const { return ptr; }
    bool operator==(nullptr_t) const { return ptr == nullptr; }
    bool operator!=(nullptr_t) const { return ptr != nullptr; }
    explicit operator bool() const { return ptr != nullptr; }
};

}  // namespace IR

#endif /* _IR_SHARED_PTR_H_ */
