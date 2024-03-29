#ifndef AP_PROJ_GAMEFIELD_H
#define AP_PROJ_GAMEFIELD_H

#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QStringList>
#include <QHash>
#include <QPair>
#include <QPoint>
#include <QTimer>
#include <QtGlobal>
#include <QRandomGenerator>
#include <QPainter>
#include <QPushButton>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsWidget>
#include <QTransform>
#include <QQueue>
#include <functional>
#include "BuffUtil.h"
#include "Grass.h"
#include "Road.h"
#include "Entity.h"
#include "Monster.h"
#include "Boar.h"
#include "Character.h"
#include "Elf.h"
#include "Knight.h"


class GameField: public QGraphicsScene{

    using Direction = QPair<int, int>;
    using AreaIndex = QPoint;

    // Style of button
    // Refer to design of Minecraft
    static constexpr const char* CHARACTER_OPTION_BUTTON_STYLE_FILE = ":/styles/character_option_button.qss";
    QString character_option_button_style_;

    static constexpr const char* ICON_UP = ":/icons/chevrons_up.svg";
    static constexpr const char* ICON_X = ":/icons/x_square.svg";
    static constexpr const char* ICON_HEALTH = ":/icons/health_point.png";
    static constexpr const char* ICON_MONSTER = ":/icons/monster_icon.png";

    static constexpr const char* BGM = "qrc:/sounds/GallantChallenge.m4a";

    static constexpr const qreal AREA_SIZE = 48; // px
    static constexpr const qreal CHARACTER_OPTION_SIZE = 32; // px
    static constexpr const qreal BUFF_OPTION_SIZE = 32; // px
    static constexpr const qreal CHARACTER_SIZE = 48; // px
    static constexpr const qreal MONSTER_SIZE = 48; // px
    static constexpr const qreal ICON_HEALTH_SIZE = 28; // px (origin image is 7*7, so...)
    static constexpr const qreal ICON_MONSTER_SIZE = 32; // px

    static constexpr const qreal REAL_COMPENSATION = 0.0000001;

    int num_rows_ = 0;
    int num_cols_ = 0;

    QTimer timer_;
    qint64 game_time_ = 0; // time (ms) since game start; It should be updated by updateField
    qreal fps_ = 59; // refresh rate

    // Queue of (monster, arrival time) pairs
    // time (second of pair) should be sorted by ascending order
    QQueue<QPair<Monster*, int>> monster_que_;

    QList<QList<Area*>> areas_;
    QList<Monster*> monsters_;
    QList<Character*> characters_;

    QList<AreaIndex> start_areas_idx_;
    QList<AreaIndex> protect_areas_idx_;

    int health_points_ = 1;

    // Below are components related to character.
    // place_options_ and upgrade_options_ each holds a layout, which may holds more than one options.
    // place_options_ is used to place characters.
    // upgrade_options_ is used to upgrade or remove characters.
    QGraphicsWidget* place_options_ = new QGraphicsWidget;
    QGraphicsWidget* upgrade_options_ = new QGraphicsWidget;
    // character_makers_ contains makers of characters that can be placed in this field.
    // character_textures_ is list of file name of corresponding textures.
    QList<std::function<Character*()>> character_makers_;
    QStringList character_textures_;

    // buff_options_ holds a layout, which contains multiple buffs that can be used.
    // This layout should be visible when an area with character on it is selected
    QGraphicsWidget* buff_options_ = new QGraphicsWidget;
    // You can use buff to control corresponding button
    QHash<Buff, QGraphicsProxyWidget*> buff_option_buttons_;

    // Used in status bar
    QGraphicsSimpleTextItem* health_point_counter_;
    QGraphicsSimpleTextItem* monster_counter_;


public:
    explicit GameField(QObject* parent = nullptr);

    /**
     * Load data from files
     * dir_path should contain 3 files:
     * field.dat: data of field
     * characters.dat: characters can be used
     * monsters.dat: monsters that will appear in this level, along with time of arrival
     * level_setting.dat: other settings of this level, such as life points of player
     * @param dir_path const QString& directory having data of field, monsters and characters cna be used
     */
    void loadLevelFromFile(const QString& dir_path);

    // Called by loadLevelFromFile()
    void loadFieldFromFile(const QString& file_path);

    // Called by loadLevelFromFile()
    void loadCharacterOptionFromFile(const QString& file_path);

    // Called by loadLevelFromFile()
    void loadMonsterQueueFromFile(const QString& file_path);

    // Called by loadLevelFromFile()
    void loadLevelSettingFromFile(const QString& file_path);

    // Called by loadLevelFromFile()
    void loadStyleFromFile();

    /**
     * Initialize place_options_ and upgrade_options_
     * Must be called explicitly, for no other functions will call it
     */
    void initCharacterOptionUi();

    /**
     * Initialize buff_options_
     * Must be called explicitly
     */
    void initBuffOptionUi();

    /**
     * Initialize media player
     * Must be called explicitly
     */
    void initMedia();

    /**
     * Initialize status bar (health point and monster counter)
     * Must be called explicitly
     */
    void initStatusBarUi();

    void setFps(qreal fps);

    /**
     * Start the game.
     * Should be called explicitly.
     */
    void startGame();

    /**
     * Pause the game.
     * Should be called explicitly.
     */
    void pauseGame();

private:

    /**
     * Called by updateField()
     * Move monsters in each frame
     */
    void moveMonsters();

    /**
     * Called by updateField()
     * Check if monsters are to be generated in each frame.
     * If so, do it.
     */
    void generateMonsters();

    /**
     * Called by updateField()
     * Update status of entities, e.g. buff, continuous damage...
     */
    void updateEntityStatus();

    /**
     * Called by updateField()
     * Check monsters and characters, handle interactions between them
     * e.g. An Elf tryAttack a Boar, a Boar attacks a Knight
     */
    void entityInteract();

    /**
     * Called by updateField()
     * Update info in status bar, including health points and monster counter
     */
    void updateStatusBar();

    /**
     * Remove dead entities, including characters and monsters
     */
    void removeDeadEntity();

    /**
     * Remove specific character
     * Note:
     * Statement of area holding the character should be reset
     * characters_ should be updated
     */
    void removeCharacter(Character* character);

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) override;

    /**
     * Called when click on an area, either holding a character or not
     *
     * @param area_idx Index of area in this->areas_
     * @param options Option GUI that is to show
     */
    void displayCharacterOptions(const AreaIndex& area_idx, QGraphicsWidget* options);

    /**
     * Update checked value of buff option buttons.
     * Set checked if character has buff, unchecked otherwise.
     * This method should be called by mouseReleaseEvent() only.
     * @param area_idx Index of area that the character is placed in
     */
    void updateBuffOptionChecked(const AreaIndex& area_idx);

    /**
     * Unlock one or more buff option buttons,
     * i.e. set member(s) of this->buff_option_buttons_ as visible.
     * If all buttons is visible, do nothing.
     */
    void getNewBuff();

    /**
     * Returns character that in specific area
     *
     * @param area: area that you want to find character from
     */
    Character* getCharacterInArea(Area* area);

    /**
    * Returns {row_idx, col_idx} of the area which pos is at.
    * If pos is out of rect of scene, a valid pair will still be returned.
    * -1 <= row_idx <= num_rows, -1 <= col_idx <= num_cols.
    */
    static AreaIndex posToIndex(QPointF pos);

    /**
     * Return if r1 equals to r2
     * Taking double precision into account
     */
    static inline bool qRealEqual(qreal r1, qreal r2) {
        return qAbs(r1 - r2) <= REAL_COMPENSATION;
    }

    /**
     * Return if pos1 equals to pos2
     * Taking double precision into account
     * Note: qRealEqual(qreal, qreal) is called in this method
     */
    static inline bool pointFloatEqual(const QPointF& p1, const QPointF& p2){
        return qRealEqual(p1.x(), p2.x()) && qRealEqual(p1.y(), p2.y());
    }

    /**
     * Check if any monster has reached the Protection Objective.
     * If so, remove it from the field and minus health_points_ by 1.
     * If
     */
    void checkReachProtectionObjective();

    /**
     * Check if game is end
     * Win: There no monster in field and monster_que_, and life_point_ > 0
     * Lose: Otherwise
     */
    void checkGameEnd();

private slots:

    /**
     * Slot for game timer
     * It should do all things controlled by game timer,
     * e.g. move monsters, make tryAttack actions
     * In order to ensure they're called in fixed order, we have this slot
     * For more, please refer to implementation
     */
    void updateField();

    /**
     * Slot for placeCharacter button
     * Note: need a lambda to pass parameter `type`
     * @param type: type of character (call typeToCharacter() to make a new character)
     */
    void placeCharacter(const std::function<Character*()>& maker);

    /**
     * Slot for upgradeCharacterFromUi button
     */
    void upgradeCharacterFromUi();

    /**
     * Slot for removeCharacterFromUi button
     */
    void removeCharacterFromUi();

    /**
     * Slot for buttons in buff_options
     * When button is triggered, the buff will be set on character selected
     * If button is unchecked, trigger leads to adding buff to character;
     * otherwise to removing buff of character
     * Note: At most 2 compatible buff can exist at same time
     */
    void manageCharacterBuffFromUI(Buff buff);


};

#endif //AP_PROJ_GAMEFIELD_H
