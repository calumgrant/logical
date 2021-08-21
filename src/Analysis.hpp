#pragma once

#include "Fwd.hpp"
#include <functional>

void AnalysePredicate(Database &db, Relation & predicate);
void AnalyseRule(Database &db, EvaluationPtr & rule);

class Optimization
{
public:
    const char * name, *description;
    int level;
    bool enabled;
    
    virtual void Analyse(EvaluationPtr & relation) const;
    virtual void Analyse(Relation & relation) const;
    virtual void Analyse(ExecutionUnit & exec) const;
protected:
    Optimization(const char * name, const char * description, int level);
};

class Optimizer
{
public:
    virtual void Visit(const std::function<void(Optimization&)> &v) =0;

    virtual void RegisterOptimization(Optimization&) =0;

    virtual void Optimize(EvaluationPtr & rule) const =0;
    virtual void Optimize(Relation & relation) const =0;
    virtual void Optimize(ExecutionUnit & exec) const =0;
    virtual void UpdateActiveList() =0;

    void Enable(const char * name, bool enabled);
    bool IsEnabled(const char * name);
    void SetLevel(int level);
    int GetLevel() const;

private:
    int level = 1;
};
