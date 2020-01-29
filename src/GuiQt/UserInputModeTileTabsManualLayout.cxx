
/*LICENSE_START*/
/*
 *  Copyright (C) 2019 Washington University School of Medicine
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

#define __USER_INPUT_MODE_TILE_TABS_MANUAL_LAYOUT_DECLARE__
#include "UserInputModeTileTabsManualLayout.h"
#undef __USER_INPUT_MODE_TILE_TABS_MANUAL_LAYOUT_DECLARE__

#include "BrainOpenGLViewportContent.h"
#include "BrowserTabContent.h"
#include "CaretAssert.h"
#include "UserInputModeTileTabsManualLayoutContextMenu.h"
using namespace caret;


    
/**
 * \class caret::UserInputModeTileTabsManualLayout 
 * \brief User Input mode for editing manual tile tabs layout
 * \ingroup GuiQt
 */

/**
 * Constructor.
 *
 * @param windowIndex
 *     Index of window
 */
UserInputModeTileTabsManualLayout::UserInputModeTileTabsManualLayout(const int32_t windowIndex)
: UserInputModeAnnotations(UserInputModeEnum::TILE_TABS_MANUAL_LAYOUT_EDITING,
                           windowIndex)
{
    
}

/**
 * Destructor.
 */
UserInputModeTileTabsManualLayout::~UserInputModeTileTabsManualLayout()
{
}

/**
 * Show a context menu (pop-up menu at mouse location)
 *
 * @param mouseEvent
 *     Mouse event information.
 * @param menuPosition
 *     Point at which menu is displayed (passed to QMenu::exec())
 * @param openGLWidget
 *     OpenGL widget in which context menu is requested
 */
void
UserInputModeTileTabsManualLayout::showContextMenu(const MouseEvent& mouseEvent,
                                          const QPoint& menuPosition,
                                          BrainOpenGLWidget* openGLWidget)
{
    BrainOpenGLViewportContent* viewportContent = mouseEvent.getViewportContent();
    BrowserTabContent* tabContent = viewportContent->getBrowserTabContent();
    
    /*
     * Select any annotation that is under the mouse.
     * There might not be an annotation under the
     * mouse and that is okay.
     */
    const bool singleSelectionModeFlag(false);
    processMouseSelectAnnotation(mouseEvent,
                                 false,
                                 singleSelectionModeFlag);
    
    UserInputModeTileTabsManualLayoutContextMenu contextMenu(this,
                                                    mouseEvent,
                                                    tabContent,
                                                    openGLWidget);
    if (contextMenu.actions().size() > 0) {
        contextMenu.exec(menuPosition);
    }
}
