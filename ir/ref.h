#ifndef IR_REF_H_
#define IR_REF_H_

namespace IR {

template <class T> class REF {
    const T *ptr;
 public:
    REF() : ptr(nullptr) {}
    REF(const T *p) : ptr(p) {}
    REF(std::nullptr_t) : ptr(nullptr) {}
    REF(const REF &) = default;
    REF(REF &&) = default;
    REF &operator=(const REF &) = default;
    REF &operator=(REF &&) = default;
    REF &operator=(std::nullptr_t) { ptr = nullptr; return *this; }
    template<class U, typename = typename std::enable_if<std::is_convertible<const U *, const T *>::value>::type>
    REF(const REF<U> &a) : ptr(a.ptr) {}

    operator const T *() const { return ptr; }
    //explicit operator bool() const { return ptr != nullptr; }

    const T *operator->() const { return ptr; }
    const T &operator*() const { return *ptr; }

    bool operator==(std::nullptr_t) const { return ptr == nullptr; }
    bool operator!=(std::nullptr_t) const { return ptr != nullptr; }

    template<class U> const U *to() const { return dynamic_cast<const U *>(ptr); }
    template<class U> bool is() const { return dynamic_cast<const U *>(ptr) != nullptr; }
    cstring toString() const;
    void dbprint(std::ostream &out) const;

    template<class U> friend class REF;
};

struct ID;

template<> class REF<Expression> {
    const Expression *ptr;
 public:
    REF() : ptr(nullptr) {}
    REF(const Expression *p) : ptr(p) {}
    REF(std::nullptr_t) : ptr(nullptr) {}
    REF(const REF &) = default;
    REF(REF &&) = default;
    REF &operator=(const REF &) = default;
    REF &operator=(REF &&) = default;
    REF &operator=(std::nullptr_t) { ptr = nullptr; return *this; }
    template<class U, typename = typename std::enable_if<std::is_convertible<const U *, const Expression *>::value>::type>
    REF(const REF<U> &a) : ptr(a.ptr) {}

    operator const Expression *() const { return ptr; }
    //explicit operator bool() const { return ptr != nullptr; }

    const Expression *operator->() const { return ptr; }
    const Expression &operator*() const { return *ptr; }

    bool operator==(std::nullptr_t) const { return ptr == nullptr; }
    bool operator!=(std::nullptr_t) const { return ptr != nullptr; }

    template<class U> const U *to() const { return dynamic_cast<const U *>(ptr); }
    template<class U> bool is() const { return dynamic_cast<const U *>(ptr) != nullptr; }
    cstring toString() const;
    void dbprint(std::ostream &out) const;

    template<class U> friend class REF;

    REF(int v);   // NOLINT(runtime/explicit)
    REF(unsigned v);   // NOLINT(runtime/explicit)
    REF(long v);   // NOLINT(runtime/explicit)
    REF(unsigned long v);   // NOLINT(runtime/explicit)
    REF(mpz_class v);   // NOLINT(runtime/explicit)
    REF(bool v);   // NOLINT(runtime/explicit)
    // -- not actually defined; will cause a link-time error if accidentally used

    REF<IR::Slice> Slice(Util::SourceInfo srcInfo, REF<Expression> hi, REF<Expression> lo) const;
    REF<IR::Slice> Slice(REF<Expression> hi, REF<Expression> lo) const {
        return Slice(Util::SourceInfo(), hi, lo); }
    REF<IR::Range> Range(Util::SourceInfo srcInfo, REF<Expression> to) const;
    REF<IR::Range> Range(REF<Expression> to) const {
        return Range(Util::SourceInfo(), to); }
    REF<IR::Concat> Concat(Util::SourceInfo srcInfo, REF<Expression> a) const;
    REF<IR::Concat> Concat(REF<Expression> a) const {
        return Concat(Util::SourceInfo(), a); }
    REF<IR::Mask> Mask(Util::SourceInfo srcInfo, REF<Expression> mask) const;
    REF<IR::Mask> Mask(REF<Expression> mask) const {
        return Mask(Util::SourceInfo(), mask); }
    REF<IR::Mux> Mux(Util::SourceInfo srcInfo, REF<Expression> t, REF<Expression> f) const;
    REF<IR::Mux> Mux(REF<Expression> t, REF<Expression> f) const {
        return Mux(Util::SourceInfo(), t, f); }
    REF<IR::Member> Member(Util::SourceInfo srcInfo, IR::ID name) const;
    REF<IR::Member> Member(IR::ID name) const;
    REF<IR::MethodCallExpression> operator()(Util::SourceInfo srcInfo,
                                             const Type *rtype = nullptr,
                                             const Vector<Type> *targs = nullptr,
                                             const Vector<Argument> *args = nullptr) const;
    REF<IR::MethodCallExpression> operator()(Util::SourceInfo srcInfo, const Vector<Type> *targs,
                                             const Vector<Argument> *args = nullptr) const {
        return (*this)(srcInfo, nullptr, targs, args); }
    REF<MethodCallExpression> operator()(Util::SourceInfo srcInfo, const Type *rtype,
                                         const Vector<Argument> *args) const {
        return (*this)(srcInfo, rtype, nullptr, args); }
    REF<MethodCallExpression> operator()(Util::SourceInfo srcInfo,
                                         const Vector<Argument> *args) const {
        return (*this)(srcInfo, nullptr, nullptr, args); }
    REF<IR::MethodCallExpression> operator()(const Type *rtype = nullptr,
                                             const Vector<Type> *targs = nullptr,
                                             const Vector<Argument> *args = nullptr) const;
    REF<IR::MethodCallExpression> operator()(const Vector<Type> *targs,
                                             const Vector<Argument> *args = nullptr) const {
        return (*this)(nullptr, targs, args); }
    REF<MethodCallExpression> operator()(const Type *rtype, const Vector<Argument> *args) const {
        return (*this)(rtype, nullptr, args); }
    REF<MethodCallExpression> operator()(const Vector<Argument> *args) const {
        return (*this)(nullptr, nullptr, args); }
    REF<IR::MethodCallExpression> operator()(const Type *rtype,
                                             std::initializer_list<const Type *> targs,
                                             std::initializer_list<const Argument *> args) const;
    REF<IR::MethodCallExpression> operator()(std::initializer_list<const Type *> targs,
                                             std::initializer_list<const Argument *> args) const;
    REF<IR::MethodCallExpression> operator()(const Type *rtype,
                                             std::initializer_list<const Argument *> args) const;
    REF<IR::MethodCallExpression> operator()(std::initializer_list<const Argument *> args) const;

    REF<ArrayIndex> operator[](REF<Expression>) const;
};

}  // end namespace IR

#endif /* IR_EXPRESSION_REF_H_ */
