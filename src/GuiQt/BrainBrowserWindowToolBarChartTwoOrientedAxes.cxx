
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

#include <QAction>
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QToolButton>
#include <QWidgetAction>

#define __BRAIN_BROWSER_WINDOW_TOOL_BAR_CHART_TWO_ORIENTED_AXES_DECLARE__
#include "BrainBrowserWindowToolBarChartTwoOrientedAxes.h"
#undef __BRAIN_BROWSER_WINDOW_TOOL_BAR_CHART_TWO_ORIENTED_AXES_DECLARE__

#include "BrowserTabContent.h"
#include "CaretAssert.h"
#include "ChartTwoAxisPropertiesEditorWidget.h"
#include "ChartTwoCartesianAxis.h"
#include "ChartTwoCartesianOrientedAxes.h"
#include "ChartTwoOverlaySet.h"
#include "ChartTwoTitle.h"
#include "ChartTwoTitleEditorWidget.h"
#include "EnumComboBoxTemplate.h"
#include "EventBrowserTabGet.h"
#include "EventBrowserWindowGraphicsRedrawn.h"
#include "EventChartTwoAttributesChanged.h"
#include "EventGraphicsUpdateAllWindows.h"
#include "EventManager.h"
#include "ModelChartTwo.h"
#include "WuQDoubleSpinBox.h"
#include "WuQMacroManager.h"
#include "WuQWidgetObjectGroup.h"
#include "WuQtUtilities.h"

using namespace caret;


    
/**
 * \class caret::BrainBrowserWindowToolBarChartTwoOrientedAxes
 * \brief Controls for chart horizontal and vertical axes.
 * \ingroup GuiQt
 */

/**
 * Constructor.
 *
 * @param parentToolBar
 *   The parent toolbar.
 * @param parentObjectName
 *   Name of parent object for macros
 */
BrainBrowserWindowToolBarChartTwoOrientedAxes::BrainBrowserWindowToolBarChartTwoOrientedAxes(BrainBrowserWindowToolBar* parentToolBar,
                                                                             const QString& parentObjectName)
: BrainBrowserWindowToolBarComponent(parentToolBar)
{
    const QString objectNamePrefix(parentObjectName
                                   + ":ChartAxes:");
    
    /*
     * Horizontal range
     */
    std::tuple<EnumComboBoxTemplate*, WuQDoubleSpinBox*, WuQDoubleSpinBox*> horizontalWidgets =
    createAxesWidgets(ChartTwoAxisOrientationTypeEnum::HORIZONTAL,
                      objectNamePrefix);
    m_horizontalRangeModeComboBox = std::get<0>(horizontalWidgets);
    m_horizontalUserMinimumValueSpinBox = std::get<1>(horizontalWidgets);
    m_horizontalUserMaximumValueSpinBox = std::get<2>(horizontalWidgets);

    /*
     * Vertical range
     */
    std::tuple<EnumComboBoxTemplate*, WuQDoubleSpinBox*, WuQDoubleSpinBox*> verticalWidgets =
    createAxesWidgets(ChartTwoAxisOrientationTypeEnum::VERTICAL,
                      objectNamePrefix);
    m_verticalRangeModeComboBox = std::get<0>(verticalWidgets);
    m_verticalUserMinimumValueSpinBox = std::get<1>(verticalWidgets);
    m_verticalUserMaximumValueSpinBox = std::get<2>(verticalWidgets);
    
    /*
     * Left Axis display and edit
     */
    std::tuple<QCheckBox*, QToolButton*> leftWidgets = createAxisEditing(ChartAxisLocationEnum::CHART_AXIS_LOCATION_LEFT,
                                                                         objectNamePrefix);
    m_leftAxisCheckBox = std::get<0>(leftWidgets);
    m_leftAxisEditToolButton = std::get<1>(leftWidgets);
    
    /*
     * Right Axis display and edit
     */
    std::tuple<QCheckBox*, QToolButton*> rightWidgets = createAxisEditing(ChartAxisLocationEnum::CHART_AXIS_LOCATION_RIGHT,
                                                                         objectNamePrefix);
    m_rightAxisCheckBox = std::get<0>(rightWidgets);
    m_rightAxisEditToolButton = std::get<1>(rightWidgets);
    
    /*
     * Bottom Axis display and edit
     */
    std::tuple<QCheckBox*, QToolButton*> bottomWidgets = createAxisEditing(ChartAxisLocationEnum::CHART_AXIS_LOCATION_BOTTOM,
                                                                         objectNamePrefix);
    m_bottomAxisCheckBox = std::get<0>(bottomWidgets);
    m_bottomAxisEditToolButton = std::get<1>(bottomWidgets);
    
    /*
     * Top Axis display and edit
     */
    std::tuple<QCheckBox*, QToolButton*> topWidgets = createAxisEditing(ChartAxisLocationEnum::CHART_AXIS_LOCATION_TOP,
                                                                         objectNamePrefix);
    m_topAxisCheckBox = std::get<0>(topWidgets);
    m_topAxisEditToolButton = std::get<1>(topWidgets);
    
    /*
     * Title display and edit
     */
    m_titleCheckBox = new QCheckBox("Title");
    QObject::connect(m_titleCheckBox, &QCheckBox::clicked,
                     this, &BrainBrowserWindowToolBarChartTwoOrientedAxes::titleCheckBoxClicked);
    m_titleCheckBox->setToolTip("Display the chart title");
    m_titleEditToolButton = new QToolButton();
    m_titleEditToolButton->setText("Edit");
    m_titleEditToolButton->setToolTip("Edit the chart title");
    WuQtUtilities::setToolButtonStyleForQt5Mac(m_titleEditToolButton);
    QObject::connect(m_titleEditToolButton, &QToolButton::clicked,
                     this, &BrainBrowserWindowToolBarChartTwoOrientedAxes::titleEditToolButtonClicked);

    m_titleEditorWidget = new ChartTwoTitleEditorWidget(m_titleEditToolButton,
                                                        objectNamePrefix);
    QWidgetAction* titleEditorWidgetAction = new QWidgetAction(m_titleEditToolButton);
    titleEditorWidgetAction->setDefaultWidget(m_titleEditorWidget);
    
    m_titleEditMenu = new QMenu(m_titleEditToolButton);
    m_titleEditMenu->addAction(titleEditorWidgetAction);
    QObject::connect(m_titleEditMenu, &QMenu::aboutToShow,
                     this, [=]() { titleEditorMenuAboutToShow(); } );
    
    /*
     * Range widgets layout
     */
    int32_t columnCounter(0);
    const int32_t COLUMN_LABEL(columnCounter++);
    const int32_t COLUMN_HORIZ(columnCounter++);
    const int32_t COLUMN_VERT(columnCounter++);
    const int32_t COLUMN_LINE(columnCounter++);
    const int32_t COLUMN_CHECKBOX(columnCounter++);
    const int32_t COLUMN_EDIT(columnCounter++);
    QGridLayout* rangeLayout = new QGridLayout(this);
    WuQtUtilities::setLayoutSpacingAndMargins(rangeLayout, 3, 0);
    int row(0);
    rangeLayout->addWidget(new QLabel("Horizontal"), row, COLUMN_HORIZ);
    rangeLayout->addWidget(new QLabel("Vertical"), row, COLUMN_VERT);
    rangeLayout->addWidget(m_leftAxisCheckBox, row, COLUMN_CHECKBOX);
    rangeLayout->addWidget(m_leftAxisEditToolButton, row, COLUMN_EDIT);
    row++;
    
    rangeLayout->addWidget(new QLabel("Range"), row, COLUMN_LABEL);
    rangeLayout->addWidget(m_horizontalRangeModeComboBox->getWidget(), row, COLUMN_HORIZ);
    rangeLayout->addWidget(m_verticalRangeModeComboBox->getWidget(), row, COLUMN_VERT);
    rangeLayout->addWidget(m_rightAxisCheckBox, row, COLUMN_CHECKBOX);
    rangeLayout->addWidget(m_rightAxisEditToolButton, row, COLUMN_EDIT);
    
    row++;
    rangeLayout->addWidget(new QLabel("Max"), row, COLUMN_LABEL);
    rangeLayout->addWidget(m_horizontalUserMaximumValueSpinBox->getWidget(), row, COLUMN_HORIZ);
    rangeLayout->addWidget(m_verticalUserMaximumValueSpinBox->getWidget(), row, COLUMN_VERT);
    rangeLayout->addWidget(m_topAxisCheckBox, row, COLUMN_CHECKBOX);
    rangeLayout->addWidget(m_topAxisEditToolButton, row, COLUMN_EDIT);

    row++;
    rangeLayout->addWidget(new QLabel("Min"), row, COLUMN_LABEL);
    rangeLayout->addWidget(m_horizontalUserMinimumValueSpinBox->getWidget(), row, COLUMN_HORIZ);
    rangeLayout->addWidget(m_verticalUserMinimumValueSpinBox->getWidget(), row, COLUMN_VERT);
    rangeLayout->addWidget(m_bottomAxisCheckBox, row, COLUMN_CHECKBOX);
    rangeLayout->addWidget(m_bottomAxisEditToolButton, row, COLUMN_EDIT);
    row++;
    rangeLayout->addWidget(m_titleCheckBox, row, COLUMN_CHECKBOX);
    rangeLayout->addWidget(m_titleEditToolButton, row, COLUMN_EDIT);

    rangeLayout->addWidget(WuQtUtilities::createVerticalLineWidget(),
                           0, COLUMN_LINE,
                           row, 1);

    EventManager::get()->addEventListener(this,
                                          EventTypeEnum::EVENT_BROWSER_WINDOW_GRAPHICS_HAVE_BEEN_REDRAWN);
}

/**
 * Destructor.
 */
BrainBrowserWindowToolBarChartTwoOrientedAxes::~BrainBrowserWindowToolBarChartTwoOrientedAxes()
{
    EventManager::get()->removeEventFromListener(this,
                                                 EventTypeEnum::EVENT_BROWSER_WINDOW_GRAPHICS_HAVE_BEEN_REDRAWN);
}

/**
 * Receive an event.
 *
 * @param event
 *    The event.
 */
void
BrainBrowserWindowToolBarChartTwoOrientedAxes::receiveEvent(Event* event)
{
    if (event->getEventType() == EventTypeEnum::EVENT_BROWSER_WINDOW_GRAPHICS_HAVE_BEEN_REDRAWN) {
        CaretAssert(dynamic_cast<EventBrowserWindowGraphicsRedrawn*>(event));
        updateContent(getTabContentFromSelectedTab());
    }
    else {
        BrainBrowserWindowToolBarComponent::receiveEvent(event);
    }
}

/**
 * @return Widgets for an axis
 * @param orientation
 *    Orientation of the axis
 * @param objectNamePrefix
 *    Object name for macros
 */
std::tuple<EnumComboBoxTemplate*, WuQDoubleSpinBox*, WuQDoubleSpinBox*>
BrainBrowserWindowToolBarChartTwoOrientedAxes::createAxesWidgets(const ChartTwoAxisOrientationTypeEnum::Enum orientation,
                                                                 const QString& objectNamePrefix)
{
    WuQMacroManager* macroManager = WuQMacroManager::instance();

    const QString macroWidgetName(objectNamePrefix
                                  + ChartTwoAxisOrientationTypeEnum::toGuiName(orientation)
                                  + ":");
    /*
     * Range controls
     */
    const AString rangeTooltip("Auto - Adjusts axis range to fit data with some\n"
                               "       padding so that scale value are usually\n"
                               "       whole numbers\n"
                               "Data - Axis range is limited to minimum and \n"
                               "       maximum values of the data\n"
                               "User - Axis range is controlled by user");
    EnumComboBoxTemplate* rangeModeComboBox = new EnumComboBoxTemplate(this);
    rangeModeComboBox->getComboBox()->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    rangeModeComboBox->setup<ChartTwoAxisScaleRangeModeEnum, ChartTwoAxisScaleRangeModeEnum::Enum>();
    QObject::connect(rangeModeComboBox, &EnumComboBoxTemplate::itemActivated,
                     this, &BrainBrowserWindowToolBarChartTwoOrientedAxes::valueChanged);
    rangeModeComboBox->getWidget()->setToolTip(rangeTooltip);
    rangeModeComboBox->getWidget()->setObjectName(macroWidgetName
                                                  + "RangeMode");
    macroManager->addMacroSupportToObject(rangeModeComboBox->getWidget(),
                                          "Select chart axis range mode");
    
    WuQDoubleSpinBox* userMinimumValueSpinBox = new WuQDoubleSpinBox(this);
    userMinimumValueSpinBox->setDecimalsModeAuto();
    userMinimumValueSpinBox->setSingleStepPercentage(1.0);
    switch (orientation) {
        case ChartTwoAxisOrientationTypeEnum::HORIZONTAL:
            QObject::connect(userMinimumValueSpinBox, static_cast<void (WuQDoubleSpinBox::*)(double)>(&WuQDoubleSpinBox::valueChanged),
                             this, &BrainBrowserWindowToolBarChartTwoOrientedAxes::horizontalAxisMinimumValueChanged);
            break;
        case ChartTwoAxisOrientationTypeEnum::VERTICAL:
            QObject::connect(userMinimumValueSpinBox, static_cast<void (WuQDoubleSpinBox::*)(double)>(&WuQDoubleSpinBox::valueChanged),
                             this, &BrainBrowserWindowToolBarChartTwoOrientedAxes::verticalAxisMinimumValueChanged);
            break;
    }
    userMinimumValueSpinBox->setToolTip("Set user scaling axis minimum value");
    userMinimumValueSpinBox->getWidget()->setObjectName(macroWidgetName
                                                          + "ScaleMinimum");
    macroManager->addMacroSupportToObject(userMinimumValueSpinBox->getWidget(),
                                          "Set chart axis minimum");
    
    WuQDoubleSpinBox* userMaximumValueSpinBox = new WuQDoubleSpinBox(this);
    userMaximumValueSpinBox->setDecimalsModeAuto();
    userMaximumValueSpinBox->setSingleStepPercentage(1.0);
    switch (orientation) {
        case ChartTwoAxisOrientationTypeEnum::HORIZONTAL:
            QObject::connect(userMaximumValueSpinBox, static_cast<void (WuQDoubleSpinBox::*)(double)>(&WuQDoubleSpinBox::valueChanged),
                             this, &BrainBrowserWindowToolBarChartTwoOrientedAxes::horizontalAxisMaximumValueChanged);
            break;
        case ChartTwoAxisOrientationTypeEnum::VERTICAL:
            QObject::connect(userMaximumValueSpinBox, static_cast<void (WuQDoubleSpinBox::*)(double)>(&WuQDoubleSpinBox::valueChanged),
                             this, &BrainBrowserWindowToolBarChartTwoOrientedAxes::verticalAxisMaximumValueChanged);
            break;
    }
    userMaximumValueSpinBox->setToolTip("Set user scaling axis maximum value");
    userMaximumValueSpinBox->getWidget()->setObjectName(macroWidgetName
                                                          + "ScaleMaximum");
    macroManager->addMacroSupportToObject(userMaximumValueSpinBox->getWidget(),
                                          "Set chart axis maximum");
    
    /*
     * Group widgets for blocking signals
     */
    m_widgetGroup = new WuQWidgetObjectGroup(this);
    m_widgetGroup->add(rangeModeComboBox->getWidget());
    m_widgetGroup->add(userMinimumValueSpinBox);
    m_widgetGroup->add(userMaximumValueSpinBox);
    
    return std::make_tuple(rangeModeComboBox,
                           userMinimumValueSpinBox,
                           userMaximumValueSpinBox);
}

std::tuple<QCheckBox*, QToolButton*>
BrainBrowserWindowToolBarChartTwoOrientedAxes::createAxisEditing(const ChartAxisLocationEnum::Enum axis,
                                                                 const QString& objectNamePrefix)
{
    WuQMacroManager* macroManager = WuQMacroManager::instance();
    const QString macroWidgetName(objectNamePrefix
                                  + ChartAxisLocationEnum::toGuiName(axis)
                                  + ":");
    
    QCheckBox* checkBox = new QCheckBox();
    checkBox->setText(ChartAxisLocationEnum::toGuiName(axis));
    checkBox->setToolTip("Enable display of the "
                         + ChartAxisLocationEnum::toGuiName(axis)
                         + " axis");
    QObject::connect(checkBox, &QCheckBox::clicked,
                     this, [=](bool checkedStatus) { axisCheckBoxOnOffClicked(axis,
                                                                              checkedStatus); });
    checkBox->setObjectName(macroWidgetName
                            + "OnOffCheckBox");
    macroManager->addMacroSupportToObject(checkBox,
                                          checkBox->toolTip());

    QToolButton* toolButton = new QToolButton();
    ChartTwoAxisPropertiesEditorWidget* editorWidget
       = new ChartTwoAxisPropertiesEditorWidget(toolButton,
                                                axis,
                                                objectNamePrefix);
    
    QWidgetAction* editorWidgetAction = new QWidgetAction(toolButton);
    editorWidgetAction->setDefaultWidget(editorWidget);
    
    QMenu* menu = new QMenu(toolButton);
    menu->addAction(editorWidgetAction);
    QObject::connect(menu, &QMenu::aboutToShow,
                     this, [=]() { axisEditorMenuAboutToShow(axis); } );
    
    switch (axis) {
        case ChartAxisLocationEnum::CHART_AXIS_LOCATION_BOTTOM:
            m_bottomAxisEditorWidget = editorWidget;
            m_bottomAxisMenu = menu;
            break;
        case ChartAxisLocationEnum::CHART_AXIS_LOCATION_LEFT:
            m_leftAxisEditorWidget = editorWidget;
            m_leftAxisMenu = menu;
            break;
        case ChartAxisLocationEnum::CHART_AXIS_LOCATION_RIGHT:
            m_rightAxisEditorWidget = editorWidget;
            m_rightAxisMenu = menu;
            break;
        case ChartAxisLocationEnum::CHART_AXIS_LOCATION_TOP:
            m_topAxisEditorWidget = editorWidget;
            m_topAxisMenu = menu;
            break;
    }
    
    WuQtUtilities::setToolButtonStyleForQt5Mac(toolButton);
    toolButton->setText("Edit");
    toolButton->setToolTip("Edit the attributes of the "
                         + ChartAxisLocationEnum::toGuiName(axis)
                         + " axis");
    QObject::connect(toolButton, &QToolButton::clicked,
                     this, [=]() { axisToolButtonEditClicked(axis); });
    toolButton->setObjectName(macroWidgetName
                            + "EditToolButton");
    macroManager->addMacroSupportToObject(toolButton,
                                          toolButton->toolTip());
    
    return std::make_tuple(checkBox,
                           toolButton);
}

/**
 * Called when an axis editor menu is about to show
 * @param axis
 *    Axis in the menu
 */
void
BrainBrowserWindowToolBarChartTwoOrientedAxes::axisEditorMenuAboutToShow(const ChartAxisLocationEnum::Enum axis)
{
    ChartTwoOverlaySet* overlaySet(NULL);
    ChartTwoCartesianOrientedAxes* horizontalAxis(NULL);
    ChartTwoCartesianOrientedAxes* verticalAxis(NULL);
    getSelectionData(overlaySet,
                     horizontalAxis,
                     verticalAxis);
    
    if (overlaySet != NULL) {
        switch (axis) {
            case ChartAxisLocationEnum::CHART_AXIS_LOCATION_BOTTOM:
                if (horizontalAxis != NULL) {
                    m_bottomAxisEditorWidget->updateControls(overlaySet,
                                                             horizontalAxis->getLeftOrBottomAxis());
                }
                break;
            case ChartAxisLocationEnum::CHART_AXIS_LOCATION_LEFT:
                if (verticalAxis != NULL) {
                    m_leftAxisEditorWidget->updateControls(overlaySet,
                                                           verticalAxis->getLeftOrBottomAxis());
                }
                break;
            case ChartAxisLocationEnum::CHART_AXIS_LOCATION_RIGHT:
                if (verticalAxis != NULL) {
                    m_rightAxisEditorWidget->updateControls(overlaySet,
                                                            verticalAxis->getRightOrTopAxis());
                }
                break;
            case ChartAxisLocationEnum::CHART_AXIS_LOCATION_TOP:
                if (horizontalAxis != NULL) {
                    m_topAxisEditorWidget->updateControls(overlaySet,
                                                          horizontalAxis->getRightOrTopAxis());
                }
                break;
        }
    }
}


/**
 * Called when an axis on/off checkbox is clicked
 * @param axis
 *    Axis of checkbox that was clicked
 * @param checkedStatus
 *    Checked status of button
 */
void
BrainBrowserWindowToolBarChartTwoOrientedAxes::axisCheckBoxOnOffClicked(const ChartAxisLocationEnum::Enum axis,
                                                                        const bool checkedStatus)
{
    ChartTwoOverlaySet* overlaySet(NULL);
    ChartTwoCartesianOrientedAxes* horizontalAxis(NULL);
    ChartTwoCartesianOrientedAxes* verticalAxis(NULL);
    getSelectionData(overlaySet,
                     horizontalAxis,
                     verticalAxis);
    
    m_widgetGroup->blockAllSignals(true);
    
    
    if ((overlaySet != NULL)
        && (horizontalAxis != NULL)
        && (verticalAxis != NULL)) {
        switch (axis) {
            case ChartAxisLocationEnum::CHART_AXIS_LOCATION_BOTTOM:
                horizontalAxis->getLeftOrBottomAxis()->setDisplayedByUser(checkedStatus);
                break;
            case ChartAxisLocationEnum::CHART_AXIS_LOCATION_LEFT:
                verticalAxis->getLeftOrBottomAxis()->setDisplayedByUser(checkedStatus);
                break;
            case ChartAxisLocationEnum::CHART_AXIS_LOCATION_RIGHT:
                verticalAxis->getRightOrTopAxis()->setDisplayedByUser(checkedStatus);
                break;
            case ChartAxisLocationEnum::CHART_AXIS_LOCATION_TOP:
                horizontalAxis->getRightOrTopAxis()->setDisplayedByUser(checkedStatus);
                break;
        }
    }
    
    updateGraphics();
}

/**
 * Called when an axis edit button is clicked
 * @param axis
 *    Axis of button that was clicked
 */
void
BrainBrowserWindowToolBarChartTwoOrientedAxes::axisToolButtonEditClicked(const ChartAxisLocationEnum::Enum axis)
{
    ChartTwoOverlaySet*            chartOverlaySet(NULL);
    ChartTwoCartesianOrientedAxes* horizontalAxes(NULL);
    ChartTwoCartesianOrientedAxes* verticalAxes(NULL);
    
    getSelectionData(chartOverlaySet,
                     horizontalAxes,
                     verticalAxes);

    if (chartOverlaySet != NULL) {
        ChartTwoCartesianAxis* chartAxis(NULL);
        QWidget* button(NULL);
        QMenu* menu(NULL);
        switch (axis) {
            case ChartAxisLocationEnum::CHART_AXIS_LOCATION_BOTTOM:
                if (horizontalAxes != NULL) {
                    chartAxis = horizontalAxes->getLeftOrBottomAxis();
                    button    = m_bottomAxisEditToolButton;
                    menu      = m_bottomAxisMenu;
                }
                break;
            case ChartAxisLocationEnum::CHART_AXIS_LOCATION_LEFT:
                if (verticalAxes != NULL) {
                    chartAxis = verticalAxes->getLeftOrBottomAxis();
                    button    = m_leftAxisEditToolButton;
                    menu      = m_leftAxisMenu;
                }
                break;
            case ChartAxisLocationEnum::CHART_AXIS_LOCATION_RIGHT:
                if (verticalAxes != NULL) {
                    chartAxis = verticalAxes->getRightOrTopAxis();
                    button    = m_rightAxisEditToolButton;
                    menu      = m_rightAxisMenu;
                }
                break;
            case ChartAxisLocationEnum::CHART_AXIS_LOCATION_TOP:
                if (horizontalAxes != NULL) {
                    chartAxis = horizontalAxes->getRightOrTopAxis();
                    button    = m_topAxisEditToolButton;
                    menu      = m_topAxisMenu;
                }
                break;
        }
        
        if ((button != NULL)
            && (menu != NULL)) {
            menu->exec(button->mapToGlobal(QPoint(0, button->height())));
        }
    }
}


/**
 * Update content of this tool bar component.
 *
 * @param browserTabContent
 *     Content of the browser tab.
 */
void
BrainBrowserWindowToolBarChartTwoOrientedAxes::updateContent(BrowserTabContent* browserTabContent)
{
    m_browserTabIndex = -1;
    
    if (browserTabContent == NULL) {
        setEnabled(false);
        return;
    }
    
    m_browserTabIndex = browserTabContent->getTabNumber();
    updateControls();
}

/**
 * Get the selection data.
 *
 * @param chartOverlaySetOut
 *     The chart overlay set (may be NULL)
 * @param validAxesLocationsOut
 *     The valid axes locations.
 * @param selectedAxisOut
 *     Output with selected axis (may be NULL)
 */
void
BrainBrowserWindowToolBarChartTwoOrientedAxes::getSelectionData(ChartTwoOverlaySet* &chartOverlaySetOut,
                                                                ChartTwoCartesianOrientedAxes* &horizontalAxesOut,
                                                                ChartTwoCartesianOrientedAxes* &verticalAxesOut) const
{
    chartOverlaySetOut = NULL;
    horizontalAxesOut  = NULL;
    verticalAxesOut    = NULL;
    
    EventBrowserTabGet tabEvent(m_browserTabIndex);
    EventManager::get()->sendEvent(tabEvent.getPointer());
    
    BrowserTabContent* browserTabContent = tabEvent.getBrowserTab();
    if (browserTabContent == NULL) {
        return;
    }
    
    if (browserTabContent != NULL) {
        ModelChartTwo* modelChartTwo = browserTabContent->getDisplayedChartTwoModel();
        const int32_t tabIndex = browserTabContent->getTabNumber();
        if (modelChartTwo != NULL) {
            switch (modelChartTwo->getSelectedChartTwoDataType(tabIndex)) {
                case ChartTwoDataTypeEnum::CHART_DATA_TYPE_INVALID:
                    break;
                case ChartTwoDataTypeEnum::CHART_DATA_TYPE_HISTOGRAM:
                    chartOverlaySetOut = modelChartTwo->getChartTwoOverlaySet(tabIndex);
                    break;
                case ChartTwoDataTypeEnum::CHART_DATA_TYPE_LINE_LAYER:
                    chartOverlaySetOut = modelChartTwo->getChartTwoOverlaySet(tabIndex);
                    break;
                case ChartTwoDataTypeEnum::CHART_DATA_TYPE_LINE_SERIES:
                    chartOverlaySetOut = modelChartTwo->getChartTwoOverlaySet(tabIndex);
                    break;
                case ChartTwoDataTypeEnum::CHART_DATA_TYPE_MATRIX:
                    chartOverlaySetOut = modelChartTwo->getChartTwoOverlaySet(tabIndex);
                    break;
            }
        }
        
        if (chartOverlaySetOut != NULL) {
            horizontalAxesOut = chartOverlaySetOut->getHorizontalAxes();
            verticalAxesOut   = chartOverlaySetOut->getVerticalAxes();
        }
    }
}

/**
 * Update the controls in this editor
 */
void
BrainBrowserWindowToolBarChartTwoOrientedAxes::updateControls()
{
    ChartTwoOverlaySet* overlaySet(NULL);
    ChartTwoCartesianOrientedAxes* horizontalAxis(NULL);
    ChartTwoCartesianOrientedAxes* verticalAxis(NULL);
    getSelectionData(overlaySet,
                     horizontalAxis,
                     verticalAxis);

    m_widgetGroup->blockAllSignals(true);

    
    if ((overlaySet != NULL)
        && (horizontalAxis != NULL)
        && (verticalAxis != NULL)) {
        /*
         * Update horizontal
         */
        m_bottomAxisCheckBox->setChecked(horizontalAxis->getLeftOrBottomAxis()->isDisplayedByUser());
        m_topAxisCheckBox->setChecked(horizontalAxis->getRightOrTopAxis()->isDisplayedByUser());
        m_horizontalRangeModeComboBox->setSelectedItem<ChartTwoAxisScaleRangeModeEnum, ChartTwoAxisScaleRangeModeEnum::Enum>(horizontalAxis->getScaleRangeMode());
        
        float horizMin(0.0), horizMax(0.0);
        horizontalAxis->getDataRange(horizMin,
                                     horizMax);
        m_horizontalUserMinimumValueSpinBox->setRangeExceedable(horizMin,
                                                                horizMax);
        m_horizontalUserMinimumValueSpinBox->setValue(horizontalAxis->getUserScaleMinimumValue());
        m_horizontalUserMaximumValueSpinBox->setRangeExceedable(horizMin,
                                                                horizMax);
        m_horizontalUserMaximumValueSpinBox->setValue(horizontalAxis->getUserScaleMaximumValue());

        /*
         * Update vertical
         */
        m_leftAxisCheckBox->setChecked(verticalAxis->getLeftOrBottomAxis()->isDisplayedByUser());
        m_rightAxisCheckBox->setChecked(verticalAxis->getRightOrTopAxis()->isDisplayedByUser());
        m_verticalRangeModeComboBox->setSelectedItem<ChartTwoAxisScaleRangeModeEnum, ChartTwoAxisScaleRangeModeEnum::Enum>(verticalAxis->getScaleRangeMode());
        
        float vertMin(0.0), vertMax(0.0);
        verticalAxis->getDataRange(vertMin,
                                   vertMax);
        m_verticalUserMinimumValueSpinBox->setRangeExceedable(vertMin,
                                                              vertMax);
        m_verticalUserMinimumValueSpinBox->setValue(verticalAxis->getUserScaleMinimumValue());
        m_verticalUserMaximumValueSpinBox->setRangeExceedable(vertMin,
                                                              vertMax);
        m_verticalUserMaximumValueSpinBox->setValue(verticalAxis->getUserScaleMaximumValue());

        ChartTwoTitle* chartTitle = overlaySet->getChartTitle();
        m_titleCheckBox->setChecked(chartTitle->isDisplayed());
        
        setEnabled(true);
    }
    else {
        setEnabled(false);
    }
    
    
    m_widgetGroup->blockAllSignals(false);
}

/**
 * Called when a widget is changed by the user.
 */
void
BrainBrowserWindowToolBarChartTwoOrientedAxes::valueChanged()
{
    ChartTwoOverlaySet* overlaySet(NULL);
    ChartTwoCartesianOrientedAxes* horizontalAxis(NULL);
    ChartTwoCartesianOrientedAxes* verticalAxis(NULL);
    getSelectionData(overlaySet,
                     horizontalAxis,
                     verticalAxis);
    
    if (horizontalAxis != NULL) {
        horizontalAxis->setScaleRangeMode(m_horizontalRangeModeComboBox->getSelectedItem<ChartTwoAxisScaleRangeModeEnum, ChartTwoAxisScaleRangeModeEnum::Enum>());
        horizontalAxis->setUserScaleMinimumValue(m_horizontalUserMinimumValueSpinBox->value());
        horizontalAxis->setUserScaleMaximumValue(m_horizontalUserMaximumValueSpinBox->value());
    }

    if (verticalAxis != NULL) {
        verticalAxis->setScaleRangeMode(m_verticalRangeModeComboBox->getSelectedItem<ChartTwoAxisScaleRangeModeEnum, ChartTwoAxisScaleRangeModeEnum::Enum>());
        verticalAxis->setUserScaleMinimumValue(m_verticalUserMinimumValueSpinBox->value());
        verticalAxis->setUserScaleMaximumValue(m_verticalUserMaximumValueSpinBox->value());
        

        CaretAssertToDoWarning();  // yoking
//        const YokingGroupEnum::Enum yokingGroup = tabContent->getChartModelYokingGroup();
//        if (yokingGroup != YokingGroupEnum::YOKING_GROUP_OFF) {
//            const ModelChartTwo* modelChartTwo = tabContent->getDisplayedChartTwoModel();
//            CaretAssert(modelChartTwo);
//            const int32_t tabIndex = tabContent->getTabNumber();
//            EventChartTwoAttributesChanged attributesEvent;
//            attributesEvent.setCartesianAxisChanged(yokingGroup,
//                                                    modelChartTwo->getSelectedChartTwoDataType(tabIndex),
//                                                    m_chartAxis);
//            EventManager::get()->sendEvent(attributesEvent.getPointer());
//        }
    }

    updateGraphics();
    
    updateContent(getTabContentFromSelectedTab());
}

/**
 * Update the graphics.
 */
void
BrainBrowserWindowToolBarChartTwoOrientedAxes::updateGraphics()
{
    EventManager::get()->sendEvent(EventGraphicsUpdateAllWindows().getPointer());
}

/**
 * Called when the minimum value is changed.
 *
 * @param minimumValue
 *     New minimum value.
 */
void
BrainBrowserWindowToolBarChartTwoOrientedAxes::horizontalAxisMinimumValueChanged(double minimumValue)
{
    ChartTwoOverlaySet* overlaySet(NULL);
    ChartTwoCartesianOrientedAxes* horizontalAxis(NULL);
    ChartTwoCartesianOrientedAxes* verticalAxis(NULL);
    getSelectionData(overlaySet,
                     horizontalAxis,
                     verticalAxis);
    if (horizontalAxis != NULL) {
        /*
         * If the minimum or maximum value is modified by user,
         * ensure Auto/User Range selection is USER
         */
        m_horizontalRangeModeComboBox->getWidget()->blockSignals(true);
        m_horizontalRangeModeComboBox->setSelectedItem<ChartTwoAxisScaleRangeModeEnum, ChartTwoAxisScaleRangeModeEnum::Enum>(ChartTwoAxisScaleRangeModeEnum::USER);
        m_horizontalRangeModeComboBox->getWidget()->blockSignals(false);

        /*
         * Ensure maximum value is always greater than or equal to minimum
         */
        if (minimumValue > m_horizontalUserMaximumValueSpinBox->value()) {
            m_horizontalUserMaximumValueSpinBox->getWidget()->blockSignals(true);
            m_horizontalUserMaximumValueSpinBox->setValue(minimumValue);
            m_horizontalUserMaximumValueSpinBox->getWidget()->blockSignals(false);
        }

        valueChanged();
    }
}

/**
 * Called when the maximum value is changed.
 *
 * @param maximumValue
 *     New maximum value.
 */
void
BrainBrowserWindowToolBarChartTwoOrientedAxes::horizontalAxisMaximumValueChanged(double maximumValue)
{
    ChartTwoOverlaySet* overlaySet(NULL);
    ChartTwoCartesianOrientedAxes* horizontalAxis(NULL);
    ChartTwoCartesianOrientedAxes* verticalAxis(NULL);
    getSelectionData(overlaySet,
                     horizontalAxis,
                     verticalAxis);
    if (horizontalAxis != NULL) {
        /*
         * If the minimum or maximum value is modified by user,
         * ensure Auto/User Range selection is USER
         */
        m_horizontalRangeModeComboBox->getWidget()->blockSignals(true);
        m_horizontalRangeModeComboBox->setSelectedItem<ChartTwoAxisScaleRangeModeEnum, ChartTwoAxisScaleRangeModeEnum::Enum>(ChartTwoAxisScaleRangeModeEnum::USER);
        m_horizontalRangeModeComboBox->getWidget()->blockSignals(false);

        /*
         * Ensure minimum value is always less than or equal to maximum
         */
        if (maximumValue < m_horizontalUserMinimumValueSpinBox->value()) {
            m_horizontalUserMinimumValueSpinBox->getWidget()->blockSignals(true);
            m_horizontalUserMinimumValueSpinBox->setValue(maximumValue);
            m_horizontalUserMinimumValueSpinBox->getWidget()->blockSignals(false);
        }

        valueChanged();
    }
}

/**
 * Called when the minimum value is changed.
 *
 * @param minimumValue
 *     New minimum value.
 */
void
BrainBrowserWindowToolBarChartTwoOrientedAxes::verticalAxisMinimumValueChanged(double minimumValue)
{
    ChartTwoOverlaySet* overlaySet(NULL);
    ChartTwoCartesianOrientedAxes* horizontalAxis(NULL);
    ChartTwoCartesianOrientedAxes* verticalAxis(NULL);
    getSelectionData(overlaySet,
                     horizontalAxis,
                     verticalAxis);
    if (verticalAxis != NULL) {
        /*
         * If the minimum or maximum value is modified by user,
         * ensure Auto/User Range selection is USER
         */
        m_verticalRangeModeComboBox->getWidget()->blockSignals(true);
        m_verticalRangeModeComboBox->setSelectedItem<ChartTwoAxisScaleRangeModeEnum, ChartTwoAxisScaleRangeModeEnum::Enum>(ChartTwoAxisScaleRangeModeEnum::USER);
        m_verticalRangeModeComboBox->getWidget()->blockSignals(false);
        
        /*
         * Ensure maximum value is always greater than or equal to minimum
         */
        if (minimumValue > m_verticalUserMaximumValueSpinBox->value()) {
            m_verticalUserMaximumValueSpinBox->getWidget()->blockSignals(true);
            m_verticalUserMaximumValueSpinBox->setValue(minimumValue);
            m_verticalUserMaximumValueSpinBox->getWidget()->blockSignals(false);
        }
        
        valueChanged();
    }
}

/**
 * Called when the maximum value is changed.
 *
 * @param maximumValue
 *     New maximum value.
 */
void
BrainBrowserWindowToolBarChartTwoOrientedAxes::verticalAxisMaximumValueChanged(double maximumValue)
{
    ChartTwoOverlaySet* overlaySet(NULL);
    ChartTwoCartesianOrientedAxes* horizontalAxis(NULL);
    ChartTwoCartesianOrientedAxes* verticalAxis(NULL);
    getSelectionData(overlaySet,
                     horizontalAxis,
                     verticalAxis);
    if (verticalAxis != NULL) {
        /*
         * If the minimum or maximum value is modified by user,
         * ensure Auto/User Range selection is USER
         */
        m_verticalRangeModeComboBox->getWidget()->blockSignals(true);
        m_verticalRangeModeComboBox->setSelectedItem<ChartTwoAxisScaleRangeModeEnum, ChartTwoAxisScaleRangeModeEnum::Enum>(ChartTwoAxisScaleRangeModeEnum::USER);
        m_verticalRangeModeComboBox->getWidget()->blockSignals(false);
        
        /*
         * Ensure minimum value is always less than or equal to maximum
         */
        if (maximumValue < m_verticalUserMinimumValueSpinBox->value()) {
            m_verticalUserMinimumValueSpinBox->getWidget()->blockSignals(true);
            m_verticalUserMinimumValueSpinBox->setValue(maximumValue);
            m_verticalUserMinimumValueSpinBox->getWidget()->blockSignals(false);
        }
        
        valueChanged();
    }
}

/**
 * Called when a widget is changed by a slot using a bool parameter.
 * Parameters must match when using function pointers.
 */
void
BrainBrowserWindowToolBarChartTwoOrientedAxes::valueChangedBool(bool)
{
    valueChanged();
}

/**
 * Called when a widget is changed by a slot using a double parameter.
 * Parameters must match when using function pointers.
 */
void
BrainBrowserWindowToolBarChartTwoOrientedAxes::valueChangedDouble(double)
{
    valueChanged();
}

/**
 * Called when a widget is changed by a slot using a int parameter.
 * Parameters must match when using function pointers.
 */
void
BrainBrowserWindowToolBarChartTwoOrientedAxes::valueChangedInt(int)
{
    valueChanged();
}

/**
 * Called when title check box is clicked
 * @param checked
 *    New checked status
 */
void
BrainBrowserWindowToolBarChartTwoOrientedAxes::titleCheckBoxClicked(bool checked)
{
    ChartTwoOverlaySet* overlaySet(NULL);
    ChartTwoCartesianOrientedAxes* horizontalAxis(NULL);
    ChartTwoCartesianOrientedAxes* verticalAxis(NULL);
    getSelectionData(overlaySet,
                     horizontalAxis,
                     verticalAxis);
    
    if (overlaySet != NULL) {
        ChartTwoTitle* chartTitle = overlaySet->getChartTitle();
        chartTitle->setDisplayed(checked);
        updateGraphics();
    }
}

/**
 * Called when title edit tool button is clicked
 */
void
BrainBrowserWindowToolBarChartTwoOrientedAxes::titleEditToolButtonClicked()
{
    m_titleEditMenu->exec(m_titleEditToolButton->mapToGlobal(QPoint(0, m_titleEditToolButton->height())));
}

/**
 * Called when title editor menu is about to show
 */
void
BrainBrowserWindowToolBarChartTwoOrientedAxes::titleEditorMenuAboutToShow()
{
    ChartTwoOverlaySet* overlaySet(NULL);
    ChartTwoCartesianOrientedAxes* horizontalAxis(NULL);
    ChartTwoCartesianOrientedAxes* verticalAxis(NULL);
    getSelectionData(overlaySet,
                     horizontalAxis,
                     verticalAxis);
    
    m_titleEditorWidget->updateControls(overlaySet);
}

