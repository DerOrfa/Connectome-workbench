
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

#define __META_DATA_CUSTOM_EDITOR_WIDGET_DECLARE__
#include "MetaDataCustomEditorWidget.h"
#undef __META_DATA_CUSTOM_EDITOR_WIDGET_DECLARE__

#include <set>

#include <QAction>
#include <QComboBox>
#include <QDateEdit>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSignalMapper>
#include <QStackedWidget>
#include <QTextDocument>
#include <QTextEdit>
#include <QToolButton>
#include <QVBoxLayout>

#include "Brain.h"
#include "CaretAssert.h"
#include "CaretLogger.h"
#include "DingOntologyTermsDialog.h"
#include "GiftiMetaData.h"
#include "GiftiMetaDataElementValues.h"
#include "GiftiMetaDataXmlElements.h"
#include "GuiManager.h"
#include "SamplesMetaDataManager.h"
#include "StructureEnum.h"
#include "WuQDataEntryDialog.h"
#include "WuQWidgetObjectGroup.h"

using namespace caret;


    
/**
 * \class caret::MetaDataCustomEditorWidget
 * \brief Widget for editing GIFTI MetaData.
 * \ingroup GuiQt
 */

/**
 * Constructor.
 * @param metaDataNames
 *    Names of metadata shown in editor
 * @param commentEditorStatus
 *    Whether or not to show comment editor
 * @param metaData
 *    Metadata instance
 * @param parent
 *    Parent widget.
 */
MetaDataCustomEditorWidget::MetaDataCustomEditorWidget(const std::vector<AString>& metaDataNames,
                                                       GiftiMetaData* metaData,
                                                       QWidget* parent)
: QWidget(parent),
m_metaData(metaData)
{
    bool hasCommentMetaDataFlag(false);
    const int32_t COLUMN_LABEL(0);
    const int32_t COLUMN_VALUE(1);
    const int32_t COLUMN_BUTTON(2);
    int32_t rowIndex(0);
    QGridLayout* gridLayout(new QGridLayout(this));
    gridLayout->setColumnStretch(COLUMN_LABEL, 0);
    gridLayout->setColumnStretch(COLUMN_VALUE, 100);
    for (const auto& name : metaDataNames) {
        if (name == GiftiMetaDataXmlElements::METADATA_NAME_COMMENT) {
            /* Comment uses a text editor, below */
            hasCommentMetaDataFlag = true;
        }
        else {
            MetaDataWidgetRow* mdwr(new MetaDataWidgetRow(this,
                                                          gridLayout,
                                                          rowIndex,
                                                          COLUMN_LABEL,
                                                          COLUMN_VALUE,
                                                          COLUMN_BUTTON,
                                                          name,
                                                          m_metaData));
            mdwr->updateValueWidget();
            m_metaDataWidgetRows.push_back(mdwr);
            ++rowIndex;
        }
    }
    
    m_commentTextEditor = NULL;
    if (hasCommentMetaDataFlag) {
        QLabel* commentLabel(new QLabel(GiftiMetaDataXmlElements::METADATA_NAME_COMMENT + ":"));
        m_commentTextEditor = new QTextEdit();
        m_commentTextEditor->setText(m_metaData->get(GiftiMetaDataXmlElements::METADATA_NAME_COMMENT));
        gridLayout->addWidget(commentLabel,
                              rowIndex, COLUMN_LABEL);
        gridLayout->addWidget(m_commentTextEditor,
                              rowIndex, COLUMN_VALUE, 1, 2);
        ++rowIndex;
    }
}

/**
 * Destructor.
 */
MetaDataCustomEditorWidget::~MetaDataCustomEditorWidget()
{
}

/**
 * Transfer values in dialog into metadata.
 */
void
MetaDataCustomEditorWidget::saveMetaData()
{
    for (auto& mdwr : m_metaDataWidgetRows) {
        mdwr->saveToMetaData();
    }

    if (m_commentTextEditor != NULL) {
        m_metaData->set(GiftiMetaDataXmlElements::METADATA_NAME_COMMENT,
                        m_commentTextEditor->toPlainText());
    }
}

/**
 * Called when the user clicks a meta data button
 * @param metaDataName
 *    Name of metadata element
 * @param parentDialogWidget
 *    Parent for any dialogs
 */
void
MetaDataCustomEditorWidget::metaDataButtonClicked(const AString& metaDataName,
                                                  QWidget* parentDialogWidget)
{
    const QString metaDataValue(m_metaData->get(metaDataName));
    
    bool dingFlag(false);
    bool listPopupFlag(false);
    switch (GiftiMetaDataElementValues::getDataTypeForElement(metaDataName)) {
        case GiftiMetaDataElementDataTypeEnum::COMMENT:
            break;
        case GiftiMetaDataElementDataTypeEnum::DATE:
            break;
        case GiftiMetaDataElementDataTypeEnum::DING_ONTOLOGY_TERM:
            dingFlag = true;
            break;
        case GiftiMetaDataElementDataTypeEnum::LIST:
            listPopupFlag = true;
            break;
        case GiftiMetaDataElementDataTypeEnum::TEXT:
            break;
    }
    
    if (dingFlag) {
        const SamplesMetaDataManager* smdm(GuiManager::get()->getBrain()->getSamplesMetaDataManager());
        CaretAssert(smdm);
        DingOntologyTermsDialog dotd(smdm->getDingOntologyTermsFile(),
                                     this);
        if (dotd.exec() == DingOntologyTermsDialog::Accepted) {
            const QString shorthandID(dotd.getAbbreviatedName());
            const QString description(dotd.getDescriptiveName());
            
            if (metaDataName == GiftiMetaDataXmlElements::SAMPLES_ALT_SHORTHAND_ID) {
                m_metaData->set(GiftiMetaDataXmlElements::SAMPLES_ALT_SHORTHAND_ID,
                                shorthandID);
                m_metaData->set(GiftiMetaDataXmlElements::SAMPLES_ALT_ATLAS_DESCRIPTION,
                                description);
                
                updateValueInMetaDataWidgetRow(GiftiMetaDataXmlElements::SAMPLES_ALT_SHORTHAND_ID);
                updateValueInMetaDataWidgetRow(GiftiMetaDataXmlElements::SAMPLES_ALT_ATLAS_DESCRIPTION);
            }
            if (metaDataName == GiftiMetaDataXmlElements::SAMPLES_SHORTHAND_ID) {
                m_metaData->set(GiftiMetaDataXmlElements::SAMPLES_SHORTHAND_ID,
                                shorthandID);
                m_metaData->set(GiftiMetaDataXmlElements::SAMPLES_DING_DESCRIPTION,
                                description);
                
                updateValueInMetaDataWidgetRow(GiftiMetaDataXmlElements::SAMPLES_SHORTHAND_ID);
                updateValueInMetaDataWidgetRow(GiftiMetaDataXmlElements::SAMPLES_DING_DESCRIPTION);
            }
        }
    }
    else if (listPopupFlag) {
        QStringList dataSelectionValues;
        
        dataSelectionValues = GiftiMetaDataElementValues::getValuesForElement(metaDataName);
        if (dataSelectionValues.empty()) {
            CaretLogSevere("Metadata not supported for selection: "
                           + metaDataName);
            CaretAssert(0);
        }
        
        if (dataSelectionValues.size() > 10) {
            WuQDataEntryDialog ded("Choose " + metaDataName,
                                   parentDialogWidget);
            QComboBox* comboBox(ded.addComboBox("Choose", dataSelectionValues));
            
            /*
             * Initialize combo box with metadata value selected
             */
            if ( ! metaDataValue.isEmpty()) {
                const int32_t index(comboBox->findText(metaDataValue,
                                                       Qt::MatchFixedString));
                if (index >= 0) {
                    comboBox->setCurrentIndex(index);
                }
            }
            
            if (ded.exec() == WuQDataEntryDialog::Accepted) {
                const QString value(comboBox->currentText().trimmed());
                m_metaData->set(metaDataName,
                                value);
                updateValueInMetaDataWidgetRow(metaDataName);
                
                /*
                 * If sample type is "Tile" and sample ID is emtpy,
                 * set sample ID to T1
                 */
                if (metaDataName == GiftiMetaDataXmlElements::SAMPLES_SAMPLE_TYPE) {
                    if (value == "Tile") {
                        const QString sampleIdText(m_metaData->get(GiftiMetaDataXmlElements::SAMPLES_SAMPLE_ID));
                        if (sampleIdText.isEmpty()) {
                            m_metaData->set(GiftiMetaDataXmlElements::SAMPLES_SAMPLE_ID,
                                            "T1");
                            updateValueInMetaDataWidgetRow(GiftiMetaDataXmlElements::SAMPLES_SAMPLE_ID);
                        }
                    }
                }
            }
        }
        else if (dataSelectionValues.size() > 0) {
            WuQDataEntryDialog ded("Choose " + metaDataName,
                                   parentDialogWidget);
            for (const QString& text : dataSelectionValues) {
                const bool defaultButtonFlag(text == metaDataValue);
                ded.addRadioButton(text,
                                   defaultButtonFlag);
            }
            if (ded.exec() == WuQDataEntryDialog::Accepted) {
                const int32_t buttonIndex(ded.getRadioButtonSelected());
                if (buttonIndex >= 0) {
                    CaretAssert(buttonIndex < dataSelectionValues.size());
                    const QString value(dataSelectionValues[buttonIndex]);
                    m_metaData->set(metaDataName,
                                    value);
                    updateValueInMetaDataWidgetRow(metaDataName);
                    
                    /*
                     * If sample type is "Tile" and sample ID is emtpy,
                     * set sample ID to T1
                     */
                    if (metaDataName == GiftiMetaDataXmlElements::SAMPLES_SAMPLE_TYPE) {
                        if (value == "Tile") {
                            const QString sampleIdText(m_metaData->get(GiftiMetaDataXmlElements::SAMPLES_SAMPLE_ID));
                            if (sampleIdText.isEmpty()) {
                                m_metaData->set(GiftiMetaDataXmlElements::SAMPLES_SAMPLE_ID,
                                                "T1");
                                updateValueInMetaDataWidgetRow(GiftiMetaDataXmlElements::SAMPLES_SAMPLE_ID);
                            }
                        }
                    }
                }
            }
        }
    }
}

/**
 * Update the value in the meta data widget row with the given name
 * @param metaDataName
 *    The metadata name
 */
void
MetaDataCustomEditorWidget::updateValueInMetaDataWidgetRow(const QString& metaDataName)
{
    MetaDataWidgetRow* mdwr(getMetaDataWidgetRow(metaDataName));
    if (mdwr != NULL) {
        mdwr->updateValueWidget();
    }
    else {
        CaretLogSevere("Failed to find metadata widget row with name: "
                       + metaDataName);
    }
}

/**
 * @return Metadata widget row with the given name
 * @param metaDataName
 *
 */
MetaDataCustomEditorWidget::MetaDataWidgetRow*
MetaDataCustomEditorWidget::getMetaDataWidgetRow(const QString& metaDataName)
{
    for (MetaDataWidgetRow* mdwr : m_metaDataWidgetRows) {
        if (mdwr->m_metaDataName == metaDataName) {
            return mdwr;
        }
    }
    return NULL;
}

/**
 * Validate the the required metadata (values must not be empty)
 * @param requiredMetaDataNames
 *    Names of required metadata
 * @param errorMessageOut
 *    Contains error message if validation fails
 * @return
 *    True if metadata is valid, else false.
 */
bool
MetaDataCustomEditorWidget::validateAndSaveRequiredMetaData(const std::vector<AString>& requiredMetaDataNames,
                                                            AString& errorMessageOut)
{
    CaretAssert(m_metaData);
    saveMetaData();
    return m_metaData->validateRequiredMetaData(requiredMetaDataNames,
                                                errorMessageOut);
}

/**
 * Constructor.
 * @param editorWidget
 *    Editor widget that contains this metadata row
 * @param gridLayout
 *    The grid layout for widgets
 * @param gridLayoutRow
 *    Row in the grid layout
 * @param gridLayoutNameColumn
 *    Column for the name
 * @param gridLayoutValueColumn
 *    Column for the value
 * @param gridLayoutButtonColumn
 *    Column for button
 * @param metaDataName
 *   The  name of the metadata
 * @param metaData
 *   The metadata.
 */
MetaDataCustomEditorWidget::MetaDataWidgetRow::MetaDataWidgetRow(MetaDataCustomEditorWidget* editorWidget,
                                                                 QGridLayout* gridLayout,
                                                                 const int32_t gridLayoutRow,
                                                                 const int32_t gridLayoutNameColumn,
                                                                 const int32_t gridLayoutValueColumn,
                                                                 const int32_t gridLayoutButtonColumn,
                                                                 const AString& metaDataName,
                                                                 GiftiMetaData* metaData)
: m_editorWidget(editorWidget),
m_metaDataName(metaDataName),
m_metaData(metaData)
{
    CaretAssert(m_metaData);
    
    const QString value(metaData->get(metaDataName));
    
    QLabel* nameLabel(new QLabel(metaDataName + ":"));
    
    m_valueComboBox = NULL;
    m_valueDateEdit = NULL;
    m_valueLineEdit = NULL;
    
    m_toolButton = NULL;
    
    const bool useComboBoxForListsFlag(true);
    bool useComboBoxFlag(false);
    bool useDateEditFlag(false);
    bool useLineEditFlag(false);
    bool useToolButtonFlag(false);
    switch (GiftiMetaDataElementValues::getDataTypeForElement(metaDataName)) {
        case GiftiMetaDataElementDataTypeEnum::COMMENT:
            CaretAssert(0); /* Should never get here */
            break;
        case GiftiMetaDataElementDataTypeEnum::DATE:
            useDateEditFlag = true;
            break;
        case GiftiMetaDataElementDataTypeEnum::DING_ONTOLOGY_TERM:
            useLineEditFlag   = true;
            useToolButtonFlag = true;
            break;
        case GiftiMetaDataElementDataTypeEnum::LIST:
            if (useComboBoxForListsFlag) {
                useComboBoxFlag = true;
            }
            else {
                useLineEditFlag   = true;
                useToolButtonFlag = true;
            }
            break;
        case GiftiMetaDataElementDataTypeEnum::TEXT:
            useLineEditFlag   = true;
            break;
    }
    
    if (useComboBoxFlag) {
        const QStringList comboBoxValuesList(GiftiMetaDataElementValues::getValuesForElement(metaDataName));
        m_valueComboBox = new QComboBox();
        m_valueComboBox->addItem(""); /* empty item for 'no value' */
        m_valueComboBox->addItems(comboBoxValuesList);
    }
    if (useDateEditFlag) {
        m_valueDateEdit = new QDateEdit();
        m_valueDateEdit->setDisplayFormat(GiftiMetaDataXmlElements::METADATA_QT_DATE_FORMAT);
        m_valueDateEdit->setCalendarPopup(true);
    }
    if (useLineEditFlag) {
        m_valueLineEdit = new QLineEdit();
    }
    if (useToolButtonFlag) {
        m_toolButton = new QToolButton();
        m_toolButton->setText("Choose...");
        QObject::connect(m_toolButton, &QToolButton::clicked,
                         [=]() { toolButtonClicked(); });
    }
    
    gridLayout->addWidget(nameLabel,
                          gridLayoutRow, gridLayoutNameColumn);
    if (m_valueComboBox != NULL) {
        gridLayout->addWidget(m_valueComboBox,
                              gridLayoutRow, gridLayoutValueColumn);
    }
    if (m_valueDateEdit != NULL) {
        gridLayout->addWidget(m_valueDateEdit,
                              gridLayoutRow, gridLayoutValueColumn);
    }
    if (m_valueLineEdit != NULL) {
        gridLayout->addWidget(m_valueLineEdit,
                              gridLayoutRow, gridLayoutValueColumn);
    }
    if (m_toolButton != NULL) {
        gridLayout->addWidget(m_toolButton,
                              gridLayoutRow, gridLayoutButtonColumn);
    }
}

/**
 * Destructor
 */
MetaDataCustomEditorWidget::MetaDataWidgetRow::~MetaDataWidgetRow()
{
}

/**
 * Update the value widget with the value from the metadata
 */
void
MetaDataCustomEditorWidget::MetaDataWidgetRow::updateValueWidget()
{
    CaretAssert(m_metaData);
    const QString value(m_metaData->get(m_metaDataName).trimmed());
    if (m_valueComboBox != NULL) {
        int32_t itemIndex(m_valueComboBox->findText(value));
        if (itemIndex < 0) {
            itemIndex = 0;
        }
        CaretAssert(itemIndex >= 0);
        if (itemIndex < m_valueComboBox->count()) {
            m_valueComboBox->setCurrentIndex(itemIndex);
        }
    }
    if (m_valueDateEdit != NULL) {
        m_valueDateEdit->setDate(QDate::fromString(value,
                                                   GiftiMetaDataXmlElements::METADATA_QT_DATE_FORMAT));
    }
    if (m_valueLineEdit != NULL) {
        m_valueLineEdit->setText(value);
    }
}


/**
 * Called when the tool button is clicked
 */
void
MetaDataCustomEditorWidget::MetaDataWidgetRow::toolButtonClicked()
{
    CaretAssert(m_editorWidget);
    m_editorWidget->metaDataButtonClicked(m_metaDataName,
                                          m_toolButton);
}

/**
 * Save the value to the metadata
 */
void
MetaDataCustomEditorWidget::MetaDataWidgetRow::saveToMetaData()
{
    QString text;
    if (m_valueComboBox != NULL) {
        text = m_valueComboBox->currentText().trimmed();
    }
    if (m_valueDateEdit != NULL) {
        const QDate date(m_valueDateEdit->date());
        text = date.toString(GiftiMetaDataXmlElements::METADATA_QT_DATE_FORMAT);
    }
    if (m_valueLineEdit != NULL) {
        text = m_valueLineEdit->text().trimmed();
    }
    m_metaData->set(m_metaDataName,
                    text);
}

