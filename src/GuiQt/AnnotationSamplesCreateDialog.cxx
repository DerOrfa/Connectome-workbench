
/*LICENSE_START*/
/*
 *  Copyright (C) 2015 Washington University School of Medicine
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

#define __ANNOTATION_SAMPLES_CREATE_DIALOG_DECLARE__
#include "AnnotationSamplesCreateDialog.h"
#undef __ANNOTATION_SAMPLES_CREATE_DIALOG_DECLARE__

#include <cmath>

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

#include "AnnotationFile.h"
#include "AnnotationManager.h"
#include "AnnotationPolyhedron.h"
#include "AnnotationRedoUndoCommand.h"
#include "Brain.h"
#include "BrainBrowserWindow.h"
#include "CaretAssert.h"
#include "CaretLogger.h"
#include "DisplayPropertiesAnnotation.h"
#include "EventGraphicsUpdateAllWindows.h"
#include "EventManager.h"
#include "EventUserInterfaceUpdate.h"
#include "GiftiMetaDataXmlElements.h"
#include "GuiManager.h"
#include "MetaDataCustomEditorWidget.h"
#include "WuQMessageBox.h"

using namespace caret;


    
/**
 * \class caret::AnnotationSamplesCreateDialog
 * \brief Dialog used for creating new annotations.
 * \ingroup GuiQt
 */

/**
 * Dialog constructor.
 *
 * @param annotationFile
 *     The annotation file.
 * @param annotation
 *     The new annotation.
 * @param viewportHeight
 *      Height of viewport
 * @param volumeSliceThickness
 *     Thickness of volume slice
 * @param browserWindowIndex
 *     Index of browser window
 * @param browserTabIndex
 *     Index of tab
 * @param parent
 *     Optional parent for this dialog.
 */
AnnotationSamplesCreateDialog::AnnotationSamplesCreateDialog(const UserInputModeEnum::Enum userInputMode,
                                                     const int32_t browserWindowIndex,
                                                     const int32_t browserTabIndex,
                                                     AnnotationFile* annotationFile,
                                                     Annotation* annotation,
                                                     const int32_t viewportHeight,
                                                     const float volumeSliceThickness,
                                                     QWidget* parent)
: WuQDialogModal("New Sample",
                 parent),
m_userInputMode(userInputMode),
m_browserWindowIndex(browserWindowIndex),
m_browserTabIndex(browserTabIndex),
m_annotationFile(annotationFile),
m_annotation(annotation),
m_viewportHeight(viewportHeight),
m_volumeSliceThickness(volumeSliceThickness)
{
    CaretAssert(m_annotationFile);
    CaretAssert(m_annotation);
    
    m_annotationType = annotation->getType();
    
    QWidget* metaDataWidget = ((m_annotationType == AnnotationTypeEnum::POLYHEDRON)
                               ? createMetaDataEditorWidget()
                               : NULL);

    QWidget* dialogWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(dialogWidget);
    
    if (metaDataWidget != NULL) {
        layout->addWidget(metaDataWidget, 0);
    }
    
    setSizePolicy(dialogWidget->sizePolicy().horizontalPolicy(),
                  QSizePolicy::Fixed);

    setCentralWidget(dialogWidget,
                     SCROLL_AREA_NEVER);
}

/**
 * Destructor.
 */
AnnotationSamplesCreateDialog::~AnnotationSamplesCreateDialog()
{
}

/**
 * @return Annotation that was created by dialog (NULL if annotation NOT created).
 */
Annotation*
AnnotationSamplesCreateDialog::getAnnotationThatWasCreated()
{
    return m_annotation;
}

/**
 * @return A metadata editor widget for polyhedrons
 */
QWidget*
AnnotationSamplesCreateDialog::createMetaDataEditorWidget()
{
    const bool polyhedronSamplesFlag(true);
    m_requiredMetaDataNames = Annotation::getDefaultMetaDataNamesForType(m_annotationType,
                                                                         polyhedronSamplesFlag);
    m_annotationMetaData.reset(new GiftiMetaData());
    for (const auto& name : m_requiredMetaDataNames) {
        m_annotationMetaData->set(name, "");
    }

    m_annotationMetaData->set(GiftiMetaDataXmlElements::SAMPLES_LOCATION_ID, "");
    m_annotationMetaData->set(GiftiMetaDataXmlElements::METADATA_NAME_COMMENT, "");

    m_metaDataEditorWidget = new MetaDataCustomEditorWidget(m_requiredMetaDataNames,
                                                            m_annotationMetaData.get());

    m_metaDataRequiredCheckBox = new QCheckBox("Require Metadata");
    m_metaDataRequiredCheckBox->setChecked(s_previousMetaDataRequiredCheckedStatus);
        
    QGroupBox* groupBox(new QGroupBox("Metadata"));
    QVBoxLayout* groupLayout = new QVBoxLayout(groupBox);
    groupLayout->addWidget(m_metaDataEditorWidget);
    groupLayout->addWidget(m_metaDataRequiredCheckBox, 0, Qt::AlignLeft);
    
    return groupBox;
}

/**
 * Gets called when the OK button is clicked.
 */
void
AnnotationSamplesCreateDialog::okButtonClicked()
{
    AString errorMessage;
    
    QString userText;
    CaretAssert(m_annotation);

    if (m_metaDataEditorWidget != NULL) {
        CaretAssert(m_metaDataRequiredCheckBox);
        s_previousMetaDataRequiredCheckedStatus = m_metaDataRequiredCheckBox->isChecked();
        if (m_metaDataRequiredCheckBox->isChecked()) {
            AString message;
            if ( ! m_metaDataEditorWidget->validateAndSaveRequiredMetaData(m_requiredMetaDataNames,
                                                                           message)) {
                message.appendWithNewLine("\nUncheck \""
                                          + m_metaDataRequiredCheckBox->text()
                                          + "\" to finish metadata entry later");
                errorMessage.appendWithNewLine(message);
            }
        }
        else {
            m_metaDataEditorWidget->saveMetaData();
        }
    }
    
    if ( ! errorMessage.isEmpty()) {
        WuQMessageBox::errorOk(this,
                               errorMessage);
        return;
    }
    
    CaretAssert(m_annotationFile);
    
    m_annotation->setDrawingNewAnnotationStatus(false);
    
    if (m_metaDataEditorWidget != NULL) {
        GiftiMetaData* annMetaData(m_annotation->getMetaData());
        CaretAssert(annMetaData);
        CaretAssert(m_annotationMetaData);
        annMetaData->replace(*m_annotationMetaData.get());
    }
    
    finishAnnotationCreation(m_userInputMode,
                             m_annotationFile,
                             m_annotation,
                             m_browserWindowIndex,
                             m_browserTabIndex);
    
    DisplayPropertiesAnnotation* dpa = GuiManager::get()->getBrain()->getDisplayPropertiesAnnotation();
    dpa->updateForNewAnnotation(m_annotation);
    EventManager::get()->sendEvent(EventGraphicsUpdateAllWindows().getPointer());
    EventManager::get()->sendEvent(EventUserInterfaceUpdate().getPointer());
    
    WuQDialog::okButtonClicked();
}

/**
 * Finish the creation of an annotation.
 *
 * @param annotationFile
 *     File to which annotation is added.
 * @param annotation
 *     Annotation that was created.
 * @param browserWindowIndex
 *     Index of window in which annotation was created.
 * @param tabIndex
 *     Index of tab in which annotation was created.
 */
void
AnnotationSamplesCreateDialog::finishAnnotationCreation(const UserInputModeEnum::Enum userInputMode,
                                                    AnnotationFile* annotationFile,
                                                    Annotation* annotation,
                                                    const int32_t browswerWindowIndex,
                                                    const int32_t tabIndex)
{
    AnnotationManager* annotationManager = GuiManager::get()->getBrain()->getAnnotationManager(userInputMode);
    
    CaretAssert(annotation);
    annotation->setDrawingNewAnnotationStatus(false);
    
    /*
     * Add annotation to its file
     */
    AnnotationRedoUndoCommand* undoCommand = new AnnotationRedoUndoCommand();
    undoCommand->setModeCreateAnnotation(annotationFile,
                                         annotation);
    
    AString errorMessage;
    if ( ! annotationManager->applyCommand(undoCommand,
                                           errorMessage)) {
        WuQMessageBox::errorOk(GuiManager::get()->getBrowserWindowByWindowIndex(browswerWindowIndex),
                               errorMessage);
    }

    
    annotationManager->selectAnnotationForEditing(browswerWindowIndex,
                                        AnnotationManager::SELECTION_MODE_SINGLE,
                                        false,
                                        annotation);
    
    /*
     * A new chart annotation is displayed only in the tab in which it was created
     */
    if (annotation->getCoordinateSpace() == AnnotationCoordinateSpaceEnum::CHART) {
        annotation->setItemDisplaySelectedInOneTab(tabIndex);
        annotation->setItemDisplaySelected(DisplayGroupEnum::DISPLAY_GROUP_TAB,
                                           tabIndex,
                                           TriStateSelectionStatusEnum::SELECTED);
    }
}

