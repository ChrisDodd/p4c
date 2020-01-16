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

namespace P4 {

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
        switch (visited->try_start(n, visitDagOnce)) {
            case VisitStatus::Busy:
                n->apply_visitor_loop_revisit(*this);
                break;
            case VisitStatus::Done:
                n->apply_visitor_revisit(*this);
                break;
            default:  // New or Revisit
                if (n->apply_visitor_preorder(*this)) {
                    n->visit_children(*this, name);
                    n->apply_visitor_postorder(*this);
                }
                visited->finish(n);
        }
        post_join_flows(n, n);
    } else if (n) {
        pending.emplace_back(Context{ctxt, n, n, nullptr, 0, ctxt ? ctxt->depth+1 : 1});
    }
    if (ctxt)
        ctxt->child_index++;
    else if (pending.empty())
        visited.reset();
    return n;
}

#if 0

const IR::Node *ControlFlowModifier::apply_visitor(const IR::Node *n, const char *name) {
    if (ctxt) ctxt->child_name = name;
    if (n) {
        PushContext local(ctxt, n);
        switch (visited->try_start(n, visitDagOnce)) {
            case VisitStatus::Busy:
                n->apply_visitor_loop_revisit(*this);
                // FIXME -- should have a way of updating the node?  Needs to be decided
                // by the visitor somehow, but it is tough
                break;
            case VisitStatus::Done:
                n->apply_visitor_revisit(*this, visited->result(n));
                n = visited->result(n);
                break;
            default: {  // New or Revisit
                IR::Node *copy = n->clone();
                local.current.node = copy;
                if (!dontForwardChildrenBeforePreorder) {
                    ForwardChildren forward_children(*visited);
                    copy->visit_children(forward_children, name);
                }
                if (copy->apply_visitor_preorder(*this)) {
                    copy->visit_children(*this, name);
                    copy->apply_visitor_postorder(*this);
                }
                if (visited->finish(n, copy)) (n = copy)->validate();
                break;
            }
        }
    }
    if (ctxt)
        ctxt->child_index++;
    else
        visited.reset();
    return n;
}

const IR::Node *ControlFlowTransform::apply_visitor(const IR::Node *n, const char *name) {
    if (ctxt) ctxt->child_name = name;
    if (n) {
        PushContext local(ctxt, n);
        switch (visited->try_start(n, visitDagOnce)) {
            case VisitStatus::Busy:
                n->apply_visitor_loop_revisit(*this);
                // FIXME -- should have a way of updating the node?  Needs to be decided
                // by the visitor somehow, but it is tough
                break;
            case VisitStatus::Done:
                n->apply_visitor_revisit(*this, visited->result(n));
                n = visited->result(n);
                break;
            default: {  // New or Revisit
                auto *copy = n->clone();
                local.current.node = copy;
                if (!dontForwardChildrenBeforePreorder) {
                    ForwardChildren forward_children(*visited);
                    copy->visit_children(forward_children, name);
                }
                bool save_prune_flag = prune_flag;
                prune_flag = false;
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
                        auto status =
                            visited->try_start(preorder_result, visited->shouldVisitOnce(n));
                        // Sanity check for IR loops
                        if (status == VisitStatus::Busy) BUG("IR loop detected ");
                        local.current.node = copy = preorder_result->clone();
                    }
                }
                if (!prune_flag) {
                    copy->visit_children(*this, name);
                    final_result = copy->apply_visitor_postorder(*this);
                }
                prune_flag = save_prune_flag;
                if (final_result == copy && final_result != preorder_result &&
                    *final_result == *preorder_result)
                    final_result = preorder_result;
                if (visited->finish(n, final_result) && (n = final_result))
                    final_result->validate();
                if (extra_clone) visited->finish(preorder_result, final_result);
                break;
            }
        }
    }
    if (ctxt)
        ctxt->child_index++;
    else
        visited.reset();
    return n;
}

#endif

}  // end namespace P4
