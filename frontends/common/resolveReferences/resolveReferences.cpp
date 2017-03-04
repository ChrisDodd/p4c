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

#include "resolveReferences.h"
#include <sstream>

namespace P4 {

static std::vector<const IR::IDeclaration*> empty;

std::vector<const IR::IDeclaration*>*
ResolutionContext::resolve(IR::ID name, P4::ResolutionType type, bool forwardOK) const {
    std::vector<const IR::IDeclaration*> *rv = &empty;

    // FIXME -- we look up 'globals' solely to find match kinds (that the only thing
    // in globals), which menas that match kinds are effectively reserved words.  Should
    // probably move resolving match kinds out of here altogether
    for (auto it = globals.begin(); rv->empty() && it != globals.end(); ++it)
        rv = lookup(*it, name, type, forwardOK);
    if (rv->empty())  {
        const Context *ctxt = nullptr;
        while (auto scope = findContext<IR::INamespace>(ctxt)) {
            rv = lookup(scope, name, type, forwardOK);
            if (!rv->empty()) break; } }
    return rv;
}

std::vector<const IR::IDeclaration*>*
ResolutionContext::lookup(const IR::INamespace *current, IR::ID name, P4::ResolutionType type,
                          bool forwardOK) const {
    LOG2("Trying to resolve in " << current->toString());

    if (current->is<IR::IGeneralNamespace>()) {
        auto gen = current->to<IR::IGeneralNamespace>();
        Util::Enumerator<const IR::IDeclaration*> *decls = gen->getDeclsByName(name);
        switch (type) {
            case P4::ResolutionType::Any:
                break;
            case P4::ResolutionType::Type: {
                std::function<bool(const IR::IDeclaration*)> kindFilter =
                        [](const IR::IDeclaration *d) {
                    return d->is<IR::Type>(); };
                decls = decls->where(kindFilter);
                break; }
            case P4::ResolutionType::TypeVariable: {
                std::function<bool(const IR::IDeclaration*)> kindFilter =
                        [](const IR::IDeclaration *d) {
                return d->is<IR::Type_Var>(); };
                decls = decls->where(kindFilter);
                break; }
        default:
            BUG("Unexpected enumeration value %1%", static_cast<int>(type)); }

        if (!forwardOK) {
            std::function<bool(const IR::IDeclaration*)> locationFilter =
                    [name](const IR::IDeclaration *d) {
                Util::SourceInfo nsi = name.srcInfo;
                Util::SourceInfo dsi = d->getNode()->srcInfo;
                bool before = dsi <= nsi;
                LOG3("\tPosition test:" << dsi << "<=" << nsi << "=" << before);
                return before; };
            decls = decls->where(locationFilter); }

        auto vector = decls->toVector();
        if (!vector->empty()) {
            LOG3("Resolved in " << dbp(current->getNode()));
            return vector; }
    } else {
        auto simple = current->to<IR::ISimpleNamespace>();
        auto decl = simple->getDeclByName(name);
        if (decl == nullptr)
            return &empty;
        switch (type) {
            case P4::ResolutionType::Any:
                break;
            case P4::ResolutionType::Type: {
                if (!decl->is<IR::Type>())
                    return &empty;
                break; }
            case P4::ResolutionType::TypeVariable: {
                if (!decl->is<IR::Type_Var>())
                    return &empty;
                break; }
        default:
            BUG("Unexpected enumeration value %1%", static_cast<int>(type)); }

        if (!forwardOK) {
            Util::SourceInfo nsi = name.srcInfo;
            Util::SourceInfo dsi = decl->getNode()->srcInfo;
            bool before = dsi <= nsi;
            LOG3("\tPosition test:" << dsi << "<=" << nsi << "=" << before);
            if (!before)
                return &empty; }

        LOG3("Resolved in " << dbp(current->getNode()));
        auto result = new std::vector<const IR::IDeclaration*>();
        result->push_back(decl);
        return result;
    }
    return &empty;
}

const IR::IDeclaration*
ResolutionContext::resolveUnique(IR::ID name,
                                 P4::ResolutionType type,
                                 bool forwardOK,
                                 const IR::INamespace *ns) const {
    auto decls = ns ? lookup(ns, name, type, forwardOK) : resolve(name, type, forwardOK);
    if (decls->empty()) {
        ::error("Could not find declaration for %1%", name);
        return nullptr;
    }
    if (decls->size() > 1) {
        ::error("Multiple matching declarations for %1%", name);
        for (auto a : *decls)
            ::error("Candidate: %1%", a);
        return nullptr;
    }
    return decls->at(0);
}

const IR::Type *
ResolutionContext::resolveType(const IR::Type *type) const {
    /// @todo: why is `true` supplied for `forwardOK` here?
    if (auto tname = type->to<IR::Type_Name>())
        return resolveUnique(tname->path->name, ResolutionType::Type, true)->to<IR::Type>();
    return type;
}

void ResolutionContext::dbprint(std::ostream& out) const {
    out << "Globals[" << globals.size() << "]" << std::endl;
    for (auto it = globals.begin(); it != globals.end(); it++) {
        const IR::INamespace *ns = *it;
        const IR::Node *node = ns->getNode();
        node->dbprint(out);
        out << std::endl;
    }
    out << "----------" << std::endl;
}

ResolveReferences::ResolveReferences(ReferenceMap *refMap,
                                     bool checkShadow) :
        refMap(refMap),
        anyOrder(false),
        checkShadow(checkShadow) {
    CHECK_NULL(refMap);
    setName("ResolveReferences");
    visitDagOnce = false;
}

void ResolveReferences::addToGlobals(const IR::INamespace *ns) {
    addGlobal(ns);
}

void ResolveReferences::resolvePath(const IR::Path *path, bool isType) const {
    LOG2("Resolving " << path << " " << (isType ? "as type" : "as identifier"));
    const IR::INamespace *ctxt = nullptr;
    if (path->absolute)
        ctxt = findContext<IR::P4Program>();
    ResolutionType k = isType ? ResolutionType::Type : ResolutionType::Any;

    BUG_CHECK(!resolveForward.empty(), "Empty resolveForward");
    bool forwardOK = resolveForward.back();

    const IR::IDeclaration *decl = resolveUnique(path->name, k, forwardOK, ctxt);
    if (decl == nullptr) {
        refMap->usedName(path->name.name);
        return;
    }

    refMap->setDeclaration(path, decl);
}

void ResolveReferences::checkShadowing(const IR::INamespace *ns) const {
    if (!checkShadow) return;
    std::map<cstring, const IR::Node *> prev; // check for shadowing within a scope
    for (auto *decl : *ns->getDeclarations()) {
        const IR::Node *node = decl->getNode();
        if (node->is<IR::StructField>())
            continue;

        if (prev.count(decl->getName()))
            ::warning("%1% shadows %2%", node, prev.at(decl->getName()));
        else if (!node->is<IR::Method>() && !node->is<IR::Function>())
            prev[decl->getName()] = node;
        auto prev = resolve(decl->getName(), ResolutionType::Any, anyOrder);
        if (prev->empty()) continue;

        for (auto p : *prev) {
            const IR::Node *pnode = p->getNode();
            if (pnode == node) continue;
            if ((pnode->is<IR::Method>() || pnode->is<IR::Type_Extern>()) &&
                (node->is<IR::Method>() || node->is<IR::Function>()))
                // Methods can overload each other if they have a different number of arguments
                // Also, the constructor is supposed to have the same name as the class
                continue;
            if (pnode->is<IR::Attribute>() && node->is<IR::AttribLocal>())
                // attribute locals often match attributes
                continue;

            ::warning("%1% shadows %2%", node, pnode);
        }
    }
}

Visitor::profile_t ResolveReferences::init_apply(const IR::Node *node) {
    anyOrder = refMap->isV1();
    if (!refMap->checkMap(node))
        refMap->clear();
    return Inspector::init_apply(node);
}

void ResolveReferences::end_apply(const IR::Node *node) {
    refMap->updateMap(node);
}

// Visitor methods

bool ResolveReferences::preorder(const IR::P4Program *program) {
    if (refMap->checkMap(program))
        return false;

    BUG_CHECK(resolveForward.empty(), "Expected empty resolvePath");
    resolveForward.push_back(anyOrder);
    return true;
}

void ResolveReferences::postorder(const IR::P4Program *) {
    resolveForward.pop_back();
    BUG_CHECK(resolveForward.empty(), "Expected empty resolvePath");
    LOG2("Reference map " << refMap);
}

bool ResolveReferences::preorder(const IR::This *pointer) {
    auto decl = findContext<IR::Declaration_Instance>();
    if (findContext<IR::Function>() == nullptr || decl == nullptr)
        ::error("%1%: can only be used in the definition of an abstract method", pointer);
    refMap->setDeclaration(pointer, decl);
    return true;
}

bool ResolveReferences::preorder(const IR::PathExpression *path) {
    resolvePath(path->path, false);
    return true;
}

bool ResolveReferences::preorder(const IR::Type_Name *type) {
    resolvePath(type->path, true);
    return true;
}

bool ResolveReferences::preorder(const IR::P4Control *c) {
    refMap->usedName(c->name.name);
    checkShadowing(c);
    return true;
}

bool ResolveReferences::preorder(const IR::P4Parser *p) {
    refMap->usedName(p->name.name);
    checkShadowing(p);
    return true;
}

bool ResolveReferences::preorder(const IR::Function *function) {
    refMap->usedName(function->name.name);
    checkShadowing(function);
    return true;
}

bool ResolveReferences::preorder(const IR::P4Table *t) {
    refMap->usedName(t->name.name);
    return true;
}

bool ResolveReferences::preorder(const IR::TableProperties *p) {
    checkShadowing(p);
    return true;
}

bool ResolveReferences::preorder(const IR::P4Action *c) {
    refMap->usedName(c->name.name);
    checkShadowing(c);
    return true;
}

bool ResolveReferences::preorder(const IR::Type_Method *t) {
    // Function return values in generic functions may depend on the type arguments:
    // T f<T>()
    // where T is declared *after* its first use
    resolveForward.push_back(true);
    checkShadowing(t);
    return true;
}

void ResolveReferences::postorder(const IR::Type_Method *) {
    resolveForward.pop_back();
}

bool ResolveReferences::preorder(const IR::Type_Extern *t) {
    refMap->usedName(t->name.name);
    checkShadowing(t); return true; }

bool ResolveReferences::preorder(const IR::ParserState *s) {
    refMap->usedName(s->name.name);
    // State references may be resolved forward
    resolveForward.push_back(true);
    checkShadowing(s);
    return true;
}

void ResolveReferences::postorder(const IR::ParserState *) {
    resolveForward.pop_back();
}

bool ResolveReferences::preorder(const IR::Declaration_MatchKind *d) {
    addToGlobals(d);
    return true;
}

bool ResolveReferences::preorder(const IR::Type_ArchBlock *t) {
    resolveForward.push_back(anyOrder);
    if (!t->is<IR::Type_Package>()) {
        // don't check shadowing in packages as they have no body
        checkShadowing(t); }
    return true;
}

void ResolveReferences::postorder(const IR::Type_ArchBlock *t) {
    refMap->usedName(t->name.name);
    resolveForward.pop_back();
}

bool ResolveReferences::preorder(const IR::Type_StructLike *t) {
    refMap->usedName(t->name.name);
    checkShadowing(t);
    return true;
}

bool ResolveReferences::preorder(const IR::BlockStatement *b) {
    checkShadowing(b);
    return true;
}

bool ResolveReferences::preorder(const IR::Declaration_Instance *decl) {
    refMap->usedName(decl->name.name);
    return true;
}

#undef PROCESS_NAMESPACE

}  // namespace P4
