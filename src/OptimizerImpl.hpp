#pragma once
#include <unordered_map>

class OptimizerImpl : public Optimizer
{
public:
    void Visit(const std::function<void(Optimization&)> &v) override;
    
    OptimizerImpl();
    void RegisterOptimization(Optimization&) override;
    void Optimize(Relation & relation) const override;
    void UpdateActiveList() override;

private:
    std::vector<Optimization*> allOptimizations, activeOptimizations;
};
