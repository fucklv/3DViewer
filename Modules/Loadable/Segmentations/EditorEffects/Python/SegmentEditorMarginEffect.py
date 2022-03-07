import os
import vtk, qt, ctk, slicer
import logging
import math
from SegmentEditorEffects import *

class SegmentEditorMarginEffect(AbstractScriptedSegmentEditorEffect):
  """ MaringEffect grows or shrinks the segment by a specified margin
  """

  def __init__(self, scriptedEffect):
    scriptedEffect.name = 'Margin'
    scriptedEffect.toolTipName = 'Margin'
    AbstractScriptedSegmentEditorEffect.__init__(self, scriptedEffect)

  def clone(self):
    import qSlicerSegmentationsEditorEffectsPythonQt as effects
    clonedEffect = effects.qSlicerSegmentEditorScriptedEffect(None)
    clonedEffect.setPythonSource(__file__.replace('\\','/'))
    return clonedEffect

  def icon(self):
    iconPath = os.path.join(os.path.dirname(__file__), 'Resources/Icons/Margin.png')
    if os.path.exists(iconPath):
      return qt.QIcon(iconPath)
    return qt.QIcon()

  def helpText(self):
    return "Grow or shrink selected segment by specified margin size."

  def setupOptionsFrame(self):

    iconFrame = qt.QHBoxLayout()
    iconImageLabel = qt.QLabel()
    iconTextLabel = qt.QLabel()
    
    iconTextLabel.setText(" Margin")
    iconTextLabel.setStyleSheet("QLabel{ font: 12pt 'Microsoft YaHei';}")
    iconImageLabel.setStyleSheet("QLabel{min-width: 13; min-height: 13; max-width: 13; max-height: 13;background-image: url(:/Icons/MarginSg1.png)}")
    
    iconFrame.addWidget(iconImageLabel)
    iconFrame.addWidget(iconTextLabel)
    iconFrame.addStretch()
    self.scriptedEffect.addOptionsWidget(iconFrame)
    iconFrame.setContentsMargins(0,0,0,10)

    operationLayout = qt.QVBoxLayout()

    self.shrinkOptionRadioButton = qt.QRadioButton("收缩")
    self.growOptionRadioButton = qt.QRadioButton("扩大")
    operationLayout.addWidget(self.shrinkOptionRadioButton)
    operationLayout.addWidget(self.growOptionRadioButton)
    self.growOptionRadioButton.setChecked(True)
    operationLayout.setSpacing(12)
    operationLayout.setContentsMargins(0,0,0,12)

    self.scriptedEffect.addLabeledOptionsWidget("操作:", operationLayout)

    self.marginSizeMMSpinBox = slicer.qMRMLSpinBox()
    self.marginSizeMMSpinBox.setMRMLScene(slicer.mrmlScene)
    #self.marginSizeMMSpinBox.setToolTip("Segment boundaries will be shifted by this distance. Positive value means the segments will grow, negative value means segment will shrink.")
    self.marginSizeMMSpinBox.quantity = "length"
    self.marginSizeMMSpinBox.value = 3.0
    self.marginSizeMMSpinBox.singleStep = 1.0

    self.marginSizeLabel = qt.QLabel()
    #self.marginSizeLabel.setToolTip("Size change in pixel. Computed from the segment's spacing and the specified margin size.")

    marginSizeFrame = qt.QHBoxLayout()
    marginSizeFrame.addWidget(self.marginSizeMMSpinBox)
    marginSizeFrame.addWidget(self.marginSizeLabel)
    self.marginSizeMMLabel = self.scriptedEffect.addLabeledOptionsWidget("边距大小:", marginSizeFrame)
    marginSizeFrame.setSpacing(10)

    vboxLayout = qt.QVBoxLayout()
    vboxLayout.addStretch(1)
    self.scriptedEffect.addOptionsWidget(vboxLayout)

    self.applyButton = qt.QPushButton("应用")
    self.applyButton.objectName = self.__class__.__name__ + 'Apply'
    #self.applyButton.setToolTip("Grows or shrinks selected segment by the specified margin.")

    finishFrames = qt.QHBoxLayout()
    finishFrames.addStretch()
    finishFrames.addWidget(self.applyButton)
    self.scriptedEffect.addOptionsWidget(finishFrames)

    self.applyButton.connect('clicked()', self.onApply)
    self.marginSizeMMSpinBox.connect("valueChanged(double)", self.updateMRMLFromGUI)
    self.growOptionRadioButton.connect("toggled(bool)", self.growOperationToggled)
    self.shrinkOptionRadioButton.connect("toggled(bool)", self.shrinkOperationToggled)
    self.applyButton.setStyleSheet("QPushButton{font: 10pt 'Microsoft YaHei';width: 68; height: 24;background: #191925; border: 1px solid #5c5c74;}QPushButton:hover{background-color: #5C5C74;}")

  def createCursor(self, widget):
    # Turn off effect-specific cursor for this effect
    return slicer.util.mainWindow().cursor

  def setMRMLDefaults(self):
    self.scriptedEffect.setParameterDefault("MarginSizeMm", 3)

  def getMarginSizePixel(self):
    selectedSegmentLabelmapSpacing = [1.0, 1.0, 1.0]
    selectedSegmentLabelmap = self.scriptedEffect.selectedSegmentLabelmap()
    if selectedSegmentLabelmap:
      selectedSegmentLabelmapSpacing = selectedSegmentLabelmap.GetSpacing()

    marginSizeMM = abs(self.scriptedEffect.doubleParameter("MarginSizeMm"))
    marginSizePixel = [int(math.floor(marginSizeMM / spacing)) for spacing in selectedSegmentLabelmapSpacing]
    return marginSizePixel

  def updateGUIFromMRML(self):
    marginSizeMM = self.scriptedEffect.doubleParameter("MarginSizeMm")
    wasBlocked = self.marginSizeMMSpinBox.blockSignals(True)
    self.marginSizeMMSpinBox.value = abs(marginSizeMM)
    self.marginSizeMMSpinBox.blockSignals(wasBlocked)

    wasBlocked = self.growOptionRadioButton.blockSignals(True)
    self.growOptionRadioButton.setChecked(marginSizeMM > 0)
    self.growOptionRadioButton.blockSignals(wasBlocked)

    wasBlocked = self.shrinkOptionRadioButton.blockSignals(True)
    self.shrinkOptionRadioButton.setChecked(marginSizeMM < 0)
    self.shrinkOptionRadioButton.blockSignals(wasBlocked)

    selectedSegmentLabelmapSpacing = [1.0, 1.0, 1.0]
    selectedSegmentLabelmap = self.scriptedEffect.selectedSegmentLabelmap()
    if selectedSegmentLabelmap:
      selectedSegmentLabelmapSpacing = selectedSegmentLabelmap.GetSpacing()
      marginSizePixel = self.getMarginSizePixel()
      if marginSizePixel[0] < 1 or marginSizePixel[1] < 1 or marginSizePixel[2] < 1:
        self.marginSizeLabel.text = "Not feasible at current resolution."
        self.applyButton.setEnabled(False)
      else:
        marginSizeMM = self.getMarginSizeMM()
        self.marginSizeLabel.text = "{3}x{4}x{5} pixel".format(*marginSizeMM, *marginSizePixel)
        self.applyButton.setEnabled(True)
    else:
      self.marginSizeLabel.text = "Empty segment"

    self.setWidgetMinMaxStepFromImageSpacing(self.marginSizeMMSpinBox, self.scriptedEffect.selectedSegmentLabelmap())

  def growOperationToggled(self, toggled):
    if toggled:
      self.scriptedEffect.setParameter("MarginSizeMm", self.marginSizeMMSpinBox.value)

  def shrinkOperationToggled(self, toggled):
    if toggled:
      self.scriptedEffect.setParameter("MarginSizeMm", -self.marginSizeMMSpinBox.value)

  def updateMRMLFromGUI(self):
    marginSizeMM = (self.marginSizeMMSpinBox.value) if self.growOptionRadioButton.checked else (-self.marginSizeMMSpinBox.value)
    self.scriptedEffect.setParameter("MarginSizeMm", marginSizeMM)

  def getMarginSizeMM(self):
    selectedSegmentLabelmapSpacing = [1.0, 1.0, 1.0]
    selectedSegmentLabelmap = self.scriptedEffect.selectedSegmentLabelmap()
    if selectedSegmentLabelmap:
      selectedSegmentLabelmapSpacing = selectedSegmentLabelmap.GetSpacing()

    marginSizePixel = self.getMarginSizePixel()
    marginSizeMM = [abs((marginSizePixel[i])*selectedSegmentLabelmapSpacing[i]) for i in range(3)]
    for i in range(3):
      if marginSizeMM[i] > 0:
        marginSizeMM[i] = round(marginSizeMM[i], max(int(-math.floor(math.log10(marginSizeMM[i]))),1))
    return marginSizeMM

  def onApply(self):
    self.scriptedEffect.saveStateForUndo()

    # Get modifier labelmap and parameters
    modifierLabelmap = self.scriptedEffect.defaultModifierLabelmap()
    selectedSegmentLabelmap = self.scriptedEffect.selectedSegmentLabelmap()

    # We need to know exactly the value of the segment voxels, apply threshold to make force the selected label value
    labelValue = 1
    backgroundValue = 0
    thresh = vtk.vtkImageThreshold()
    thresh.SetInputData(selectedSegmentLabelmap)
    thresh.ThresholdByLower(0)
    thresh.SetInValue(backgroundValue)
    thresh.SetOutValue(labelValue)
    thresh.SetOutputScalarType(selectedSegmentLabelmap.GetScalarType())

    marginSizeMM = self.scriptedEffect.doubleParameter("MarginSizeMm")
    if (marginSizeMM < 0):
      # The border voxel starts at zero, if we need to account for this single voxel thickness in the margin size.
      # Currently this is done by reducing the magnitude of the margin size by 0.9 * the smallest spacing.
      voxelDiameter = min(selectedSegmentLabelmap.GetSpacing())
      marginSizeMM += 0.9*voxelDiameter

    import vtkITK
    margin = vtkITK.vtkITKImageMargin()
    margin.SetInputConnection(thresh.GetOutputPort())
    margin.CalculateMarginInMMOn()
    margin.SetOuterMarginMM(marginSizeMM)
    margin.Update()
    modifierLabelmap.ShallowCopy(margin.GetOutput())

    # Apply changes
    self.scriptedEffect.modifySelectedSegmentByLabelmap(modifierLabelmap, slicer.qSlicerSegmentEditorAbstractEffect.ModificationModeSet)

    qt.QApplication.restoreOverrideCursor()
