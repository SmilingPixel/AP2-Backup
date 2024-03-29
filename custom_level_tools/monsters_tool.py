from typing import *
import sys
from PySide6 import QtWidgets, QtGui
from PySide6.QtCore import Qt, QRect, QRunnable, Slot, QThreadPool, QDir
from PySide6.QtGui import QAction
from PySide6.QtWidgets import QApplication, QMainWindow, QListWidget, QVBoxLayout, QHBoxLayout, QComboBox, \
    QDoubleSpinBox, QWidget, QPushButton, QMenuBar, QMenu, QLabel, QFileDialog, QLineEdit


class MonsterRecord:

    global_monster_id = 1

    def __init__(self):
        self.id = MonsterRecord.global_monster_id
        MonsterRecord.global_monster_id += 1
        self.monster: str = ''
        self.arrival_time: int = 0  # ms
        self.buffs: List[str] = []  # can only be 'WINDFALL' or 'EVER_CHANGING'

    def __str__(self):
        return f'id: {self.id} {self.monster} {self.arrival_time}(ms) {self.buffs}'

    def __eq__(self, other):
        return self.id == other.id

    def __lt__(self, other):
        return self.arrival_time < other.arrival_time


class MonsterToolWindow(QMainWindow):

    def __init__(self):
        super().__init__()

        menu_bar: QMenuBar = self.menuBar()
        file_menu: QMenu = menu_bar.addMenu('File')
        save_action: QAction = file_menu.addAction('Save')
        save_action.triggered.connect(self.save_as_file)

        self.records: List[MonsterRecord] = []
        self.record_list: QListWidget = QListWidget()
        self.monster_selector: QComboBox = QComboBox()
        self.monster_selector.addItems([
            'Boar',
        ])
        self.arrival_time_selector: QDoubleSpinBox = QDoubleSpinBox()
        self.arrival_time_selector.setRange(0, 600)  # 0 - 10 min
        self.buff_setting: QLineEdit = QLineEdit()
        self.buff_setting.setPlaceholderText('Buffs separated by space. e.g. "WINDFALL EVER_CHANGING')
        self.add_record_btn: QPushButton = QPushButton('Add')
        self.add_record_btn.released.connect(self.add_record)
        self.remove_record_btn: QPushButton = QPushButton('Remove')
        self.remove_record_btn.released.connect(self.remove_record)

        self.main_layout: QVBoxLayout = QVBoxLayout()
        self.edit_bar: QHBoxLayout = QHBoxLayout()
        self.edit_bar.addWidget(self.monster_selector)
        self.edit_bar.addWidget(self.arrival_time_selector)
        self.edit_bar.addWidget(self.buff_setting)
        self.edit_bar.addWidget(self.add_record_btn)
        self.edit_bar.addWidget(self.remove_record_btn)
        self.main_layout.addWidget(self.record_list)
        self.main_layout.addLayout(self.edit_bar)

        self.central_widget = QWidget()
        self.central_widget.setLayout(self.main_layout)
        self.setCentralWidget(self.central_widget)
        self.setWindowTitle('Monsters Tool')

    def add_record(self):
        record: MonsterRecord = MonsterRecord()
        record.monster = self.monster_selector.currentText()
        record.arrival_time = int(self.arrival_time_selector.value() * 1000)
        record.buffs = list(sorted(self.buff_setting.text().strip().split()))
        self.record_list.addItem(record.__str__())
        self.records.append(record)

    def remove_record(self):
        curr_row = self.record_list.currentRow()
        if 0 <= curr_row < self.record_list.count():
            record_info = self.record_list.takeItem(curr_row).text().split()
            record: MonsterRecord = MonsterRecord()
            record.id = int(record_info[1])  # info[0] is 'id:'
            self.records.remove(record)

    def save_as_file(self):
        file_info: Tuple[str, str, int] = QFileDialog.getSaveFileName(
            self, "Save File",
            "monsters.dat",
            "Data (*.dat)"
        )  # Tuple[file directory, filter, length]
        if len(file_info[0]) == 0:
            return

        with open(file_info[0], 'w') as out_file:
            for record in sorted(self.records):
                out_file.write(f'{record.monster} {record.arrival_time}')
                for buff in record.buffs:
                    out_file.write(' ' + buff)
                out_file.write('\n')


if __name__ == '__main__':
    app = QApplication(sys.argv)
    main_window = MonsterToolWindow()
    main_window.show()
    app.exec()
