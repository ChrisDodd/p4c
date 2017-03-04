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

#ifndef _COMMON_RESOLVEREFERENCES_RESOLVEREFERENCES_H_
#define _COMMON_RESOLVEREFERENCES_RESOLVEREFERENCES_H_

#include "ir/ir.h"
#include "referenceMap.h"
#include "lib/exceptions.h"
#include "lib/cstring.h"

namespace P4 {

/// Helper class to indicate types of nodes that may be returned during resolution.
enum class ResolutionType {
    Any,
    Type,
    TypeVariable
};

/// Data structure representing a stack of nested namespaces.
class ResolutionContext : public IHasDbPrint, virtual public Visitor {
 protected:
    /// Stack of namespaces for global declarations (e.g., match_kind)
    /// @todo: what about errors?
    std::vector<const IR::INamespace*> globals;

    std::vector<const IR::IDeclaration*>*
    lookup(const IR::INamespace *ns, IR::ID name, ResolutionType type, bool forwardOK) const;

    ResolutionContext() {}

    void dbprint(std::ostream& out) const;

    /// Add name space @p e to `globals`.
    void addGlobal(const IR::INamespace *e) {
        globals.push_back(e);
    }

    /// Resolve references for @p name, restricted to @p type declarations.
    /// If @p forwardOK is `false`, the referenced location must precede the location of @p name.
    std::vector<const IR::IDeclaration*>*
    resolve(IR::ID name, ResolutionType type, bool forwardOK) const;

    /// Resolve reference for @p name, restricted to @p type declarations, and expect one result.
    /// If @p forwardOK is `false`, the referenced location must precede the location of @p name.
    const IR::IDeclaration*
    resolveUnique(IR::ID name, ResolutionType type, bool forwardOK,
                  const IR::INamespace * = nullptr) const;

    // Resolve a refrence to a type @p type.
    const IR::Type *resolveType(const IR::Type *type) const;
};

/** Inspector that computes `refMap`: a map from paths to declarations.
 *
 * @pre: None
 *
 * @post: produces an up-to-date `refMap`
 *
 */
class ResolveReferences : public Inspector, virtual public ResolutionContext {
    /// Reference map -- essentially from paths to declarations.
    ReferenceMap *refMap;

    /// Tracks whether forward references are permitted in a context.
    std::vector<bool> resolveForward;

    /// Indicates if _all_ forward references are allowed
    bool anyOrder;

    /// If @true, then warn if one declaration shadows another.
    bool checkShadow;

 private:
    /// Add namespace @p ns to `globals`
    void addToGlobals(const IR::INamespace *ns);

    /// Resolve @p path; if @p isType is `true` then resolution will
    /// only return type nodes.
    void resolvePath(const IR::Path *path, bool isType) const;

 public:
    explicit ResolveReferences(/* out */ P4::ReferenceMap *refMap,
                               bool checkShadow = false);

    Visitor::profile_t init_apply(const IR::Node *node) override;
    void end_apply(const IR::Node *node) override;
    using Inspector::preorder;
    using Inspector::postorder;

    bool preorder(const IR::Type_Name *type) override;
    bool preorder(const IR::PathExpression *path) override;
    bool preorder(const IR::This *pointer) override;
    bool preorder(const IR::Declaration_Instance *decl) override;

    bool preorder(const IR::P4Program *t) override;
    void postorder(const IR::P4Program *t) override;
    bool preorder(const IR::P4Control *t) override;
    bool preorder(const IR::P4Parser *t) override;
    bool preorder(const IR::P4Action *t) override;
    bool preorder(const IR::Function *t) override;
    bool preorder(const IR::TableProperties *t) override;
    bool preorder(const IR::Type_Method *t) override;
    void postorder(const IR::Type_Method *t) override;
    bool preorder(const IR::ParserState *t) override;
    void postorder(const IR::ParserState *t) override;
    bool preorder(const IR::Type_Extern *t) override;
    bool preorder(const IR::Type_ArchBlock *t) override;
    void postorder(const IR::Type_ArchBlock *t) override;
    bool preorder(const IR::Type_StructLike *t) override;
    bool preorder(const IR::BlockStatement *t) override;

    bool preorder(const IR::P4Table *table) override;
    bool preorder(const IR::Declaration_MatchKind *d) override;
    bool preorder(const IR::Declaration *d) override
    { refMap->usedName(d->getName().name); return true; }
    bool preorder(const IR::Type_Declaration *d) override
    { refMap->usedName(d->getName().name); return true; }

    void checkShadowing(const IR::INamespace*ns) const;
};

}  // namespace P4

#endif /* _COMMON_RESOLVEREFERENCES_RESOLVEREFERENCES_H_ */
