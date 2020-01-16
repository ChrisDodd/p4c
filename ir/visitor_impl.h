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

#ifndef _IR_VISITOR_IMPL_H_
#define _IR_VISITOR_IMPL_H_

#include <time.h>
#include "ir.h"
#include "lib/log.h"

/** @class Visitor::ChangeTracker
 *  @brief Assists visitors in traversing the IR.

 *  A ChangeTracker object assists visitors traversing the IR by tracking each
 *  node.  The `start` method begins tracking, and `finish` ends it.  The
 *  `done` method determines whether the node has been visited, and `result`
 *  returns the new IR if it changed.
 */
class Visitor::ChangeTracker {
    struct visit_info_t {
        bool            visit_in_progress;
        bool            visitOnce;
        const IR::Node  *result;
    };
    typedef std::unordered_map<const IR::Node *, visit_info_t>  visited_t;
    visited_t           visited;

 public:
    /** Begin tracking @n during a visiting pass.  Use `finish(@n)` to mark @n as
     * visited once the pass completes.
     */
    void start(const IR::Node *n, bool defaultVisitOnce) {
        // Initialization
        visited_t::iterator visited_it;
        bool inserted;
        bool visit_in_progress = true;
        std::tie(visited_it, inserted) =
            visited.emplace(n, visit_info_t{visit_in_progress, defaultVisitOnce, n});

        // Sanity check for IR loops
        bool already_present = !inserted;
        visit_info_t *visit_info = &(visited_it->second);
        if (already_present && visit_info->visit_in_progress)
            BUG("IR loop detected ");
    }

    /** Mark the process of visiting @orig as finished, with @final being the
     * final state of the node, or nullptr if the node was removed from the
     * tree.  `done(@orig)` will return true, and `result(@orig)` will return
     * the resulting node, if any.
     *
     * If @final is a new node, that node is marked as finished as well, as if
     * `start(@final); finish(@final);` were invoked.
     *
     * @return true if the node has changed or been removed or coalesced.
     *
     * @exception Util::CompilerBug This method fails if `start(@orig)` has not
     * previously been invoked.
     */
    bool finish(const IR::Node *orig, const IR::Node *final) {
        auto it = visited.find(orig);
        if (it == visited.end())
            BUG("visitor state tracker corrupted");

        visit_info_t *orig_visit_info = &(it->second);
        orig_visit_info->visit_in_progress = false;
        if (!final) {
            orig_visit_info->result = final;
            return true;
        } else if (final != orig && *final != *orig) {
            orig_visit_info->result = final;
            visited.emplace(final, visit_info_t{false, orig_visit_info->visitOnce, final});
            return true;
            // coalescing with some previously visited node, so we don't want to undo
            // the coalesce
            orig_visit_info->result = final;
            return true;
        } else {
            // FIXME -- not safe if the visitor resurrects the node (which it shouldn't)
            // if (final && final->id == IR::Node::currentId - 1)
            //     --IR::Node::currentId;
            return false; } }

    /** Return a pointer to the visitOnce flag for node @n so that it can be changed
     */
    bool *refVisitOnce(const IR::Node *n) {
        if (!visited.count(n))
            BUG("visitor state tracker corrupted");
        return &visited.at(n).visitOnce;
    }

    /** Forget nodes that have already been visited, allowing them to be visited
     * again. */
    void revisit_visited() {
        for (auto it = visited.begin(); it != visited.end();) {
            if (!it->second.visit_in_progress)
                it = visited.erase(it);
            else
                ++it; } }

    /** Determine whether @n is currently being visited and the visitor has not finished
     * That is, `start(@n)` has been invoked, and `finish(@n)` has not,
     *
     * @return true if @n is being visited and has not finished
     */
    bool busy(const IR::Node *n) const {
        auto it = visited.find(n);
        return it != visited.end() && it->second.visit_in_progress; }

    /** Determine whether @n has been visited and the visitor has finished
     *  and we don't want to visit @n again the next time we see it.
     * That is, `start(@n)` has been invoked, followed by `finish(@n)`,
     * and the visitOnce field is true.
     *
     * @return true if @n has been visited and the visitor is finished and visitOnce is true
     */
    bool done(const IR::Node *n) const {
        auto it = visited.find(n);
        return it != visited.end() && !it->second.visit_in_progress && it->second.visitOnce;
    }

    /** Produce the result of visiting @n.
     *
     * @return The result of visiting @n, or the intermediate result of
     * visiting @n if `start(@n)` has been invoked but not `finish(@n)`, or @n
     * if `start(@n)` has not been invoked.
     */
    const IR::Node *result(const IR::Node *n) const {
        if (!visited.count(n))
            return n;
        return visited.at(n).result;
    }
};

struct Visitor::PushContext {
    Visitor::Context current;
    const Visitor::Context *&stack;
    PushContext(const Visitor::Context *&stck, const IR::Node *node) : stack(stck) {
        current.parent = stack;
        current.node = current.original = node;
        current.child_index = 0;
        current.child_name = "";
        current.depth = stack ? stack->depth+1 : 1;
        assert(current.depth < 10000);    // stack overflow?
        stack = &current; }
    ~PushContext() { stack = current.parent; }
};

class Visitor::ForwardChildren : public Visitor {
    const ChangeTracker &visited;
    const IR::Node *apply_visitor(const IR::Node *n, const char * = 0) {
        if (visited.done(n))
            return visited.result(n);
        return n; }
 public:
    explicit ForwardChildren(const ChangeTracker &v) : visited(v) {}
};

#endif /* _IR_VISITOR_IMPL_H_ */
