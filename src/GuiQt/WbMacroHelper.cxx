
/*LICENSE_START*/
/*
 *  Copyright (C) 2018 Washington University School of Medicine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*LICENSE_END*/

#define __WB_MACRO_HELPER_DECLARE__
#include "WbMacroHelper.h"
#undef __WB_MACRO_HELPER_DECLARE__

#include "Brain.h"
#include "BrainBrowserWindow.h"
#include "CaretAssert.h"
#include "CaretLogger.h"
#include "CaretPreferences.h"
#include "EventCaretDataFilesGet.h"
#include "EventGraphicsUpdateAllWindows.h"
#include "EventGraphicsUpdateOneWindow.h"
#include "EventManager.h"
#include "EventSceneActive.h"
#include "EventUserInterfaceUpdate.h"
#include "GuiManager.h"
#include "MovieRecorder.h"
#include "Scene.h"
#include "SceneFile.h"
#include "SceneInfo.h"
#include "SessionManager.h"
#include "WbMacroCustomOperationTypeEnum.h"
#include "WbMacroCustomDataTypeEnum.h"
#include "WuQMacroCommand.h"
#include "WuQMacroCommandParameter.h"
#include "WuQMacroExecutorOptions.h"
#include "WuQMacroGroup.h"
#include "WuQMessageBox.h"

using namespace caret;


    
/**
 * \class caret::WbMacroHelper
 * \brief Implementation of WuQMacroHelperInterface that provides macro groups
 * \ingroup GuiQt
 */

/**
 * Constructor.
 */
WbMacroHelper::WbMacroHelper(QObject* parent)
: WuQMacroHelperInterface(parent)
{
    EventManager::get()->addEventListener(this, EventTypeEnum::EVENT_USER_INTERFACE_UPDATE);
}

/**
 * Destructor.
 */
WbMacroHelper::~WbMacroHelper()
{
    EventManager::get()->removeAllEventsFromListener(this);
}

/**
 * Receive an event
 *
 * @param event
 *     The event.
 */
void
WbMacroHelper::receiveEvent(Event* event)
{
    CaretAssert(event);
    if (event->getEventType() == EventTypeEnum::EVENT_USER_INTERFACE_UPDATE) {
        event->setEventProcessed();
        emit requestDialogsUpdate();
    }
}

/**
 * @return All 'active' available macro groups.
 *         Macros groups that are editible.  Other macro
 *         groups are exluded.
 */
std::vector<WuQMacroGroup*>
WbMacroHelper::getActiveMacroGroups()
{
    std::vector<WuQMacroGroup*> macroGroups;
    
    const bool includePreferencesFlag(false);
    if (includePreferencesFlag) {
        CaretPreferences* preferences = SessionManager::get()->getCaretPreferences();
        CaretAssert(preferences);
        macroGroups.push_back(preferences->getMacros());
    }
    
    EventSceneActive activeSceneEvent(EventSceneActive::MODE_GET);
    EventManager::get()->sendEvent(activeSceneEvent.getPointer());
    if ( ! activeSceneEvent.isError()) {
        Scene* activeScene = activeSceneEvent.getScene();
        if (activeScene != NULL) {
            macroGroups.push_back(activeScene->getMacroGroup());
        }
    }
    
    return macroGroups;
}

/**
 * @return All macro groups including those that are
 *         be valid (editable) at this time.
 */
std::vector<const WuQMacroGroup*>
WbMacroHelper::getAllMacroGroups() const
{
    std::vector<const WuQMacroGroup*> macroGroups;
    
    const bool includePreferencesFlag(false);
    if (includePreferencesFlag) {
        CaretPreferences* preferences = SessionManager::get()->getCaretPreferences();
        CaretAssert(preferences);
        macroGroups.push_back(preferences->getMacros());
    }
    
    const auto sceneFiles = EventCaretDataFilesGet::getCaretDataFilesForType(DataFileTypeEnum::SCENE);
    for (const auto dataFile : sceneFiles) {
        const SceneFile* sceneFile = dynamic_cast<const SceneFile*>(dataFile);
        CaretAssert(sceneFile);
        const int32_t numberOfScenes = sceneFile->getNumberOfScenes();
        for (int32_t i = 0; i < numberOfScenes; i++) {
            macroGroups.push_back(sceneFile->getSceneAtIndex(i)->getMacroGroup());
        }
    }
    
    return macroGroups;
}

/**
 * Is called when the given macro is modified
 *
 * @param macro
 *     Macro that is modified
 */
void
WbMacroHelper::macroWasModified(WuQMacro* macro)
{
    /*
     * Need to write to preferences if macro is from preferences
     */
    CaretPreferences* preferences = SessionManager::get()->getCaretPreferences();
    CaretAssert(preferences);
    WuQMacroGroup* prefMacroGroup = preferences->getMacros();
    if (prefMacroGroup->containsMacro(macro)) {
        preferences->writeMacros();
    }
    
    EventManager::get()->sendEvent(EventUserInterfaceUpdate().getPointer());
}

/**
 * Is called when the given macro group is modified
 *
 * @param macroGroup
 *     Macro Group that is modified
 */
void
WbMacroHelper::macroGroupWasModified(WuQMacroGroup* macroGroup)
{
    /**
     * Need to write to preferences if macro group is from preferences
     */
    CaretPreferences* preferences = SessionManager::get()->getCaretPreferences();
    CaretAssert(preferences);
    if (macroGroup == preferences->getMacros()) {
        preferences->writeMacros();
    }
    
    EventManager::get()->sendEvent(EventUserInterfaceUpdate().getPointer());
}

/**
 * @return Identifiers of all available windows in which macros may be run
 */
std::vector<QString>
WbMacroHelper::getMainWindowIdentifiers()
{
    std::vector<QString> identifiers;
    
    std::vector<BrainBrowserWindow*> windows = GuiManager::get()->getAllOpenBrainBrowserWindows();
    for (auto w : windows) {
        identifiers.push_back(QString::number(w->getBrowserWindowIndex() + 1));
    }
    
    return identifiers;
}

/**
 * Get the main window with the given identifier
 *
 * @param identifier
 *     Window identifier
 * @return
 *     Window with the given identifier or NULL if not available
 */
QMainWindow*
WbMacroHelper::getMainWindowWithIdentifier(const QString& identifier)
{
    const int32_t windowIndex(identifier.toInt() - 1);
    QMainWindow* window = GuiManager::get()->getBrowserWindowByWindowIndex(windowIndex);
    
    return window;
}

/**
 * Called just before executing the macro
 *
 * @param macro
 *    Macro that is run
 * @param window
 *     Widget for parent
 * @param executorOptions
 *    Executor options
 */
void
WbMacroHelper::macroExecutionStarting(const WuQMacro* /*macro*/,
                                      QWidget* /*window*/,
                                      const WuQMacroExecutorOptions* executorOptions)
{
    MovieRecorder* movieRecorder = SessionManager::get()->getMovieRecorder();
    m_savedRecordingMode = movieRecorder->getRecordingMode();
    if (executorOptions->isRecordMovieDuringExecution()) {
        movieRecorder->setRecordingMode(MovieRecorderModeEnum::AUTOMATIC);
    }
    EventManager::get()->sendSimpleEvent(EventTypeEnum::EVENT_MOVIE_RECORDING_DIALOG_UPDATE);
}

/**
 * Called just after executing the macro
 *
 * @param macro
 *    Macro that is run
 * @param window
 *     Widget for parent
 * @param executorOptions
 *    Executor options
 */
void
WbMacroHelper::macroExecutionEnding(const WuQMacro* /*macro*/,
                                    QWidget* /*window*/,
                                    const WuQMacroExecutorOptions* /*executorOptions*/)
{
    MovieRecorder* movieRecorder = SessionManager::get()->getMovieRecorder();
    movieRecorder->setRecordingMode(m_savedRecordingMode);
    EventManager::get()->sendSimpleEvent(EventTypeEnum::EVENT_MOVIE_RECORDING_DIALOG_UPDATE);
}
/**
 * Called by macro executor just after a command has completed execution
 *
 * @param window
 *     Widget for parent
 * @param command
 *     Command that has just finished
 * @param allowDelayFlagOut
 *     Output indicating if delay after command is enabled
 */
void
WbMacroHelper::macroCommandHasCompleted(QWidget* window,
                                        const WuQMacroCommand* command,
                                        bool& allowDelayFlagOut)
{
    const bool doDelayAfterCommandFlag(false);
    allowDelayFlagOut = doDelayAfterCommandFlag;
    if (doDelayAfterCommandFlag) {
        recordImagesForDelay(window,
                             command,
                             allowDelayFlagOut);
    }
}

/**
 * If movie recording is on, capture images for the command's delay time
 *
 * @param window
 *     Widget for parent
 * @param command
 *     The command
 * @param delay
 *     The delay in seconds
 */
void
WbMacroHelper::recordImagesForDelay(QWidget* window,
                                    const WuQMacroCommand* command,
                                    bool& allowDelayFlagOut) const
{
    MovieRecorder* movieRecorder = SessionManager::get()->getMovieRecorder();
    CaretAssert(movieRecorder);
    
    if (command->getCommandType() != WuQMacroCommandTypeEnum::MOUSE) {
        if (command->getDelayInSeconds() > 0.0) {
            switch (movieRecorder->getRecordingMode()) {
                case MovieRecorderModeEnum::AUTOMATIC:
                {
                    BrainBrowserWindow* bbw = dynamic_cast<BrainBrowserWindow*>(window);
                    const int32_t numberOfFrames = static_cast<int32_t>(movieRecorder->getFramesRate()
                                                                        * command->getDelayInSeconds());
                    for (int32_t i = 0; i < numberOfFrames; i++) {
                        movieRecorder->setManualRecordingOfImageRequested(true);
                        if (bbw != NULL) {
                            EventManager::get()->sendEvent(EventGraphicsUpdateOneWindow(bbw->getBrowserWindowIndex()).getPointer());
                        }
                        else {
                            EventManager::get()->sendEvent(EventGraphicsUpdateAllWindows().getPointer());
                        }
                        movieRecorder->setManualRecordingOfImageRequested(false);
                    }
                    
                    allowDelayFlagOut = false;
                }
                    break;
                case MovieRecorderModeEnum::MANUAL:
                    break;
            }
        }
    }
}

/**
 * Called by macro executor just before starting execution of a command
 *
 * @param window
 *     Widget for parent
 * @param command
 *     Command that is about to start
 * @param allowDelayFlagOut
 *     Output indicating if delay before command is enabled
 */
void
WbMacroHelper::macroCommandAboutToStart(QWidget* window,
                                        const WuQMacroCommand* command,
                                        bool& allowDelayFlagOut)
{
    const bool doDelayBeforeCommandFlag(true);
    allowDelayFlagOut = doDelayBeforeCommandFlag;
    if (doDelayBeforeCommandFlag) {
        recordImagesForDelay(window,
                             command,
                             allowDelayFlagOut);
    }
}

