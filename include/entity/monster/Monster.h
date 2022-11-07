#ifndef AP_PROJ_MONSTER_H
#define AP_PROJ_MONSTER_H

#include <QGraphicsPixmapItem>
#include <QPair>
#include "Entity.h"


class Monster: public Entity{

    using Direction = QPair<int, int>;

protected:

    Direction direction_ = qMakePair(0, 0);
    qreal speed_ = 10.0; // num of px per second to move

    static qreal MonsterSize; // Must be set before construct

    virtual void checkImageOrientation();

public:

    explicit Monster(QGraphicsItem *parent = nullptr);

    qreal getSpeed() const;

    void setSpeed(qreal speed);

    Direction getDirection() const;

    void setDirection(const Direction& direction);

    static void setMonsterSize(qreal size);
};

#endif //AP_PROJ_MONSTER_H