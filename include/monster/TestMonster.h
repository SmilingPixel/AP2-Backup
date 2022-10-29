#ifndef AP_PROJ_TESTMONSTER_H
#define AP_PROJ_TESTMONSTER_H

#include "Monster.h"

class TestMonster: public Monster{

    static constexpr const char* TEXTURE = ":/images/test_monster.png";

public:

    // Used in qgraphicsitem_cast
    int type() const override;

    explicit TestMonster(QGraphicsItem *parent = nullptr);
};

#endif //AP_PROJ_TESTMONSTER_H