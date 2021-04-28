#include <memory>

class UnaryRelation;
class Entity;

class Evaluation
{
public:
    virtual ~Evaluation();
    virtual void Evaluate() =0;

    std::shared_ptr<Evaluation> next;
};

class EvaluateF : public Evaluation
{
public:
    EvaluateF(UnaryRelation &rel, Entity*);
    void Evaluate() override;
private:
    Entity * e;
    UnaryRelation & relation;
};

class EvaluateB : public Evaluation
{
public:
    EvaluateB(UnaryRelation &rel, Entity*);
    void Evaluate() override;
private:
    Entity * e;
    UnaryRelation & relation;
};

class WriterB : public Evaluation
{
public:
    WriterB(UnaryRelation &rel, Entity *e);
    void Evaluate() override;

    Entity *e;
    UnaryRelation &relation;
};
