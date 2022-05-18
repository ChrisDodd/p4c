/*
Copyright 2013-present Barefoot Networks, Inc.

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

#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <boost/format.hpp>

#include "frontends/common/parser_options.h"
#include "ir/configuration.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "lib/safe_vector.h"
#include "lib/source_file.h"

namespace IR {

#if HAVE_LIBGC
typedef const Type::Bits *TypeBitCache;
#else  /* !HAVE_LIBGC */
typedef IR::shared_ptr<Type::Bits> TypeBitCache;
#endif /* !HAVE_LIBGC */

TypeBitCache Type_Bits::get(int width, bool isSigned) {
    // map (width, signed) to type
    using bit_type_key = std::pair<int, bool>;
    static std::map<bit_type_key, TypeBitCache> *type_map = nullptr;
    if (type_map == nullptr) type_map = new std::map<bit_type_key, TypeBitCache>();
    auto &result = (*type_map)[std::make_pair(width, isSigned)];
    if (!result) result = new Type_Bits(width, isSigned);
    if (width > P4CContext::getConfig().maximumWidthSupported())
        ::error(ErrorType::ERR_UNSUPPORTED, "%1%: Compiler only supports widths up to %2%", result,
                P4CContext::getConfig().maximumWidthSupported());
    return result;
}

TypeBitCache Type::Bits::get(Util::SourceInfo si, int sz, bool isSigned) {
    if (sz < 0) ::error(ErrorType::ERR_INVALID, "%1%: Width cannot be negative", si);
    if (sz == 0 && isSigned) ::error(ErrorType::ERR_INVALID, "%1%: Width cannot be zero", si);
    return get(sz, isSigned);
}

bool Type_ActionEnum::contains(cstring name) const {
    for (auto a : actionList->actionList) {
        if (a->getName() == name) return true;
    }
    return false;
}

size_t Type_MethodBase::minParameterCount() const {
    size_t rv = 0;
    for (auto p : *parameters)
        if (!p->isOptional()) ++rv;
    return rv;
}

const Type *Type_List::getP4Type() const {
    auto args = new IR::Vector<Type>();
    for (auto a : components) {
        auto at = a->getP4Type();
        if (!at) return nullptr;
        args->push_back(at);
    }
    return new IR::Type_List(srcInfo, *args);
}

const Type *Type_Tuple::getP4Type() const {
    auto args = new IR::Vector<Type>();
    for (auto a : components) {
        auto at = a->getP4Type();
        if (!at) return nullptr;
        args->push_back(at);
    }
    return new IR::Type_Tuple(srcInfo, *args);
}

const Type *Type_P4List::getP4Type() const {
    return new IR::Type_P4List(srcInfo, elementType->getP4Type());
}

const Type *Type_Specialized::getP4Type() const {
    auto args = new IR::Vector<Type>();
    for (auto a : *arguments) {
        auto at = a->getP4Type();
        args->push_back(at);
    }
    return new IR::Type_Specialized(srcInfo, baseType, args);
}

const Type *Type_SpecializedCanonical::getP4Type() const {
    auto args = new IR::Vector<Type>();
    for (auto a : *arguments) {
        auto at = a->getP4Type();
        args->push_back(at);
    }
    auto bt = baseType->getP4Type();
    if (auto tn = bt->to<IR::Type_Name>()) return new IR::Type_Specialized(srcInfo, tn, args);
    auto st = baseType->to<IR::Type_StructLike>();
    BUG_CHECK(st != nullptr, "%1%: expected a struct", baseType);
    return new IR::Type_Specialized(srcInfo, new IR::Type_Name(st->getName()), args);
}

}  // namespace IR
