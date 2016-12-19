#ifndef IR_EXPRESSION_REF_H_
#define IR_EXPRESSION_REF_H_

#include "ir.h"

namespace IR {

template<class T> inline cstring REF<T>::toString() const {
    return ptr ? ptr->toString() : cstring("null"); }
template<class T> inline void REF<T>::dbprint(std::ostream &out) const {
    if (ptr)
        ptr->dbprint(out);
    else
        out << "(null)";
}

inline cstring REF<Expression>::toString() const {
    return ptr ? ptr->toString() : cstring("null"); }
inline void REF<Expression>::dbprint(std::ostream &out) const {
    if (ptr)
        ptr->dbprint(out);
    else
        out << "(null)";
}

inline REF<IR::Slice> REF<Expression>::Slice(Util::SourceInfo srcInfo, REF<Expression> hi,
                                             REF<Expression> lo) const {
    return new IR::Slice(srcInfo, *this, hi, lo); }
inline REF<IR::Range> REF<Expression>::Range(Util::SourceInfo srcInfo, REF<Expression> to) const {
    return new IR::Range(srcInfo, *this, to); }
inline REF<IR::Concat> REF<Expression>::Concat(Util::SourceInfo srcInfo, REF<Expression> a) const {
    return new IR::Concat(srcInfo, *this, a); }
inline REF<IR::Mask> REF<Expression>::Mask(Util::SourceInfo srcInfo, REF<Expression> mask) const {
    return new IR::Mask(srcInfo, *this, mask); }
inline REF<IR::Mux> REF<Expression>::Mux(Util::SourceInfo srcInfo, REF<Expression> t,
                                         REF<Expression> f) const {
    return new IR::Mux(srcInfo, *this, t, f); }
inline REF<IR::Member> REF<Expression>::Member(Util::SourceInfo srcInfo, IR::ID name) const {
    return new IR::Member(srcInfo + (*this)->srcInfo + name.srcInfo, *this, name); }
inline REF<IR::Member> REF<Expression>::Member(IR::ID name) const {
    return new IR::Member((*this)->srcInfo + name.srcInfo, *this, name); }
inline REF<IR::MethodCallExpression> REF<Expression>::operator()(
    Util::SourceInfo srcInfo, const Type *rtype, const Vector<Type> *targs,
    const Vector<Argument> *args) const
{
    if (!rtype) rtype = Type::Unknown::get();
    return new MethodCallExpression(srcInfo, rtype, *this, targs ? targs : new Vector<Type>(),
                                    args ? args : new Vector<Argument>()); }
inline REF<IR::MethodCallExpression> REF<Expression>::operator()(
    const Type *rtype, const Vector<Type> *targs, const Vector<Argument> *args) const
{
    auto srcInfo = (*this)->srcInfo;
    if (args) {
        srcInfo += args->srcInfo;
        if (!args->empty()) srcInfo += args->back()->srcInfo;
    } else if (targs) {
        srcInfo += targs->srcInfo;
        if (!targs->empty()) srcInfo += targs->back()->srcInfo; }
    if (!rtype) rtype = Type::Unknown::get();
    return new MethodCallExpression(srcInfo, rtype, *this, targs ? targs : new Vector<Type>(),
                                    args ? args : new Vector<Argument>()); }
inline REF<IR::MethodCallExpression> REF<Expression>::operator()(const Type *rtype,
            std::initializer_list<const Type *> targs,
            std::initializer_list<const Argument *> args) const {
    return (*this)(rtype, new Vector<Type>(targs), new Vector<Argument>(args)); }
inline REF<IR::MethodCallExpression> REF<Expression>::operator()(
            std::initializer_list<const Type *> targs,
            std::initializer_list<const Argument *> args) const {
    return (*this)(nullptr, new Vector<Type>(targs), new Vector<Argument>(args)); }
inline REF<IR::MethodCallExpression> REF<Expression>::operator()(const Type *rtype,
            std::initializer_list<const Argument *> args) const {
    return (*this)(rtype, nullptr, new Vector<Argument>(args)); }
inline REF<IR::MethodCallExpression> REF<Expression>::operator()(
            std::initializer_list<const Argument *> args) const {
    return (*this)(nullptr, nullptr, new Vector<Argument>(args)); }



inline REF<Expression>::REF(int v) : REF(new Constant(v)) {}
inline REF<Expression>::REF(unsigned v) : REF(new Constant(v)) {}
inline REF<Expression>::REF(long v) : REF(new Constant(v)) {}
inline REF<Expression>::REF(unsigned long v) : REF(new Constant(v)) {}
inline REF<Expression>::REF(mpz_class v) : REF(new Constant(v)) {}

typedef REF<Expression> ERef;

inline REF<ArrayIndex> REF<Expression>::operator[](REF<Expression> idx) const {
    return new ArrayIndex(ptr, idx); }

}  // end namespace IR

inline IR::ERef operator-(IR::ERef a) { return new IR::Neg(a); }
inline IR::ERef operator~(IR::ERef a) { return new IR::Cmpl(a); }
// FIXME -- often use !ref to check for null refs
//inline IR::ERef operator!(IR::ERef a) { return new IR::LNot(a); }
inline IR::ERef NOT(IR::ERef a) { return new IR::LNot(a); }
inline IR::ERef operator+(IR::ERef a, IR::ERef b) { return new IR::Add(a, b); }
inline IR::ERef operator-(IR::ERef a, IR::ERef b) { return new IR::Sub(a, b); }
inline IR::ERef operator*(IR::ERef a, IR::ERef b) { return new IR::Mul(a, b); }
inline IR::ERef operator/(IR::ERef a, IR::ERef b) { return new IR::Div(a, b); }
inline IR::ERef operator%(IR::ERef a, IR::ERef b) { return new IR::Mod(a, b); }
inline IR::ERef operator<<(IR::ERef a, IR::ERef b) { return new IR::Shl(a, b); }
inline IR::ERef operator>>(IR::ERef a, IR::ERef b) { return new IR::Shr(a, b); }
// FIXME -- can't overlod these as they need to compare for equality in visitors
//inline IR::ERef operator==(IR::ERef a, IR::ERef b) { return new IR::Equ(a, b); }
inline IR::ERef EQ(IR::ERef a, IR::ERef b) { return new IR::Equ(a, b); }
//inline IR::ERef operator!=(IR::ERef a, IR::ERef b) { return new IR::Neq(a, b); }
inline IR::ERef NEQ(IR::ERef a, IR::ERef b) { return new IR::Neq(a, b); }
inline IR::ERef operator<=(IR::ERef a, IR::ERef b) { return new IR::Leq(a, b); }
inline IR::ERef operator>=(IR::ERef a, IR::ERef b) { return new IR::Geq(a, b); }
inline IR::ERef operator<(IR::ERef a, IR::ERef b) { return new IR::Lss(a, b); }
inline IR::ERef operator>(IR::ERef a, IR::ERef b) { return new IR::Grt(a, b); }
inline IR::ERef operator&(IR::ERef a, IR::ERef b) { return new IR::BAnd(a, b); }
inline IR::ERef operator|(IR::ERef a, IR::ERef b) { return new IR::BOr(a, b); }
inline IR::ERef operator^(IR::ERef a, IR::ERef b) { return new IR::BXor(a, b); }
inline IR::ERef operator&&(IR::ERef a, IR::ERef b) { return new IR::LAnd(a, b); }
inline IR::ERef operator||(IR::ERef a, IR::ERef b) { return new IR::LOr(a, b); }

#endif /* IR_EXPRESSION_REF_H_ */
