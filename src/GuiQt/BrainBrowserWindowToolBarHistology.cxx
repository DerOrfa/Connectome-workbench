
/*LICENSE_START*/
/*
 *  Copyright (C) 2014  Washington University School of Medicine
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

#define __BRAIN_BROWSER_WINDOW_TOOL_BAR_HISTOLOGY_DECLARE__
#include "BrainBrowserWindowToolBarHistology.h"
#undef __BRAIN_BROWSER_WINDOW_TOOL_BAR_HISTOLOGY_DECLARE__

#include <QAction>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

#include "Brain.h"
#include "BrowserTabContent.h"
#include "BrainBrowserWindow.h"
#include "BrainBrowserWindowToolBar.h"
#include "BrainBrowserWindowToolBarSliceSelection.h"
#include "BrainOpenGLWidget.h"
#include "CaretAssert.h"
#include "CaretUndoStack.h"
#include "CursorDisplayScoped.h"
#include "DisplayPropertiesCziImages.h"
#include "EnumComboBoxTemplate.h"
#include "EventBrowserWindowGraphicsRedrawn.h"
#include "EventBrowserTabValidate.h"
#include "EventGraphicsUpdateAllWindows.h"
#include "EventManager.h"
#include "GraphicsObjectToWindowTransform.h"
#include "GuiManager.h"
#include "HistologySlice.h"
#include "HistologySlicesFile.h"
#include "HistologyOverlay.h"
#include "HistologyOverlaySet.h"
#include "ModelHistology.h"
#include "WuQDoubleSpinBox.h"
#include "WuQMacroManager.h"
#include "WuQMessageBox.h"
#include "WuQSpinBox.h"
#include "WuQtUtilities.h"

using namespace caret;


    
/**
 * \class caret::BrainBrowserWindowToolBarHistology
 * \brief Histology Component of Brain Browser Window ToolBar
 * \ingroup GuiQt
 */

/**
 * Constructor.
 *
 * @param parentToolBar
 *    parent toolbar.
 */
BrainBrowserWindowToolBarHistology::BrainBrowserWindowToolBarHistology(BrainBrowserWindowToolBar* parentToolBar,
                                                                       const QString& parentObjectName)
: BrainBrowserWindowToolBarComponent(parentToolBar),
m_parentToolBar(parentToolBar)
{
    const AString objectNamePrefix(parentObjectName
                                   + ":BrainBrowserWindowToolBarHistology");
    WuQMacroManager* macroManager(WuQMacroManager::instance());
    
    /*
     * Slice controls
     */
    const int32_t sliceIndexNumberWidth(60);
    
    QLabel* sliceLabel(new QLabel("Slice"));
    QLabel* sliceIndexLabel(new QLabel("Index"));
    m_sliceIndexSpinBox = new WuQSpinBox();
    m_sliceIndexSpinBox->setSingleStep(1);
    m_sliceIndexSpinBox->setFixedWidth(sliceIndexNumberWidth);
    QObject::connect(m_sliceIndexSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                     this, &BrainBrowserWindowToolBarHistology::sliceIndexValueChanged);
    
    QLabel* sliceNumberLabel(new QLabel("Number"));
    m_sliceNumberSpinBox = new WuQSpinBox();
    m_sliceNumberSpinBox->setSingleStep(1);
    m_sliceNumberSpinBox->setFixedWidth(sliceIndexNumberWidth);
    QObject::connect(m_sliceNumberSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                     this, &BrainBrowserWindowToolBarHistology::sliceNumberValueChanged);

    /*
     * Plane and stereotaxic coordinates
     */
    const int numberSpinBoxWidth(80);
    QLabel* planeLabel(new QLabel("Plane"));
    QLabel* stereotaxicLabel(new QLabel("XYZ"));
    for (int32_t i = 0; i < 3; i++) {
        float maxValue(1000000);
        float minValue(-maxValue);
        int decimals(1);
        float step(1.0);
        
        m_planeXyzSpinBox[i] = new WuQDoubleSpinBox(this);
        m_planeXyzSpinBox[i]->setFixedWidth(numberSpinBoxWidth);
        m_planeXyzSpinBox[i]->setDecimalsModeAuto();
        m_planeXyzSpinBox[i]->setSingleStepPercentage(1.0);
        QObject::connect(m_planeXyzSpinBox[i], &WuQDoubleSpinBox::valueChanged,
                         this, &BrainBrowserWindowToolBarHistology::planeXyzSpinBoxValueChanged);

        maxValue = 1000;
        minValue = -maxValue;
        decimals = 4;
        step     = 0.01;
        m_stereotaxicXyzSpinBox[i] = new WuQDoubleSpinBox(this);
        m_stereotaxicXyzSpinBox[i]->setFixedWidth(numberSpinBoxWidth);
        m_stereotaxicXyzSpinBox[i]->setDecimalsModeAuto();
        m_stereotaxicXyzSpinBox[i]->setSingleStepPercentage(1.0);
        QObject::connect(m_stereotaxicXyzSpinBox[i], &WuQDoubleSpinBox::valueChanged,
                         this, &BrainBrowserWindowToolBarHistology::stereotaxicXyzSpinBoxValueChanged);
    }
    
    /*
     * Identification moves slices button
     */
    const AString idToolTipText = ("When selected: If there is an identification operation "
                                   "in ths tab or any other tab with the same yoking status "
                                   "(not Off), the volume slices will move to the location "
                                   "of the identified brainordinate.");
    m_identificationMovesSlicesAction = new QAction(this);
    m_identificationMovesSlicesAction->setCheckable(true);
    m_identificationMovesSlicesAction->setText("");
    WuQtUtilities::setWordWrappedToolTip(m_identificationMovesSlicesAction,
                                         idToolTipText);
    QAction::connect(m_identificationMovesSlicesAction, &QAction::triggered,
                     this, &BrainBrowserWindowToolBarHistology::identificationMovesSlicesActionTriggered);
    QIcon volumeCrossHairIcon;
    const bool volumeCrossHairIconValid =
    WuQtUtilities::loadIcon(":/ToolBar/volume-crosshair-pointer.png",
                            volumeCrossHairIcon);
    QToolButton* identificationMovesSlicesToolButton = new QToolButton;
    if (volumeCrossHairIconValid) {
        m_identificationMovesSlicesAction->setIcon(volumeCrossHairIcon);
        m_identificationMovesSlicesAction->setIcon(BrainBrowserWindowToolBarSliceSelection::createVolumeIdentificationUpdatesSlicesIcon(identificationMovesSlicesToolButton));
    }
    else {
        m_identificationMovesSlicesAction->setText("ID");
    }
    identificationMovesSlicesToolButton->setDefaultAction(m_identificationMovesSlicesAction);
    WuQtUtilities::setToolButtonStyleForQt5Mac(identificationMovesSlicesToolButton);
    m_identificationMovesSlicesAction->setObjectName(objectNamePrefix
                                                             + "MoveSliceToID");
    macroManager->addMacroSupportToObject(m_identificationMovesSlicesAction,
                                          "Enable move volume slice to ID location");
    
    /*
     * Move to center action button
     */
    m_moveToCenterAction = new QAction(this);
    m_moveToCenterAction->setText("Center");
    m_moveToCenterAction->setToolTip("Move to center of slices");
    QObject::connect(m_moveToCenterAction, &QAction::triggered,
                     this, &BrainBrowserWindowToolBarHistology::moveToCenterActionTriggered);
    m_moveToCenterAction->setObjectName(objectNamePrefix
                                                     + "MoveSliceToCenter");
    macroManager->addMacroSupportToObject(m_moveToCenterAction,
                                          "Moves to center of slices");
    QToolButton* moveToCenterToolButton = new QToolButton();
    moveToCenterToolButton->setDefaultAction(m_moveToCenterAction);
    WuQtUtilities::setToolButtonStyleForQt5Mac(moveToCenterToolButton);
    
    /*
     * Drawing mode
     */
//    QLabel* modeLabel(new QLabel("Coord"));
    m_histologyDisplayCoordinateModeEnumComboBox = new EnumComboBoxTemplate(this);
    m_histologyDisplayCoordinateModeEnumComboBox->setup<MediaDisplayCoordinateModeEnum,MediaDisplayCoordinateModeEnum::Enum>();
    QObject::connect(m_histologyDisplayCoordinateModeEnumComboBox, &EnumComboBoxTemplate::itemActivated,
                     this, &BrainBrowserWindowToolBarHistology::histologyDisplayCoordinateModeEnumComboBoxItemActivated);
    m_histologyDisplayCoordinateModeEnumComboBox->getWidget()->setObjectName(parentObjectName
                                                                         + ":histologyDisplayModeComboBox");
    m_histologyDisplayCoordinateModeEnumComboBox->getWidget()->setToolTip("Coordinate Display Mode");
    WuQMacroManager::instance()->addMacroSupportToObject(m_histologyDisplayCoordinateModeEnumComboBox->getWidget(),
                                                         "Set media coordinate mode for display");
    

    /*
     * Layout widgets
     */
    int columnIndex(0);
    const int columnSliceLabels(columnIndex++);
    const int columnSliceSpinBoxes(columnIndex++);
    const int columnPlaneSpinBoxes(columnIndex++);
    const int columnStereotaxicSpinBoxes(columnIndex++);
    
    QGridLayout* controlsLayout(new QGridLayout());
    int row(0);
    controlsLayout->addWidget(sliceLabel,
                              row, columnSliceLabels, 1, 2, Qt::AlignHCenter);
    controlsLayout->addWidget(planeLabel,
                              row, columnPlaneSpinBoxes);
    controlsLayout->addWidget(stereotaxicLabel,
                              row, columnStereotaxicSpinBoxes);
    ++row;
    controlsLayout->addWidget(sliceIndexLabel,
                              row, columnSliceLabels);
    controlsLayout->addWidget(m_sliceIndexSpinBox,
                              row, columnSliceSpinBoxes);
    controlsLayout->addWidget(m_planeXyzSpinBox[0]->getWidget(),
                              row, columnPlaneSpinBoxes);
    controlsLayout->addWidget(m_stereotaxicXyzSpinBox[0]->getWidget(),
                              row, columnStereotaxicSpinBoxes);
    ++row;
    controlsLayout->addWidget(sliceNumberLabel,
                              row, columnSliceLabels);
    controlsLayout->addWidget(m_sliceNumberSpinBox,
                              row, columnSliceSpinBoxes);
    controlsLayout->addWidget(m_planeXyzSpinBox[1]->getWidget(),
                              row, columnPlaneSpinBoxes);
    controlsLayout->addWidget(m_stereotaxicXyzSpinBox[1]->getWidget(),
                              row, columnStereotaxicSpinBoxes);
    ++row;
    controlsLayout->addWidget(m_planeXyzSpinBox[2]->getWidget(),
                              row, columnPlaneSpinBoxes);
    controlsLayout->addWidget(m_stereotaxicXyzSpinBox[2]->getWidget(),
                              row, columnStereotaxicSpinBoxes);
    ++row;
    controlsLayout->addWidget(identificationMovesSlicesToolButton,
                              row, columnSliceLabels, Qt::AlignLeft);
    controlsLayout->addWidget(m_histologyDisplayCoordinateModeEnumComboBox->getWidget(),
                              row, columnSliceSpinBoxes, 1, 2, Qt::AlignHCenter);
    controlsLayout->addWidget(moveToCenterToolButton,
                              row, columnStereotaxicSpinBoxes, Qt::AlignHCenter);
    ++row;

    QVBoxLayout* layout = new QVBoxLayout(this);
    WuQtUtilities::setLayoutSpacingAndMargins(layout, 4, 5);
    layout->addLayout(controlsLayout);
    
    EventManager::get()->addEventListener(this,
                                          EventTypeEnum::EVENT_BROWSER_WINDOW_GRAPHICS_HAVE_BEEN_REDRAWN);

}

/**
 * Destructor.
 */
BrainBrowserWindowToolBarHistology::~BrainBrowserWindowToolBarHistology()
{
}


/**
 * @return The selected histology file (NULL if none selected)
 */
HistologySlicesFile*
BrainBrowserWindowToolBarHistology::getHistologySlicesFile(BrowserTabContent* browserTabContent)
{
    HistologySlicesFile* histologySlicesFile(NULL);
    
    if (browserTabContent != NULL) {
        ModelHistology* histologyModel = browserTabContent->getDisplayedHistologyModel();
        if (histologyModel != NULL) {
            HistologyOverlaySet* histologyOverlaySet = browserTabContent->getHistologyOverlaySet();
            HistologyOverlay* underlay = histologyOverlaySet->getBottomMostEnabledOverlay();
            if (underlay != NULL) {
                const HistologyOverlay::SelectionData selectionData(underlay->getSelectionData());

                if (selectionData.m_selectedFile != NULL) {
                    histologySlicesFile = selectionData.m_selectedFile->castToHistologySlicesFile();
                }
            }
        }
    }
    
    return histologySlicesFile;
}

/**
 * Update content of this tool bar component.
 *
 * @param browserTabContent
 *     Content of the browser tab.
 */
void
BrainBrowserWindowToolBarHistology::updateContent(BrowserTabContent* browserTabContent)
{
    m_browserTabContent = browserTabContent;

    HistologySlicesFile* histologySlicesFile = getHistologySlicesFile(browserTabContent);
    if (histologySlicesFile != NULL) {
        const HistologyCoordinate histologyCoordinate(m_browserTabContent->getHistologySelectedCoordinate(histologySlicesFile));
        QSignalBlocker indexBlocker(m_sliceIndexSpinBox);
        m_sliceIndexSpinBox->setRange(0, histologySlicesFile->getNumberOfHistologySlices() - 1);
        m_sliceIndexSpinBox->setValue(histologyCoordinate.getSliceIndex());

        QSignalBlocker numberBlocker(m_sliceNumberSpinBox);
        m_sliceNumberSpinBox->setRange(0, 100000);
        m_sliceNumberSpinBox->setValue(histologyCoordinate.getSliceNumber());
        
        const BoundingBox planeBB(histologySlicesFile->getPlaneXyzBoundingBox());
        const Vector3D planeXYZ(histologyCoordinate.getPlaneXYZ());
        for (int32_t i = 0; i < 3; i++) {
            if (histologyCoordinate.isPlaneXYValid()) {
                double minValue(0.0);
                double maxValue(0.0);
                switch (i) {
                    case 0:
                        minValue = planeBB.getMinX();
                        maxValue = planeBB.getMaxX();
                        break;
                    case 1:
                        minValue = planeBB.getMinY();
                        maxValue = planeBB.getMaxY();
                        break;
                    case 2:
                        minValue = planeBB.getMinZ();
                        maxValue = planeBB.getMaxZ();
                        break;
                }
                QSignalBlocker blocker(m_planeXyzSpinBox[i]);
                m_planeXyzSpinBox[i]->setRangeExceedable(minValue, maxValue);
                m_planeXyzSpinBox[i]->setValue(planeXYZ[i]);
                m_planeXyzSpinBox[i]->getWidget()->setEnabled(true);
            }
            else {
                m_planeXyzSpinBox[i]->getWidget()->setEnabled(true);
            }
        }
        
        const BoundingBox stereotaxicBB(histologySlicesFile->getStereotaxicXyzBoundingBox());
        const Vector3D stereotaxicXYZ(histologyCoordinate.getStereotaxicXYZ());
        for (int32_t i = 0; i < 3; i++) {
            if (histologyCoordinate.isStereotaxicXYZValid()) {
                double minValue(0.0);
                double maxValue(0.0);
                switch (i) {
                    case 0:
                        minValue = stereotaxicBB.getMinX();
                        maxValue = stereotaxicBB.getMaxX();
                        break;
                    case 1:
                        minValue = stereotaxicBB.getMinY();
                        maxValue = stereotaxicBB.getMaxY();
                        break;
                    case 2:
                        minValue = stereotaxicBB.getMinZ();
                        maxValue = stereotaxicBB.getMaxZ();
                        break;
                }
                QSignalBlocker blocker(m_stereotaxicXyzSpinBox[i]);
                m_stereotaxicXyzSpinBox[i]->setRangeExceedable(minValue, maxValue);
                m_stereotaxicXyzSpinBox[i]->setValue(stereotaxicXYZ[i]);
                m_stereotaxicXyzSpinBox[i]->getWidget()->setEnabled(true);
            }
            else {
                m_stereotaxicXyzSpinBox[i]->getWidget()->setEnabled(true);
            }
        }
        
        const MediaDisplayCoordinateModeEnum::Enum histologyDisplayMode(browserTabContent->getHistologyDisplayCoordinateMode());
        m_histologyDisplayCoordinateModeEnumComboBox->setSelectedItem<MediaDisplayCoordinateModeEnum,MediaDisplayCoordinateModeEnum::Enum>(histologyDisplayMode);
    }
    
    if (m_browserTabContent != NULL) {
        m_identificationMovesSlicesAction->setChecked(m_browserTabContent->isIdentificationUpdateHistologySlices());
    }
    
    setEnabled(histologySlicesFile != NULL);
}

/**
 * @return Viewport content for the selected tab (NULL if not available)
 */
const BrainOpenGLViewportContent*
BrainBrowserWindowToolBarHistology::getBrainOpenGLViewportContent()
{
    std::vector<const BrainOpenGLViewportContent*> viewportContent;
    getParentToolBar()->m_parentBrainBrowserWindow->getAllBrainOpenGLViewportContent(viewportContent);
    for (auto v : viewportContent) {
        if (v->getBrowserTabContent() == m_browserTabContent) {
            return v;
        }
    }
    
    return NULL;
}

/**
 * Receive events from the event manager.
 *
 * @param event
 *   Event sent by event manager.
 */
void
BrainBrowserWindowToolBarHistology::receiveEvent(Event* event)
{
    BrainBrowserWindowToolBarComponent::receiveEvent(event);
    
    if (event->getEventType() == EventTypeEnum::EVENT_BROWSER_WINDOW_GRAPHICS_HAVE_BEEN_REDRAWN) {
        EventBrowserWindowGraphicsRedrawn* redrawnEvent =
        dynamic_cast<EventBrowserWindowGraphicsRedrawn*>(event);
        CaretAssert(redrawnEvent);
        redrawnEvent->setEventProcessed();
        
        EventBrowserTabValidate tabEvent(m_browserTabContent);
        EventManager::get()->sendEvent(tabEvent.getPointer());
        
        if ( ! tabEvent.isValid()) {
            m_browserTabContent = NULL;
        }

        updateContent(m_browserTabContent);
    }
}

/**
 * Called when slice index is changed
 * @param sliceIndex
 *    New slice index
 */
void
BrainBrowserWindowToolBarHistology::sliceIndexValueChanged(int sliceIndex)
{
    if (m_browserTabContent != NULL) {
        HistologySlicesFile* histologySlicesFile = getHistologySlicesFile(m_browserTabContent);
        if (histologySlicesFile != NULL) {
            CursorDisplayScoped cursor;
            cursor.showWaitCursor();
            
            HistologyCoordinate previousHistCoord(m_browserTabContent->getHistologySelectedCoordinate(histologySlicesFile));
            HistologyCoordinate hc(HistologyCoordinate::newInstanceSliceIndexChanged(histologySlicesFile,
                                                                                     previousHistCoord,
                                                                                     sliceIndex));
            
            Vector3D newPlaneCoordXYZ;
            bool newPlaneCoordXYZValid(false);
            
            const int32_t diffSlices(std::fabs(hc.getSliceIndex() - previousHistCoord.getSliceIndex()));
            if (diffSlices == 1) {
                const HistologyCoordinate currentHC(m_browserTabContent->getHistologySelectedCoordinate(histologySlicesFile));
                if (currentHC.isPlaneXYValid()) {
                    const Vector3D oldPlaneXYZ(currentHC.getPlaneXYZ());
                    const HistologySlice* oldSlice(histologySlicesFile->getHistologySliceByIndex(previousHistCoord.getSliceIndex()));
                    if (oldSlice != NULL) {
                        Vector3D centerSteretotaxicXYZ;
                        if (oldSlice->planeXyzToStereotaxicXyz(oldPlaneXYZ,
                                                               centerSteretotaxicXYZ)) {
                            const HistologySlice* newSlice(histologySlicesFile->getHistologySliceByIndex(hc.getSliceIndex()));
                            if (newSlice != NULL) {
                                Vector3D newStereotaxicXYZ;
                                if (newSlice->projectStereotaxicXyzToSlice(centerSteretotaxicXYZ,
                                                                           newStereotaxicXYZ)) {
                                    Vector3D newPlaneXYZ;
                                    if (newSlice->stereotaxicXyzToPlaneXyz(newStereotaxicXYZ,
                                                                           newPlaneXYZ)) {
                                        /*
                                         * Adjacent slices may not be aligned in plane coordinates
                                         */
                                        const Vector3D diffPlaneXYZ(newPlaneXYZ - oldPlaneXYZ);
                                        std::cout << "Switch slices: " << std::endl;
                                        std::cout << "   Old Stereotaxic: " << AString::fromNumbers(centerSteretotaxicXYZ) << std::endl;
                                        std::cout << "   Old Plane: " << AString::fromNumbers(oldPlaneXYZ) << std::endl;
                                        std::cout << "   New Stereotaxic: " << AString::fromNumbers(newStereotaxicXYZ) << std::endl;
                                        std::cout << "   New Plane: " << AString::fromNumbers(newPlaneXYZ) << std::endl;
                                        std::cout << "   Diff Plane: " << AString::fromNumbers(diffPlaneXYZ) << std::endl;
                                        
                                        
                                        hc = HistologyCoordinate::newInstancePlaneXYZChanged(histologySlicesFile,
                                                                                             sliceIndex,
                                                                                             newPlaneXYZ);
//                                        hc.setPlaneXYZ(newPlaneXYZ);
                                    }
                                }
                            }
                        }
                    }
                }
                
//                const BrainOpenGLViewportContent* vpContent(getBrainOpenGLViewportContent());
//                if (vpContent != NULL) {
//                    const GraphicsObjectToWindowTransform* transform(vpContent->getHistologyGraphicsObjectToWindowTransform());
//                    if (transform != NULL) {
//                        int32_t viewport[4];
//                        vpContent->getModelViewport(viewport);
//                        const Vector3D vpCenter(viewport[0] + (viewport[2] / 2.0),
//                                                viewport[1] + (viewport[3] / 2.0),
//                                                0.0);
//                        Vector3D centerPlaneXYZ;
//                        transform->inverseTransformPoint(vpCenter, centerPlaneXYZ);
//
//                        const HistologySlice* oldSlice(histologySlicesFile->getHistologySliceByIndex(previousHistCoord.getSliceIndex()));
//                        if (oldSlice != NULL) {
//                            Vector3D centerSteretotaxicXYZ;
//                            if (oldSlice->planeXyzToStereotaxicXyz(centerPlaneXYZ, centerSteretotaxicXYZ)) {
//                                const HistologySlice* newSlice(histologySlicesFile->getHistologySliceByIndex(hc.getSliceIndex()));
//                                if (newSlice != NULL) {
//                                    Vector3D newStereotaxicXYZ;
//                                    if (newSlice->projectStereotaxicXyzToSlice(centerSteretotaxicXYZ,
//                                                                               newStereotaxicXYZ)) {
//                                        Vector3D newPlaneXYZ;
//                                        if (newSlice->stereotaxicXyzToPlaneXyz(newStereotaxicXYZ,
//                                                                               newPlaneXYZ)) {
//                                            /*
//                                             * Adjacent slices may not be aligned in plane coordinates
//                                             */
//                                            const Vector3D diffPlaneXYZ(newPlaneXYZ - centerPlaneXYZ);
//                                            std::cout << "Old Stereotaxic: " << AString::fromNumbers(centerSteretotaxicXYZ) << std::endl;
//                                            std::cout << "   Old Plane: " << AString::fromNumbers(centerPlaneXYZ) << std::endl;
//                                            std::cout << "   New Stereotaxic: " << AString::fromNumbers(newStereotaxicXYZ) << std::endl;
//                                            std::cout << "   New Plane: " << AString::fromNumbers(newPlaneXYZ) << std::endl;
//                                            std::cout << "   Diff Plane: " << AString::fromNumbers(diffPlaneXYZ) << std::endl;
//
////                                            Vector3D translation;
////                                            m_browserTabContent->getTranslation(translation);
////                                            translation[0] += diffPlaneXYZ[0];
////                                            translation[1] += diffPlaneXYZ[1];
////                                            m_browserTabContent->setTranslation(translation);
//
//                                            newPlaneCoordXYZ = newPlaneXYZ;
//                                            newPlaneCoordXYZValid = true;
//                                        }
//                                    }
//                                }
//                            }
//                        }
//                    }
//                }
            }
            
            m_browserTabContent->setHistologySelectedCoordinate(hc);
            updateGraphicsWindowAndYokedWindows();
            if (newPlaneCoordXYZValid) {
                HistologyCoordinate hc2(HistologyCoordinate::newInstancePlaneXYZChanged(histologySlicesFile,
                                                                                        sliceIndex,
                                                                                        newPlaneCoordXYZ));
                m_browserTabContent->setHistologySelectedCoordinate(hc2);
            }
            updateUserInterface();
        }
    }
}

/**
 * Called when slice number is changed
 * @param sliceNumber
 *    New slice number
 */
void
BrainBrowserWindowToolBarHistology::sliceNumberValueChanged(int sliceNumber)
{
    if (m_browserTabContent != NULL) {
        HistologySlicesFile* histologySlicesFile = getHistologySlicesFile(m_browserTabContent);
        if (histologySlicesFile != NULL) {
            const int32_t sliceIndex(histologySlicesFile->getSliceIndexFromSliceNumber(sliceNumber));
            if (sliceIndex >= 0) {
                sliceIndexValueChanged(sliceIndex);
            }
        }
    }
}

/**
 * Called when a plane XYZ spin box value is changed
 */
void
BrainBrowserWindowToolBarHistology::planeXyzSpinBoxValueChanged()
{
    if (m_browserTabContent != NULL) {
        HistologySlicesFile* histologySlicesFile = getHistologySlicesFile(m_browserTabContent);
        if (histologySlicesFile != NULL) {
            CursorDisplayScoped cursor;
            cursor.showWaitCursor();

            Vector3D planeXYZ(m_planeXyzSpinBox[0]->value(),
                              m_planeXyzSpinBox[1]->value(),
                              m_planeXyzSpinBox[2]->value());

            const int32_t sliceIndex(m_sliceIndexSpinBox->value());
            HistologyCoordinate hc(HistologyCoordinate::newInstancePlaneXYZChanged(histologySlicesFile,
                                                                                    sliceIndex,
                                                                                    planeXYZ));
            m_browserTabContent->setHistologySelectedCoordinate(hc);
            updateGraphicsWindowAndYokedWindows();
            updateUserInterface();
        }
    }
}

/**
 * Called when a stereotaxic XYZ spin box value is changed
 */
void
BrainBrowserWindowToolBarHistology::stereotaxicXyzSpinBoxValueChanged()
{
    if (m_browserTabContent != NULL) {
        HistologySlicesFile* histologySlicesFile = getHistologySlicesFile(m_browserTabContent);
        if (histologySlicesFile != NULL) {
            CursorDisplayScoped cursor;
            cursor.showWaitCursor();

            Vector3D xyz(m_stereotaxicXyzSpinBox[0]->value(),
                         m_stereotaxicXyzSpinBox[1]->value(),
                         m_stereotaxicXyzSpinBox[2]->value());

            HistologyCoordinate hc(HistologyCoordinate::newInstanceStereotaxicXYZ(histologySlicesFile,
                                                                                  xyz));
            m_browserTabContent->setHistologySelectedCoordinate(hc);
            updateGraphicsWindowAndYokedWindows();
            updateUserInterface();
        }
    }
}

/**
 * Called when identification moves slices action is toggledf
 * @param checked
 *    New 'checked' status
 */
void
BrainBrowserWindowToolBarHistology::identificationMovesSlicesActionTriggered(bool checked)
{
    if (m_browserTabContent != NULL) {
        CursorDisplayScoped cursor;
        cursor.showWaitCursor();

        m_browserTabContent->setIdentificationUpdatesHistologySlices(checked);
        updateUserInterface();
    }
}


/**
 * Called when identification move to center action is toggledf
 */
void
BrainBrowserWindowToolBarHistology::moveToCenterActionTriggered()
{
    if (m_browserTabContent != NULL) {
        if (m_browserTabContent != NULL) {
            CursorDisplayScoped cursor;
            cursor.showWaitCursor();

            HistologySlicesFile* histologySlicesFile = getHistologySlicesFile(m_browserTabContent);
            if (histologySlicesFile != NULL) {
                m_browserTabContent->selectHistologySlicesAtOrigin(histologySlicesFile);
                updateGraphicsWindowAndYokedWindows();
                updateUserInterface();
            }
        }
    }
}

/**
 * Called when media coordinate display mode is changed
 */
void
BrainBrowserWindowToolBarHistology::histologyDisplayCoordinateModeEnumComboBoxItemActivated()
{
    if (m_browserTabContent != NULL) {
        const MediaDisplayCoordinateModeEnum::Enum mode(m_histologyDisplayCoordinateModeEnumComboBox->getSelectedItem<MediaDisplayCoordinateModeEnum,MediaDisplayCoordinateModeEnum::Enum>());
        m_browserTabContent->setHistologyDisplayCoordinateMode(mode);
        EventManager::get()->sendEvent(EventGraphicsUpdateAllWindows().getPointer());
    }
}

