/******************************************************************************
* Copyright(c) 2025 NVIDIA Corporation
*
* For licensing information, see the file ‘LICENSE’ in the root folder
******************************************************************************/
#ifndef MIDEND_GLOBAL_FLOW_H_
#define MIDEND_GLOBAL_FLOW_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "ir/ir.h"

/**
 * @ingroup midend
 * @brief Flow analysis base class for global analysis
 *
 * Support for global analysis across P4Parser and P4Control blocks in the midend.
 * Will resolve apply calls and visit tables and other controls/parser as they are applied
 *
 * Generally, a pass derived from this should be applied to a 'top level' block extracted
 * from the P4Program by a P4::EvaluatorPass.  It will look up PathExpressions visited in
 * the context and visit their definitions recursively as appropriate.
 *
 */
class GlobalFlowInspector : public Inspector,
                            public ControlFlowVisitor,
                            public P4WriteContext,
                            public P4::ResolutionContext {
 protected:
    void flow_merge(Visitor &) override;
    void flow_copy(ControlFlowVisitor &) override;
    enum { SKIPPING, NORMAL, READ_ONLY, WRITE_ONLY } state = SKIPPING;

    profile_t init_apply(const IR::Node *root) override {
        auto rv = Inspector::init_apply(root);
        state = SKIPPING;
        return rv;
    }
    bool preorder(const IR::ToplevelBlock *) override;
    bool preorder(const IR::P4Program *) override;
    bool preorder(const IR::P4Control *) override;
    bool preorder(const IR::P4Table *) override;
    bool preorder(const IR::P4Action *) override;
    bool preorder(const IR::P4Parser *) override;
    bool preorder(const IR::ParserState *) override;
    void revisit(const IR::ParserState *) override;
    void loop_revisit(const IR::ParserState *) override;
    void postorder(const IR::ParserState *) override;
    bool preorder(const IR::KeyElement *) override;
    bool preorder(const IR::Type *) override { return false; }
    bool preorder(const IR::Annotations *) override { return false; }
    bool preorder(const IR::PathExpression *) override;
    void loop_revisit(const IR::PathExpression *) override;
    bool preorder(const IR::ConstructorCallExpression *) override;
    bool preorder(const IR::MethodCallExpression *) override;

    class SetupJoinPoints;
    void applySetupJoinPoints(const IR::Node *root) override;
    bool filter_join_point(const IR::Node *) override;

    GlobalFlowInspector() : ResolutionContext(true) { joinFlows = true; }

 public:
    bool isRead(bool root_value = false) {
        return state != WRITE_ONLY && P4WriteContext::isRead(root_value);
    }
    bool isWrite(bool root_value = false) {
        return state != READ_ONLY && P4WriteContext::isWrite(root_value);
    }
};

#endif /* MIDEND_GLOBAL_FLOW_H_ */
