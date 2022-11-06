#include "Monster.h"

qreal Monster::MonsterSize = 0;

Monster::Monster(QGraphicsItem *parent): QGraphicsPixmapItem(parent){
    setZValue(2); // Ensure it wouldn't be covered by area selected
}

void Monster::checkImageOrientation() {
    // Do nothing here
}

qreal Monster::getSpeed() const {
    return speed_;
}

void Monster::setSpeed(qreal speed) {
    if(speed < 0.0)
        return;
    speed_ = speed;
}

Monster::Direction Monster::getDirection() const {
    return direction_;
}

void Monster::setDirection(const Monster::Direction &direction) {
    direction_ = direction;
    checkImageOrientation();
}

int Monster::getHealth() const {
    return health_;
}

void Monster::setHealth(int health) {
    health_ = health;
}

bool Monster::isAlive() const {
    return getHealth() > 0;
}

void Monster::attacked(int damage) {
    setHealth(getHealth() - damage);
}

void Monster::setMonsterSize(qreal size) {
    MonsterSize = size;
}
