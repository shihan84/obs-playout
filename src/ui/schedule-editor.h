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
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QTimeEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <memory>

class ScheduleEditor : public QDialog {
    Q_OBJECT

public:
    explicit ScheduleEditor(const std::string& file_path = "", QWidget *parent = nullptr);
    ~ScheduleEditor();

private slots:
    void on_ok_clicked();
    void on_cancel_clicked();
    void on_apply_clicked();
    void on_new_playlist_clicked();
    void on_delete_playlist_clicked();
    void on_playlist_selection_changed();
    void on_new_item_clicked();
    void on_delete_item_clicked();
    void on_item_selection_changed();
    void on_browse_media_file_clicked();
    void on_move_item_up_clicked();
    void on_move_item_down_clicked();
    void on_duplicate_item_clicked();
    void on_validate_schedule_clicked();
    void on_preview_schedule_clicked();

private:
    void setup_ui();
    void setup_playlist_tab();
    void setup_items_tab();
    void setup_preview_tab();
    
    void load_schedule_file();
    void save_schedule_file();
    void update_playlist_list();
    void update_items_table();
    void update_item_form();
    void update_preview();
    
    // Form handling
    void clear_item_form();
    void populate_item_form(int row);
    void get_form_data();
    void set_form_data();
    
    // JSON handling
    bool parse_schedule_json(const QString& json_content);
    QString generate_schedule_json();
    bool validate_json_structure();
    
    // UI Components
    QTabWidget* tab_widget_;
    
    // Playlist tab
    QTableWidget* playlist_table_;
    QPushButton* new_playlist_button_;
    QPushButton* delete_playlist_button_;
    QLineEdit* playlist_name_edit_;
    QCheckBox* playlist_enabled_checkbox_;
    QList<QCheckBox*> day_checkboxes_;
    
    // Items tab
    QTableWidget* items_table_;
    QPushButton* new_item_button_;
    QPushButton* delete_item_button_;
    QPushButton* move_item_up_button_;
    QPushButton* move_item_down_button_;
    QPushButton* duplicate_item_button_;
    
    // Item form
    QGroupBox* item_form_group_;
    QLineEdit* item_name_edit_;
    QTimeEdit* item_time_edit_;
    QComboBox* item_source_combo_;
    QLineEdit* item_file_edit_;
    QPushButton* browse_media_file_button_;
    QSpinBox* item_duration_spinbox_;
    QCheckBox* item_loop_checkbox_;
    QComboBox* item_scene_combo_;
    
    // Preview tab
    QTextEdit* preview_text_edit_;
    QPushButton* validate_schedule_button_;
    QLabel* validation_status_label_;
    
    // Data
    std::string file_path_;
    QJsonObject schedule_data_;
    QString current_playlist_id_;
    int current_item_row_;
    
    // Constants
    static const QStringList DAYS_OF_WEEK;
    static const QStringList JSON_REQUIRED_FIELDS;
    
    // Helper methods
    QString get_current_playlist_id() const;
    QJsonObject get_current_playlist() const;
    QJsonArray get_current_items() const;
    void set_current_playlist(const QJsonObject& playlist);
    void set_current_items(const QJsonArray& items);
    
    QString generate_playlist_id(const QString& name) const;
    QString generate_item_id(const QJsonObject& item) const;
    
    bool validate_item(const QJsonObject& item) const;
    bool validate_playlist(const QJsonObject& playlist) const;
    
    void show_error_message(const QString& title, const QString& message);
    void show_info_message(const QString& title, const QString& message);
    void show_warning_message(const QString& title, const QString& message);
    
    // Media source helpers
    void update_media_sources();
    void update_scenes();
};
