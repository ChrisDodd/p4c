/******************************************************************************
* Copyright(c) 2025 NVIDIA Corporation
*
* For licensing information, see the file ‘LICENSE’ in the root folder
******************************************************************************/
#include "global_flow.h"

#include "frontends/p4/methodInstance.h"

void GlobalFlowInspector::flow_merge(Visitor &a_) {
    GlobalFlowInspector &a = dynamic_cast<GlobalFlowInspector &>(a_);
    BUG_CHECK(state == a.state, "inconsistent state in GlobalFlowInspector::flow_merge");
}
void GlobalFlowInspector::flow_copy(ControlFlowVisitor &a_) {
    GlobalFlowInspector &a = dynamic_cast<GlobalFlowInspector &>(a_);
    BUG_CHECK(state == a.state, "inconsistent state in GlobalFlowInspector::flow_copy");
}

class GlobalFlowInspector::SetupJoinPoints : public ControlFlowVisitor::SetupJoinPoints,
                                             public P4::ResolutionContext {
    bool preorder(const IR::ParserState *n) override {
        LOG6("SetupJoinPoints(ParserState " << n->name << ")" << Log::indent);
        return true;
    }
    void revisit(const IR::ParserState *n) override {
        if (n->isBuiltin() && n->components.empty() && !n->selectExpression) {
            // FIXME -- P4-14->16 conversion uses a single accept and reject state for
            // both ingress and egress, which will cause problems, so we avoid it.
            // Perhaps we should ignore/not revisit all states with components.empty()
            // as they by definition don't do anything?
            return;
        }
        ++join_points[n].count;
        LOG6("SetupJoinPoints::revisit(ParserState " << n->name << ") [" << join_points[n].count
                                                     << "]");
    }
    void loop_revisit(const IR::ParserState *n) override { LOG6("  * loop into " << n->name); }
    void postorder(const IR::ParserState *) override { LOG6_UNINDENT; }
    void revisit(const IR::Node *) override {}
    bool preorder(const IR::PathExpression *pe) override {
        if (pe->type->is<IR::Type_State>()) {
            auto *d = resolveUnique(pe->path->name, P4::ResolutionType::Any);
            BUG_CHECK(d, "failed to resolve %s", pe);
            auto ps = d->to<IR::ParserState>();
            BUG_CHECK(ps, "%s is not a parser state", d);
            visit(ps, "transition");
        }
        return false;
    }
    bool preorder(const IR::P4Parser *p) override {
        IndentCtl::TempIndent indent;
        LOG6("SetupJoinPoints(P4Parser " << p->name << ")" << indent);
        LOG8("    " << Log::indent << Log::indent << *p << Log::unindent << Log::unindent);
        if (auto start = p->states.getDeclaration<IR::ParserState>("start")) visit(start, "start");
        return false;
    }
    bool preorder(const IR::P4Control *) override { return false; }
    bool preorder(const IR::Type *) override { return false; }

 public:
    explicit SetupJoinPoints(decltype(join_points) &fjp)
        : ControlFlowVisitor::SetupJoinPoints(fjp) {}
};

void GlobalFlowInspector::applySetupJoinPoints(const IR::Node *root) {
    root->apply(SetupJoinPoints(*flow_join_points));
}

bool GlobalFlowInspector::filter_join_point(const IR::Node *n) {
    LOG6("init_join_flows " << n->to<IR::ParserState>()->name << " = "
                            << flow_join_points->at(n).count);
    return false;
}

bool GlobalFlowInspector::preorder(const IR::ToplevelBlock *tlb) {
    visit(tlb->node, "program");
    return false;
}

bool GlobalFlowInspector::preorder(const IR::P4Program *prog) {
    if (auto tlb = getParent<IR::ToplevelBlock>()) {
        auto main = tlb->getMain();
        BUG_CHECK(main, "no main() in program");
        visit(main->node, "main");
    } else {
        auto main = prog->getDeclsByName("main")->toVector();
        BUG_CHECK(main->size() == 1, "not a single main() in program");
        auto obj = main->front()->getNode();  // FIXME -- should be able to visit an INode
        visit(obj, "main");
    }
    return false;
}

bool GlobalFlowInspector::preorder(const IR::P4Control *c) {
    BUG_CHECK(state == SKIPPING, "Nested %s not supported in GlobalFlowInspector", c);
    IndentCtl::TempIndent indent;
    LOG5("GlobalFlowInspector(P4Control " << c->name << ")" << indent);
    // for (auto *p : c->getApplyParameters()->parameters)
    //     if (p->direction == IR::Direction::In || p->direction == IR::Direction::InOut)
    //         def_info[p].defs.insert(getLoc(p));
    state = NORMAL;
    visit(c->body, "body");  // just visit the body; tables/actions will be visited when applied
    // for (auto *p : c->getApplyParameters()->parameters)
    //     if (p->direction == IR::Direction::Out || p->direction == IR::Direction::InOut)
    //         add_uses(getLoc(p), def_info[p]);
    // def_info.clear();
    state = SKIPPING;
    return false;
}

bool GlobalFlowInspector::preorder(const IR::P4Table *tbl) {
    if (state == SKIPPING) return false;
    IndentCtl::TempIndent indent;
    LOG5("GlobalFlowInspector(P4Table " << tbl->name << ")" << indent);
    if (auto key = tbl->getKey()) visit(key, "key");
    if (auto actions = tbl->getActionList()) {
        parallel_visit(actions->actionList, "actions");
    } else {
        BUG("No actions in %s", tbl);
    }
    return false;
}

bool GlobalFlowInspector::preorder(const IR::P4Action *act) {
    if (state == SKIPPING) return false;
    // for (auto *p : *act->parameters)
    //     def_info[p].defs.insert(getLoc(p));
    IndentCtl::TempIndent indent;
    LOG5("GlobalFlowInspector(P4Action " << act->name << ")" << indent);
    visit(act->body, "body");
    return false;
}

bool GlobalFlowInspector::preorder(const IR::P4Parser *p) {
    BUG_CHECK(state == SKIPPING, "Nested %s not supported in GlobalFlowInspector", p);
    IndentCtl::TempIndent indent;
    LOG5("GlobalFlowInspector(P4Parser " << p->name << ")" << indent);
    // for (auto *a : p->getApplyParameters()->parameters)
    //     if (a->direction == IR::Direction::In || a->direction == IR::Direction::InOut)
    //         def_info[a].defs.insert(getLoc(a));
    state = NORMAL;
    if (auto start = p->states.getDeclaration<IR::ParserState>("start")) {
        visit(start, "start");
    } else {
        BUG("No start state in %s", p);
    }
    // for (auto *a : p->getApplyParameters()->parameters)
    //     if (a->direction == IR::Direction::Out || a->direction == IR::Direction::InOut)
    //         add_uses(getLoc(a), def_info[a]);
    // def_info.clear();
    state = SKIPPING;
    return false;
}
bool GlobalFlowInspector::preorder(const IR::ParserState *p) {
    LOG5("GlobalFlowInspector(ParserState " << p->name << ")" << Log::indent);
    return true;
}
void GlobalFlowInspector::revisit(const IR::ParserState *p) { LOG5("  * revisit " << p->name); }
void GlobalFlowInspector::loop_revisit(const IR::ParserState *p) {
    LOG5("  * loop into " << p->name);
}
void GlobalFlowInspector::postorder(const IR::ParserState *) { LOG5_UNINDENT; }

bool GlobalFlowInspector::preorder(const IR::KeyElement *ke) {
    visit(ke->expression, "expression");
    return false;
}

bool GlobalFlowInspector::preorder(const IR::PathExpression *pe) {
    LOG7("GlobalFlowInspector(PathExpression " << *pe << ")");
    if (state == SKIPPING) return false;
    if (pe->type->is<IR::Type_State>()) {
        auto *d = resolveUnique(pe->path->name, P4::ResolutionType::Any);
        BUG_CHECK(d, "failed to resolve %s", pe);
        auto ps = d->to<IR::ParserState>();
        BUG_CHECK(ps, "%s is not a parser state", d);
        visit(ps, "transition");
        return false;
    }
    return false;
}
void GlobalFlowInspector::loop_revisit(const IR::PathExpression *pe) {
    LOG5("  * not visiting PathExpresion " << pe->path->name << " to avoid loop!");
}

bool GlobalFlowInspector::preorder(const IR::ConstructorCallExpression *cce) {
    auto *cc = P4::ConstructorCall::resolve(cce, this, nullptr);
    if (auto *ccc = cc->to<P4::ContainerConstructorCall>()) {
        auto obj = ccc->container->getNode();
        visit(obj, "container");
    } else if (auto *ecc = cc->to<P4::ExternConstructorCall>()) {
        visit(ecc->constructor, "constructor");
    }
    return false;
}

bool GlobalFlowInspector::preorder(const IR::MethodCallExpression *mc) {
    auto *mi = P4::MethodInstance::resolve(mc, this);
    if (state == WRITE_ONLY) {
        BUG_CHECK(!isWrite(), "Method call in out or inout arg should have failed typechecking");
        return false;
    } else if (state == READ_ONLY) {
        if (!isRead()) return false;
    }
    auto saved_state = state;
    state = READ_ONLY;
    visit(mc->arguments, "arguments");
    state = NORMAL;
    if (auto *ac = mi->to<P4::ActionCall>()) {
        visit(ac->action, "action");
    } else if (auto *bi = mi->to<P4::BuiltInMethod>()) {
        if (bi->name == "isValid") {
            state = READ_ONLY;
        } else if (bi->name == "setValid" || bi->name == "setInvalid") {
            state = WRITE_ONLY;
        } else if (bi->name == "push_front" || bi->name == "pop_front") {
            // push/pop we deal with as writes.  Maybe should be both?
            state = WRITE_ONLY;
        } else {
            BUG("unknown BuiltInMethod: %s", mc);
        }
        visit(mc->method, "method");
    } else {
        if (mi->object) {
            auto obj = mi->object->getNode();  // FIXME -- should be able to visit an INode
            if (!isInContext(obj)) visit(obj, "object");
        }
    }
    state = WRITE_ONLY;
    visit(mc->arguments, "arguments");
    state = saved_state;
    return false;
}
