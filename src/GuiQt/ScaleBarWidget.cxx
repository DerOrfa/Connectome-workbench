
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

#define __SCALE_BAR_WIDGET_DECLARE__
#include "ScaleBarWidget.h"
#undef __SCALE_BAR_WIDGET_DECLARE__

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "AnnotationScaleBar.h"
#include "BrowserTabContent.h"
#include "CaretAssert.h"
#include "EnumComboBoxTemplate.h"
#include "EventGraphicsUpdateAllWindows.h"
#include "EventManager.h"

using namespace caret;


    
/**
 * \class caret::ScaleBarWidget 
 * \brief Widget for editing a scale bar
 * \ingroup GuiQt
 */

/**
 * Constructor.
 */
ScaleBarWidget::ScaleBarWidget()
: QWidget()
{
    m_showLengthTextCheckBox = new QCheckBox("Show Length");
    QObject::connect(m_showLengthTextCheckBox, &QCheckBox::clicked,
                     this, &ScaleBarWidget::showLengthCheckBoxClicked);
    
    m_showLengthUnitsCheckbox = new QCheckBox("Show Length Units");
    QObject::connect(m_showLengthUnitsCheckbox, &QCheckBox::clicked,
                     this, &ScaleBarWidget::showLengthUnitsCheckBoxClicked);
    
    QLabel* lengthLabel = new QLabel("Length");
    m_lengthSpinBox = new QDoubleSpinBox();
    m_lengthSpinBox->setRange(0.0, 100000.0);
    QObject::connect(m_lengthSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                     this, &ScaleBarWidget::lengthSpinBoxValueChanged);
    
    QLabel* positionModeLabel = new QLabel("Positioning");
    m_positionModeEnumComboBox = new EnumComboBoxTemplate(this);
    m_positionModeEnumComboBox->setup<AnnotationColorBarPositionModeEnum,AnnotationColorBarPositionModeEnum::Enum>();
    QObject::connect(m_positionModeEnumComboBox, &EnumComboBoxTemplate::itemActivated,
                     this, &ScaleBarWidget::positionModeEnumComboBoxItemActivated);
    m_positionModeEnumComboBox->getWidget()->setToolTip("AUTOMATIC - color bars are stacked\n"
                                                        "   in lower left corner of Tab/Window\n"
                                                        "MANUAL - user must set the X and Y\n"
                                                        "   coordinates in the Tab/Window\n");
    
    QLabel* unitsLabel = new QLabel("Length Units");
    m_lengthUnitsComboBox = new EnumComboBoxTemplate(this);
    m_lengthUnitsComboBox->setup<AnnotationScaleBarUnitsTypeEnum,AnnotationScaleBarUnitsTypeEnum::Enum>();
    QObject::connect(m_lengthUnitsComboBox, &EnumComboBoxTemplate::itemActivated,
                     this, &ScaleBarWidget::lengthEnumComboBoxItemActivated);
    m_lengthUnitsComboBox->getWidget()->setToolTip("Units displayed at end of scale bar");

    
    QGridLayout* gridLayout = new QGridLayout(this);
    int32_t row(0);
    gridLayout->addWidget(m_showLengthTextCheckBox, row, 0, 1, 2, Qt::AlignLeft);
    row++;
    gridLayout->addWidget(m_showLengthUnitsCheckbox, row, 0, 1, 2, Qt::AlignLeft);
    row++;
    gridLayout->addWidget(lengthLabel, row, 0);
    gridLayout->addWidget(m_lengthSpinBox, row, 1);
    row++;
    gridLayout->addWidget(unitsLabel, row, 0);
    gridLayout->addWidget(m_lengthUnitsComboBox->getWidget(), row, 1);
    row++;
    gridLayout->addWidget(positionModeLabel, row, 0);
    gridLayout->addWidget(m_positionModeEnumComboBox->getWidget(), row, 1);
    row++;
}

/**
 * Destructor.
 */
ScaleBarWidget::~ScaleBarWidget()
{
}

/**
 * Update the widgets content
 *  @param browserTabContent
 *       The tab's content
 */
void
ScaleBarWidget::updateContent(BrowserTabContent* browserTabContent)
{
    m_scaleBar = NULL;
    if (browserTabContent != NULL) {
        m_scaleBar = browserTabContent->getScaleBar();
    }
    
    if (m_scaleBar != NULL) {
        const AnnotationColorBarPositionModeEnum::Enum positionMode = m_scaleBar->getPositionMode();
        m_positionModeEnumComboBox->setSelectedItem<AnnotationColorBarPositionModeEnum,AnnotationColorBarPositionModeEnum::Enum>(positionMode);
    
        const AnnotationScaleBarUnitsTypeEnum::Enum unitsType = m_scaleBar->getLengthUnits();
        m_lengthUnitsComboBox->setSelectedItem<AnnotationScaleBarUnitsTypeEnum,AnnotationScaleBarUnitsTypeEnum::Enum>(unitsType);
        
        m_showLengthTextCheckBox->setChecked(m_scaleBar->isShowLengthText());
        
        m_showLengthUnitsCheckbox->setChecked(m_scaleBar->isShowLengthUnitsText());
        
        QSignalBlocker lengthBlocker(m_lengthSpinBox);
        m_lengthSpinBox->setValue(m_scaleBar->getLength());
    }

    setEnabled(m_scaleBar != NULL);
}

/**
 * Called when show length check box is clicked
 *
 *  @param status
 *    New checked status
 */
void
ScaleBarWidget::showLengthCheckBoxClicked(bool status)
{
    if (m_scaleBar != NULL) {
        m_scaleBar->setShowLengthText(status);
        updateGraphics();
    }
}

/**
 * Called when show length units check box is clicked
 *
 *  @param status
 *    New checked status
 */
void
ScaleBarWidget::showLengthUnitsCheckBoxClicked(bool status)
{
    if (m_scaleBar != NULL) {
        m_scaleBar->setShowLengthUnitsText(status);
        updateGraphics();
    }
}

/**
 * Called when  length double spin box value is changed
 *
 *  @param status
 *    New checked status
 */
void
ScaleBarWidget::lengthSpinBoxValueChanged(double value)
{
    if (m_scaleBar != NULL) {
        m_scaleBar->setLength(value);
        updateGraphics();
    }
}

/**
 * Caleld when position mode combo box selection is made
 */
void
ScaleBarWidget::positionModeEnumComboBoxItemActivated()
{
    if (m_scaleBar != NULL) {
        m_scaleBar->setPositionMode(m_positionModeEnumComboBox->getSelectedItem<AnnotationColorBarPositionModeEnum, AnnotationColorBarPositionModeEnum::Enum>());
        updateGraphics();
    }
}

/**
 * Caleld when length units combo box selection is made
 */
void
ScaleBarWidget::lengthEnumComboBoxItemActivated()
{
    if (m_scaleBar != NULL) {
        m_scaleBar->setLengthUnits(m_lengthUnitsComboBox->getSelectedItem<AnnotationScaleBarUnitsTypeEnum, AnnotationScaleBarUnitsTypeEnum::Enum>());
        updateGraphics();
    }
}

/**
 * Update graphics
 */
void
ScaleBarWidget::updateGraphics()
{
    EventManager::get()->sendEvent(EventGraphicsUpdateAllWindows().getPointer());
}
