/**
 * @file procdatawdg.h
 * @brief Declarations for ProcDataWdg class
 * @author Nicu Tofan <nicu.tofan@gmail.com>
 * @copyright Copyright 2014 piles contributors. All rights reserved.
 * This file is released under the
 * [MIT License](http://opensource.org/licenses/mit-license.html)
 */

#ifndef GUARD_PROCRUNGUI_H_INCLUDE
#define GUARD_PROCRUNGUI_H_INCLUDE

#include <procrungui/procrungui-config.h>
#include <procrun/procrundata.h>

#include <QStringList>
#include <QWidget>
#include <QList>
#include <QMovie>

QT_BEGIN_NAMESPACE
class QSettings;
class QAbstractButton;
class QListWidgetItem;
QT_END_NAMESPACE

namespace Ui {
class ProcDataWdg;
}

class PrgProcess;
class ProcRunModel;
class ProcRunItem;
class ProcRunItemBase;
struct ProcRunData;

//! A widget that shows and manages processes.
class PROCRUNGUI_EXPORT ProcDataWdg : public QWidget {
    Q_OBJECT

public:

    //! Default constructor.
    ProcDataWdg (
            QWidget *parent = NULL);

    //! Constructor that also loads the data.
    ProcDataWdg (
            const ProcRunData & data,
            QWidget *parent = NULL);

    //! Destructor.
    virtual ~ProcDataWdg();

    //! Remove all entries from the new program run form.
    void
    clearProgForm ();

    //! Reload the information from cached entry to gui.
    void
    reloadFromCache ();

    //! Load the information from an entry to gui.
    void
    loadData (
            const ProcRunData & data);

    //! Save the information from gui to cached entry.
    void
    saveToCache ();

    //! Get the information from gui in an entry.
    void
    getData (
            ProcRunData & data);

    //! Get the cached data in an entry.
    const ProcRunData &
    cachedData () const {
        return item_in_form_;
    }

    //! Set the cached data from an entry and optionally reload.
    void
    setCachedData (
            const ProcRunData & data,
            bool b_reload = false) {
        item_in_form_ = data;
        if (b_reload)
            reloadFromCache ();
    }

    //! Get the program from the gui.
    QString
    program () const;

    //! Get the arguments from the gui.
    QStringList
    arguments () const;

    //! Get the working directory from the gui.
    QString
    workingDirectory () const;

    //! Get the inputs from the gui.
    QStringList
    inputs () const;




private slots:

    void
    on_argumentsListWidget_itemChanged (
            QListWidgetItem *item);

    void
    on_inputListWidget_itemChanged (
            QListWidgetItem *item);

    void
    on_programButton_clicked();

    void
    on_wrkDirButton_clicked();

private:

    void
    commonCtor ();

    Ui::ProcDataWdg *ui; /**< ui components */
    ProcRunData item_in_form_; /**< the item that is presented in the form */
    bool b_list_lock_; /**< prevent multiple events in list widgets */
};

#endif // GUARD_PROCRUNGUI_H_INCLUDE
