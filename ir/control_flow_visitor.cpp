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

#include "control_flow_visitor.h"
#include "visitor_impl.h"

ControlFlowInspector::ControlFlowInspector(ControlFlowInspector &from)
: Inspector(from), ControlFlowVisitor(from), clones(from.clones) {
    clones.insert(this);
    if (!from.pending.empty()) {
        from.pending.push_back(this);
        wait_on = &from; }
}

ControlFlowInspector::~ControlFlowInspector() {
    clones.erase(this);
    if (clones.empty()) delete &clones;
}



const IR::Node *ControlFlowInspector::apply_visitor(const IR::Node *n, const char *name) {
    if (ctxt)
        ctxt->child_name = name;
    if (n && !join_flows(n)) {
        PushContext local(ctxt, n);
        auto vp = visited->emplace(n, info_t{false, visitDagOnce});
        if (!vp.second && !vp.first->second.done)
            BUG("IR loop detected");
        if (!vp.second && vp.first->second.visitOnce) {
            n->apply_visitor_revisit(*this);
        } else {
            vp.first->second.done = false;
            visitCurrentOnce = &vp.first->second.visitOnce;
            if (n->apply_visitor_preorder(*this)) {
                n->visit_children(*this);
                visitCurrentOnce = &vp.first->second.visitOnce;
                n->apply_visitor_postorder(*this); }
            if (vp.first != visited->find(n))
                BUG("visitor state tracker corrupted");
            vp.first->second.done = true; }
    } else if (n) {
        pending.emplace_back(Context{ctxt, n, n, 0, nullptr, ctxt ? ctxt->depth+1 : 1});
    }
    if (ctxt) {
        ctxt->child_index++;
    } else if (pending.empty()) {
        delete visited;
        visited = nullptr;
    }
    return n;
}

#if 0

const IR::Node *ControlFlowModifier::apply_visitor(const IR::Node *n, const char *name) {
    if (ctxt) ctxt->child_name = name;
    if (n) {
        PushContext local(ctxt, n);
        if (visited->done(n)) {
            n->apply_visitor_revisit(*this, visited->result(n));
            n = visited->result(n);
        } else {
            visited->start(n, visitDagOnce);
            IR::Node *copy = n->clone();
            local.current.node = copy;
            if (!dontForwardChildrenBeforePreorder) {
                ForwardChildren forward_children(*visited);
                copy->visit_children(forward_children); }
            visitCurrentOnce = visited->refVisitOnce(n);
            if (copy->apply_visitor_preorder(*this)) {
                copy->visit_children(*this);
                visitCurrentOnce = visited->refVisitOnce(n);
                copy->apply_visitor_postorder(*this); }
            if (visited->finish(n, copy))
                (n = copy)->validate(); } }
    if (ctxt)
        ctxt->child_index++;
    else
        visited = nullptr;
    return n;
}

const IR::Node *ControlFlowTransform::apply_visitor(const IR::Node *n, const char *name) {
    if (ctxt) ctxt->child_name = name;
    if (n) {
        PushContext local(ctxt, n);
        if (visited->done(n)) {
            n->apply_visitor_revisit(*this, visited->result(n));
            n = visited->result(n);
        } else {
            visited->start(n, visitDagOnce);
            auto copy = n->clone();
            local.current.node = copy;
            if (!dontForwardChildrenBeforePreorder) {
                ForwardChildren forward_children(*visited);
                copy->visit_children(forward_children); }
            bool save_prune_flag = prune_flag;
            prune_flag = false;
            visitCurrentOnce = visited->refVisitOnce(n);
            bool extra_clone = false;
            const IR::Node *preorder_result = copy->apply_visitor_preorder(*this);
            assert(preorder_result != n);  // should never happen
            const IR::Node *final_result = preorder_result;
            if (preorder_result != copy) {
                // FIXME -- not safe if the visitor resurrects the node (which it shouldn't)
                // if (copy->id == IR::Node::currentId - 1)
                //     --IR::Node::currentId;
                if (!preorder_result) {
                    prune_flag = true;
                } else if (visited->done(preorder_result)) {
                    final_result = visited->result(preorder_result);
                    prune_flag = true;
                } else {
                    extra_clone = true;
                    visited->start(preorder_result, *visitCurrentOnce);
                    local.current.node = copy = preorder_result->clone(); } }
            if (!prune_flag) {
                copy->visit_children(*this);
                visitCurrentOnce = visited->refVisitOnce(n);
                final_result = copy->apply_visitor_postorder(*this); }
            prune_flag = save_prune_flag;
            if (final_result == copy
                && final_result != preorder_result
                && *final_result == *preorder_result)
                final_result = preorder_result;
            if (visited->finish(n, final_result) && (n = final_result))
                final_result->validate();
            if (extra_clone)
                visited->finish(preorder_result, final_result); } }
    if (ctxt)
        ctxt->child_index++;
    else
        visited = nullptr;
    return n;
}

#endif
