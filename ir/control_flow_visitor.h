/*
Copyright 2019-present Barefoot Networks, Inc.

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

#ifndef _IR_CONTROL_FLOW_VISITOR_H_
#define _IR_CONTROL_FLOW_VISITOR_H_

#include <boost/variant.hpp>
#include <deque>
#include "visitor.h"

class ControlFlowInspector : public Inspector, public ControlFlowVisitor {
    std::deque<boost::variant<Visitor::Context, ControlFlowInspector *>>        pending;
    std::set<ControlFlowInspector *>                                            &clones;
    ControlFlowVisitor                                                          *wait_on = nullptr;
    const IR::Node *apply_visitor(const IR::Node *, const char *name = 0) override;
    ControlFlowInspector() : clones(*new std::set<ControlFlowInspector *> ) {
        joinFlows = true;
        clones.insert(this); }
    ControlFlowInspector(ControlFlowInspector &);
    ControlFlowInspector &operator=(const ControlFlowInspector &) = delete;
    ~ControlFlowInspector();
};

#if 0
class ControlFlowModifier : public Modifier, public ControlFlowVisitor {
    const IR::Node *apply_visitor(const IR::Node *n, const char *name = 0) override;
};

class ControlFlowTransform : public Transform, public ControlFlowVisitor {
    const IR::Node *apply_visitor(const IR::Node *, const char *name = 0) override;
};
#endif

#endif /* _IR_CONTROL_FLOW_VISITOR_H_ */
