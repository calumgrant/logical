#pragma once
#include <unordered_map>

class OptimizerImpl : public Optimizer
{
public:
    void Visit(const std::function<void(Optimization&)> &v) override;
    
    OptimizerImpl();
    void RegisterOptimization(Optimization&) override;
    void Optimize(EvaluationPtr & rule) const override;
    void Optimize(Relation & relation) const override;
    void Optimize(ExecutionUnit & exec) const override;
    void UpdateActiveList() override;

private:
    std::vector<Optimization*> allOptimizations, activeOptimizations;
};
