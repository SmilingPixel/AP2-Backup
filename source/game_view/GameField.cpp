#include "game_view/GameField.h"


// TODO: init from file should be called by constructor
GameField::GameField(QObject* parent):
    QGraphicsScene(parent)
{
    // handle process events
    timer_.setInterval(static_cast<int>(1000 /* ms */ / fps_));
    connect(&timer_, &QTimer::timeout, this, &GameField::updateField);

    // Initialize some info of Character, Area and Monster
    // including area size
    Character::setCharacterSize(CHARACTER_SIZE);
    Area::setAreaSize(AREA_SIZE);
    Monster::setMonsterSize(MONSTER_SIZE);
}


void GameField::loadLevelFromFile(const QString& dir_path) {
    // Check if the dir exists
    if(!QDir(dir_path).exists())
        throw std::runtime_error("Directory does not exist");
    // Load field data
    loadFieldFromFile(QString("%1/field.dat").arg(dir_path));
    // Load characters from data
    loadCharacterOptionFromFile(QString("%1/characters.dat").arg(dir_path));
    // Load monsters
    loadMonsterQueueFromFile(QString("%1/monsters.dat").arg(dir_path));
    // Load level settings
    loadLevelSettingFromFile(QString("%1/level_setting.dat").arg(dir_path));
    // Some UI need to set up after initialization above
    initOptionUi();
}

void GameField::loadFieldFromFile(const QString &file_path) {
    QFile in_file(file_path);
    if(!in_file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QString line;
    // read length and width
    line = in_file.readLine().simplified();
    QStringList size = line.split(u' ', Qt::SkipEmptyParts);
    num_rows_ = size[0].toInt();
    num_cols_ = size[1].toInt();
    if(num_rows_ <= 0 || num_cols_ <= 0)
        throw std::invalid_argument("Invalid field size");

    // read indices of road areas
    // begin with a line, with size of roads in it
    // then comes size lines, each of them looks like this:
    // "row_idx,col_idx,to_direction,to_direction,to_direction,to_direction,road_type"
    // The 4 directions are values mapped from {-1, 0}, {0, 1}, {1, 0}, {0, -1}
    // Each direction_ looks like "x,y", so it should be split by ',' as well
    // road_type: 1 to start, 2 to Protection Objective, otherwise 0
    // TODO: Invalid input
    QHash<AreaIndex, Area*> pos2road;
    while(!in_file.atEnd()){
        line = in_file.readLine().simplified();
        QStringList info = line.split(u' ', Qt::SkipEmptyParts);
        Road* road = new Road();
        int directions[5] = {-1, 0, 1, 0, -1};
        for(int k = 0; k < 4; ++k){
            QPair<int, int> from = qMakePair(directions[k], directions[k + 1]);
            QPair<int, int> to = qMakePair(info[2 + k * 2].toInt(), info[3 + k * 2].toInt());
            road->setDirection(from, to);
        }
        AreaIndex pos = QPoint(info[0].toInt(), info[1].toInt());
        pos2road[pos] = road;
        if(info[10].toInt() == 1)
            start_areas_idx_.push_back(pos);
        else if(info[10].toInt() == 2)
            protect_areas_idx_.push_back(pos);
    }
    in_file.close();

    // fill the field
    areas_ = QList<QList<Area*>>(num_rows_);
    for(int i = 0; i < num_rows_; ++i){
        for(int j = 0; j < num_cols_; ++j){
            auto pos = QPoint(i, j);
            Area* item = pos2road.value(pos, new Grass());
            addItem(item);
            item->setPos(AREA_SIZE * j, AREA_SIZE * i);
            areas_[i].push_back(item);
        }
    }
}

void GameField::loadCharacterOptionFromFile(const QString& file_path) {
    // There are strings indicating characters in the file
    // Each line represents a character, which may be "Elf", "Knight", etc.
    // Read them out, and fill character_makers_ and character_textures_ correspondingly
    QFile in_file(file_path);
    if(!in_file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QString line;
    while(!in_file.atEnd()){
        line = in_file.readLine().trimmed();
        std::function<Character*()> maker;
        QString texture_file_path;
        if(line.size() == 0 || line.startsWith("//")){
            continue;
        }
        else if(line == "Elf"){
            maker = []{return new Elf;};
            texture_file_path = Elf::TEXTURE;
        }
        else if(line == "Knight"){
            maker = []{return new Knight;};
            texture_file_path = Knight::TEXTURE;
        }
        else{
            throw std::invalid_argument("Invalid character in characters.dat");
        }
        character_makers_.push_back(maker);
        character_textures_.push_back(texture_file_path);
    }
    in_file.close();
}

void GameField::loadMonsterQueueFromFile(const QString& file_path) {
    // There lines in this file
    // Each line is made up of (Monster name, arrival time(ms))
    // e.g. one line is "Boar 30")
    QFile in_file(file_path);
    if(!in_file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QString line;
    while(!in_file.atEnd()){
        line = in_file.readLine().simplified();
        if(line.size() == 0 || line.startsWith("//"))
            continue;
        QStringList info = line.split(u' ', Qt::SkipEmptyParts);
        QPair<Monster*, int> monster_arrival;
        if(info[0] == "Boar")
            monster_arrival.first = new Boar;
        else
            throw std::invalid_argument("Invalid monster in monsters.dat");
        monster_arrival.second = info[1].toInt();
        this->monster_que_.enqueue(monster_arrival);
    }
    in_file.close();
}

void GameField::loadLevelSettingFromFile(const QString& file_path) {
    // Line 1: Life points of player
    QFile in_file(file_path);
    if(!in_file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    this->life_points_ = in_file.readLine().simplified().toInt();

    in_file.close();
}

void GameField::initOptionUi() {

    // Construct place buttons and set as invisible
    auto* place_options_layout = new QGraphicsLinearLayout;
    for(int i = 0; i < character_makers_.size(); ++i){
        auto& maker = character_makers_[i];
        auto file_name = character_textures_[i];
        auto button_pixmap = QPixmap(file_name).scaled(CHARACTER_OPTION_SIZE, CHARACTER_OPTION_SIZE);
        /*
         * Add a background (not needed when a default background is given by QPushButton)
        auto mask = button_pixmap.createMaskFromColor(Qt::transparent, Qt::MaskOutColor);
        auto painter = QPainter(&button_pixmap);
        painter.setPen(QColor(255, 255, 255, 128));
        painter.drawPixmap(button_pixmap.rect(), mask, mask.rect());
        painter.end();
         */
        auto* button = new QPushButton();
        button->setIcon(button_pixmap);
        button->setIconSize(QSize(CHARACTER_OPTION_SIZE, CHARACTER_OPTION_SIZE));
        connect(button, &QPushButton::released,
                [maker = maker, this](){
                    this->placeCharacter(maker);
                });
        auto* proxy = new QGraphicsProxyWidget;
        proxy->setWidget(button);
        place_options_layout->addItem(proxy);
    }
    this->place_options_->setLayout(place_options_layout);
    addItem(place_options_);
    place_options_->setVisible(false);
    place_options_->setZValue(1);

    // Construct upgrade buttons and set as invisible
    auto* upgrade_options_layout = new QGraphicsLinearLayout;
    QStringList upgrade_options_icons = {UP_ICON, X_ICON};
    for(const auto& icon: upgrade_options_icons){
        auto button_pixmap = QPixmap(icon).scaled(CHARACTER_OPTION_SIZE, CHARACTER_OPTION_SIZE);

        auto* button = new QPushButton();
        button->setIcon(button_pixmap);
        button->setIconSize(QSize(CHARACTER_OPTION_SIZE, CHARACTER_OPTION_SIZE));

        // Connect button's signal to slots
        if(icon == UP_ICON)
            connect(button, &QPushButton::released, this, &GameField::upgradeCharacter);
        else if(icon == X_ICON)
            connect(button, &QPushButton::released, this, &GameField::removeCharacter);
        else
            throw std::invalid_argument("Invalid upgrade option type");

        auto* proxy = new QGraphicsProxyWidget;
        proxy->setWidget(button);
        upgrade_options_layout->addItem(proxy);
    }
    this->upgrade_options_->setLayout(upgrade_options_layout);
    addItem(upgrade_options_);
    upgrade_options_->setVisible(false);
    upgrade_options_->setZValue(1);
}

void GameField::setFps(qreal fps) {
    fps_ = fps;
    timer_.setInterval(static_cast<int>(1000 /* ms */ / fps_));
}


void GameField::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) {
    QGraphicsScene::mouseReleaseEvent(mouseEvent);
    if(place_options_->isVisible() || upgrade_options_->isVisible()) {
        place_options_->setVisible(false);
        upgrade_options_->setVisible(false);
        return;
    }
    auto pos = mouseEvent->scenePos();
    auto area_idx = posToIndex(pos);
    auto* area = areas_[area_idx.x()][area_idx.y()];
    displayCharacterOptions(
            area_idx,
            area->isOccupied() ? upgrade_options_ : place_options_
    );
}


void GameField::displayCharacterOptions(const AreaIndex& area_idx, QGraphicsWidget* options) {
    auto* area = areas_[area_idx.x()][area_idx.y()];
    // Set the buttons below the area vertically
    // and at the center of the area horizontally

    // Reset old parent's z value, so that it wouldn't cover the options
    auto* old_parent = dynamic_cast<Area*>(options->parentItem());
    if(old_parent)
        old_parent->setZValue(0);
    options->setParentItem(area);
    //Set old parent's z value, so that its child, options cannot be covered
    area->setZValue(1);
    options->setPos(area->boundingRect().center() - options->rect().center());
    options->setY(area->boundingRect().height());
    options->setVisible(true);
}


typename GameField::AreaIndex GameField::posToIndex(QPointF pos) {
    qreal x = pos.x();
    qreal y = pos.y();
    auto res = QPoint();
    res.ry() = x < 0 ? -1 : qFloor(x + REAL_COMPENSATION) / qRound(AREA_SIZE);
    res.rx() = y < 0 ? -1 : qFloor(y + REAL_COMPENSATION) / qRound(AREA_SIZE);
    return res;
}


bool GameField::qRealEqual(qreal r1, qreal r2) {
    return qAbs(r1 - r2) <= REAL_COMPENSATION;
}


bool GameField::pointFloatEqual(const QPointF& p1, const QPointF& p2){
    return qRealEqual(p1.x(), p2.x()) && qRealEqual(p1.y(), p2.y());
}


void GameField::startGame() {
    timer_.start();
}

void GameField::checkReachProtectionObjective() {
    auto it = monsters_.begin();
    while(it != monsters_.end()){
        Monster* monster = *it;
        // Still heading for protection objective
        if(monster->getDirection() != qMakePair(0, 0)) {
            ++it;
            continue;
        }
        auto cur_pos = monster->pos();
        auto cur_area_idx = posToIndex(cur_pos);
        if(!protect_areas_idx_.contains(cur_area_idx)){
            throw std::runtime_error(
                    "Game Error: monster direction is {0, 0} "
                    "but hasn't reach protection objective"
                    );
        }
        life_points_--;
        removeItem(monster);
        it = monsters_.erase(it);
    }
}

void GameField::checkGameEnd() {
    if(life_points_ > 0 && (!monsters_.empty() || !monster_que_.empty()))
        return;

    auto* background = new QGraphicsRectItem;
    background->setPen(Qt::NoPen);
    background->setBrush(QBrush(
            (monsters_.empty() && monster_que_.empty()) ?
            QColor(0, 255, 0, 128) : // Win
            QColor(255, 0, 0, 128) // Lose
    ));
    background->setRect(this->sceneRect());
    background->setZValue(3); // Above all other
    addItem(background);
    timer_.stop();
}

void GameField::moveMonsters() {
    for(auto* monster: monsters_){
        qreal total_move_dis = monster->getSpeed() * timer_.interval() / 1000;
        while(total_move_dis > REAL_COMPENSATION){
            // Limit move_dis in one loop
            // so that it is not more than one area.
            // For security, the limit is half the size of area
            qreal move_dis = qMin(AREA_SIZE / 2, total_move_dis);
            total_move_dis -= move_dis;

            auto cur_direction = monster->getDirection();
            if(cur_direction == qMakePair(0, 0))
                break;
            auto cur_pos = monster->pos();
            auto next_pos = cur_pos;
            next_pos.setX(next_pos.x() + cur_direction.first * move_dis);
            next_pos.setY(next_pos.y() + cur_direction.second * move_dis);

            auto cur_area_idx = posToIndex(cur_pos);
            auto next_area_idx = posToIndex(next_pos);

            // After moving, monster is in the origin area.
            if(cur_area_idx == next_area_idx){
                monster->setPos(next_pos);
                move_dis = 0.0;
                auto cur_area = qgraphicsitem_cast<Road*>(areas_[cur_area_idx.x()][cur_area_idx.y()]);
                // If pos is same as any area,
                // direction may need to be changed
                if(pointFloatEqual(monster->pos(), cur_area->pos()))
                    monster->setDirection(cur_area->getToDirection(monster->getDirection()));
                break;
            }

            /* Go into another area
             *
             * In posToIndex(), each area has two edges,
             * so there are 2 situations.
             *
             * If direction is {-1, 0} or {0, -1},
             * new direction should come from next_area
             *
             * Else if direction is {1, 0} or {0, 1},
             * new direction should come from cur_area
             *
             * For more details, refer to code below
             */

            /* Note:
             * Under normal conditions, next_area would not out of range,
             * so just ignore the condition
             */

            auto cur_area = qgraphicsitem_cast<Road*>(areas_[cur_area_idx.x()][cur_area_idx.y()]);
            if(cur_direction == qMakePair(-1, 0)
               || cur_direction == qMakePair(0, -1)){
                auto cur_area_pos = cur_area->pos();
                move_dis -= qAbs(cur_area_pos.x() - cur_pos.x());
                move_dis -= qAbs(cur_area_pos.y() - cur_pos.y());
                // Edge condition: monster's pos is exactly the pos of area,
                // direction has been set before, and cannot be reset.
                if(!pointFloatEqual(monster->pos(), cur_area->pos()))
                    monster->setDirection(cur_area->getToDirection(monster->getDirection()));
                monster->setPos(cur_area_pos);

                // Reach protection obj
                if(protect_areas_idx_.contains(cur_area_idx)){
                    monster->setDirection(qMakePair(0, 0));
                    break;
                }

                // Attention: if new direction is same as th old one
                // and monster's pos is exactly the one of some area,
                // we need to continue moving.
                qreal cont_move_dis = qMin(AREA_SIZE / 2, move_dis);
                cur_direction = monster->getDirection();
                cur_pos = monster->pos();
                next_pos = cur_pos;
                next_pos.setX(next_pos.x() + cur_direction.first * cont_move_dis);
                next_pos.setY(next_pos.y() + cur_direction.second * cont_move_dis);
                move_dis -= cont_move_dis;
                monster->setPos(next_pos);
            }
            else{
                auto next_area = qgraphicsitem_cast<Road*>(areas_[next_area_idx.x()][next_area_idx.y()]);
                auto next_area_pos = next_area->pos();
                move_dis -= qAbs(next_area_pos.x() - cur_pos.x());
                move_dis -= qAbs(next_area_pos.y() - cur_pos.y());
                monster->setPos(next_area_pos);
                monster->setDirection(next_area->getToDirection(monster->getDirection()));
                // Reach protection obj
                if(protect_areas_idx_.contains(next_area_idx)){
                    monster->setDirection(qMakePair(0, 0));
                    break;
                }
            }
            // if move_dis is greater than 0, it needs to be back to total distance
            if(move_dis > REAL_COMPENSATION)
                total_move_dis += move_dis;
        }
    }
}


void GameField::generateMonsters() {
    while(!monster_que_.empty() && monster_que_.head().second <= game_time_){
        auto monster_arrival = monster_que_.dequeue();
        auto *monster = monster_arrival.first;
        addItem(monster);
        monsters_.push_back(monster);
        // Select a start area randomly
        auto start_idx = start_areas_idx_[QRandomGenerator::global()->bounded(start_areas_idx_.size())];
        auto start_area = areas_[start_idx.x()][start_idx.y()];
        auto start_pos = start_area->pos();
        monster->setPos(start_pos);
        // Set position as center of area
        monster->setOffset(start_area->boundingRect().center() - monster->boundingRect().center());
        // Init moving direction of the monster
        int directions[5] = {-1, 0, 1, 0, -1};
        for(int i = 0; i < 4; ++i){
            Direction from = qMakePair(directions[i], directions[i + 1]);
            // C++17 If statement with initializer
            if(auto to = qgraphicsitem_cast<Road*>(start_area)->getToDirection(from); to != qMakePair(0, 0)) {
                monster->setDirection(to);
                break;
            }
        }
    }
}


void GameField::updateField() {
    game_time_ += timer_.interval();
    generateMonsters();
    moveMonsters();
    checkReachProtectionObjective();
    checkGameEnd();
}


void GameField::placeCharacter(const std::function<Character*()>& maker) {
    auto* character = maker();
    auto* area = dynamic_cast<Area*>(place_options_->parentItem());
    if(!area)
        throw std::runtime_error("place_options_ has invalid parent");

    // Check if the character can be placed on this area
    // e.g. Character Elf cannot be placed on road
    if((qgraphicsitem_cast<Grass*>(area) && !character->testAreaCond(Character::ON_GRASS))
        || (qgraphicsitem_cast<Road*>(area) && !character->testAreaCond(Character::ON_ROAD))
       )
        return;

    // Set area as parent of the character
    // So we don't need to handle coordinates
    character->setParentItem(area);
    // Set position as center of area
    character->setOffset(area->boundingRect().center() - character->boundingRect().center());
    area->setOccupied(true);
    characters_.push_back(character);
}

void GameField::upgradeCharacter() {

}

void GameField::removeCharacter() {
    auto* area = dynamic_cast<Area*>(upgrade_options_->parentItem());
    if(!area)
        throw std::runtime_error("upgrade_options_ has invalid parent");
    // If not has Character as a child, exception will be thrown
    Character* character = nullptr;
    for(auto* child: area->childItems()){
        character = dynamic_cast<Character *>(child);
        if(character)
            break;
    }
    if(!character)
        throw std::runtime_error("area doesn't has a Character");
    // Update info of the area and remove the character
    area->setOccupied(false);
    if(!characters_.removeOne(character))
        throw std::runtime_error("Fail to move character from list");
    removeItem(character);
}