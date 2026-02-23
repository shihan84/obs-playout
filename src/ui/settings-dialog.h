#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QListWidget>
#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <memory>

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

private slots:
    void on_ok_clicked();
    void on_cancel_clicked();
    void on_apply_clicked();
    void on_browse_schedule_file_clicked();
    void on_add_schedule_file_clicked();
    void on_remove_schedule_file_clicked();
    void on_edit_schedule_file_clicked();
    void on_schedule_file_selection_changed();
    void on_test_connection_clicked();
    void on_reload_schedules_clicked();
    void on_force_check_clicked();
    void on_toggle_scheduler_clicked();
    void update_status();

private:
    void setup_ui();
    void setup_general_tab();
    void setup_schedule_files_tab();
    void setup_status_tab();
    void setup_advanced_tab();
    
    void load_settings();
    void save_settings();
    void update_schedule_files_list();
    void update_status_display();
    void update_media_sources();
    void update_scenes();
    
    // UI Components
    QTabWidget* tab_widget_;
    
    // General tab
    QCheckBox* enabled_checkbox_;
    QSpinBox* check_interval_spinbox_;
    QLineEdit* timezone_edit_;
    QCheckBox* debug_mode_checkbox_;
    QCheckBox* auto_switch_scenes_checkbox_;
    QCheckBox* fade_transitions_checkbox_;
    QSpinBox* transition_duration_spinbox_;
    
    // Schedule files tab
    QListWidget* schedule_files_list_;
    QPushButton* add_schedule_file_button_;
    QPushButton* remove_schedule_file_button_;
    QPushButton* edit_schedule_file_button_;
    QPushButton* browse_schedule_file_button_;
    
    // Status tab
    QLabel* scheduler_status_label_;
    QLabel* current_item_label_;
    QLabel* next_item_label_;
    QLabel* total_items_label_;
    QLabel* active_items_label_;
    QLabel* next_trigger_label_;
    QTextEdit* log_text_edit_;
    QPushButton* toggle_scheduler_button_;
    QPushButton* reload_schedules_button_;
    QPushButton* force_check_button_;
    
    // Advanced tab
    QComboBox* media_source_combo_;
    QComboBox* scene_combo_;
    QLineEdit* idle_content_edit_;
    QPushButton* browse_idle_content_button_;
    QPushButton* test_connection_button_;
    
    // Status update timer
    QTimer* status_timer_;
    
    // Helper methods
    std::string get_selected_schedule_file() const;
    void show_schedule_file_dialog(const std::string& file_path = "");
    void validate_and_add_schedule_file(const std::string& file_path);
    void show_error_message(const std::string& title, const std::string& message);
    void show_info_message(const std::string& title, const std::string& message);
};
