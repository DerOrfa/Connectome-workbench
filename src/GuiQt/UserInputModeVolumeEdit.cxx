
/*LICENSE_START*/
/*
 *  Copyright (C) 2014 Washington University School of Medicine
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

#define __USER_INPUT_MODE_VOLUME_EDIT_DECLARE__
#include "UserInputModeVolumeEdit.h"
#undef __USER_INPUT_MODE_VOLUME_EDIT_DECLARE__

#include "Brain.h"
#include "BrainOpenGLWidget.h"
#include "BrowserTabContent.h"
#include "CaretAssert.h"
#include "EventBrowserWindowContentGet.h"
#include "EventGraphicsUpdateAllWindows.h"
#include "EventManager.h"
#include "EventUserInterfaceUpdate.h"
#include "GiftiLabel.h"
#include "GiftiLabelTable.h"
#include "GuiManager.h"
#include "MouseEvent.h"
#include "Overlay.h"
#include "OverlaySet.h"
#include "SelectionItemVoxelEditing.h"
#include "SelectionManager.h"
#include "UserInputModeVolumeEditWidget.h"
#include "VolumeFile.h"
#include "VolumeFileEditorDelegate.h"
#include "VolumeSliceViewPlaneEnum.h"
#include "WuQMessageBox.h"

using namespace caret;


    
/**
 * \class caret::UserInputModeVolumeEdit 
 * \brief User input processor for editing volume voxels
 * \ingroup GuiQt
 */

/**
 * Constructor.
 *
 * Index of window using this volume editor input handler.
 */
UserInputModeVolumeEdit::UserInputModeVolumeEdit(const int32_t windowIndex)
: UserInputModeView(),
m_windowIndex(windowIndex)
{
    m_inputModeVolumeEditWidget = new UserInputModeVolumeEditWidget(this,
                                                                    windowIndex);
}

/**
 * Destructor.
 */
UserInputModeVolumeEdit::~UserInputModeVolumeEdit()
{
}

/**
 * Called when 'this' user input receiver is set
 * to receive events.
 */
void
UserInputModeVolumeEdit::initialize()
{
    m_inputModeVolumeEditWidget->updateWidget();
}

/**
 * Called when 'this' user input receiver is no
 * longer set to receive events.
 */
void
UserInputModeVolumeEdit::finish()
{
}

/**
 * Called to update the input receiver for various events.
 */
void
UserInputModeVolumeEdit::update()
{
    m_inputModeVolumeEditWidget->updateWidget();
}


/**
 * @return A widget for display at the bottom of the
 * Browser Window Toolbar when this mode is active.
 * If no user-interface controls are needed, return NULL.
 * The toolbar will take ownership of the widget and
 * delete it so derived class MUST NOT delete the widget.
 */
QWidget*
UserInputModeVolumeEdit::getWidgetForToolBar()
{
    return m_inputModeVolumeEditWidget;
}

/**
 * @return The input mode enumerated type.
 */
UserInputModeVolumeEdit::UserInputMode
UserInputModeVolumeEdit::getUserInputMode() const
{
    
    return UserInputReceiverInterface::VOLUME_EDIT;
}

/**
 * Process a mouse left click event.
 *
 * @param mouseEvent
 *     Mouse event information.
 */
void
UserInputModeVolumeEdit::mouseLeftClick(const MouseEvent& mouseEvent)
{
    if (mouseEvent.getViewportContent() == NULL) {
        return;
    }
    
    VolumeEditInfo volumeEditInfo;
    if ( ! getVolumeEditInfo(volumeEditInfo)) {
        return;
    }
    
    BrainOpenGLWidget* openGLWidget = mouseEvent.getOpenGLWidget();
    const int mouseX = mouseEvent.getX();
    const int mouseY = mouseEvent.getY();
    
    SelectionManager* idManager = GuiManager::get()->getBrain()->getSelectionManager();
    SelectionItemVoxelEditing* idEditVoxel = idManager->getVoxelEditingIdentification();
    idEditVoxel->setVolumeFileForEditing(volumeEditInfo.m_volumeFile);
    idManager = openGLWidget->performIdentificationVoxelEditing(volumeEditInfo.m_volumeFile,
                                                                mouseX,
                                                                mouseY);
    if ((volumeEditInfo.m_volumeFile == idEditVoxel->getVolumeFile())
        && idEditVoxel->isValid()) {
        int64_t ijk[3];
        idEditVoxel->getVoxelIJK(ijk);
        
        VolumeEditingModeEnum::Enum editMode = VolumeEditingModeEnum::VOLUME_EDITING_MODE_ON;
        int32_t brushSizes[3] = { 0, 0, 0 };
        float paletteMappedValue = 0;
        AString labelMappedName;
        m_inputModeVolumeEditWidget->getEditingParameters(editMode,
                                                          brushSizes,
                                                          paletteMappedValue,
                                                          labelMappedName);
        const int64_t brushSizesInt64[3] = {
            brushSizes[0],
            brushSizes[1],
            brushSizes[2]
        };
        
        VolumeFileEditorDelegate* editor = volumeEditInfo.m_volumeFileEditorDelegate;
        CaretAssert(editor);
        
        VolumeSliceViewPlaneEnum::Enum slicePlane = volumeEditInfo.m_sliceViewPlane;
        
        bool successFlag = true;
        AString errorMessage;
        
        float voxelValueOn = paletteMappedValue;
        float voxelValueOff = 0.0;
        
        if (volumeEditInfo.m_volumeFile->isMappedWithLabelTable()) {
            const GiftiLabelTable* labelTable = volumeEditInfo.m_volumeFile->getMapLabelTable(volumeEditInfo.m_mapIndex);
            const GiftiLabel* label = labelTable->getLabel(labelMappedName);
            if (label != NULL) {
                voxelValueOn = label->getKey();
            }
            else {
                errorMessage = ("Label name "
                                + labelMappedName
                                + " is not in label table.");
                successFlag = false;
            }
            
            const GiftiLabel* unassignedLabel = labelTable->getLabel(labelTable->getUnassignedLabelKey());
            if (unassignedLabel != NULL) {
                voxelValueOff = unassignedLabel->getKey();
            }
        }
        
        if (successFlag) {
            successFlag = editor->performEditingOperation(volumeEditInfo.m_mapIndex,
                                                          editMode,
                                                          slicePlane,
                                                          ijk,
                                                          brushSizesInt64,
                                                          voxelValueOn,
                                                          voxelValueOff,
                                                          errorMessage);
        }
        
        if ( ! successFlag) {
            WuQMessageBox::errorOk(m_inputModeVolumeEditWidget,
                                   errorMessage);
        }
        
        updateGraphicsAfterEditing(volumeEditInfo.m_volumeFile,
                                   volumeEditInfo.m_mapIndex);
    }
}

/**
 * Update the graphics after editing.
 *
 * @param volumeFile
 *    Volume file that needs coloring update.
 * @param mapIndex
 *    Index of the map.
 */
void
UserInputModeVolumeEdit::updateGraphicsAfterEditing(VolumeFile* volumeFile,
                                                    const int32_t mapIndex)
{
    CaretAssert(volumeFile);
    CaretAssert((mapIndex >= 0) && (mapIndex < volumeFile->getNumberOfMaps()));

    volumeFile->clearVoxelColoringForMap(mapIndex);
    PaletteFile* paletteFile = GuiManager::get()->getBrain()->getPaletteFile();
    volumeFile->updateScalarColoringForMap(mapIndex,
                                           paletteFile);
    EventManager::get()->sendEvent(EventGraphicsUpdateAllWindows().getPointer());
}


/**
 * @return The cursor for display in the OpenGL widget.
 */
CursorEnum::Enum
UserInputModeVolumeEdit::getCursor() const
{
    
    CursorEnum::Enum cursor = CursorEnum::CURSOR_DEFAULT;
    
//    switch (m_mode) {
//        case MODE_CREATE:
//            break;
//        case MODE_EDIT:
//            cursor = CursorEnum::CURSOR_POINTING_HAND;
//            switch (m_editOperation) {
//                case EDIT_OPERATION_DELETE:
//                    cursor = CursorEnum::CURSOR_CROSS;
//                    break;
//                case EDIT_OPERATION_PROPERTIES:
//                    cursor = CursorEnum::CURSOR_WHATS_THIS;
//                    break;
//            }
//            break;
//        case MODE_OPERATIONS:
//            cursor = CursorEnum::CURSOR_POINTING_HAND;
//            break;
//    }
    
    return cursor;
}

/**
 * Get information about volume data being edited.
 *
 * @param volumeEditInfo
 *    Loaded with editing information upon sucess.
 * @return
 *    True if volumeEditInfo is valid, else false.
 */
bool
UserInputModeVolumeEdit::getVolumeEditInfo(VolumeEditInfo& volumeEditInfo)
{
    volumeEditInfo.m_volumeFile     = NULL;
    volumeEditInfo.m_mapIndex       = -1;
    volumeEditInfo.m_sliceViewPlane = VolumeSliceViewPlaneEnum::ALL;
    volumeEditInfo.m_modelVolume    = NULL;
    
    EventBrowserWindowContentGet windowEvent(m_windowIndex);
    EventManager::get()->sendEvent(windowEvent.getPointer());
    
    BrowserTabContent* tabContent = windowEvent.getSelectedBrowserTabContent();
    if (tabContent != NULL) {
        ModelVolume* modelVolume = tabContent->getDisplayedVolumeModel();
        if (modelVolume != NULL) {
            OverlaySet* overlaySet = tabContent->getOverlaySet();
            const int32_t numOverlays = overlaySet->getNumberOfDisplayedOverlays();
            for (int32_t i = 0; i < numOverlays; i++) {
                Overlay* overlay = overlaySet->getOverlay(i);
                CaretMappableDataFile* mapFile = NULL;
                int32_t mapIndex;
                overlay->getSelectionData(mapFile,
                                          mapIndex);
                if (mapFile != NULL) {
                    VolumeFile* vf = dynamic_cast<VolumeFile*>(mapFile);
                    if (vf != NULL) {
                        volumeEditInfo.m_volumeFile     = vf;
                        volumeEditInfo.m_mapIndex       = mapIndex;
                        volumeEditInfo.m_sliceViewPlane = tabContent->getSliceViewPlane();
                        volumeEditInfo.m_modelVolume    = modelVolume;
                        volumeEditInfo.m_volumeFileEditorDelegate = vf->getVolumeFileEditorDelegate();
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}

