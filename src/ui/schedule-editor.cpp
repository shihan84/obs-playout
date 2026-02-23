#include "schedule-editor.h"
#include "media-controller.h"
#include "utils/logger.h"
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QStandardPaths>

// Static constants
const QStringList ScheduleEditor::DAYS_OF_WEEK = {
    "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
};

const QStringList ScheduleEditor::JSON_REQUIRED_FIELDS = {
    "version", "playlists"
};

ScheduleEditor::ScheduleEditor(const std::string& file_path, QWidget *parent)
    : QDialog(parent)
    , tab_widget_(nullptr)
    , file_path_(file_path)
    , current_item_row_(-1)
{
    setWindowTitle("Schedule Editor");
    setModal(true);
    resize(1000, 700);
    
    setup_ui();
    load_schedule_file();
    
    if (file_path_.empty()) {
        setWindowTitle("Schedule Editor - New Schedule");
    } else {
        setWindowTitle(QString("Schedule Editor - %1").arg(QString::fromStdString(file_path_)));
    }
}

ScheduleEditor::~ScheduleEditor() {
}

void ScheduleEditor::setup_ui() {
    QVBoxLayout* main_layout = new QVBoxLayout(this);
    
    // Create tab widget
    tab_widget_ = new QTabWidget(this);
    
    // Setup tabs
    setup_playlist_tab();
    setup_items_tab();
    setup_preview_tab();
    
    main_layout->addWidget(tab_widget_);
    
    // Create button box
    QHBoxLayout* button_layout = new QHBoxLayout();
    button_layout->addStretch();
    
    QPushButton* ok_button = new QPushButton("OK", this);
    QPushButton* cancel_button = new QPushButton("Cancel", this);
    QPushButton* apply_button = new QPushButton("Apply", this);
    
    button_layout->addWidget(ok_button);
    button_layout->addWidget(cancel_button);
    button_layout->addWidget(apply_button);
    
    main_layout->addLayout(button_layout);
    
    // Connect signals
    connect(ok_button, &QPushButton::clicked, this, &ScheduleEditor::on_ok_clicked);
    connect(cancel_button, &QPushButton::clicked, this, &ScheduleEditor::on_cancel_clicked);
    connect(apply_button, &QPushButton::clicked, this, &ScheduleEditor::on_apply_clicked);
}

void ScheduleEditor::setup_playlist_tab() {
    QWidget* playlist_widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(playlist_widget);
    
    // Playlist list
    QGroupBox* list_group = new QGroupBox("Playlists", this);
    QVBoxLayout* list_layout = new QVBoxLayout(list_group);
    
    playlist_table_ = new QTableWidget(0, 3, this);
    playlist_table_->setHorizontalHeaderLabels({"Name", "Enabled", "Items"});
    playlist_table_->horizontalHeader()->setStretchLastSection(true);
    playlist_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    playlist_table_->setSelectionMode(QAbstractItemView::SingleSelection);
    list_layout->addWidget(playlist_table_);
    
    // Playlist buttons
    QHBoxLayout* playlist_button_layout = new QHBoxLayout();
    new_playlist_button_ = new QPushButton("New Playlist", this);
    delete_playlist_button_ = new QPushButton("Delete Playlist", this);
    
    playlist_button_layout->addWidget(new_playlist_button_);
    playlist_button_layout->addWidget(delete_playlist_button_);
    playlist_button_layout->addStretch();
    
    list_layout->addLayout(playlist_button_layout);
    layout->addWidget(list_group);
    
    // Playlist properties
    QGroupBox* properties_group = new QGroupBox("Playlist Properties", this);
    QGridLayout* properties_layout = new QGridLayout(properties_group);
    
    properties_layout->addWidget(new QLabel("Name:", this), 0, 0);
    playlist_name_edit_ = new QLineEdit(this);
    properties_layout->addWidget(playlist_name_edit_, 0, 1);
    
    playlist_enabled_checkbox_ = new QCheckBox("Enabled", this);
    playlist_enabled_checkbox_->setChecked(true);
    properties_layout->addWidget(playlist_enabled_checkbox_, 1, 0, 1, 2);
    
    properties_layout->addWidget(new QLabel("Active Days:", this), 2, 0);
    QWidget* days_widget = new QWidget(this);
    QGridLayout* days_layout = new QGridLayout(days_widget);
    
    for (int i = 0; i < DAYS_OF_WEEK.size(); ++i) {
        QCheckBox* checkbox = new QCheckBox(DAYS_OF_WEEK[i], this);
        checkbox->setChecked(true);
        day_checkboxes_.append(checkbox);
        days_layout->addWidget(checkbox, i / 4, i % 4);
    }
    
    properties_layout->addWidget(days_widget, 2, 1);
    layout->addWidget(properties_group);
    
    tab_widget_->addTab(playlist_widget, "Playlists");
    
    // Connect signals
    connect(new_playlist_button_, &QPushButton::clicked, this, &ScheduleEditor::on_new_playlist_clicked);
    connect(delete_playlist_button_, &QPushButton::clicked, this, &ScheduleEditor::on_delete_playlist_clicked);
    connect(playlist_table_, &QTableWidget::itemSelectionChanged, this, &ScheduleEditor::on_playlist_selection_changed);
}

void ScheduleEditor::setup_items_tab() {
    QWidget* items_widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(items_widget);
    
    // Items table
    QGroupBox* items_group = new QGroupBox("Scheduled Items", this);
    QVBoxLayout* items_layout = new QVBoxLayout(items_group);
    
    items_table_ = new QTableWidget(0, 6, this);
    items_table_->setHorizontalHeaderLabels({"Time", "Name", "Source", "File", "Duration", "Loop"});
    items_table_->horizontalHeader()->setStretchLastSection(true);
    items_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    items_table_->setSelectionMode(QAbstractItemView::SingleSelection);
    items_layout->addWidget(items_table_);
    
    // Item buttons
    QHBoxLayout* item_button_layout = new QHBoxLayout();
    new_item_button_ = new QPushButton("New Item", this);
    delete_item_button_ = new QPushButton("Delete Item", this);
    move_item_up_button_ = new QPushButton("Move Up", this);
    move_item_down_button_ = new QPushButton("Move Down", this);
    duplicate_item_button_ = new QPushButton("Duplicate", this);
    
    item_button_layout->addWidget(new_item_button_);
    item_button_layout->addWidget(delete_item_button_);
    item_button_layout->addWidget(move_item_up_button_);
    item_button_layout->addWidget(move_item_down_button_);
    item_button_layout->addWidget(duplicate_item_button_);
    item_button_layout->addStretch();
    
    items_layout->addLayout(item_button_layout);
    layout->addWidget(items_group);
    
    // Item form
    item_form_group_ = new QGroupBox("Item Properties", this);
    QGridLayout* form_layout = new QGridLayout(item_form_group_);
    
    form_layout->addWidget(new QLabel("Name:", this), 0, 0);
    item_name_edit_ = new QLineEdit(this);
    form_layout->addWidget(item_name_edit_, 0, 1);
    
    form_layout->addWidget(new QLabel("Time:", this), 1, 0);
    item_time_edit_ = new QTimeEdit(this);
    item_time_edit_->setDisplayFormat("HH:mm");
    form_layout->addWidget(item_time_edit_, 1, 1);
    
    form_layout->addWidget(new QLabel("Media Source:", this), 2, 0);
    item_source_combo_ = new QComboBox(this);
    form_layout->addWidget(item_source_combo_, 2, 1);
    
    form_layout->addWidget(new QLabel("Media File:", this), 3, 0);
    item_file_edit_ = new QLineEdit(this);
    browse_media_file_button_ = new QPushButton("Browse...", this);
    form_layout->addWidget(item_file_edit_, 3, 1);
    form_layout->addWidget(browse_media_file_button_, 3, 2);
    
    form_layout->addWidget(new QLabel("Duration (seconds):", this), 4, 0);
    item_duration_spinbox_ = new QSpinBox(this);
    item_duration_spinbox_->setRange(0, 86400);
    item_duration_spinbox_->setSpecialValueText("Auto-detect");
    form_layout->addWidget(item_duration_spinbox_, 4, 1);
    
    item_loop_checkbox_ = new QCheckBox("Loop", this);
    form_layout->addWidget(item_loop_checkbox_, 5, 0, 1, 2);
    
    form_layout->addWidget(new QLabel("Scene:", this), 6, 0);
    item_scene_combo_ = new QComboBox(this);
    item_scene_combo_->addItem("No scene change");
    form_layout->addWidget(item_scene_combo_, 6, 1);
    
    layout->addWidget(item_form_group_);
    
    tab_widget_->addTab(items_widget, "Items");
    
    // Connect signals
    connect(new_item_button_, &QPushButton::clicked, this, &ScheduleEditor::on_new_item_clicked);
    connect(delete_item_button_, &QPushButton::clicked, this, &ScheduleEditor::on_delete_item_clicked);
    connect(move_item_up_button_, &QPushButton::clicked, this, &ScheduleEditor::on_move_item_up_clicked);
    connect(move_item_down_button_, &QPushButton::clicked, this, &ScheduleEditor::on_move_item_down_clicked);
    connect(duplicate_item_button_, &QPushButton::clicked, this, &ScheduleEditor::on_duplicate_item_clicked);
    connect(browse_media_file_button_, &QPushButton::clicked, this, &ScheduleEditor::on_browse_media_file_clicked);
    connect(items_table_, &QTableWidget::itemSelectionChanged, this, &ScheduleEditor::on_item_selection_changed);
}

void ScheduleEditor::setup_preview_tab() {
    QWidget* preview_widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(preview_widget);
    
    // Validation status
    validation_status_label_ = new QLabel("Schedule not validated", this);
    validation_status_label_->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    layout->addWidget(validation_status_label_);
    
    // Validate button
    validate_schedule_button_ = new QPushButton("Validate Schedule", this);
    layout->addWidget(validate_schedule_button_);
    
    // Preview text
    preview_text_edit_ = new QTextEdit(this);
    preview_text_edit_->setReadOnly(true);
    preview_text_edit_->setFontFamily("Consolas, monospace");
    layout->addWidget(preview_text_edit_);
    
    tab_widget_->addTab(preview_widget, "Preview");
    
    // Connect signals
    connect(validate_schedule_button_, &QPushButton::clicked, this, &ScheduleEditor::on_validate_schedule_clicked);
}

void ScheduleEditor::load_schedule_file() {
    if (file_path_.empty()) {
        // Create new empty schedule
        schedule_data_ = QJsonObject();
        schedule_data_["version"] = "1.0";
        schedule_data_["timezone"] = "UTC";
        schedule_data_["default_idle"] = "";
        schedule_data_["playlists"] = QJsonArray();
        
        update_playlist_list();
        update_preview();
        return;
    }
    
    QFile file(QString::fromStdString(file_path_));
    if (!file.open(QIODevice::ReadOnly)) {
        show_error_message("Error", "Cannot open schedule file: " + QString::fromStdString(file_path_));
        return;
    }
    
    QByteArray file_data = file.readAll();
    file.close();
    
    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(file_data, &parse_error);
    
    if (parse_error.error != QJsonParseError::NoError) {
        show_error_message("JSON Error", "Failed to parse JSON: " + parse_error.errorString());
        return;
    }
    
    if (!doc.isObject()) {
        show_error_message("Format Error", "Schedule file must contain a JSON object");
        return;
    }
    
    schedule_data_ = doc.object();
    
    if (!validate_json_structure()) {
        show_warning_message("Warning", "Schedule file has missing required fields. Defaults will be added.");
    }
    
    update_playlist_list();
    update_preview();
}

void ScheduleEditor::save_schedule_file() {
    if (file_path_.empty()) {
        // Ask for file path
        QString save_path = QFileDialog::getSaveFileName(
            this,
            "Save Schedule File",
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/schedule.json",
            "JSON Files (*.json);;All Files (*.*)"
        );
        
        if (save_path.isEmpty()) {
            return false;
        }
        
        file_path_ = save_path.toStdString();
    }
    
    QString json_content = generate_schedule_json();
    
    QFile file(QString::fromStdString(file_path_));
    if (!file.open(QIODevice::WriteOnly)) {
        show_error_message("Error", "Cannot save schedule file: " + QString::fromStdString(file_path_));
        return false;
    }
    
    file.write(json_content.toUtf8());
    file.close();
    
    show_info_message("Success", "Schedule file saved successfully");
    return true;
}

void ScheduleEditor::update_playlist_list() {
    playlist_table_->setRowCount(0);
    
    if (!schedule_data_.contains("playlists")) {
        return;
    }
    
    QJsonArray playlists = schedule_data_["playlists"].toArray();
    
    for (int i = 0; i < playlists.size(); ++i) {
        QJsonObject playlist = playlists[i].toObject();
        
        int row = playlist_table_->rowCount();
        playlist_table_->insertRow(row);
        
        playlist_table_->setItem(row, 0, new QTableWidgetItem(playlist["name"].toString()));
        playlist_table_->setItem(row, 1, new QTableWidgetItem(playlist["enabled"].toBool() ? "Yes" : "No"));
        
        QJsonArray items = playlist["items"].toArray();
        playlist_table_->setItem(row, 2, new QTableWidgetItem(QString::number(items.size())));
    }
    
    if (playlist_table_->rowCount() > 0) {
        playlist_table_->selectRow(0);
    }
}

void ScheduleEditor::update_items_table() {
    items_table_->setRowCount(0);
    
    QJsonObject playlist = get_current_playlist();
    if (playlist.isEmpty()) {
        clear_item_form();
        return;
    }
    
    QJsonArray items = playlist["items"].toArray();
    
    for (int i = 0; i < items.size(); ++i) {
        QJsonObject item = items[i].toObject();
        
        int row = items_table_->rowCount();
        items_table_->insertRow(row);
        
        items_table_->setItem(row, 0, new QTableWidgetItem(item["time"].toString()));
        items_table_->setItem(row, 1, new QTableWidgetItem(item["name"].toString()));
        items_table_->setItem(row, 2, new QTableWidgetItem(item["source"].toString()));
        items_table_->setItem(row, 3, new QTableWidgetItem(item["file"].toString()));
        
        int duration = item["duration"].toInt(0);
        QString duration_str = duration > 0 ? QString::number(duration) : "Auto";
        items_table_->setItem(row, 4, new QTableWidgetItem(duration_str));
        
        items_table_->setItem(row, 5, new QTableWidgetItem(item["loop"].toBool() ? "Yes" : "No"));
    }
    
    if (items_table_->rowCount() > 0) {
        items_table_->selectRow(0);
    } else {
        clear_item_form();
    }
}

void ScheduleEditor::update_item_form() {
    if (current_item_row_ >= 0 && current_item_row_ < items_table_->rowCount()) {
        populate_item_form(current_item_row_);
    } else {
        clear_item_form();
    }
}

void ScheduleEditor::update_preview() {
    QString json_content = generate_schedule_json();
    
    QJsonDocument doc = QJsonDocument::fromJson(json_content.toUtf8());
    preview_text_edit_->setPlainText(doc.toJson(QJsonDocument::Indented));
}

// Slot implementations

void ScheduleEditor::on_ok_clicked() {
    if (save_schedule_file()) {
        accept();
    }
}

void ScheduleEditor::on_cancel_clicked() {
    reject();
}

void ScheduleEditor::on_apply_clicked() {
    save_schedule_file();
}

void ScheduleEditor::on_new_playlist_clicked() {
    QJsonObject new_playlist;
    new_playlist["name"] = "New Playlist";
    new_playlist["enabled"] = true;
    new_playlist["days"] = QJsonArray::fromStringList(QStringList{"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"});
    new_playlist["items"] = QJsonArray();
    
    QJsonArray playlists = schedule_data_["playlists"].toArray();
    playlists.append(new_playlist);
    schedule_data_["playlists"] = playlists;
    
    update_playlist_list();
    update_preview();
}

void ScheduleEditor::on_delete_playlist_clicked() {
    int current_row = playlist_table_->currentRow();
    if (current_row < 0) {
        return;
    }
    
    QJsonArray playlists = schedule_data_["playlists"].toArray();
    playlists.removeAt(current_row);
    schedule_data_["playlists"] = playlists;
    
    update_playlist_list();
    update_items_table();
    update_preview();
}

void ScheduleEditor::on_playlist_selection_changed() {
    int current_row = playlist_table_->currentRow();
    if (current_row < 0) {
        return;
    }
    
    QJsonArray playlists = schedule_data_["playlists"].toArray();
    QJsonObject playlist = playlists[current_row].toObject();
    
    playlist_name_edit_->setText(playlist["name"].toString());
    playlist_enabled_checkbox_->setChecked(playlist["enabled"].toBool());
    
    QJsonArray days = playlist["days"].toArray();
    QStringList day_list;
    for (const auto& day : days) {
        day_list.append(day.toString());
    }
    
    for (int i = 0; i < day_checkboxes_.size(); ++i) {
        day_checkboxes_[i]->setChecked(day_list.contains(DAYS_OF_WEEK[i]));
    }
    
    update_items_table();
}

void ScheduleEditor::on_new_item_clicked() {
    QJsonObject new_item;
    new_item["name"] = "New Item";
    new_item["time"] = "09:00";
    new_item["source"] = "";
    new_item["file"] = "";
    new_item["duration"] = 0;
    new_item["loop"] = false;
    new_item["scene"] = "";
    
    QJsonObject playlist = get_current_playlist();
    QJsonArray items = playlist["items"].toArray();
    items.append(new_item);
    playlist["items"] = items;
    set_current_playlist(playlist);
    
    update_items_table();
    update_preview();
}

void ScheduleEditor::on_delete_item_clicked() {
    int current_row = items_table_->currentRow();
    if (current_row < 0) {
        return;
    }
    
    QJsonObject playlist = get_current_playlist();
    QJsonArray items = playlist["items"].toArray();
    items.removeAt(current_row);
    playlist["items"] = items;
    set_current_playlist(playlist);
    
    update_items_table();
    update_preview();
}

void ScheduleEditor::on_item_selection_changed() {
    current_item_row_ = items_table_->currentRow();
    update_item_form();
}

void ScheduleEditor::on_browse_media_file_clicked() {
    QString file_path = QFileDialog::getOpenFileName(
        this,
        "Select Media File",
        QString(),
        "Video Files (*.mp4 *.avi *.mov *.mkv);;Audio Files (*.mp3 *.wav *.ogg);;All Files (*.*)"
    );
    
    if (!file_path.isEmpty()) {
        item_file_edit_->setText(file_path);
    }
}

void ScheduleEditor::on_move_item_up_clicked() {
    int current_row = items_table_->currentRow();
    if (current_row <= 0) {
        return;
    }
    
    QJsonObject playlist = get_current_playlist();
    QJsonArray items = playlist["items"].toArray();
    
    // Swap items
    QJsonValue temp = items[current_row];
    items[current_row] = items[current_row - 1];
    items[current_row - 1] = temp;
    
    playlist["items"] = items;
    set_current_playlist(playlist);
    
    update_items_table();
    items_table_->selectRow(current_row - 1);
    update_preview();
}

void ScheduleEditor::on_move_item_down_clicked() {
    int current_row = items_table_->currentRow();
    QJsonObject playlist = get_current_playlist();
    QJsonArray items = playlist["items"].toArray();
    
    if (current_row >= items.size() - 1) {
        return;
    }
    
    // Swap items
    QJsonValue temp = items[current_row];
    items[current_row] = items[current_row + 1];
    items[current_row + 1] = temp;
    
    playlist["items"] = items;
    set_current_playlist(playlist);
    
    update_items_table();
    items_table_->selectRow(current_row + 1);
    update_preview();
}

void ScheduleEditor::on_duplicate_item_clicked() {
    int current_row = items_table_->currentRow();
    if (current_row < 0) {
        return;
    }
    
    QJsonObject playlist = get_current_playlist();
    QJsonArray items = playlist["items"].toArray();
    
    if (current_row < items.size()) {
        QJsonObject original_item = items[current_row].toObject();
        QJsonObject duplicated_item = original_item;
        duplicated_item["name"] = original_item["name"].toString() + " (Copy)";
        
        items.insert(current_row + 1, duplicated_item);
        playlist["items"] = items;
        set_current_playlist(playlist);
        
        update_items_table();
        items_table_->selectRow(current_row + 1);
        update_preview();
    }
}

void ScheduleEditor::on_validate_schedule_clicked() {
    bool is_valid = validate_json_structure();
    
    if (is_valid) {
        validation_status_label_->setText("Schedule is valid ✓");
        validation_status_label_->setStyleSheet("QLabel { color: green; font-weight: bold; }");
    } else {
        validation_status_label_->setText("Schedule has errors ✗");
        validation_status_label_->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    }
}

void ScheduleEditor::on_preview_schedule_clicked() {
    update_preview();
}

// Helper methods

void ScheduleEditor::clear_item_form() {
    item_name_edit_->clear();
    item_time_edit_->setTime(QTime(9, 0));
    item_source_combo_->setCurrentIndex(0);
    item_file_edit_->clear();
    item_duration_spinbox_->setValue(0);
    item_loop_checkbox_->setChecked(false);
    item_scene_combo_->setCurrentIndex(0);
}

void ScheduleEditor::populate_item_form(int row) {
    QJsonObject playlist = get_current_playlist();
    QJsonArray items = playlist["items"].toArray();
    
    if (row >= items.size()) {
        return;
    }
    
    QJsonObject item = items[row].toObject();
    
    item_name_edit_->setText(item["name"].toString());
    
    QTime time = QTime::fromString(item["time"].toString(), "HH:mm");
    item_time_edit_->setTime(time);
    
    QString source = item["source"].toString();
    int source_index = item_source_combo_->findText(source);
    item_source_combo_->setCurrentIndex(source_index >= 0 ? source_index : 0);
    
    item_file_edit_->setText(item["file"].toString());
    item_duration_spinbox_->setValue(item["duration"].toInt(0));
    item_loop_checkbox_->setChecked(item["loop"].toBool());
    
    QString scene = item["scene"].toString();
    int scene_index = item_scene_combo_->findText(scene);
    item_scene_combo_->setCurrentIndex(scene_index >= 0 ? scene_index : 0);
}

QString ScheduleEditor::generate_schedule_json() const {
    QJsonObject schedule = schedule_data_;
    
    // Update current playlist if any
    if (!current_playlist_id_.isEmpty()) {
        QJsonArray playlists = schedule["playlists"].toArray();
        for (int i = 0; i < playlists.size(); ++i) {
            QJsonObject playlist = playlists[i].toObject();
            if (playlist["id"].toString() == current_playlist_id_) {
                playlist["name"] = playlist_name_edit_->text();
                playlist["enabled"] = playlist_enabled_checkbox_->isChecked();
                
                QStringList selected_days;
                for (int j = 0; j < day_checkboxes_.size(); ++j) {
                    if (day_checkboxes_[j]->isChecked()) {
                        selected_days.append(DAYS_OF_WEEK[j]);
                    }
                }
                playlist["days"] = QJsonArray::fromStringList(selected_days);
                
                playlists[i] = playlist;
                break;
            }
        }
        schedule["playlists"] = playlists;
    }
    
    QJsonDocument doc(schedule);
    return doc.toJson(QJsonDocument::Indented);
}

bool ScheduleEditor::validate_json_structure() const {
    // Check required fields
    for (const QString& field : JSON_REQUIRED_FIELDS) {
        if (!schedule_data_.contains(field)) {
            return false;
        }
    }
    
    // Validate playlists
    if (!schedule_data_["playlists"].isArray()) {
        return false;
    }
    
    QJsonArray playlists = schedule_data_["playlists"].toArray();
    for (const auto& playlist_val : playlists) {
        if (!playlist_val.isObject()) {
            return false;
        }
        
        QJsonObject playlist = playlist_val.toObject();
        if (!validate_playlist(playlist)) {
            return false;
        }
    }
    
    return true;
}

bool ScheduleEditor::validate_playlist(const QJsonObject& playlist) const {
    if (!playlist.contains("name") || !playlist.contains("items")) {
        return false;
    }
    
    if (!playlist["items"].isArray()) {
        return false;
    }
    
    QJsonArray items = playlist["items"].toArray();
    for (const auto& item_val : items) {
        if (!item_val.isObject()) {
            return false;
        }
        
        QJsonObject item = item_val.toObject();
        if (!validate_item(item)) {
            return false;
        }
    }
    
    return true;
}

bool ScheduleEditor::validate_item(const QJsonObject& item) const {
    QStringList required_fields = {"name", "time", "source"};
    
    for (const QString& field : required_fields) {
        if (!item.contains(field)) {
            return false;
        }
    }
    
    // Validate time format
    QString time_str = item["time"].toString();
    QRegularExpression time_regex(R"(^([01]?[0-9]|2[0-3]):[0-5][0-9]$)");
    if (!time_regex.match(time_str).hasMatch()) {
        return false;
    }
    
    return true;
}

QString ScheduleEditor::get_current_playlist_id() const {
    int current_row = playlist_table_->currentRow();
    if (current_row < 0) {
        return QString();
    }
    
    QJsonArray playlists = schedule_data_["playlists"].toArray();
    QJsonObject playlist = playlists[current_row].toObject();
    return playlist["id"].toString();
}

QJsonObject ScheduleEditor::get_current_playlist() const {
    int current_row = playlist_table_->currentRow();
    if (current_row < 0) {
        return QJsonObject();
    }
    
    QJsonArray playlists = schedule_data_["playlists"].toArray();
    return playlists[current_row].toObject();
}

QJsonArray ScheduleEditor::get_current_items() const {
    QJsonObject playlist = get_current_playlist();
    return playlist["items"].toArray();
}

void ScheduleEditor::set_current_playlist(const QJsonObject& playlist) {
    int current_row = playlist_table_->currentRow();
    if (current_row < 0) {
        return;
    }
    
    QJsonArray playlists = schedule_data_["playlists"].toArray();
    playlists[current_row] = playlist;
    schedule_data_["playlists"] = playlists;
}

void ScheduleEditor::set_current_items(const QJsonArray& items) {
    QJsonObject playlist = get_current_playlist();
    playlist["items"] = items;
    set_current_playlist(playlist);
}

void ScheduleEditor::show_error_message(const QString& title, const QString& message) {
    QMessageBox::critical(this, title, message);
}

void ScheduleEditor::show_info_message(const QString& title, const QString& message) {
    QMessageBox::information(this, title, message);
}

void ScheduleEditor::show_warning_message(const QString& title, const QString& message) {
    QMessageBox::warning(this, title, message);
}

void ScheduleEditor::update_media_sources() {
    // This would need to be implemented to get actual media sources from OBS
    item_source_combo_->clear();
    item_source_combo_->addItem("Select media source...");
    item_source_combo_->addItem("Media Source 1");
    item_source_combo_->addItem("Media Source 2");
}

void ScheduleEditor::update_scenes() {
    // This would need to be implemented to get actual scenes from OBS
    item_scene_combo_->clear();
    item_scene_combo_->addItem("No scene change");
    item_scene_combo_->addItem("Scene 1");
    item_scene_combo_->addItem("Scene 2");
}

#include "schedule-editor.moc"
