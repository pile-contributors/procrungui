/**
 * @file procdatawdg.cc
 * @brief Definitions for ProcDataWdg class.
 * @author Nicu Tofan <nicu.tofan@gmail.com>
 * @copyright Copyright 2014 piles contributors. All rights reserved.
 * This file is released under the
 * [MIT License](http://opensource.org/licenses/mit-license.html)
 */

#include "procdatawdg.h"
#include "ui_procdatawdg.h"

#include "procrungui-private.h"

#include <procrun/procrunmodel.h>

#include <QProcess>
#include <QThread>
#include <QDateTime>
#include <QLabel>
#include <QMessageBox>
#include <QCloseEvent>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QMenu>
#include <QAction>
#include <QStyle>
#include <QApplication>
#include <QFileDialog>

#include <assert.h>

/**
 * @class ProcDataWdg
 *
 */

/* ------------------------------------------------------------------------- */
ProcDataWdg::ProcDataWdg(QWidget * parent) :
    QWidget(parent),
    ui(new Ui::ProcDataWdg ()),
    item_in_form_(),
    b_list_lock_(false)
{
    PROCRUNGUI_TRACE_ENTRY;
    commonCtor ();
    PROCRUNGUI_TRACE_EXIT;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
ProcDataWdg::ProcDataWdg (const ProcRunData & data, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProcDataWdg ()),
    item_in_form_(data),
    b_list_lock_(false)
{
    PROCRUNGUI_TRACE_ENTRY;
    commonCtor ();
    reloadFromCache ();
    PROCRUNGUI_TRACE_EXIT;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/**
 */
ProcDataWdg::~ProcDataWdg()
{
    PROCRUNGUI_TRACE_ENTRY;
    delete ui;
    PROCRUNGUI_TRACE_EXIT;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcDataWdg::clearProgForm ()
{
    ui->programLineEdit->clear ();
    while (ui->argumentsListWidget->count() > 1) {
        if (ui->argumentsListWidget->item (0)->icon().isNull()) {
            delete ui->argumentsListWidget->takeItem (0);
        }
    }
    while (ui->inputListWidget->count() > 1) {
        if (ui->inputListWidget->item (0)->icon().isNull()) {
            delete ui->inputListWidget->takeItem (0);
        }
    }
    ui->wrkDirLineEdit->clear ();
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcDataWdg::reloadFromCache ()
{
    loadData (item_in_form_);
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcDataWdg::loadData (const ProcRunData &data)
{
    ui->programLineEdit->setText (data.s_program_);

    foreach(const QString & s_iter, data.sl_arguments_) {
        QListWidgetItem * li = new QListWidgetItem (s_iter);
        li->setFlags (li->flags() |
                      Qt::ItemIsEditable |
                      Qt::ItemIsEnabled |
                      Qt::ItemIsSelectable);
        ui->argumentsListWidget->addItem (li);
    }
    // ui->argumentsListWidget->addItems (data.sl_arguments_);
    ui->argumentsListWidget->addItem (ui->argumentsListWidget->takeItem(0));

    //ui->inputListWidget->addItems (data.sl_input_);
    foreach(const QString & s_iter, data.sl_input_) {
        QListWidgetItem * li = new QListWidgetItem (s_iter);
        li->setFlags (li->flags() |
                      Qt::ItemIsEditable |
                      Qt::ItemIsEnabled |
                      Qt::ItemIsSelectable);
        ui->inputListWidget->addItem (li);
    }
    ui->inputListWidget->addItem (ui->inputListWidget->takeItem(0));

    ui->wrkDirLineEdit->setText (data.s_wrk_dir_);
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcDataWdg::saveToCache ()
{
    getData (item_in_form_);
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcDataWdg::getData (ProcRunData &data)
{
    data.s_program_ = ui->programLineEdit->text ();

    data.sl_arguments_.clear ();
    int i_max = ui->argumentsListWidget->count ();
    for (int i = 0; i < i_max; ++i) {
        QListWidgetItem * item = ui->argumentsListWidget->item (i);
        if (item->icon().isNull()) {
            data.sl_arguments_.append (item->text ());
        }
    }

    data.sl_input_.clear ();
    i_max = ui->inputListWidget->count ();
    for (int i = 0; i < i_max; ++i) {
        QListWidgetItem * item = ui->inputListWidget->item (i);
        if (item->icon().isNull()) {
            data.sl_input_.append (item->text ());
        }
    }

    data.s_wrk_dir_= ui->wrkDirLineEdit->text ();
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
QString ProcDataWdg::program () const
{
    return ui->programLineEdit->text ();
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
QStringList ProcDataWdg::arguments () const
{
    QStringList result;
    int i_max = ui->argumentsListWidget->count ();
    for (int i = 0; i < i_max; ++i) {
        QListWidgetItem * item = ui->argumentsListWidget->item (i);
        if (item->icon().isNull()) {
            result.append (item->text ());
        }
    }
    return result;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
QString ProcDataWdg::workingDirectory () const
{
    return ui->wrkDirLineEdit->text ();
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
QStringList ProcDataWdg::inputs () const
{
    QStringList result;
    int i_max = ui->inputListWidget->count ();
    for (int i = 0; i < i_max; ++i) {
        QListWidgetItem * item = ui->inputListWidget->item (i);
        if (item->icon().isNull()) {
            result.append (item->text ());
        }
    }
    return result;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcDataWdg::on_argumentsListWidget_itemChanged (QListWidgetItem *item)
{
    if (b_list_lock_)
        return;
    b_list_lock_ = true;
    if (!item->icon().isNull()) {
        QListWidgetItem * li = new QListWidgetItem (item->text ());
        li->setFlags (li->flags() |
                      Qt::ItemIsEditable |
                      Qt::ItemIsEnabled |
                      Qt::ItemIsSelectable);
        int inspos = ui->argumentsListWidget->row (item);
        ui->argumentsListWidget->insertItem (inspos, li);
        item->setText (item->data (Qt::UserRole).toString ());
    }
    b_list_lock_ = false;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcDataWdg::on_inputListWidget_itemChanged (QListWidgetItem *item)
{
    if (b_list_lock_)
        return;
    b_list_lock_ = true;
    if (!item->icon().isNull()) {
        QListWidgetItem * li = new QListWidgetItem (item->text ());
        li->setFlags (li->flags() |
                      Qt::ItemIsEditable |
                      Qt::ItemIsEnabled |
                      Qt::ItemIsSelectable);
        ui->inputListWidget->insertItem (
                    ui->inputListWidget->count() - 2, li);
        item->setText (item->data (Qt::UserRole).toString ());
    }
    b_list_lock_ = false;
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcDataWdg::on_programButton_clicked()
{
    QFileDialog dialog (this);
    dialog.setFileMode (QFileDialog::ExistingFile);
    dialog.setViewMode (QFileDialog::Detail);

    // filters
    QStringList filters;
    filters.append (QLatin1String ("Programs (*.exe)"));
    filters.append (trUtf8 ("All Files (*)"));
    dialog.setNameFilters (filters);

    dialog.selectFile (
                QDir::fromNativeSeparators (
                    ui->programLineEdit->text ()));

    if (dialog.exec()) {
        if (dialog.selectedFiles().count() == 1) {
            ui->programLineEdit->setText (QDir::toNativeSeparators (
                        dialog.selectedFiles ().at (0)));
        }
    }
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcDataWdg::on_wrkDirButton_clicked()
{
    QFileDialog dialog (this);
    dialog.setFileMode (QFileDialog::Directory);
    dialog.setViewMode (QFileDialog::Detail);
    dialog.setOption (QFileDialog::ShowDirsOnly, true);
    QString s_path = ui->wrkDirLineEdit->text ();
    dialog.selectFile (QDir::fromNativeSeparators (s_path));

    if (dialog.exec()) {
        if (dialog.selectedFiles().count() == 1) {
            ui->wrkDirLineEdit->setText (
                        QDir::toNativeSeparators (
                            dialog.selectedFiles ().at (0)));
        }
    }
}
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
void ProcDataWdg::commonCtor ()
{
    PROCRUNGUI_TRACE_ENTRY;
    ui->setupUi (this);

    QListWidgetItem * it;
    it = new QListWidgetItem (
        qApp->style()->standardIcon (
                    QStyle::SP_ArrowRight),
                tr ("Add new argument"));
    it->setData (Qt::UserRole, it->text());
    it->setFlags (it->flags() |
                  Qt::ItemIsEditable |
                  Qt::ItemIsEnabled |
                  Qt::ItemIsSelectable);
    ui->argumentsListWidget->addItem (it);

    it = new QListWidgetItem (
        qApp->style()->standardIcon (
                    QStyle::SP_ArrowRight),
                tr ("Add new input"));
    it->setData (Qt::UserRole, it->text());
    it->setFlags (it->flags() |
                  Qt::ItemIsEditable |
                  Qt::ItemIsEnabled |
                  Qt::ItemIsSelectable);
    ui->inputListWidget->addItem (it);
    PROCRUNGUI_TRACE_EXIT;
}
/* ========================================================================= */
