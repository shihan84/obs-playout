#include "settings-dialog.h"
#include "scheduler-core.h"
#include "media-controller.h"
#include "utils/config.h"
#include "utils/logger.h"
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>

// Global scheduler instance (should be defined in plugin-main.cpp)
extern SchedulerCore* scheduler;

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , tab_widget_(nullptr)
    , status_timer_(new QTimer(this))
{
    setWindowTitle("Time Scheduler Settings");
    setModal(true);
    resize(800, 600);
    
    setup_ui();
    load_settings();
    
    // Setup status update timer
    connect(status_timer_, &QTimer::timeout, this, &SettingsDialog::update_status);
    status_timer_->start(1000); // Update every second
}

SettingsDialog::~SettingsDialog() {
    if (status_timer_) {
        status_timer_->stop();
    }
}

void SettingsDialog::setup_ui() {
    QVBoxLayout* main_layout = new QVBoxLayout(this);
    
    // Create tab widget
    tab_widget_ = new QTabWidget(this);
    
    // Setup tabs
    setup_general_tab();
    setup_schedule_files_tab();
    setup_status_tab();
    setup_advanced_tab();
    
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
    connect(ok_button, &QPushButton::clicked, this, &SettingsDialog::on_ok_clicked);
    connect(cancel_button, &QPushButton::clicked, this, &SettingsDialog::on_cancel_clicked);
    connect(apply_button, &QPushButton::clicked, this, &SettingsDialog::on_apply_clicked);
}

void SettingsDialog::setup_general_tab() {
    QWidget* general_widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(general_widget);
    
    // Basic settings group
    QGroupBox* basic_group = new QGroupBox("Basic Settings", this);
    QGridLayout* basic_layout = new QGridLayout(basic_group);
    
    enabled_checkbox_ = new QCheckBox("Enable Time Scheduler", this);
    basic_layout->addWidget(enabled_checkbox_, 0, 0, 1, 2);
    
    basic_layout->addWidget(new QLabel("Check Interval (seconds):", this), 1, 0);
    check_interval_spinbox_ = new QSpinBox(this);
    check_interval_spinbox_->setRange(1, 3600);
    check_interval_spinbox_->setValue(1);
    basic_layout->addWidget(check_interval_spinbox_, 1, 1);
    
    basic_layout->addWidget(new QLabel("Timezone:", this), 2, 0);
    timezone_edit_ = new QLineEdit(this);
    timezone_edit_->setText("UTC");
    basic_layout->addWidget(timezone_edit_, 2, 1);
    
    debug_mode_checkbox_ = new QCheckBox("Enable Debug Mode", this);
    basic_layout->addWidget(debug_mode_checkbox_, 3, 0, 1, 2);
    
    layout->addWidget(basic_group);
    
    // Transition settings group
    QGroupBox* transition_group = new QGroupBox("Transition Settings", this);
    QGridLayout* transition_layout = new QGridLayout(transition_group);
    
    auto_switch_scenes_checkbox_ = new QCheckBox("Auto-switch scenes", this);
    auto_switch_scenes_checkbox_->setChecked(true);
    transition_layout->addWidget(auto_switch_scenes_checkbox_, 0, 0, 1, 2);
    
    fade_transitions_checkbox_ = new QCheckBox("Use fade transitions", this);
    fade_transitions_checkbox_->setChecked(true);
    transition_layout->addWidget(fade_transitions_checkbox_, 1, 0, 1, 2);
    
    transition_layout->addWidget(new QLabel("Transition Duration (ms):", this), 2, 0);
    transition_duration_spinbox_ = new QSpinBox(this);
    transition_duration_spinbox_->setRange(0, 5000);
    transition_duration_spinbox_->setValue(500);
    transition_duration_spinbox_->setSingleStep(100);
    transition_layout->addWidget(transition_duration_spinbox_, 2, 1);
    
    layout->addWidget(transition_group);
    layout->addStretch();
    
    tab_widget_->addTab(general_widget, "General");
}

void SettingsDialog::setup_schedule_files_tab() {
    QWidget* files_widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(files_widget);
    
    // Schedule files list
    QGroupBox* files_group = new QGroupBox("Schedule Files", this);
    QVBoxLayout* files_layout = new QVBoxLayout(files_group);
    
    schedule_files_list_ = new QListWidget(this);
    schedule_files_list_->setSelectionMode(QAbstractItemView::SingleSelection);
    files_layout->addWidget(schedule_files_list_);
    
    // File management buttons
    QHBoxLayout* button_layout = new QHBoxLayout();
    
    add_schedule_file_button_ = new QPushButton("Add File...", this);
    remove_schedule_file_button_ = new QPushButton("Remove", this);
    edit_schedule_file_button_ = new QPushButton("Edit...", this);
    browse_schedule_file_button_ = new QPushButton("Browse...", this);
    
    button_layout->addWidget(add_schedule_file_button_);
    button_layout->addWidget(remove_schedule_file_button_);
    button_layout->addWidget(edit_schedule_file_button_);
    button_layout->addWidget(browse_schedule_file_button_);
    button_layout->addStretch();
    
    files_layout->addLayout(button_layout);
    layout->addWidget(files_group);
    
    // Instructions
    QTextEdit* instructions = new QTextEdit(this);
    instructions->setMaximumHeight(100);
    instructions->setReadOnly(true);
    instructions->setHtml(
        "<p><b>Instructions:</b></p>"
        "<p>Add JSON schedule files that define your media playlists. "
        "Each file should contain playlists with scheduled items.</p>"
        "<p>See the documentation for the JSON format specification.</p>"
    );
    layout->addWidget(instructions);
    
    tab_widget_->addTab(files_widget, "Schedule Files");
    
    // Connect signals
    connect(add_schedule_file_button_, &QPushButton::clicked, this, &SettingsDialog::on_add_schedule_file_clicked);
    connect(remove_schedule_file_button_, &QPushButton::clicked, this, &SettingsDialog::on_remove_schedule_file_clicked);
    connect(edit_schedule_file_button_, &QPushButton::clicked, this, &SettingsDialog::on_edit_schedule_file_clicked);
    connect(browse_schedule_file_button_, &QPushButton::clicked, this, &SettingsDialog::on_browse_schedule_file_clicked);
    connect(schedule_files_list_, &QListWidget::itemSelectionChanged, this, &SettingsDialog::on_schedule_file_selection_changed);
}

void SettingsDialog::setup_status_tab() {
    QWidget* status_widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(status_widget);
    
    // Scheduler status group
    QGroupBox* scheduler_group = new QGroupBox("Scheduler Status", this);
    QGridLayout* scheduler_layout = new QGridLayout(scheduler_group);
    
    scheduler_layout->addWidget(new QLabel("Status:", this), 0, 0);
    scheduler_status_label_ = new QLabel("Unknown", this);
    scheduler_layout->addWidget(scheduler_status_label_, 0, 1);
    
    scheduler_layout->addWidget(new QLabel("Current Item:", this), 1, 0);
    current_item_label_ = new QLabel("None", this);
    scheduler_layout->addWidget(current_item_label_, 1, 1);
    
    scheduler_layout->addWidget(new QLabel("Next Item:", this), 2, 0);
    next_item_label_ = new QLabel("None", this);
    scheduler_layout->addWidget(next_item_label_, 2, 1);
    
    scheduler_layout->addWidget(new QLabel("Next Trigger:", this), 3, 0);
    next_trigger_label_ = new QLabel("None", this);
    scheduler_layout->addWidget(next_trigger_label_, 3, 1);
    
    scheduler_layout->addWidget(new QLabel("Total Items:", this), 4, 0);
    total_items_label_ = new QLabel("0", this);
    scheduler_layout->addWidget(total_items_label_, 4, 1);
    
    scheduler_layout->addWidget(new QLabel("Active Items:", this), 5, 0);
    active_items_label_ = new QLabel("0", this);
    scheduler_layout->addWidget(active_items_label_, 5, 1);
    
    layout->addWidget(scheduler_group);
    
    // Control buttons
    QGroupBox* control_group = new QGroupBox("Controls", this);
    QHBoxLayout* control_layout = new QHBoxLayout(control_group);
    
    toggle_scheduler_button_ = new QPushButton("Toggle Scheduler", this);
    reload_schedules_button_ = new QPushButton("Reload Schedules", this);
    force_check_button_ = new QPushButton("Force Check", this);
    
    control_layout->addWidget(toggle_scheduler_button_);
    control_layout->addWidget(reload_schedules_button_);
    control_layout->addWidget(force_check_button_);
    control_layout->addStretch();
    
    layout->addWidget(control_group);
    
    // Log display
    QGroupBox* log_group = new QGroupBox("Recent Log", this);
    QVBoxLayout* log_layout = new QVBoxLayout(log_group);
    
    log_text_edit_ = new QTextEdit(this);
    log_text_edit_->setMaximumHeight(200);
    log_text_edit_->setReadOnly(true);
    log_text_edit_->setFontFamily("Consolas, monospace");
    log_layout->addWidget(log_text_edit_);
    
    layout->addWidget(log_group);
    layout->addStretch();
    
    tab_widget_->addTab(status_widget, "Status");
    
    // Connect signals
    connect(toggle_scheduler_button_, &QPushButton::clicked, this, &SettingsDialog::on_toggle_scheduler_clicked);
    connect(reload_schedules_button_, &QPushButton::clicked, this, &SettingsDialog::on_reload_schedules_clicked);
    connect(force_check_button_, &QPushButton::clicked, this, &SettingsDialog::on_force_check_clicked);
}

void SettingsDialog::setup_advanced_tab() {
    QWidget* advanced_widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(advanced_widget);
    
    // Media source settings
    QGroupBox* media_group = new QGroupBox("Media Source Settings", this);
    QGridLayout* media_layout = new QGridLayout(media_group);
    
    media_layout->addWidget(new QLabel("Default Media Source:", this), 0, 0);
    media_source_combo_ = new QComboBox(this);
    media_layout->addWidget(media_source_combo_, 0, 1);
    
    media_layout->addWidget(new QLabel("Default Scene:", this), 1, 0);
    scene_combo_ = new QComboBox(this);
    media_layout->addWidget(scene_combo_, 1, 1);
    
    media_layout->addWidget(new QLabel("Idle Content:", this), 2, 0);
    idle_content_edit_ = new QLineEdit(this);
    browse_idle_content_button_ = new QPushButton("Browse...", this);
    media_layout->addWidget(idle_content_edit_, 2, 1);
    media_layout->addWidget(browse_idle_content_button_, 2, 2);
    
    layout->addWidget(media_group);
    
    // Connection test
    QGroupBox* test_group = new QGroupBox("Connection Test", this);
    QVBoxLayout* test_layout = new QVBoxLayout(test_group);
    
    test_connection_button_ = new QPushButton("Test OBS Connection", this);
    test_layout->addWidget(test_connection_button_);
    
    layout->addWidget(test_group);
    layout->addStretch();
    
    tab_widget_->addTab(advanced_widget, "Advanced");
    
    // Connect signals
    connect(browse_idle_content_button_, &QPushButton::clicked, this, &SettingsDialog::on_browse_schedule_file_clicked);
    connect(test_connection_button_, &QPushButton::clicked, this, &SettingsDialog::on_test_connection_clicked);
}

void SettingsDialog::load_settings() {
    // Load general settings
    enabled_checkbox_->setChecked(Config::is_enabled());
    check_interval_spinbox_->setValue(Config::get_check_interval_seconds());
    timezone_edit_->setText(QString::fromStdString(Config::get_timezone()));
    debug_mode_checkbox_->setChecked(Config::is_debug_mode());
    
    // Load schedule files
    update_schedule_files_list();
    
    // Update media sources and scenes
    update_media_sources();
    update_scenes();
    
    // Update status
    update_status_display();
}

void SettingsDialog::save_settings() {
    // Save general settings
    Config::set_enabled(enabled_checkbox_->isChecked());
    Config::set_check_interval_seconds(check_interval_spinbox_->value());
    Config::set_timezone(timezone_edit_->text().toStdString());
    Config::set_debug_mode(debug_mode_checkbox_->isChecked());
}

void SettingsDialog::update_schedule_files_list() {
    schedule_files_list_->clear();
    
    auto schedule_files = Config::get_schedule_files();
    for (const auto& file_info : schedule_files) {
        QListWidgetItem* item = new QListWidgetItem();
        item->setText(QString::fromStdString(file_info.name));
        item->setData(Qt::UserRole, QString::fromStdString(file_info.path));
        item->setCheckState(file_info.enabled ? Qt::Checked : Qt::Unchecked);
        schedule_files_list_->addItem(item);
    }
}

void SettingsDialog::update_status_display() {
    if (!scheduler) {
        return;
    }
    
    // Update scheduler status
    scheduler_status_label_->setText(QString::fromStdString(scheduler->get_status()));
    current_item_label_->setText(QString::fromStdString(scheduler->get_current_item()));
    next_item_label_->setText(QString::fromStdString(scheduler->get_next_item()));
    
    // Update item counts (these would need to be implemented in scheduler)
    total_items_label_->setText("N/A");
    active_items_label_->setText("N/A");
    
    // Update next trigger time (would need to be implemented)
    next_trigger_label_->setText("N/A");
}

void SettingsDialog::update_media_sources() {
    media_source_combo_->clear();
    
    if (scheduler) {
        // This would need to be implemented in the media controller
        // For now, add placeholder
        media_source_combo_->addItem("Select media source...");
    }
}

void SettingsDialog::update_scenes() {
    scene_combo_->clear();
    
    if (scheduler) {
        // This would need to be implemented in the media controller
        // For now, add placeholder
        scene_combo_->addItem("Select scene...");
    }
}

void SettingsDialog::update_status() {
    update_status_display();
    
    // Update log with recent entries (simplified)
    static int log_counter = 0;
    log_counter++;
    if (log_counter % 5 == 0) { // Update log every 5 seconds
        log_text_edit_->append(QString("Status update #%1").arg(log_counter));
    }
}

// Slot implementations

void SettingsDialog::on_ok_clicked() {
    save_settings();
    accept();
}

void SettingsDialog::on_cancel_clicked() {
    reject();
}

void SettingsDialog::on_apply_clicked() {
    save_settings();
}

void SettingsDialog::on_browse_schedule_file_clicked() {
    QString file_path = QFileDialog::getOpenFileName(
        this,
        "Select Schedule File",
        QString(),
        "JSON Files (*.json);;All Files (*.*)"
    );
    
    if (!file_path.isEmpty()) {
        idle_content_edit_->setText(file_path);
    }
}

void SettingsDialog::on_add_schedule_file_clicked() {
    QString file_path = QFileDialog::getOpenFileName(
        this,
        "Add Schedule File",
        QString(),
        "JSON Files (*.json);;All Files (*.*)"
    );
    
    if (!file_path.isEmpty()) {
        validate_and_add_schedule_file(file_path.toStdString());
    }
}

void SettingsDialog::on_remove_schedule_file_clicked() {
    std::string file_path = get_selected_schedule_file();
    if (!file_path.empty()) {
        Config::remove_schedule_file(file_path);
        update_schedule_files_list();
    }
}

void SettingsDialog::on_edit_schedule_file_clicked() {
    std::string file_path = get_selected_schedule_file();
    if (!file_path.empty()) {
        show_schedule_file_dialog(file_path);
    }
}

void SettingsDialog::on_schedule_file_selection_changed() {
    bool has_selection = !get_selected_schedule_file().empty();
    remove_schedule_file_button_->setEnabled(has_selection);
    edit_schedule_file_button_->setEnabled(has_selection);
}

void SettingsDialog::on_test_connection_clicked() {
    // Test OBS connection
    QMessageBox::information(this, "Connection Test", "OBS connection test not implemented yet");
}

void SettingsDialog::on_reload_schedules_clicked() {
    if (scheduler) {
        scheduler->reload_schedules();
        show_info_message("Reload", "Schedules reloaded successfully");
    }
}

void SettingsDialog::on_force_check_clicked() {
    if (scheduler) {
        scheduler->force_check();
        show_info_message("Force Check", "Manual check triggered");
    }
}

void SettingsDialog::on_toggle_scheduler_clicked() {
    if (scheduler) {
        scheduler->toggle_enabled();
        update_status_display();
    }
}

// Helper methods

std::string SettingsDialog::get_selected_schedule_file() const {
    QListWidgetItem* current_item = schedule_files_list_->currentItem();
    if (current_item) {
        return current_item->data(Qt::UserRole).toString().toStdString();
    }
    return "";
}

void SettingsDialog::show_schedule_file_dialog(const std::string& file_path) {
    // This would open a schedule editor dialog
    show_info_message("Edit Schedule", "Schedule editor not implemented yet");
}

void SettingsDialog::validate_and_add_schedule_file(const std::string& file_path) {
    // Validate file exists
    if (!std::filesystem::exists(file_path)) {
        show_error_message("Error", "File does not exist: " + file_path);
        return;
    }
    
    // Add to configuration
    Config::ScheduleFile file_info;
    file_info.path = file_path;
    file_info.enabled = true;
    file_info.name = std::filesystem::path(file_path).filename().string();
    
    Config::add_schedule_file(file_info);
    update_schedule_files_list();
    
    show_info_message("Success", "Schedule file added successfully");
}

void SettingsDialog::show_error_message(const std::string& title, const std::string& message) {
    QMessageBox::critical(this, QString::fromStdString(title), QString::fromStdString(message));
}

void SettingsDialog::show_info_message(const std::string& title, const std::string& message) {
    QMessageBox::information(this, QString::fromStdString(title), QString::fromStdString(message));
}

#include "settings-dialog.moc"
