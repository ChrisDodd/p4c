/*
Copyright 2025 Nvidia, Inc.

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

#ifndef FRONTENDS_P4_REMOVEOPASSIGN_H_
#define FRONTENDS_P4_REMOVEOPASSIGN_H_

#include "ir/ir.h"

namespace P4 {

class RemoveOpAssign : public Transform {
    const IR::Node *finish(IR::AssignmentStatement *as, const IR::Expression *e);

#define PREORDER(OP)                                        \
    const IR::Node *preorder(IR::OP##Assign *as) override { \
        prune();                                            \
        return finish(as, new IR::OP(as->left, as->right)); \
    }
    PREORDER(Mul)
    PREORDER(Div)
    PREORDER(Mod)
    PREORDER(Add)
    PREORDER(Sub)
    PREORDER(AddSat)
    PREORDER(SubSat)
    PREORDER(Shl)
    PREORDER(Shr)
    PREORDER(BAnd)
    PREORDER(BOr)
    PREORDER(BXor)
#undef PREORDER

    const IR::Node *preorder(IR::AssignmentStatement *s) override {
        prune();
        return s;
    }
    const IR::Node *preorder(IR::Expression *e) override {
        prune();
        return e;
    }
    const IR::Node *preorder(IR::Annotation *a) override {
        prune();
        return a;
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_REMOVEOPASSIGN_H_ */
