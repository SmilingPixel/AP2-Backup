#ifndef AP_PROJ_GRASS_H
#define AP_PROJ_GRASS_H

#include "Area.h"

class Grass: public Area{

    static const QString TEXTURE;

public:
    explicit Grass(QGraphicsItem *parent = nullptr);
};

#endif //AP_PROJ_GRASS_H