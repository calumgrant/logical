#include <memory>
#include <vector>

class Relation;
class Entity;

class Evaluation
{
public:
    virtual ~Evaluation();
    virtual void Evaluate() =0;

    std::shared_ptr<Evaluation> next;
};

class UnaryEvaluation : public Evaluation
{
protected:
    UnaryEvaluation(Relation &rel, Entity *e);
    Relation & relation;
    Entity * e;
};

class EvaluateF : public Evaluation
{
public:
    EvaluateF(Relation &rel, Entity*);
    void Evaluate() override;
};

class EvaluateB : public Evaluation
{
public:
    EvaluateB(Relation &rel, Entity*);
    void Evaluate() override;
};

class WriterB : public Evaluation
{
public:
    WriterB(Relation &rel, Entity *e);
    void Evaluate() override;
};

class RuleEvaluation : public Evaluation
{
public:
    // Local data, pre-initialised with constants
    std::vector<Entity> data;
    void Evaluate() override;
};
