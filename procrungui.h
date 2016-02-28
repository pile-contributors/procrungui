/**
 * @file procrungui.h
 * @brief Declarations for ProcRunGui class
 * @author Nicu Tofan <nicu.tofan@gmail.com>
 * @copyright Copyright 2014 piles contributors. All rights reserved.
 * This file is released under the
 * [MIT License](http://opensource.org/licenses/mit-license.html)
 */

#ifndef GUARD_PROCRUNGUI_H_INCLUDE
#define GUARD_PROCRUNGUI_H_INCLUDE

#include <procrungui/procrungui-config.h>

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
class ProcRunGui;
}

class PrgProcess;
class ProcRunModel;
class ProcRunItem;
class ProcRunItemBase;

//! A widget that shows and manages processes.
class PROCRUNGUI_EXPORT ProcRunGui : public QWidget {
    Q_OBJECT

    friend class PrgProcess;

public:

    //! The callback used to inform the caller that a process finished.
    typedef void (*Kb) (ProcRunGui *, PrgProcess *, void*);


    //! Default constructor.
    ProcRunGui (
            QWidget *parent = NULL);

    //! Destructor.
    virtual ~ProcRunGui();

    //! We should run a program.
    PrgProcess *
    runProgram (
            const QString & s_program,
            const QStringList & sl_args = QStringList(),
            const QString s_crt_path = QString (),
            const QStringList & sl_input = QStringList(),
            Kb kb = NULL,
            void * user_data = NULL);

    //! Find the index of the program given its process.
    int
    programIndex (
            PrgProcess * prg);

    //! Find the pointer for the program given its index.
    PrgProcess *
    program (
            int idx);

    //! Reads saved commands from a file.
    bool
    loadCommands (
            const QString & s_file = QString ());

    //! Reads saved commands from settings.
    bool
    loadCommands (
            QSettings & s_data );

    //! Reads saved commands from a file.
    bool
    saveCommands (
            const QString & s_file = QString ());

    //! Reads saved commands from settings.
    bool
    saveCommands (
            QSettings & s_data );

    //! Remove all entries from the new program run form.
    void
    clearProgForm ();

    //! Save all entries from the new program run form.
    void
    saveProgramForm ();

    //! Fill in the form from a program entry.
    void
    loadProgramForm (
            ProcRunItem *item);

    //! Change the model.
    void
    setCmdModel (
            ProcRunModel *mdl);

    //! Get current item in the tree.
    ProcRunItemBase *
    selectedCmdEntry ();

    //! Ask the user if sure and remove from model.
    void
    removeItem (
            ProcRunItemBase *item);

public slots:

    //! Creates a new group around selected item.
    void
    addNewGroup();

protected:

    //! Used by running processes to inform the instance about activity.
    QString
    processGeneratedText (
            PrgProcess *proc,
            const QString & s_input,
            bool is_error);

    //! A process has finished its execution.
    void
    finishProcess (
            PrgProcess *proc);

    //! A process Is removed right now.
    void
    processDone (
            PrgProcess *proc,
            int idx = -1);

    virtual void
    closeEvent (
            QCloseEvent *);


    virtual void
    timerEvent (
            QTimerEvent *);

private slots:

    void
    on_tabWidget_currentChanged (
            int index);

    bool
    on_tabWidget_tabCloseRequested (
            int index);

    void on_buttonBox_clicked(QAbstractButton *button);

    void on_runButton_clicked();

    void on_terminateButton_clicked();

    void on_killButton_clicked();

    void on_treeView_customContextMenuRequested(const QPoint &pos);

    void on_argumentsListWidget_itemChanged(QListWidgetItem *item);

    void on_inputListWidget_itemChanged(QListWidgetItem *item);

    void on_programButton_clicked();

    void on_wrkDirButton_clicked();

signals:

    //! The window is about to be closed.
    void
    aboutToClose ();

protected slots:
    void treeviewSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
private:
    Ui::ProcRunGui *ui; /**< ui components */
    QList<PrgProcess*> processes_; /**< the list of processes */
    bool close_on_last_; /**< should we also close when last process is closed? */
    bool autoclose_finished_; /**< when a process terminates do we remove the tab? */
    QMovie running_mov_; /**< .gif showing that it's running */
    ProcRunModel * cmdmodl_; /**< the model for saved commands */
    ProcRunItem * item_in_form_; /**< the item that is presented in the form */
    bool b_list_lock_; /**< prevent multiple events in list widgets */
};

#endif // GUARD_PROCRUNGUI_H_INCLUDE
